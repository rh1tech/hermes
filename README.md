# Hermes

ESP8266EX/ESP32-D0WD based RS232 Serial WiFi Modem Emulator for vintage computing and the Protea development platform.

Hermes is a fork of [The Old Net - RS232 Serial WIFI Modem](https://github.com/ssshake/vintage-computer-wifi-modem) by [Richard Bettridge](https://theoldnet.com/), adapted and optimized for integration with the [Protea](https://github.com/rh1tech/protea) development board ecosystem. Named after the Greek messenger god who bridged the divine and mortal realms, Hermes provides a seamless bridge between vintage computing hardware and the modern internet.

This firmware transforms an ESP8266/ESP32 based board into a Hayes-compatible serial modem that connects Protea to the Internet via WiFi, maintaining the authentic experience of classic terminal communications.

Documentation: [rh1.tech](https://rh1.tech/projects/hermes)

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

No driver installation or network configuration required on the host system - it just works like the modems of yesteryear.

## Integration with Protea

Hermes serves as the WiFi connectivity component within the [Protea](https://github.com/rh1tech/protea) development platform, providing network access capabilities that complement the [Iris](https://github.com/rh1tech/iris) terminal software. This integration enables complete vintage computing communication solutions with modern internet connectivity.

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.

## Credits

Original Project: [The Old Net - RS232 Serial WIFI Modem](https://github.com/ssshake/vintage-computer-wifi-modem) by [Richard Bettridge](https://theoldnet.com/). Hermes is a fork adapted and enhanced by [Mikhail Matveev](https://rh1.tech) for the [Protea](https://github.com/rh1tech/protea) platform.