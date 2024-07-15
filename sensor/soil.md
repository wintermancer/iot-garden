# iot-garden - Let's turn the garden into an I(o)T problem!
## soil sensor documentation
Fritzing diagram

![A fritzing showing the Soilsensor cabling](Soilsensor.png?raw=true)

### Components used
1. 10x24 PCB
2. D1 mini v3
4. Capacitive soil moisture sensor v1.2 
5. TP4056 lithium battery charge controller
6. 350mAh AAA lithium battery
7. MCP1700-3302E TO92 LDO  3.3V / 250mA
8. 10µF electrolytic capacitor
9. Ceramic capacitor
10. Cables

### Specific config
* Chip is running on 160MHz. When running on 80MHz adjust runtime calculation
* MCP1700-3302E might be an issue depending on your sensor loadout.  Working fine for me so far. Read paragraph 2 here https://arduinodiy.wordpress.com/2020/01/18/very-deepsleep-and-energy-saving-on-esp8266/




back to main page https://github.com/wintermancer/iot-garden/blob/main/README.md
