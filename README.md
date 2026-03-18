To get the project up and running, follow these steps:

1. Configure the `src/secrets.h` file with your Wi-Fi credentials and MQTT broker details.

2. Start the Mosquitto MQTT broker using the following command:

```bash
mosquitto -c local.conf -v
```

3. Start the simulation on Wokwi.

4. Open the `index.html` file in your preferred web browser to visualize the parking slot status in real-time.