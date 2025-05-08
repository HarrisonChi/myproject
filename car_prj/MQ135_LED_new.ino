#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <DHT.h>
#include <PubSubClient.h>  // MQTT 函式庫

// ---------- 基本設定 ----------
#define LED_PIN    2
#define NUM_LEDS   8
#define UDP_PORT   5005
#define DHTPIN     4
#define DHTTYPE    DHT11

// ---------- 建立感測與通訊物件 ----------
DHT dht(DHTPIN, DHTTYPE);
WiFiUDP Udp;
WiFiClient espClient;
PubSubClient mqttClient(espClient);  // MQTT 用的 client
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- WiFi 與 UDP 設定 ----------
const char* ssid = "iSpan-R206";
const char* password = "66316588";
IPAddress serverIP(192, 168, 56,153);     // Node.js Server IP
unsigned int serverPort = 5006;          // Node.js 監聽 UDP 的 Port

// ---------- MQTT 設定 ----------
const char* mqtt_server = "192.168.56.153";   // Mosquitto broker IP
//const char* topic = "air_quality/data";      // RPi 發送的主題

// ---------- 狀態與數據 ----------
unsigned long lastDHTRead = 0;
float temperature = 0;
float humidity = 0;
int co2 = 0, nh3 = 0, alcohol = 0, co = 0;

bool autoMode = true;

// ---------- 將資料傳送給 Node.js ----------
void sendDataToWeb(int co2, int nh3, int alcohol, int co, float t, float h) {
  char data[128];
  snprintf(data, sizeof(data), "CO2=%d&NH3=%d&Alcohol=%d&CO=%d&T=%.1f&H=%.1f", co2, nh3, alcohol, co, t, h);
  Udp.beginPacket(serverIP, serverPort);
  Udp.write(data);
  Udp.endPacket();
}

// ---------- MQTT 回呼函式，當 Pico 收到訊息時執行 ----------
void callback(char* topic, byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.print("MQTT Message from topic [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

if (strcmp(topic, "air_quality/data") == 0) {
    // 處理 air_quality/data 主題傳來的數據
    if (sscanf(message, "CO2=%d&NH3=%d&Alcohol=%d&CO=%d", &co2, &nh3, &alcohol, &co) == 4) {
      Serial.printf("CO2: %d  NH3: %d  Alcohol: %d  CO: %d\n", co2, nh3, alcohol, co);
      if (autoMode) {
        controlLED(co2, nh3, alcohol, co);
      }
    }
  }
  else if (strcmp(topic, "picow/control") == 0) {
    // 處理 picow/control 主題傳來的指令
    if (strcmp(message, "turnon") == 0) {
      lightAllWhite();
      autoMode = false;
    }
    else if (strcmp(message, "air auto") == 0) {
      autoMode = true;
    }
    else if (strcmp(message, "camera") == 0) {
      lightAllWhite();
      autoMode = false;
      delay(5000);
      autoMode = true;
      turnOffLEDs();
    }
    else if (strcmp(message, "help") == 0) {
      lightAllRed();
      autoMode = false;
      delay(5000);
      autoMode = true;
      turnOffLEDs();
    }
    else if (strcmp(message, "turnoff") == 0) {
      turnOffLEDs();
      autoMode = false;
    }
  }
}

// ---------- 設定 MQTT ----------
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("PicoClient")) {
      Serial.println("connected.");
      mqttClient.subscribe("air_quality/data");
      mqttClient.subscribe("picow/control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 2 seconds");
      delay(2000);
    }
  }
}

// ---------- 初始化 ----------
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqtt_server, 1883);  // 設定 MQTT broker IP
  mqttClient.setCallback(callback);         // 設定回呼函式

  strip.begin();
  turnOffLEDs();
  strip.show();

  Udp.begin(UDP_PORT);
  dht.begin();
}

// ---------- 主程式循環 ----------
void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();  // 處理 MQTT 收到的訊息

  // 每 2 秒讀取一次溫溼度
  if (millis() - lastDHTRead > 2000) {
    lastDHTRead = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      temperature = t;
      humidity = h;
      Serial.printf("Temperature: %.1f °C  Humidity: %.1f %%\n", temperature, humidity);
    } else {
      Serial.println("Failed to read from DHT sensor!");
    }
  }

  sendDataToWeb(co2, nh3, alcohol, co, temperature, humidity);  // 一樣透過 UDP 傳送給 Node.js
}

// ---------- 控制 LED 顏色 ----------
void controlLED(int co2, int nh3, int alcohol, int co) {
  int color;

  // CO2
  if (co2 > 1000) color = strip.Color(255, 0, 0);
  else if (co2 > 400) color = strip.Color(255, 0, 255);
  else if (co2 > 360) color = strip.Color(255, 255, 0);
  else color = strip.Color(0, 255, 0);
  strip.setPixelColor(0, color);
  strip.setPixelColor(1, color);

  // NH3
  if (nh3 > 100) color = strip.Color(255, 0, 0);
  else if (nh3 > 50) color = strip.Color(255, 0, 255);
  else if (nh3 > 25) color = strip.Color(255, 255, 0);
  else color = strip.Color(0, 255, 0);
  strip.setPixelColor(2, color);
  strip.setPixelColor(3, color);

  // Alcohol
  if (alcohol > 1000) color = strip.Color(255, 0, 0);
  else if (alcohol > 500) color = strip.Color(255, 0, 255);
  else if (alcohol > 100) color = strip.Color(255, 255, 0);
  else color = strip.Color(0, 255, 0);
  strip.setPixelColor(4, color);
  strip.setPixelColor(5, color);

  // CO
  if (co > 100) color = strip.Color(255, 0, 0);
  else if (co > 35) color = strip.Color(255, 0, 255);
  else if (co > 9) color = strip.Color(255, 255, 0);
  else color = strip.Color(0, 255, 0);
  strip.setPixelColor(6, color);
  strip.setPixelColor(7, color);

  strip.show();
}

// ---------- 所有燈設為白色 ----------
void lightAllWhite() {
  Serial.println("Setting all LEDs to white...");
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
}
// ---------- 所有燈設為白色 ----------
void lightAllRed() {
  Serial.println("Setting all LEDs to red...");
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
}
// ---------- 關閉所有 LED ----------
void turnOffLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}
