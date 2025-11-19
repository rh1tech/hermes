# Hermes

ESP8266/ESP based RS232 Serial WiFi Modem Emulator for vintage computing and the Protea development platform.

Hermes is a fork of [The Old Net - RS232 Serial WIFI Modem](https://github.com/ssshake/vintage-computer-wifi-modem) by [Richard Bettridge](https://theoldnet.com/), adapted and optimized for integration with the [Protea](https://github.com/rh1tech/protea) board ecosystem. Named after the Greek messenger god who bridged the divine and mortal realms, Hermes provides a seamless bridge between vintage computing hardware and the modern internet.

This firmware transforms an ESP8266/ESP32 based board into a Hayes-compatible serial modem that connects Protea to the Internet via WiFi, maintaining the authentic experience of classic terminal communications.

Documentation: [rh1.tech](https://rh1.tech/projects/hermes)

## Integration with Protea

Hermes serves as the WiFi connectivity component within the [Protea](https://github.com/rh1tech/protea) platform, providing network access capabilities that complement the [Iris](https://github.com/rh1tech/iris) terminal software. This integration enables complete vintage computing communication solutions with modern internet connectivity.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.

## Credits

Original Project: [The Old Net - RS232 Serial WIFI Modem](https://github.com/ssshake/vintage-computer-wifi-modem) by [Richard Bettridge](https://theoldnet.com/). Hermes is a fork adapted and enhanced by [Mikhail Matveev](https://rh1.tech) for the [Protea](https://github.com/rh1tech/protea) platform.

## Features

### Core Functionality:

- Hayes AT command compatibility - works with any vintage terminal software
- Telnet protocol support - connect to remote systems using IP addresses instead of phone numbers
- Transparent operation - no special software required on the host computer

### Technical Specifications:

- ESP8266 or ESP32 based platform
- Standard baud rates - compatible with vintage computing standards
- AT command set - Hayes modem command emulation
- IP dialing - simply “dial” IP addresses like phone numbers
- Multiple connection support - handle various destinations

## Usage

Hermes operates exactly like a traditional dial-up modem from the perspective of your vintage computer:

- Connect the RS232 cable between Hermes and your vintage computer
- Configure your terminal software for the appropriate baud rate
- “Dial” IP addresses using standard AT commands
- Connect to telnet services, BBSes, and other network resources

### Getting started

No driver installation or network configuration required on the host system - it just works like the old modems.

Default baud rate is 9600 bps. You can change it using the command: <code>AT$SB=N</code>, where N is one of: 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200.

You need to connect to a Wi-Fi network before making calls. To do this, set the SSID and password using the following commands:

- <code>AT$SSID=YOUR_WIFI_NETWORK</code>
- <code>AT$PASS=YOUR_WIFI_PASSWORD</code>
  
When done, connect using the command: <code>ATC1</code>.

Save your settings by entering: <code>AT&W</code>.

### AT Command Summary

| Command | Description |
| :--- | :--- |
| `ATDTHOST[:PORT]` | Dial Host (Telnet), default port is 23 |
| `ATDSN` | Speed Dial (N=0-9) |
| `ATDTPPP` | Start PPP Session |
| `AT&ZN=HOST:PORT` | Set Speed Dial entry (N=0-9) |
| `ATNETN` | Handle Telnet (N=0,1) |
| `ATI` | Network Information |
| `ATGET<URL>` | HTTP GET Request |
| `ATGPH<URL>` | Gopher Request |
| `ATS0=N` | Auto Answer (N=0,1) |
| `AT$BM=Your Message` | Set BUSY Message |
| `ATZ` | Load settings from NVRAM |
| `AT&W` | Save settings to NVRAM |
| `AT&V` | Show current settings |
| `AT&F` | Reset to factory defaults |
| `AT&PN` | Set Pin Polarity (N=0/INV, 1/NORM) |
| `ATE0` / `ATE1` | Echo Off / On |
| `ATQ0` / `ATQ1` | Quiet Mode Off / On (send result codes) |
| `ATV1` / `ATV0` | Verbose On / Off (word vs. numeric codes) |
| `AT$SSID=YOUR_WIFI_NETWORK` | Set Wi-Fi SSID |
| `AT$PASS=.YOUR_WIFI_PASSWORD` | Set Wi-Fi Password |
| `AT$SB=N` | Set Baud Rate (300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200) |
| `AT&KN` | Set Flow Control (N=0/None, 1/HW, 2/SW) |
| `ATC1` / `ATC0` | Wi-Fi On / Off |
| `ATH` | Hang Up |
| `+++` | Enter Command Mode |
| `ATO` | Exit Command Mode (return to online data mode) |
| `AT$FW` | Update Firmware |
