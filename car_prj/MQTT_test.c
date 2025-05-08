#include "MQTTClient.h"
#include <string.h>
#include <unistd.h>   // 為了 sleep()

#define ADDRESS     "tcp://192.168.51.100:1883"
#define CLIENTID    "rpi_pub"
#define TOPIC       "air/quality"
#define QOS         1
#define TIMEOUT     10000L

int main() {
    MQTTClient client;
    MQTTClient_create(&client, ADDRESS, CLIENTID,
                      MQTTCLIENT_PERSISTENCE_NONE, NULL);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_connect(client, &conn_opts);

    while (1) {
        const char* PAYLOAD = "CO2: 420ppm"; // 假資料，可用 sprintf() 改動
        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        pubmsg.payload = (void*)PAYLOAD;
        pubmsg.payloadlen = strlen(PAYLOAD);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;

        MQTTClient_deliveryToken token;
        MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
        MQTTClient_waitForCompletion(client, token, TIMEOUT);

        printf("送出: %s\n", PAYLOAD);

        sleep(5);  // 每 5 秒傳一次
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}

