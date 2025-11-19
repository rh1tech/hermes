#include <Arduino.h>
#include <globals.h>

#ifdef ESP32
    #include <SD.h>
    #include <SPI.h>
#elif defined(ESP8266)
    #include <SD.h>
    #include <SPI.h>
#endif

// SD Card SPI pins - adjust these based on your hardware setup
// Common ESP32 SD card pins: MOSI=23, MISO=19, SCK=18, CS=5
// Common ESP8266 SD card pins: MOSI=13, MISO=12, SCK=14, CS=15
// 
// CRITICAL WARNING: GPIO15 is a boot mode pin on ESP8266!
// - GPIO15 MUST be LOW during boot for normal flash boot
// - If SD card pulls GPIO15 HIGH at power-on, ESP8266 won't boot
// - Hardware fix: Add 10K pull-DOWN resistor on GPIO15
// - Or use a different CS pin that's not a boot strapping pin
// 
// NOTE: Pin 15 also shared with CTS_PIN (flow control disabled)

// Set to 0 to skip SD card init at startup (prevents hanging)
// You can then manually initialize with AT$SDINIT command
#define SD_INIT_AT_STARTUP 0

#ifdef ESP32
    #define SD_CS_PIN 5
    #define SD_MOSI_PIN 23
    #define SD_MISO_PIN 19
    #define SD_SCK_PIN 18
#elif defined(ESP8266)
    // Pin 15 is shared with CTS_PIN (flow control disabled to avoid conflict)
    // Hardware SPI: MOSI=13, MISO=12, SCK=14 (standard ESP8266 SPI pins)
    #define SD_CS_PIN 15
    #define SD_MOSI_PIN 13
    #define SD_MISO_PIN 12
    #define SD_SCK_PIN 14
#endif

bool sdCardAvailable = false;

void initSDCard()
{
#if SD_INIT_AT_STARTUP == 0
    // SD card init kipped at startup - use AT$SDINIT to initialize manually
    return;
#endif
    
#ifdef ESP32
    Serial.println("Platform: ESP32");
    Serial.print("SD Pins - CS: ");
    Serial.print(SD_CS_PIN);
    Serial.print(", MOSI: ");
    Serial.print(SD_MOSI_PIN);
    Serial.print(", MISO: ");
    Serial.print(SD_MISO_PIN);
    Serial.print(", SCK: ");
    Serial.println(SD_SCK_PIN);
    
    // Initialize SPI with custom pins
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    Serial.println("SPI initialized with custom pins");
#elif defined(ESP8266)
    Serial.println("Platform: ESP8266");
    Serial.print("SD Pins - CS: ");
    Serial.print(SD_CS_PIN);
    Serial.print(", MOSI: ");
    Serial.print(SD_MOSI_PIN);
    Serial.print(", MISO: ");
    Serial.print(SD_MISO_PIN);
    Serial.print(", SCK: ");
    Serial.println(SD_SCK_PIN);
    
    // ESP8266 uses default SPI pins
    SPI.begin();
    Serial.println("SPI initialized (default pins)");
    
    // Enable internal pull-ups on SPI pins for ESP8266
    pinMode(SD_MOSI_PIN, INPUT_PULLUP);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    pinMode(SD_SCK_PIN, INPUT_PULLUP);
    Serial.println("Internal pull-ups enabled on MOSI, MISO, SCK");
#endif
    
    // Set CS pin as output with pull-up
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    Serial.println("CS pin configured as OUTPUT and set HIGH");
    
    delay(100); // Give the card some time to stabilize
    
    Serial.print("Attempting SD.begin(");
    Serial.print(SD_CS_PIN);
    Serial.print(")... ");
    Serial.flush(); // Ensure message is printed before potential hang
    
    // Try to initialize the SD card with timeout protection
    // Note: SD.begin() can hang on ESP8266 if card is slow or faulty
    unsigned long startTime = millis();
    bool sdInitSuccess = false;
    
    // Feed the watchdog to prevent timeout during slow SD init
#ifdef ESP8266
    ESP.wdtFeed();
#elif defined(ESP32)
    yield();
#endif
    
    // Attempt initialization (this may take a while)
    sdInitSuccess = SD.begin(SD_CS_PIN);
    
    // Feed watchdog again after init
#ifdef ESP8266
    ESP.wdtFeed();
#elif defined(ESP32)
    yield();
#endif
    
    unsigned long elapsed = millis() - startTime;
    
    if (!sdInitSuccess) {
        Serial.println("FAILED (took " + String(elapsed) + "ms)");
        Serial.println("\x1b[37;41m No card inserted \x1b[0m");
        Serial.println();
        Serial.println("Possible reasons:");
        Serial.println("1. Card not properly inserted");
        Serial.println("2. Wrong CS pin (current: " + String(SD_CS_PIN) + ")");
        Serial.println("3. Wrong SPI pins wiring");
        Serial.println("4. SD card module not powered (needs 3.3V or 5V)");
        Serial.println("5. Faulty SD card or module");
        Serial.println("6. Card format not supported (try FAT32)");
        Serial.println("7. Add external pull-ups (10k to 3.3V) if needed");
        Serial.println();
        Serial.println("Tip: Insert card and run AT$SDTEST to find correct CS pin");
        Serial.println("=========================");
        Serial.println();
        sdCardAvailable = false;
        return;
    }
    
    Serial.println("SUCCESS (took " + String(elapsed) + "ms)");
    Serial.println("=========================");
    
    sdCardAvailable = true;
    Serial.println("\x1b[30;42m Card detected \x1b[0m");
    
    // Get card type
    Serial.println();
    Serial.println("\016lqqqqqqqqqqqqqqqqqqqqq SD CARD INFORMATION qqqqqqqqqqqqqqqqqqqqqk");
    
#ifdef ESP32
    uint8_t cardType = SD.cardType();
    
    Serial.print("\016x\017 Card Type:   ");
    switch(cardType) {
        case CARD_NONE:
            Serial.println("No SD card attached");
            sdCardAvailable = false;
            break;
        case CARD_MMC:
            Serial.println("MMC                                        \016x");
            break;
        case CARD_SD:
            Serial.println("SDSC                                       \016x");
            break;
        case CARD_SDHC:
            Serial.println("SDHC                                       \016x");
            break;
        default:
            Serial.println("UNKNOWN                                    \016x");
    }
#elif defined(ESP8266)
    // ESP8266 SD library doesn't provide card type info
    Serial.println("\016x\017 Card Type:   SD/MMC                                  \016x");
#endif
    
    if (!sdCardAvailable) {
        Serial.println("\016mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj\017");
        return;
    }
    
#ifdef ESP32
    // ESP32 has more detailed SD card info methods
    // Get card size
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("\016x\017 Card Size:   ");
    Serial.print(cardSize);
    Serial.println(" MB                                  \016x");
    
    // Get total space
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    Serial.print("\016x\017 Total Space: ");
    Serial.print(totalBytes);
    Serial.println(" MB                                  \016x");
    
    // Get used space
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    Serial.print("\016x\017 Used Space:  ");
    Serial.print(usedBytes);
    Serial.println(" MB                                  \016x");
    
    // Get free space
    uint64_t freeBytes = totalBytes - usedBytes;
    Serial.print("\016x\017 Free Space:  ");
    Serial.print(freeBytes);
    Serial.println(" MB                                  \016x");
#elif defined(ESP8266)
    // ESP8266 SD library has limited functionality
    // Open root directory to confirm card is accessible
    File root = SD.open("/");
    if (root) {
        Serial.println("\016x\017 Status:      Ready                                  \016x");
        Serial.println("\016x\017 Note:        Detailed size info not available       \016x");
        Serial.println("\016x\017              on ESP8266 platform                    \016x");
        root.close();
    } else {
        Serial.println("\016x\017 Status:      Card mounted but not accessible       \016x");
    }
#endif
    
    Serial.println("\016mqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqj\017");
    Serial.println();
}

