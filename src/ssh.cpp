#include <Arduino.h>
#include <globals.h>

#ifdef ESP32
    #include <WiFi.h>
    #include <WiFiClient.h>
    #include <libssh/libssh.h>
    #include "libssh_esp32.h"
    
    static ssh_session sshSession = NULL;
    static ssh_channel sshChannel = NULL;
    
    // SSH connection parameters
    struct SSHConnectParams {
        String username;
        String password;
        String host;
        String port;
    };
    
    static SSHConnectParams sshParams;
#endif

void cleanupSSHSession()
{
#ifdef ESP32
    if (sshChannel != NULL)
    {
        ssh_channel_send_eof(sshChannel);
        ssh_channel_close(sshChannel);
        ssh_channel_free(sshChannel);
        sshChannel = NULL;
    }
    
    if (sshSession != NULL)
    {
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        sshSession = NULL;
    }
    
    sshConnected = false;
#endif
}

#ifdef ESP32
// SSH connection task with large stack
void sshConnectTask(void *parameter)
{
    SSHConnectParams *params = (SSHConnectParams *)parameter;
    
    // Initialize libssh library
    libssh_begin();
    
    // Create new SSH session
    sshSession = ssh_new();
    if (sshSession == NULL)
    {
        Serial.println("Failed to create SSH session");
        sendResult(RES_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    // Set SSH options
    int portInt = params->port.toInt();
    ssh_options_set(sshSession, SSH_OPTIONS_HOST, params->host.c_str());
    ssh_options_set(sshSession, SSH_OPTIONS_PORT, &portInt);
    ssh_options_set(sshSession, SSH_OPTIONS_USER, params->username.c_str());
    
    // Set timeout
    long timeout = 10; // 10 seconds
    ssh_options_set(sshSession, SSH_OPTIONS_TIMEOUT, &timeout);
    
    // Connect to server
    Serial.print("Connecting... ");
    int rc = ssh_connect(sshSession);
    if (rc != SSH_OK)
    {
        Serial.println("FAILED");
        Serial.print("Error: ");
        Serial.println(ssh_get_error(sshSession));
        cleanupSSHSession();
        sendResult(RES_NOANSWER);
        vTaskDelete(NULL);
        return;
    }
    Serial.println("OK");
    
    // Authenticate with password
    Serial.print("Authenticating... ");
    Serial.println();
    rc = ssh_userauth_password(sshSession, NULL, params->password.c_str());
    if (rc != SSH_AUTH_SUCCESS)
    {
        Serial.println("FAILED");
        Serial.println("Authentication error (check username/password)");
        cleanupSSHSession();
        sendResult(RES_ERROR);
        vTaskDelete(NULL);
        return;
    }
    Serial.println("OK");
    
    // Open a channel
    Serial.print("Opening shell... ");
    sshChannel = ssh_channel_new(sshSession);
    if (sshChannel == NULL)
    {
        Serial.println("FAILED");
        cleanupSSHSession();
        sendResult(RES_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    rc = ssh_channel_open_session(sshChannel);
    if (rc != SSH_OK)
    {
        Serial.println("FAILED");
        Serial.print("Error: ");
        Serial.println(ssh_get_error(sshSession));
        cleanupSSHSession();
        sendResult(RES_ERROR);
        vTaskDelete(NULL);
        return;
    }
    
    // Request PTY
    rc = ssh_channel_request_pty(sshChannel);
    if (rc != SSH_OK)
    {
        Serial.println("Failed to request PTY");
    }
    
    // Request shell
    rc = ssh_channel_request_shell(sshChannel);
    if (rc != SSH_OK)
    {
        Serial.println("FAILED");
        Serial.print("Error: ");
        Serial.println(ssh_get_error(sshSession));
        cleanupSSHSession();
        sendResult(RES_ERROR);
        vTaskDelete(NULL);
        return;
    }
    Serial.println("OK");
    
    // Mark as connected
    Serial.println();
    Serial.println("SSH session established");
    Serial.println();
    
    sshConnected = true;
    callConnected = true;
    cmdMode = false;
    connectTime = millis();
    Serial.flush();
    setCarrierDCDPin(callConnected);
    
    // Task completed, delete itself
    vTaskDelete(NULL);
}
#endif

void connectSSH(String cmd)
{
#ifndef ESP32
    Serial.println();
    Serial.println("SSH is only implemented for ESP32 based Protea board");
    sendResult(RES_ERROR);
    return;
#else
    if (callConnected)
    {
        sendResult(RES_ERROR);
        return;
    }

    // Parse command: ATSSH username@host:port (case-sensitive args)
    // Find where "ssh" ends (case-insensitive) to preserve case of username/host
    String upperCmd = cmd;
    upperCmd.toUpperCase();
    int sshPos = upperCmd.indexOf("SSH");
    if (sshPos == -1)
    {
        sendResult(RES_ERROR);
        return;
    }
    
    String fullCmd = cmd.substring(sshPos + 3); // Skip past "SSH"
    fullCmd.trim();
    
    String username = "";
    String password = "";
    String host = "";
    String port = "22"; // Default SSH port
    
    // Parse username@host:port
    int atIndex = fullCmd.indexOf("@");
    if (atIndex == -1)
    {
        Serial.println();
        Serial.println("Invalid format. Use: ATSSH username@host:port");
        Serial.println("Example: ATSSH user@example.com:22");
        Serial.println("Port is optional (default: 22)");
        sendResult(RES_ERROR);
        return;
    }
    
    username = fullCmd.substring(0, atIndex);
    String hostPort = fullCmd.substring(atIndex + 1);
    
    // Parse host:port
    int colonIndex = hostPort.indexOf(":");
    if (colonIndex != -1)
    {
        host = hostPort.substring(0, colonIndex);
        port = hostPort.substring(colonIndex + 1);
    }
    else
    {
        host = hostPort;
    }
    
    username.trim();
    host.trim();
    port.trim();
    
    if (username.length() == 0 || host.length() == 0)
    {
        Serial.println();
        Serial.println("Username and host are required");
        sendResult(RES_ERROR);
        return;
    }
    
    // Ask for password
    Serial.println();
    Serial.print("Password: ");
    Serial.flush();
    
    // Wait for password input (terminated by carriage return)
    password = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 30000) // 30 second timeout
    {
        if (Serial.available())
        {
            char c = Serial.read();
            if (c == '\r' || c == '\n')
            {
                if (password.length() > 0)
                {
                    break;
                }
            }
            else if (c == 8 || c == 127) // Backspace or DEL
            {
                if (password.length() > 0)
                {
                    password.remove(password.length() - 1);
                }
            }
            else if (c >= 32 && c <= 126) // Printable characters
            {
                password += c;
            }
        }
        delay(10);
    }
    
    Serial.println(); // New line after password entry
    
    if (password.length() == 0)
    {
        Serial.println("Password required");
        sendResult(RES_ERROR);
        return;
    }
    
    Serial.println();
    Serial.print("Connecting SSH to ");
    Serial.print(host);
    Serial.print(":");
    Serial.print(port);
    Serial.print(" as ");
    Serial.println(username);
    
    // Clean up any existing session
    cleanupSSHSession();
    
    // Store parameters
    sshParams.username = username;
    sshParams.password = password;
    sshParams.host = host;
    sshParams.port = port;
    
    // Create SSH connection task with large stack (51200 bytes)
    BaseType_t taskCreated = xTaskCreate(
        sshConnectTask,       // Task function
        "sshConnect",         // Task name
        51200,                // Stack size in bytes
        &sshParams,           // Parameters
        tskIDLE_PRIORITY + 1, // Priority
        NULL                  // Task handle (not needed)
    );
    
    if (taskCreated != pdPASS)
    {
        Serial.println("Failed to create SSH connection task");
        sendResult(RES_ERROR);
        return;
    }
    
    // Task will handle the rest and send result
#endif
}

void handleSSHData()
{
#ifdef ESP32
    if (!sshConnected || sshChannel == NULL)
    {
        return;
    }
    
    // Read from SSH channel and write to Serial (non-blocking)
    char buffer[256];
    int nbytes = ssh_channel_read_nonblocking(sshChannel, buffer, sizeof(buffer), 0);
    
    if (nbytes > 0)
    {
        Serial.write((uint8_t*)buffer, nbytes);
        Serial.flush();
    }
    else if (nbytes < 0)
    {
        // Error reading - channel is likely closed
        if (ssh_channel_is_eof(sshChannel))
        {
            cleanupSSHSession();
            hangUp();
            return;
        }
    }
    // nbytes == 0 means no data available, which is normal
    
    // Read from Serial and write to SSH channel
    while (Serial.available())
    {
        char c = Serial.read();
        ssh_channel_write(sshChannel, &c, 1);
    }
#endif
}
