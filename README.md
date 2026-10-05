# CYD-2432S028 Wi-Fi CSI Sensing

Complete GitHub Actions + PlatformIO project for the ESP32 CYD-2432S028.

Features:
- 2.4 GHz Wi-Fi scan
- Touch network selection
- On-screen password keyboard
- Wi-Fi connection
- CSI configuration
- CSI graph
- RSSI and packet counter
- Basic motion detection
- Stop button

Hardware:
Display: MOSI 13, MISO 12, SCLK 14, CS 15, DC 2
Touch: CLK 25, MOSI 32, MISO 39, CS 33, IRQ 36

CSI is Wi-Fi channel-state-information sensing; it is not a camera and does not literally see through walls.

The project is designed for a cloud GitHub Actions build, so local PlatformIO is not required.
