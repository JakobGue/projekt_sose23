#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Credentials.h>
#include <DHT.h>
#include <BH1750.h>
#include <Wire.h>
#include <CertStoreBearSSL.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>  

// Update these with values suitable for your network.
const char* ssid = WIFI_SSID;
const char* password =  WIFI_PW;
const char* mqttServer = MQTT_BROKER;
const int mqtt_port = MQTT_PORT;
const char* mqtt_user = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_topic = MQTT_TOPIC;


const int LIGHT_SENSOR = LIGHT_SENSOR_ID;
const int SOIL_SENSOR = SOIL_SENSOR_ID;
const int ENV_SENSOR = ENV_SENSOR_ID;

const int ENV_SENSOR_PIN = D5;
const int SOIL_SENSOR_PIN = A0;

const int SOIL_MOISTURE_OFFSET = 300;


// GLOBALS
const int POT = POT_ID;

int i = 0;



WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT dht(ENV_SENSOR_PIN, DHT11);
BH1750 GY302;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4, D3, NEO_GRB + NEO_KHZ800);


void setup_wifi() 
{
  delay(10);
  WiFiManager wifiManager;
  wifiManager.autoConnect("SmartPlantSetup");
}


void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  String type = incommingMessage.substring(0, incommingMessage.indexOf("-"));
  String exceed = incommingMessage.substring(incommingMessage.indexOf("-")+1, incommingMessage.length());
  Serial.println(type);
  if (type == "soil_moist") {
    if (exceed == "min") {
      pixels.setPixelColor(3, pixels.Color(50, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(3, pixels.Color(0, 0, 50));
    }
    } else if (type == "light_lux") {
    if (exceed == "min") {
      pixels.setPixelColor(2, pixels.Color(50, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(2, pixels.Color(0, 0, 50));
    }
  } else if (type == "temp") {
    if (exceed == "min") {
      pixels.setPixelColor(1, pixels.Color(50, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(1, pixels.Color(0, 0, 50));
    }
  } else if (type == "env_humid") {
    if (exceed == "min") {
      pixels.setPixelColor(0, pixels.Color(50, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(0, pixels.Color(0, 0, 50));
    }
  }
  pixels.show();
}

void setup() {
  Serial.begin(115200);

  pixels.begin();
  pixels.fill(pixels.Color(50, 0, 50));
  pixels.show();

  setup_wifi();

  pixels.fill(pixels.Color(0, 100, 0));
  pixels.show();

  espClient.setInsecure();

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  Wire.begin();
  GY302.begin();
  dht.begin();

  pinMode(SOIL_SENSOR_PIN, INPUT);
}


void reconnect() 
{
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    char clientId[10];
    sprintf(clientId, "%d", POT); 

    if (client.connect(clientId, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println(" Mqtt connected");
      client.subscribe(ALERT_TOPIC);

    } else {
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}


void loop() 
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (i == 60) {
    pixels.fill(pixels.Color(0, 50, 0));
    pixels.show();
    float light_lux = GY302.readLightLevel();
    float temp = dht.readTemperature();
    float env_humid = dht.readHumidity();
    float soil_moist =  (1 - (analogRead(SOIL_SENSOR_PIN)-SOIL_MOISTURE_OFFSET)/(1023-SOIL_MOISTURE_OFFSET)) * 100;

    DynamicJsonDocument doc(256);

    Serial.println("env_humid: " + String(env_humid));
    Serial.println("light_lux: " + String(light_lux));
    Serial.println("temp: " + String(temp));
    if (soil_moist < 0) soil_moist = 0;
    if (soil_moist > 100) soil_moist = 100;
    Serial.println("soil_moist: " + String(soil_moist));

    doc["data"][0]["pot_id"] = POT_ID;
    doc["data"][0]["sensor_id"] = ENV_SENSOR;
    doc["data"][0]["measurement_type"] = "env_humid";
    doc["data"][0]["value"] = env_humid;

    doc["data"][1]["pot_id"] = POT_ID;
    doc["data"][1]["sensor_id"] = LIGHT_SENSOR;
    doc["data"][1]["measurement_type"] = "light_lux";
    doc["data"][1]["value"] = light_lux;

    doc["data"][2]["pot_id"] = POT_ID;
    doc["data"][2]["sensor_id"] = ENV_SENSOR;
    doc["data"][2]["measurement_type"] = "temp";
    doc["data"][2]["value"] = temp;
    
    char buffer[256];
    serializeJson(doc, buffer);
    client.publish(mqtt_topic, buffer);

    doc.clear();

    doc["data"][0]["pot_id"] = POT_ID;
    doc["data"][0]["sensor_id"] = SOIL_SENSOR;
    doc["data"][0]["measurement_type"] = "soil_moist";
    doc["data"][0]["value"] = soil_moist;

    serializeJson(doc, buffer);
    client.publish(mqtt_topic, buffer);
    i = 0;
  }

  delay(500);
  i++;

}
