# Garage Lights Controller - based on Nano RP2040 Connect

Pins:
- PIN D17/A3 - digital out - relay for ceiling light
- PIN D16/A2 - digital in - wall pulse switch
- IC2 - for 2-channel DAC (0-10V) module (DFR0971) to controll LED stripe around garage gate

Features
- Listens on wall-switch and pushes NOTIFY to server via IP/UDP
- Control LED stripe via DAC (0-10V) - including an algorith for smoth transistions
- Control ceiling lightning via relay
- Code implemented as non-blocking event loop to handle concurrency, like light switch triggered during smoth LED transitions

