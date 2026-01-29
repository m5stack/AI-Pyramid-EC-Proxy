# EC Interaction Protocol

## 1. EC Communication Protocol

1. Communication between the host and EC uses the **Modbus Protocol**.  
2. Host Address: `0x01`  
   Default Baud Rate: `921600 bps`  

---

## 2. EC Internal Logic Description

1. **RGB Indicator Light Logic**  
   - When powered on, the RGB light below the button displays red by default.  
   - After system initialization is complete, the light automatically switches to the user-configured color.

2. **Download Mode**  
   - When the host is powered on, if the top button is pressed, the device enters download mode and the light displays the corresponding mode effect.

3. **LCD Display Logic**  
   - At initial startup, the screen displays the "M5STACK" logo, and after about 15 seconds it switches to display content based on the current mode.  
   - Press the button next to the LCD screen to switch display modes; long-press for 3 seconds to change the display method.

4. **Wake-up Mechanism**  
   - When the internal 5V main power is shut down, the device can be woken up by pressing the top button, or auto-wake can be enabled by setting the timed wake-up register.

5. **Flash Operation Description**  
   - Each save operation triggers a Flash erase process (taking approximately 20 ms); avoid frequent writes to extend Flash lifespan.

---

## 3. Register Table Description

### 3.1 Coil Registers

**Description:** Writing `1` means enable, writing `0` means disable.

| Type/ID | Address | Power-off Save | Pin Mapping | Function Description                                                                                                                                                                                                              |
| :------ | :-----: | :------------- | :---------: | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Coil 1  |    1    | NO             |    PA11     | External 5V main power control. When disabled, coils 3–14 are forced to zero; when enabled, status is restored based on Flash. **High level active**, enabled by default on power-up.                                             |
| Coil 2  |    2    | NO             |    PA15     | Internal 5V main power control. Writing 0 will delay shutdown based on holding register 1 (restarting during countdown will cancel it). **High level active**, supports button and timed wake-up, enabled by default on power-up. |
| Coil 3  |    3    | NO             |     NC      | 0: NGFF PCIE clock follows external main power; 1: Clock output controlled by coil 4. Default 0.                                                                                                                                  |
| Coil 4  |    4    | YES            |    PB10     | NGFF PCIE clock output control. **High level active**. When coil 3 is 1, output follows coil 4 control; otherwise follows external main power.                                                                                    |
| Coil 5  |    5    | YES            |     PB2     | PCIE1 power supply control. **High level enables**, enabled by default.                                                                                                                                                           |
| Coil 6  |    6    | YES            |     PB4     | PCIE2 power supply control. **High level enables**, enabled by default.                                                                                                                                                           |
| Coil 7  |    7    | YES            |    PB11     | GL3510 chip reset control. **High level active**, enabled by default.                                                                                                                                                             |
| Coil 8  |    8    | YES            |    PB12     | DS1 interface high current mode control. **High level enables**, enabled by default.                                                                                                                                              |
| Coil 9  |    9    | YES            |    PB15     | DS2 interface high current mode control. **High level enables**, enabled by default.                                                                                                                                              |
| Coil 10 |   10    | YES            |    PB13     | DS1 power control. **Low level active**, enabled by default.                                                                                                                                                                      |
| Coil 11 |   11    | YES            |    PB14     | DS2 power control. **Low level active**, enabled by default.                                                                                                                                                                      |
| Coil 12 |   12    | YES            |    PC11     | DS3 power control. **Low level active**, enabled by default.                                                                                                                                                                      |
| Coil 13 |   13    | YES            |    PC14     | Grove Port1 power control. **Low level active**, enabled by default.                                                                                                                                                              |
| Coil 14 |   14    | YES            |     PB7     | Grove Port3 power control. **Low level active**, enabled by default.                                                                                                                                                              |
| Coil 15 |   15    | NC             |     NC      | Writing `1` saves coils 4–14 status to Flash; supports write operation only.                                                                                                                                                      |
| Coil 16 |   16    | NC             |     NC      | Writing `1` saves holding registers 1, 2, 11, 12, 14, 16–21 to Flash; supports write operation only.                                                                                                                              |
| Coil 17 |   17    | NC             |     NC      | Writing `1` executes shutdown command, closing internal and peripheral 5V power. Supports button and timed wake-up. Write operation only.                                                                                         |
| Coil 18 |   18    | NC             |     NC      | System ready flag. Write operation only.                                                                                                                                                                                          |

---

### 3.2 Discrete Input Registers

| Type/ID  | Address | Power-off Save | Pin Mapping | Function Description                                                                     |
| :------- | :-----: | :------------- | :---------: | :--------------------------------------------------------------------------------------- |
| Input 1  |    1    | NC             |    PA12     | PCIE1 presence signal. 1 indicates present (low level), 0 indicates absent (high level). |
| Input 2  |    2    | NC             |     PB3     | PCIE2 presence signal. 1 indicates present (low level), 0 indicates absent (high level). |
| Input 3  |    3    | NC             |     PB6     | 3.3V power good signal: 1 normal (high level), 0 abnormal.                               |
| Input 4  |    4    | NC             |     PF1     | 1.8V power good signal: 1 normal (high level), 0 abnormal.                               |
| Input 5  |    5    | NC             |    PC10     | Top button input: 1 pressed (low level), 0 not pressed.                                  |
| Input 6  |    6    | NC             |     NC      | Top button falling edge event: 1 when triggered, auto-reset to 0 after read.             |
| Input 7  |    7    | NC             |     NC      | Top button rising edge event: 1 when triggered, auto-reset to 0 after read.              |
| Input 8  |    8    | NC             |     NC      | Top button long-press event: 1 when triggered, auto-reset to 0 after read.               |
| Input 9  |    9    | NC             |     PC6     | LCD button input: 1 pressed (low level), 0 not pressed (high level).                     |
| Input 10 |   10    | NC             |     NC      | LCD button falling edge event: 1 when triggered, auto-reset to 0 after read.             |
| Input 11 |   11    | NC             |     NC      | LCD button rising edge event: 1 when triggered, auto-reset to 0 after read.              |
| Input 12 |   12    | NC             |     NC      | LCD button long-press event: 1 when triggered, auto-reset to 0 after read.               |

