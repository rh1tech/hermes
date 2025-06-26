# Hermes

ESP8266-based RS232 Serial WiFi Modem Emulator for vintage computing and the Protea development platform.

Hermes is a fork of [The Old Net - RS232 Serial WIFI Modem](https://github.com/ssshake/vintage-computer-wifi-modem) by [Richard Bettridge](https://theoldnet.com/), adapted and optimized for integration with the [Protea](https://github.com/rh1tech/protea) development board ecosystem. Named after the Greek messenger god who bridged the divine and mortal realms, Hermes provides a seamless bridge between vintage computing hardware and the modern internet.

This firmware transforms an ESP8266 microcontroller into a Hayes-compatible serial modem that connects vintage computers to the internet via WiFi, maintaining the authentic experience of classic terminal communications.

## Features

### Core Functionality:

- Hayes AT Command Compatibility - Works with any vintage terminal software
- RS232 DB9 Serial Interface - Standard connection for retro computers
- WiFi to Serial Bridge - Internet connectivity without network card installation
- Telnet Protocol Support - Connect to remote systems using IP addresses instead of phone numbers
- Transparent Operation - No special software required on the host computer
Technical Specifications:
- ESP8266 Platform - Reliable, cost-effective WiFi microcontroller
- Standard Baud Rates - Compatible with vintage computing standards
- AT Command Set - Full Hayes modem command emulation
- IP Dialing - Simply “dial” IP addresses like phone numbers
- Multiple Connection Support - Handle various telnet destinations

## Usage

Hermes operates exactly like a traditional dial-up modem from the perspective of your vintage computer:

- Connect the RS232 cable between Hermes and your vintage computer
- Configure your terminal software for the appropriate baud rate
- “Dial” IP addresses using standard AT commands
- Connect to telnet services, BBSes, and other network resources

No driver installation or network configuration required on the host system - it just works like the modems of yesteryear.

## Integration with Protea

Hermes serves as the WiFi connectivity component within the [Protea](https://github.com/rh1tech/protea) development platform, providing network access capabilities that complement the [Iris](https://github.com/rh1tech/iris) terminal software. This integration enables complete vintage computing communication solutions with modern internet connectivity.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.

## Credits

Original Project: The Old Net - RS232 Serial WIFI Modem by Richard Bettridge

Hermes Fork: Adapted and enhanced by Mikhail Matveev for the Protea platform