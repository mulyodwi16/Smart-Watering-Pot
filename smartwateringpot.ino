#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

// Wi-Fi settings
char custom_ssid[32] = "";
char custom_password[32] = "";

// MQTT settings
const char* mqtt_server = "core-mosquitto";  // Ganti dengan alamat IP atau nama host broker MQTT
const int mqtt_port = 1883;
const char* mqtt_username = "homeassistant";       // Ganti sesuai dengan username yang Anda konfigurasi di broker MQTT
const char* mqtt_password = "xaexae1ongahdisae2raingaeYao6queigh8Xai6beihohYaeshie6cheida2tu7";  // Ganti sesuai dengan password yang Anda konfigurasi di broker MQTT

// DHT settings
#define DHTPIN 15  // Pin GPIO untuk sensor DHT11
#define DHTTYPE DHT11
#define relayPin 5
#define relayOff 0
#define relayOn 1

DHT dht(DHTPIN, DHTTYPE);

int soilMoistureValue = 0;
int soilmoisturepercent = 0;
// Soil Moisture sensor settings
int soilMoisturePin = 34;  // Ganti dengan pin GPIO yang sesuai

float waterValue = 100;
float airValue = 4000;

int pumpStatus = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void saveConfigCallback() {
  Serial.println("Saving custom parameters to flash");
  strcpy(custom_ssid, WiFi.SSID().c_str());
  strcpy(custom_password, WiFi.psk().c_str());
}

void setup() {
  Serial.begin(115200);
  WiFiManager wifiManager;

  // Uncomment the following line for a fresh start (clears previously saved credentials)
  // wifiManager.resetSettings();

  // Set callback to save custom parameters
  wifiManager.setSaveParamsCallback(saveConfigCallback);

  // Try to connect to saved Wi-Fi credentials; if not found, enter configuration mode
  if (!wifiManager.autoConnect("ESP32-Config")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    // Reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  // If you get here, you have connected to the Wi-Fi
  Serial.println("Connected to Wi-Fi");

  // Update MQTT settings with dynamic credentials
  strcpy(mqtt_username, custom_ssid);
  strcpy(mqtt_password, custom_password);

  dht.begin();
  pinMode(relayPin, OUTPUT);

  // Menghubungkan ke broker MQTT
  client.setServer(mqtt_server, mqtt_port);

  while (!client.connected()) {
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Terhubung ke broker MQTT");
    } else {
      Serial.print("Gagal terhubung ke broker MQTT, coba lagi dalam 5 detik...");
      delay(5000);
    }
  }
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int soilMoistureValue = analogRead(soilMoisturePin);
  soilmoisturepercent = map(soilMoistureValue, waterValue, airValue, 100, 0);

  Serial.print("Kelembaban: ");
  Serial.print(humidity);
  Serial.print("%  Suhu: ");
  Serial.print(temperature);
  Serial.print("°C  Kelembaban Tanah: ");
  Serial.print(soilmoisturepercent);
  Serial.println("%");

  // Kirim data ke topik MQTT
  char humidityStr[6];
  char temperatureStr[6];
  char soilMoistureStr[4];
  dtostrf(humidity, 4, 2, humidityStr);
  dtostrf(temperature, 4, 2, temperatureStr);
  itoa(soilmoisturepercent, soilMoistureStr, 10);

  client.publish("openhab/sensor/humidity", humidityStr);
  client.publish("openhab/sensor/temperature", temperatureStr);
  client.publish("openhab/sensor/soil_moisture", soilMoistureStr);

  if (soilmoisturepercent < 45 && pumpStatus == 0) {
    pumpStatus = 1;
    digitalWrite(relayPin, relayOn);
    Serial.println("Relay menyala");
    client.publish("openhab/relay/status", "nyala");
    client.publish("openhab/scheduler/irrigation/on", "1");
  } else if (soilmoisturepercent >= 45 && pumpStatus == 1) {
    pumpStatus = 0;
    digitalWrite(relayPin, relayOff);
    Serial.println("Relay mati");
    client.publish("openhab/relay/status", "mati");
    client.publish("openhab/scheduler/irrigation/on", "0");
  }
  delay(3000);  // Baca data dan kirim ke broker MQTT setiap 3 detik
}
