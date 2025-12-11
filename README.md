# WiFi smart switch

An IoT **smart switch** project that allow remote activation of 2 relay channels with redundancy through different interface solutions. It provides a WiFi connection acessible through: **Telegram bot** commands, HTML plus **Message Queuing Telemetry Transport (MQTT) Broker** or local **webserver**.

The following programs are available:
- [WiFi relay - Telegram](./wifi_relay_telegram)
- [WiFi relay - MQTT](./wifi_relay_mqtt)
- [WiFi relay - webserver](./wifi_relay_server)
- [WiFi relay - Telegram and MQTT](./wifi_relay_mqtt_telegram)
- [WiFi relay - Telegram, MQTT and webserver](./wifi_relay_mqtt_telegram_server)

## Physical experiment

The physical experiment is presented bellow:
<p align="center">
<img width="500" height="395" alt="dc_motor_data_pid" src="https://github.com/user-attachments/assets/1827a471-b4b9-4613-8f13-e94df5b0988c"/>
</p>

The HTML page for local and MQTT commands:
<p align="center">
<img width="500" height="700" alt="dc_motor_data_pid" src="https://github.com/user-attachments/assets/2e39654b-1670-441f-b32a-12a9cbda2a8e"/>
</p>

The Telegram bot interface:
<p align="center">
<img width="500" height="700" alt="dc_motor_data_pid" src="https://github.com/user-attachments/assets/7fe55c68-748c-4a2b-a88b-32b1efe33024"/>
</p>
