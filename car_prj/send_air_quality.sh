#!/bin/bash

PICO_MQTT_TOPIC="air_quality/data"  # 這是您希望從 Raspberry Pi 傳送給 Pico W 的 MQTT 主題
RPI_MQTT_BROKER=$(ip -4 addr show wlan0 | grep -oP '(?<=inet\s)\d+(\.\d+){3}')  # Raspberry Pi 的 MQTT Broker IP
RPI_MQTT_PORT=1883  # 默認 MQTT 端口

while true; do
    echo "Reading /dev/air_quality..."
    if data=$(cat /dev/air_quality 2>/dev/null); then
        echo "Got data: $data"
        
        # 使用 mosquitto_pub 發送資料到 MQTT broker
        mosquitto_pub -h "$RPI_MQTT_BROKER" -p "$RPI_MQTT_PORT" -t "$PICO_MQTT_TOPIC" -m "$data"
        
        echo "Sent data to MQTT topic $PICO_MQTT_TOPIC at $RPI_MQTT_BROKER:$RPI_MQTT_PORT"
    else
        echo "Failed to read from /dev/air_quality"
    fi
    sleep 1
done

