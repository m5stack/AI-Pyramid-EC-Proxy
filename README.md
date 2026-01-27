# EC Proxy - Embedded Controller Communication System

<p align="center"><img src="https://static-cdn.m5stack.com/resource/public/assets/m5logo2022.svg" alt="M5Stack" width="300" height="300"></p>

<p align="center">
  EC Proxy is a high-performance embedded controller communication system that provides hardware abstraction and control for embedded devices. It bridges Modbus RTU communication with modern ZeroMQ-based RPC/PUB-SUB architecture, enabling intuitive hardware control through simple command-line tools or programmatic interfaces.
</p>

## Table of Contents

* [Features](#features)
* [Architecture](#architecture)
* [Components](#components)
* [System Requirements](#system-requirements)
* [Compilation](#compilation)
* [Installation](#installation)
* [Usage](#usage)
* [Hardware Support](#hardware-support)
* [Documentation](#documentation)
* [Contributing](#contributing)
* [License](#license)

## Features

* **Modbus RTU Bridge**: Seamless communication with embedded controllers via serial interface
* **ZeroMQ RPC Service**: Modern, high-performance remote procedure call interface
* **Event Broadcasting**: Real-time PUB/SUB event system for hardware state changes
* **Comprehensive Hardware Control**: Manage power, USB ports, PCIe slots, RGB LEDs, LCD displays, fans, and sensors
* **Network Interface Management**: Automatic IP address synchronization for multiple network interfaces
* **Configuration Persistence**: Flash storage for critical configuration data
* **Multi-threaded Architecture**: Efficient concurrent handling of multiple hardware subsystems
* **Simple CLI Tool**: User-friendly command-line interface for all hardware operations
* **Cross-platform**: Built for embedded Linux devices (ARM64 architecture)
* **Open Source**: Licensed under MIT License

## Architecture

The EC Proxy system consists of two main components that work together to provide hardware abstraction:

```
┌─────────────────┐     Modbus RTU      ┌──────────────────┐      ZMQ RPC/PUB     ┌─────────────┐
│   Hardware EC   │ ◄─────────────────► │   EC Proxy       │ ◄──────────────────► │   Client    │
│   (STM32/MCU)   │   /dev/ttyS3        │   Server         │   IPC/TCP Sockets    │   (CLI/App) │
└─────────────────┘   115200-921600bps  └──────────────────┘                      └─────────────┘
```

### Communication Flow

1. **Hardware Layer**: Embedded controller (EC) exposes Modbus RTU interface over serial port
2. **Proxy Layer**: EC Proxy server translates Modbus commands to ZMQ RPC calls
3. **Client Layer**: CLI tools or applications interact via simple RPC interface
4. **Event Layer**: Hardware events (button presses, status changes) broadcast via PUB/SUB

## Components

### 1. EC Proxy Server (`ec_proxy`)

The proxy server daemon that manages all hardware communication:

- Modbus RTU serial communication
- ZeroMQ RPC service (`ipc:///tmp/rpc.ec_prox`)
- Event publisher (`ipc:///tmp/llm/ec_prox.event.socket`)
- Network interface monitoring
- Automatic fan control
- Button event handling

### 2. EC CLI Tool (`cli`)

Command-line interface for hardware control:

```bash
# Control hardware peripherals
ec_cli device --fan -d 80              # Set fan to 80% speed
ec_cli device --rgb -d 5               # Set RGB LED mode
ec_cli device --board                  # Get power consumption info
ec_cli device --poweroff -d 1          # Trigger system poweroff

# Execute custom RPC calls
ec_cli exec -l                         # List all available functions
ec_cli exec -f <function> -d <data>    # Call custom function

# Monitor hardware events
ec_cli echo --button                   # Subscribe to button events
```

See [detailed documentation](./projects/ec_proxy/README.md) for complete command reference.

## System Requirements

### Hardware
- ARM64-based embedded Linux device (AX630C, AX650N, or compatible)
- Serial port for Modbus communication (e.g., `/dev/ttyS3`)
- Embedded controller with Modbus RTU support

### Software
- **Operating System**: Ubuntu 20.04+ (ARM64) or compatible Linux distribution
- **Cross-compilation Toolchain**: aarch64-none-linux-gnu-gcc 10.3+
- **Build Tools**: SCons, Python 3.8+
- **Libraries**: ZeroMQ, libmodbus, fmt, nlohmann-json

## Compilation

### 1. Install Cross-compilation Toolchain

```bash
# Download and install ARM64 cross-compilation toolchain
wget https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/linaro/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz
sudo tar Jxvf gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu.tar.xz -C /opt

# Add to PATH (add to ~/.bashrc for persistence)
export PATH=/opt/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin:$PATH
```

### 2. Install Build Dependencies

```bash
# Install required packages
sudo apt update
sudo apt install -y python3 python3-pip libffi-dev git

# Install Python build tools
pip3 install parse scons requests kconfiglib
```

### 3. Clone and Build

```bash
# Clone repository
git clone https://github.com/m5stack/Ai_Pyramid_ec_proxy.git
cd Ai_Pyramid_ec_proxy

# Initialize submodules (SDK components)
git submodule update --init --recursive

# Navigate to EC Proxy project
cd projects/ec_proxy

# Clean previous builds (optional)
scons distclean

# Compile (use -j flag for parallel compilation)
scons -j$(nproc)
```

### 4. Build Output

After successful compilation, binaries will be located in:

```
projects/ec_proxy/build/
├── main_ec_proxy/
│   └── ec_proxy          # Proxy server executable
└── main_ec_cli/
    └── ec_cli               # CLI tool executable
```

## Installation

### Deploy to Target Device

```bash
# Copy binaries to target device
scp build/main_ec_proxy/ec_proxy root@<target-ip>:/usr/local/bin/
scp build/main_ec_cli/ec_cli root@<target-ip>:/usr/local/bin/

# On target device, make executable
chmod +x /usr/local/bin/ec_proxy
chmod +x /usr/local/bin/cli
```

### Create Systemd Service (Optional)

Create `/etc/systemd/system/ec-proxy.service`:

```ini
[Unit]
Description=EC Proxy Service
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/ec_proxy
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable ec-proxy
sudo systemctl start ec-proxy
```

## Usage

### Start EC Proxy Server

```bash
# Run directly
./ec_proxy

# Or use systemd service
sudo systemctl start ec-proxy
```

### Using the CLI Tool

```bash
# Get hardware status
ec_cli device --board                    # Power consumption info
ec_cli device --version                  # EC firmware version
ec_cli device --fanspeed                 # Current fan speed

# Control peripherals
ec_cli device --fan -d 60                # Set fan PWM to 60%
ec_cli device --rgb -d 1                 # Set RGB mode to 1
ec_cli device --lcd_brightness -d 200    # Set LCD brightness

# Network configuration
ec_cli device --ip_eth0 -d "192.168.1.100"
ec_cli device --flash_value              # Save to flash memory

# Monitor events
ec_cli echo --button                     # Watch for button presses
```

## Hardware Support

EC Proxy supports control of the following hardware peripherals:

### Power Management
- Board power control
- External power switch
- USB PD power info
- Power consumption monitoring
- Scheduled power on/off

### Peripherals
- 3× USB downstream ports (with high-power mode)
- 2× PCIe slots
- GL3510 USB hub
- Grove I2C/UART interfaces

### Display & LEDs
- RGB LED array (up to 64 LEDs)
- LCD display with brightness control
- Multiple display modes

### Sensors & Control
- Fan PWM control with RPM monitoring
- Temperature-based auto fan control
- CPU voltage (VDD) adjustment
- Button input detection

### Network
- Ethernet interface management (eth0, eth1)
- WLAN interface support
- Automatic IP address synchronization

See [complete hardware documentation](./projects/ec_proxy/README.md#supported-hardware-peripherals) for detailed register mappings.

## Documentation

- **[EC Proxy Full Documentation](./projects/ec_proxy/README.md)** - Complete guide with all commands and features
- **[SDK Documentation](./SDK/README.md)** - Build system and component information
- **[API Reference](./projects/ec_proxy/README.md#supported-hardware-peripherals)** - Modbus register mapping

## Configuration

### Environment Variables

```bash
# RPC socket path (default: ipc:///tmp/rpc.ec_prox)
export AX650C_EC_PROXY_RPC_SOCKET="ipc:///tmp/rpc.ec_prox"

# PUB socket path (default: ipc:///tmp/llm/ec_prox.event.socket)
export AX650C_EC_PROXY_PUB_SOCKET="ipc:///tmp/llm/ec_prox.event.socket"

# Initial RGB LED mode (default: 1)
export AX650_EC_RGB_MODE=1

# Initial LCD display mode (default: 2)
export AX650_EC_LCD_MODE=2

# Auto power-off timer in seconds (default: 30000)
export AX650_EC_POWER_OFF_TIME=30000
```

### Modbus Configuration

- **Serial Port**: `/dev/ttyS3` (configurable in code)
- **Baud Rate**: 921600 bps (auto-negotiable)
- **Data Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Slave Address**: 1

## Troubleshooting

### Common Issues

**1. Compilation Errors**
```bash
# Ensure toolchain is in PATH
which aarch64-none-linux-gnu-gcc

# Update submodules
git submodule update --init --recursive

# Clean and rebuild
scons distclean && scons -j$(nproc)
```

**2. Connection Issues**
```bash
# Check if proxy is running
systemctl status ec-proxy

# Verify serial port
ls -l /dev/ttyS3

# Check socket files
ls -l /tmp/rpc.ec_prox
```

**3. Permission Issues**
```bash
# Add user to dialout group for serial access
sudo usermod -aG dialout $USER

# Set proper permissions
sudo chmod 666 /dev/ttyS3
```

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Reporting Issues

Please report bugs and feature requests via [GitHub Issues](https://github.com/m5stack/Ai_Pyramid_ec_proxy/issues).

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- M5Stack team for hardware platform support
- AXERA for AI acceleration platform
- Open source community for excellent libraries (ZeroMQ, libmodbus, fmt, nlohmann-json)

## Support

- **Documentation**: [M5Stack Docs](https://docs.m5stack.com)
- **Forum**: [M5Stack Community](https://community.m5stack.com)
- **Issues**: [GitHub Issues](https://github.com/m5stack/Ai_Pyramid_ec_proxy/issues)

---

**Note**: This project provides hardware abstraction for embedded controllers. It can be integrated with AI frameworks for advanced applications like voice-controlled hardware or automated system management.
