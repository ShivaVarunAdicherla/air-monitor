# Air Monitor
## Status
- PCB Design - Completed
- Components - Arrived
- Firmware - BLE advertisement tested
- Enclosure - In progress - [Here](https://cad.onshape.com/documents/7cc096864f16c057a6e9d4d9/w/91c0470fa62f2376e8363204/e/8dff4a441c052faf6f8fa123)
- Order Status - Placed and Inprogress/Pre-Engineering

# $\mu$​Air
$\mu$​Air is a compact air monitoring device, capable of measuring concentrations of multiple essential local atmospheric parameters like: 
- PM2.5 - Particulate Matter
- Nitrogen Oxides ($NO_x$) 
- Carbon Dioxide ($CO_2$)
- Volatile Organic Compounds (VOCs)
- Temperature and Humidity

Each of these is measured using the following:
- BMV080 - Particulate Matter:
  - Compact - 4.4mm x 3.0mm x 4.4mm
  - $1\frac{\mu g}{m^3}$ PM mass concentration resolution
  - Fanless, uses lasers 
- SGP41 - VOCs and $NO_x$ 
- SCD40 - $CO_2$, temperature and humidity

While each of these parameters is measured and can be monitored/inspected individually, an overall quantitative measure of the ambient atmosphere, the AQI (Air Quality Index), is also calculated using these parameters. 

## User Interaction:
AQI, as well as each of the parameters, individually, can be accessed by the user in two ways:
1. 0.96'' OLED Display on device - Push button to wake
2. HomeAssistant App/Server

## Implementation
<img width="936" height="474" alt="image" src="https://github.com/user-attachments/assets/ced04e90-704b-43b4-b2c4-de0b72340a31" />

## Schematic
<img width="2199" height="1423" alt="image" src="https://github.com/user-attachments/assets/8450c576-bd13-4067-9372-c4b48eab0f4a" />

### Explanation
1. Power
   - Primary Source: 1000mAh Li-Po Battery, or direct power from USB, when connected - Bypass charging handled using a PMOS and a schottky diode, as seen in the schematic.
   - Regulated Power Supply: Buck converter (whose input comes from the output of the bypass charging module - PMOS + Schottky) steps $VIN$ down to $3.3V (V_{DD})$.
   - All other modules, including ESP32-C3 and all the sensors use 3.3V $V_{DD}$ as their power supply.
   - Battery Charging: Handled by DW01 (battery protection IC, inbuilt into the 1000mAh battery), and TC4056, battery charging IC (CC-CV).
2. Sensors
   - BMV080: Measuring PM10, PM2.5 and PM1.0 - Comes in a flex PCB package, with a corresponding female connector placed on the PCB.
   - SCD40: Measuring $CO_2$
   - SGP41: Measuring VOCs and $NO_x$
   - Communication via common I2C bus (except for the BMV080, which has a separate I2C bus) to the ESP32-C3
   - Enclosure designed with suitable openings to allow normal/optimal functioning of all the sensors, as mentioned in their respective application manuals.
3. MCU: ESP32-C3
   - Controls the common I2C bus to receive measurments from the sensors.
   - Handles BLE advertising at regular intervals.
   - Goes into sleep when not broadcasting, saving battery life.
   - Handles the display via the common I2C bus along with a wake button to view parameters on the display.

## PCB Layout
<img width="1170" height="1383" alt="image" src="https://github.com/user-attachments/assets/5ef56341-2813-416e-8e19-cc9f20598bd8" />
<img width="1168" height="1405" alt="image" src="https://github.com/user-attachments/assets/b1f6d486-d7f0-42ec-b303-62904a4ae48a" />
<img width="1159" height="1405" alt="image" src="https://github.com/user-attachments/assets/1c5418f0-02eb-43df-96ec-b4bbf9542ee6" />

## Firmware Status:
### Framework: ESP-IDF, MicroController: ESP32-C3
For Integration with **Home Assistant** without any other extra configuration for BLE based data broadcast devices, there's an open standard called **BTHome**.
The Second version of the standard has been released quite recently, and the official **BTHOME** component from Espressif doesn't seem to work because it has probably been **depreciated**.

Right now, the code initializes Bluetooth's BLE Stack by Apache Foundation (NimBLE), and starts advertising packets with an interval of 450ms (We've decided on
this rate to save power, while maintaining the balance of packet reception). Right now, since actually don't have the sensor data, we're just generating a random
number/data then building the advertisement packet and sending $\approx 16$ packets in a task loop.

**Note:** We're are building our own implementation of *BTHOME* which sends the sensor data, in the service region of the advertisement packet.

Since ESP-IDF has a native port of FreeRTOS, we plan to take advantage of it's powerful scheduling capabilities to make it easier to organise and build modules
which perform certain tasks. The flowchart will look something like this.

<img width="1264" height="964" alt="image" src="https://github.com/user-attachments/assets/2dbde7b3-c3be-4cbb-8722-3ff323e7405e" />

