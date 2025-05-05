# Rocket Switch Reaction Timer

This repository contains code for a wireless reaction timer system using ESP32 modules and ESP-NOW communication. The system operates in two modes:

- **Light Mode**: Player starts timing by pressing a color button; Controller stops timing when Player releases their LED; timing results displayed on both ends.
- **Sound Mode**: Controller starts timing and triggers a buzzer on the Player; Player stops timing with a stop button; timing results displayed on both ends.

## Repository Structure
```
/Controller
  └── controller.ino   # ESP32 code for controller device
/Player
  └── player.ino       # ESP32 code for player device
README.md              # Project documentation
```  

## Features
- **Wireless communication** via ESP-NOW (no Wi-Fi network required)
- **Dual modes**: Light Mode for LED-based reaction timing; Sound Mode for buzzer-based reaction timing
- **Real-time LCD display** on Controller (I2C 16×2)
- **Edge detection** for buttons to prevent bouncing
- Automatic display of "Ready for next round" after each timing session

## Hardware Requirements
- 2 × ESP32 development boards
- 1 × I2C LCD 16×2 module (for Controller; Player LCD optional)
- Push buttons:
  - Controller: Mode switch (ROCKER), Start button
  - Player: 3 color buttons (RED, YELLOW, GREEN), Stop button
- LEDs (for Player indicator in Light Mode)
- Active-LOW buzzer (in Sound Mode)
- Jumper wires, breadboard

## Wiring
### Controller
```
ESP32       | Component
------------|------------------------
GPIO14      | Mode switch (ROCKER)
GPIO27      | Start button
GPIO32,13,15| LEDs for Light Mode
GPIO5       | Light mode indicator LED
GPIO33      | Sound mode indicator LED
I2C SDA     | LCD SDA (e.g. GPIO21)
I2C SCL     | LCD SCL (e.g. GPIO22)
GND         | LCD GND, buttons, switches
3.3V        | LCD VCC, pull-ups
```  

### Player
```
ESP32       | Component
------------|--------------------
GPIO33,25,26| Color buttons (RED,YELLOW,GREEN)
GPIO23      | Stop button (Sound Mode)
GPIO22      | Buzzer (active-LOW)
GPIO18,19,21| LEDs for Light Mode indicator
GND         | Buzzer GND, buttons
3.3V        | Buttons pull-ups
```

## Getting Started
1. Clone this repository:
   ```sh
   git clone https://github.com/yourusername/rocket-switch-reaction-timer.git
   cd rocket-switch-reaction-timer
   ```
2. Install the **ESP32** board support in Arduino IDE.
3. Install libraries:
   - `LiquidCrystal_I2C`
   - `esp32` (ESP32 board support)
4. Open `Controller/controller.ino`, adjust the peer MAC address to match your Player.
5. Upload `controller.ino` to the first ESP32 (Controller).
6. Open `Player/player.ino`, set the Controller's MAC address.
7. Upload `player.ino` to the second ESP32 (Player).

## Usage
1. Power both ESP32 boards.
2. On Controller, flip Mode switch to select **Light** or **Sound** mode.
3. **Light Mode**:
   - Controller LED indicates mode.
   - Player presses one of the 3 color buttons to start.
   - Controller LED lights corresponding color and LCD shows timing.
   - Player releases button to stop; Controller displays result.
   - Player manually turns off LED; Controller shows "Ready for next round".
4. **Sound Mode**:
   - On Controller, press Start to trigger buzzer on Player.
   - Player presses Stop button to stop buzzer; both ends display timing.
   - Controller LCD shows "Ready for next round".

## Troubleshooting
- **ESP-NOW init failed**: Ensure `WiFi.mode(WIFI_STA)` and unique MAC addresses.
- **Button bounce**: Debounce by hardware (capacitor) or adjust `delay(50)`.
- **LCD not displaying**: Confirm I2C address (0x27) and correct SDA/SCL pins.
- **Communication issues**: Check line-of-sight and same channel (default 1).

## License
This project is licensed under the MIT License. See LICENSE for details.
