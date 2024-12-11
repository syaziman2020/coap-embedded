#include <WiFi.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi
const char* ssid = "Siskom4";
const char* password = "rahasia123";

// Konfigurasi Server CoAP
IPAddress serverIP(192, 168, 8, 114);
const int serverPort = 5683;

// Inisialisasi UDP dan CoAP
WiFiUDP udp;
Coap coap(udp);

// Node ID dan sequence
const int nodeId = 1;
int sequence = 1;

// Variabel untuk metrics
unsigned long startTime = 0;
unsigned long endTime = 0;
int totalPayloadSize = 0;
float lastDelay = 0;
float lastThroughput = 0;

// Fungsi untuk mengirim data ke endpoint tertentu
void sendCoapPost(const char* endpoint, const char* jsonData) {
  Serial.println(jsonData);
  coap.send(
    serverIP,
    serverPort,
    endpoint,
    COAP_CON,
    COAP_POST,
    NULL,
    0,
    (uint8_t*)jsonData,
    strlen(jsonData),
    COAP_APPLICATION_JSON);
}

// Callback untuk response dari server
void responseHandler(CoapPacket& packet, IPAddress ip, int port) {
  if (packet.code == COAP_CREATED) {
    Serial.println("Data sent successfully");
  } else {
    Serial.print("Error: ");
    Serial.println(packet.code);
  }
}

void setup() {
  Serial.begin(115200);

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start CoAP
  coap.start();
  coap.response(responseHandler);

  delay(1000);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Mulai mengukur waktu
    startTime = millis();
    Serial.print("Processing sequence: ");
    Serial.println(sequence);

    // Generate random sensor values
    float current = random(200, 300) / 100.0;           // 2.00-3.00A
    float voltage = random(220, 240) / 100.0;           // 220-240V
    float power = random(180, 220) / 100.0;             // 180-220W
    float powerConsumption = random(140, 160) / 100.0;  // 140-160W
    float temperature = random(2400, 2800) / 100.0;     // 24-28°C
    float humidity = random(5500, 6500) / 100.0;        // 55-65%
    float moisture1 = random(2800, 3200) / 100.0;       // 28-32%
    float moisture2 = random(3800, 4200) / 100.0;       // 38-42%

    // 1. Kirim node log dengan metrics dari sequence sebelumnya
    StaticJsonDocument<128> logDoc;
    char logBuffer[128];

    logDoc["n"] = nodeId;
    logDoc["s"] = sequence;
    if (lastDelay > 0) {
      logDoc["d"] = lastDelay;
      logDoc["p"] = totalPayloadSize;
      logDoc["t"] = lastThroughput;
    }
    totalPayloadSize = 0;

    // Hitung size menggunakan measureJson
    totalPayloadSize += measureJson(logDoc);
    serializeJson(logDoc, logBuffer);
    sendCoapPost("sensor/log", logBuffer);
    delay(2000);


    // 2. Kirim INA data
    StaticJsonDocument<128> inaDoc;
    char inaBuffer[128];

    inaDoc["n"] = nodeId;
    inaDoc["s"] = sequence;
    inaDoc["c"] = current;
    inaDoc["v"] = voltage;
    inaDoc["p"] = power;
    inaDoc["w"] = powerConsumption;

    // Hitung size INA data
    totalPayloadSize += measureJson(inaDoc);
    serializeJson(inaDoc, inaBuffer);
    sendCoapPost("sensor/ina", inaBuffer);
    delay(2000);

    // 3. Kirim DHT data
    StaticJsonDocument<128> dhtDoc;
    char dhtBuffer[128];

    dhtDoc["n"] = nodeId;
    dhtDoc["s"] = sequence;
    dhtDoc["t"] = temperature;
    dhtDoc["h"] = humidity;

    // Hitung size DHT data
    totalPayloadSize += measureJson(dhtDoc);
    serializeJson(dhtDoc, dhtBuffer);
    sendCoapPost("sensor/dht", dhtBuffer);
    delay(2000);

    // 4. Kirim soil moisture data
    StaticJsonDocument<128> soilDoc;
    char soilBuffer[128];

    soilDoc["n"] = nodeId;
    soilDoc["s"] = sequence;
    JsonArray m = soilDoc.createNestedArray("m"); 

    JsonObject soil1 = m.createNestedObject();
    soil1["o"] = 1;
    soil1["v"] = moisture1;

    JsonObject soil2 = m.createNestedObject();
    soil2["o"] = 2;
    soil2["v"] = moisture2;

    // Hitung size soil moisture data
    totalPayloadSize += measureJson(soilDoc);
    serializeJson(soilDoc, soilBuffer);
    sendCoapPost("sensor/moisture", soilBuffer);
    delay(2000);

    // Process responses
    for (int i = 0; i < 8; i++) {
      coap.loop();
      delay(250);
    }

    // Hitung metrics untuk sequence ini
    endTime = millis();
    lastDelay = endTime - startTime - 8000;  // dalam milliseconds
    // Throughput = (total bytes * 8 bits/byte) / (delay in seconds)
    lastThroughput = (totalPayloadSize * 8.0) / (lastDelay);  // bits per second

    Serial.println("Metrics for this sequence:");
    Serial.print("Total Payload Size: ");
    Serial.print(totalPayloadSize);
    Serial.println(" bytes");
    Serial.print("Delay: ");
    Serial.print(lastDelay);
    Serial.println(" ms");
    Serial.print("Throughput: ");
    Serial.print(lastThroughput);
    Serial.println(" bps");

    // Increment sequence untuk pengiriman berikutnya
    sequence++;

    // Tunggu sebelum pengiriman berikutnya
    delay(5000);
  } else {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }
}