---

### 3.3 Input Registers

| Type/ID  | Address | Power-off Save | Pin Mapping | Function Description                          |
| :------- | :-----: | :------------- | :---------: | :-------------------------------------------- |
| Input 1  |    1    | NC             |     NC      | PCIE1 Voltage (mV)                            |
| Input 2  |    2    | NC             |     NC      | PCIE1 Current (mA)                            |
| Input 3  |    3    | NC             |     NC      | PCIE2 Voltage (mV)                            |
| Input 4  |    4    | NC             |     NC      | PCIE2 Current (mA)                            |
| Input 5  |    5    | NC             |     NC      | USB1 Voltage (mV)                             |
| Input 6  |    6    | NC             |     NC      | USB1 Current (mA)                             |
| Input 7  |    7    | NC             |     NC      | USB2 Voltage (mV)                             |
| Input 8  |    8    | NC             |     NC      | USB2 Current (mA)                             |
| Input 9  |    9    | NC             |     NC      | Internal Voltage (mV)                         |
| Input 10 |   10    | NC             |     NC      | Internal Current (mA)                         |
| Input 11 |   11    | NC             |     NC      | External Voltage (mV)                         |
| Input 12 |   12    | NC             |     NC      | External Current (mA)                         |
| Input 13 |   13    | NC             |     NC      | Fan Speed (RPM), `0xFFFF` indicates abnormal. |
| Input 14 |   14    | NC             |     NC      | Firmware Version Number (e.g., `0xF1`).       |

---

### 3.4 Holding Registers

| Type/ID          | Address | Power-off Save | Pin Mapping | Function Description                                                                                                                                                                         |
| :--------------- | :-----: | :------------- | :---------: | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Register 1       |    1    | YES            |     NC      | Main power delayed shutdown time (ms), range 1000–65535, default 1000. Values <1000 are invalid.                                                                                             |
| Register 2       |    2    | YES            |     NC      | Fan target speed percentage, range 0–100%, default 100%. Invalid values are ignored.                                                                                                         |
| Register 3       |    3    | YES            |     NC      | VDD_CPU(1.0V) voltage offset (mV): range 0–40, step 5mV, default 20. Actual voltage = 1.0V + (offset – 20)×0.005V, range 0.9–1.1V. Saved immediately after setting, effective after restart. |
| Register 4       |    4    | NO             |     NC      | Serial baud rate high byte.                                                                                                                                                                  |
| Register 5       |    5    | NO             |     NC      | Serial baud rate low byte.                                                                                                                                                                   |
| Register 6       |    6    | NO             |     NC      | Timed wake-up time high byte (seconds), `0` to disable.                                                                                                                                      |
| Register 7       |    7    | NO             |     NC      | Timed wake-up time low byte (seconds), `0` to disable.                                                                                                                                       |
| Register 8       |    8    | NO             |     NC      | I2C command address register, high byte is device address, low byte is register address.                                                                                                     |
| Register 9       |    9    | NO             |     NC      | I2C read command register. Returns data on success, `0xFFFF` on failure.                                                                                                                     |
| Register 10      |   10    | NO             |     NC      | I2C write command register. Returns 0 on success, `0xFFFF` on failure.                                                                                                                       |
| Register 11      |   11    | YES            |     NC      | RGB LED count, default 48, range 1–64.                                                                                                                                                       |
| Register 12      |   12    | YES            |     NC      | RGB mode: 0=ARM mapping, 1=gradient, 2=flashing, 3=flowing, 4=download mode, 5=audio capture mode, default 0, invalid values are ignored.                                                    |
| Register 13      |   13    | NO             |     NC      | LCD brightness, default `0xCF`, range `0x00`–`0xFF`. Invalid values are ignored.                                                                                                             |
| Register 14      |   14    | YES            |     NC      | LCD display mode: 0=M5Stack Log, 1=Voltage display (3s refresh), 2=IP display (3 IPs, 3s refresh), 3=ARM mapping, 4=Character display mode. Default 0.                                       |
| Register 15      |   15    | NO             |     NC      | Character display mapping area. Supports ASCII range 0x20–0x7E; supports 0x7F clear screen, 0x0A line feed.                                                                                  |
| Register 16      |   16    | YES            |     NC      | eth0 host IP address high byte.                                                                                                                                                              |
| Register 17      |   17    | YES            |     NC      | eth0 host IP address low byte.                                                                                                                                                               |
| Register 18      |   18    | YES            |     NC      | eth1 host IP address high byte.                                                                                                                                                              |
| Register 19      |   19    | YES            |     NC      | eth1 host IP address low byte.                                                                                                                                                               |
| Register 20      |   20    | YES            |     NC      | wlan host IP address high byte.                                                                                                                                                              |
| Register 21      |   21    | YES            |     NC      | wlan host IP address low byte.                                                                                                                                                               |
| Register 64–127  | 64–127  | NO             |     NC      | RGB1 LED RAM mapping area (RGB 565).                                                                                                                                                         |
| Register 256–511 | 256–511 | NO             |     NC      | LCD RAM mapping area.                                                                                                                                                                        |

---