bool isSDCardAvailable()
{
    return sdCardAvailable;
}

// Manual SD card initialization (safe to call anytime)
void manualInitSDCard()
{
    bool sdInitSuccess = false;
    
#ifdef ESP32
    // ESP32 requires explicit SPI initialization with custom pins
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    delay(10);
    
    // For ESP32, pass SPI instance and specify frequency (lower for compatibility)
    sdInitSuccess = SD.begin(SD_CS_PIN, SPI, 4000000);
    
#elif defined(ESP8266)
    // ESP8266 uses default SPI pins
    SPI.begin();
    
    // Enable internal pull-ups on SPI pins for ESP8266
    pinMode(SD_MOSI_PIN, INPUT_PULLUP);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    pinMode(SD_SCK_PIN, INPUT_PULLUP);
    
    // Set CS pin as output
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    
    delay(100);
    
    ESP.wdtFeed();
    
    sdInitSuccess = SD.begin(SD_CS_PIN);
    
    ESP.wdtFeed();
#endif
    
    yield();
    
    if (!sdInitSuccess) {
        Serial.println();
        Serial.println("\x1b[37;41m No card detected \x1b[0m");
        Serial.println();
        sdCardAvailable = false;
        return;
    }
    
    sdCardAvailable = true;
    Serial.println();
    Serial.println("SD CARD INFORMATION");
    
#ifdef ESP32
    uint8_t cardType = SD.cardType();
    Serial.print("Card Type:   ");
    switch(cardType) {
        case CARD_MMC: Serial.println("MMC"); break;
        case CARD_SD: Serial.println("SDSC"); break;
        case CARD_SDHC: Serial.println("SDHC"); break;
        default: Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("Card Size:   ");
    Serial.print(cardSize);
    Serial.println(" MB");
    
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
    Serial.print("Total Space: ");
    Serial.print(totalBytes);
    Serial.println(" MB");
    
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
    Serial.print("Used Space:  ");
    Serial.print(usedBytes);
    Serial.println(" MB");
    
    uint64_t freeBytes = totalBytes - usedBytes;
    Serial.print("Free Space:  ");
    Serial.print(freeBytes);
    Serial.println(" MB");
#elif defined(ESP8266)
    Serial.println("Card Type:   SD/MMC");
    File root = SD.open("/");
    if (root) {
        Serial.println("Status:      Ready");
        root.close();
    } else {
        Serial.println("Status:      Error accessing card");
    }
#endif
    
    Serial.println();
}


