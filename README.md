# esp32-ds4-roomba
Just messing around with a Roomba 6 series + ESP32-CAM + Dualshock 4

---

### Hardware Connections

Using the "AI Thinker ESP32-CAM" module (camera is irrelevant as it is not used here).

Supply:
- Roomba raw battery voltage on SCI port is ~7.8V (for my model)
- use a DC/DC buck converter from battery voltage to 5V
- connect the 5V to the ESP32 board 5V

Communication:
- a level shifter from 3.3V (ESP32) to 5V (Roomba) does not seem to be required in my case
- ESP32 Tx --> Roomba Rx
- ESP32 GPIO 13 --> Roomba Device Detect input

### What this Arduino Sketch does:

- will wait for a Dualshock 4 controller to connect (with the paired Bluetooth MAC)
- Dualshock Options button --> will pair/unpair the Roomba (it toggles the device detect GPIO and puts the Roomba in "full" mode)
- Dualshock Square button --> turns on/off the white LED on the ESP32-CAM module (flash)
- Dualshock Cross button --> turns vacuum on/off
- Dualshock Left Stick --> up/down motion of a servo motor (to attach another ESP32-CAM for actual video --> TODO)
- Dualshock Right Stick --> drive the Roomba 


Dualshock 4 library will only work with ESP32 library version <= 3.1.3 for Arduino.

