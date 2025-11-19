#include <Arduino.h>
#include <globals.h>

#ifdef ESP32
    #include <SD.h>
#elif defined(ESP8266)
    #include <SD.h>
#endif

extern bool sdCardAvailable;

void testSDCardSpeed()
{
    if (!sdCardAvailable) {
        Serial.println();
        Serial.println("\x1b[37;41m SD card not initialized \x1b[0m");
        Serial.println("Run AT$SDINIT first");
        Serial.println();
        return;
    }
    
    Serial.println();
    Serial.println("SD CARD SPEED TEST");
    Serial.println("==================");
    
    const char* testFile = "/sdtest.tmp";
    const int testSize = 8192; // 8KB test
    const int iterations = 10;
    
    // Create test data buffer
    byte* testData = new byte[testSize];
    for (int i = 0; i < testSize; i++) {
        testData[i] = i % 256;
    }
    
    // Test 1: Write speed
    Serial.print("Write test (" + String(testSize * iterations / 1024) + " KB)... ");
    Serial.flush();
    
    unsigned long writeStart = millis();
    File writeFile = SD.open(testFile, FILE_WRITE);
    
    if (!writeFile) {
        Serial.println("FAILED - Cannot create file");
        delete[] testData;
        return;
    }
    
    for (int i = 0; i < iterations; i++) {
        writeFile.write(testData, testSize);
#ifdef ESP8266
        ESP.wdtFeed();
#endif
    }
    
    writeFile.close();
    unsigned long writeEnd = millis();
    unsigned long writeTime = writeEnd - writeStart;
    
    float writeSpeedKB = (float)(testSize * iterations) / (float)writeTime;
    Serial.print("Done (");
    Serial.print(writeTime);
    Serial.print(" ms, ");
    Serial.print(writeSpeedKB, 2);
    Serial.println(" KB/s)");
    
    // Test 2: Read speed
    Serial.print("Read test (" + String(testSize * iterations / 1024) + " KB)... ");
    Serial.flush();
    
    unsigned long readStart = millis();
    File readFile = SD.open(testFile, FILE_READ);
    
    if (!readFile) {
        Serial.println("FAILED - Cannot open file");
        SD.remove(testFile);
        delete[] testData;
        return;
    }
    
    byte* readBuffer = new byte[testSize];
    int bytesRead = 0;
    
    while (readFile.available()) {
        bytesRead += readFile.read(readBuffer, testSize);
#ifdef ESP8266
        ESP.wdtFeed();
#endif
    }
    
    readFile.close();
    unsigned long readEnd = millis();
    unsigned long readTime = readEnd - readStart;
    
    float readSpeedKB = (float)bytesRead / (float)readTime;
    Serial.print("Done (");
    Serial.print(readTime);
    Serial.print(" ms, ");
    Serial.print(readSpeedKB, 2);
    Serial.println(" KB/s)");
    
    // Test 3: Verify data integrity
    Serial.print("Verify integrity... ");
    Serial.flush();
    
    readFile = SD.open(testFile, FILE_READ);
    bool dataOk = true;
    int errorCount = 0;
    
    for (int iter = 0; iter < iterations && dataOk; iter++) {
        int bytesRead = readFile.read(readBuffer, testSize);
        if (bytesRead != testSize) {
            dataOk = false;
            break;
        }
        
        for (int i = 0; i < testSize; i++) {
            if (readBuffer[i] != (byte)(i % 256)) {
                errorCount++;
                if (errorCount > 10) {
                    dataOk = false;
                    break;
                }
            }
        }
#ifdef ESP8266
        ESP.wdtFeed();
#endif
    }
    
    readFile.close();
    
    if (dataOk && errorCount == 0) {
        Serial.println("PASS (no errors)");
    } else {
        Serial.print("FAIL (");
        Serial.print(errorCount);
        Serial.println(" errors)");
    }
    
    // Test 4: File size check
    Serial.print("File size... ");
    Serial.flush();
    
    File sizeFile = SD.open(testFile, FILE_READ);
    size_t fileSize = sizeFile.size();
    sizeFile.close();
    
    if (fileSize == (size_t)(testSize * iterations)) {
        Serial.print("OK (");
        Serial.print(fileSize);
        Serial.println(" bytes)");
    } else {
        Serial.print("ERROR (expected ");
        Serial.print(testSize * iterations);
        Serial.print(", got ");
        Serial.print(fileSize);
        Serial.println(")");
    }
    
    delete[] testData;
    delete[] readBuffer;
    
    Serial.println("==================");
    Serial.print("Test file: ");
    Serial.println(testFile);
    Serial.println("Test complete");
    Serial.println();
}
