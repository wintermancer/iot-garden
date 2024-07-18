# iot-garden - Let's turn the garden into an I(o)T problem!
Architecture diagram:

![An architecture diagram showing sensors in the garden, REDIS, HTTPD and MQTT on a Raspberry Pi and CloudWatch, DynamoDB and Lambda in an AWS Region](docs/IoT-Diagram.png?raw=true)

Disclaimer: This project is learning by doing. Be advised that you use all code / ideas / instructions from this repository at your own risk

## Project goals
1. Build wireless standalone sensor for long term soil moisture monitoring.
2. Build wireless standalone weather station for long term weather monitoring.
4. Build wireless standalone irrigation control to water plants based on soil moisture and weather data.
5. Integrate with free tier public cloud services for fancy features like automatic calibration.

### Part specific documentation
* Soil sensor https://github.com/wintermancer/iot-garden/blob/main/docs/soil.md
