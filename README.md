# LoRa and SAMD21 based Garage Lights Controller
This is the Arduino based firmware for my [Garage controller hardware](https://github.com/sebdehne/GarageHeaterController-Hardware).

Features
- LoRa based [secure wireless communication](https://dehnes.com/software/2021/04/18/secure-wireless-communication-for-iot-devices.html) to my [SmartHomeServer server](https://github.com/sebdehne/SmartHomeServer)
- Listens on wall-switch and pushes NOTIFY to server
- Control LED stripe via DAC (0-10V) - including an algorith for smoth transistions
- Control ceiling lightning via relay
- code implemented as non-blocking event loop to handle concurrency, like light switch triggered during smoth LED transitions

Interfaces / Pins:

Changes made to [board](https://github.com/sebdehne/GarageHeaterController-Hardware) (Binds J6-2 to GND and J6-1 to PIN PB02):
- R4: pull-down (300 Ohm);
- R3: bridged (closed);
- Q2: left unsoldered (open);

- PIN PA17 / D13 - PCB LED
- PIN PB02 / A1/D15 - Digital input - when (impulse) wall switch closes, it pulls D15 down

- PIN PB08 / I2C_SDA - I2C to RF-Robot DFR0971 DAC (LED light control around door)
- PIN PB09 / I2C_SLC - 

- PIN PA07 / D4 - Digital output - Relay control - Ceiling light control

- PIN PB22 / UART_TX - LoRa module on board
- PIN PB23 / UART_RX - LoRa module on board

