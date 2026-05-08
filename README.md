# Air Monitor
## Status
- PCB Design - Completed
- Components - Arrived
- Firmware - BLE advertisement tested
- Enclosure - In progress - [Here](https://cad.onshape.com/documents/7cc096864f16c057a6e9d4d9/w/91c0470fa62f2376e8363204/e/8dff4a441c052faf6f8fa123)
- Order Status - To be placed (Expected - 08/05/26)


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

