# LoRa and SAMD21 based Garage Controller
This is the Arduino based firmware for my [Garage controller hardware](https://github.com/sebdehne/GarageHeaterController-Hardware).

Features
- LoRa based [secure wireless communication](https://dehnes.com/software/2021/04/18/secure-wireless-communication-for-iot-devices.html) to my [SmartHomeServer server](https://github.com/sebdehne/SmartHomeServer)
- Over-the-air firmware updates
- Switch relay

Interfaces / Pins:

- PIN PA02 / A0 - unused
- PIN PB02 / A1 - Digital output 1 towards impulse wall switch (R4 left unsoldered (open); R3 - bridged (closed))
- PIN PA11 / A2 - Digital input - read back impulse wall switch (from A1) (R6 left unsoldered (open); R5 - bridged (closed))

- PIN PB08 / I2C_SDA - I2C to Hörmann E4 Bridge AND RF-Robot DFR0971 DAC (LED light control around door)
- PIN PB09 / I2C_SLC - 

- PIN PA07 / D4 - Digital output - Relay control - Ceiling light control

- PIN PB22 / UART_TX - LoRa module on board
- PIN PB23 / UART_RX - LoRa module on board

