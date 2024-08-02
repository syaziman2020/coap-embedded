#include <WiFi.h>
#include <coap-simple.h>
#include <ArduinoJson.h>

// Ganti dengan nama jaringan WiFi dan kata sandi Anda
const char* ssid = "flower";
const char* password = "2024skom";

// Alamat server CoAP
// 192.168.96.169
IPAddress coap_server(192, 168, 96, 169); // Ganti dengan IP atau domain server CoAP Anda
const uint16_t coap_port = 5683; // Port default untuk CoAP

// Pin untuk sensor ultrasonik
#define TRIG_PIN 25
#define ECHO_PIN 26

WiFiUDP udp;
Coap coap(udp);

void setup() {
  // Inisialisasi serial monitor
  Serial.begin(115200);

  // Inisialisasi pin sensor ultrasonik
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Menghubungkan ke jaringan WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Menghubungkan ke WiFi...");
  }
  Serial.println("Terhubung ke WiFi");

   // Inisialisasi CoAP
  coap.start();
}

void loop() {
  // Mengirimkan pulsa trigger
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Membaca durasi pulsa echo
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Menghitung jarak dalam cm
  float distance = (duration / 2.0) * 0.0343;

  // Menampilkan jarak ke Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Membuat payload JSON untuk CoAP
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["sensor"] = "ultrasonic";
  jsonDoc["distance"] = distance;
  char payload[200];
  serializeJson(jsonDoc, payload);


  // Mengirimkan data menggunakan CoAP
   int response = coap.put(coap_server, coap_port, "sensor/ultrasonic", payload, strlen(payload));
   Serial.println("ini response : ");
  Serial.println(response);
  // Delay untuk memberikan jeda pembacaan
  delay(5000);
}
