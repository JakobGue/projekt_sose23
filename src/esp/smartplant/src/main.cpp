// INPUTS
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Credentials.h>

// CONSTANTS
const char* ssid = WIFI_SSID;
const char* password =  WIFI_PW;

const char* mqttServer = MQTT_BROKER;
const int mqttPort = MQTT_PORT;
const char* mqttUser = MQTT_USERNAME;
const char* mqttPassword = MQTT_PASSWORD;
const char* mqttTopic = MQTT_TOPIC;

// GLOBALS
const int POT = 1;

WiFiClient espClient;
PubSubClient client(espClient);




// Check WiFi Connectio
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
}
void reconnect() 
{
  client.setServer(mqttServer, mqttPort);

  while (!client.connected()) 
  {
    if (client.connect("pot_" + POT))
    {
      client.subscribe(mqttTopic);
    } 
    else 
    {
      delay(6000);
    }
  }
}

void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  // TODO read sensor data
  // TODO store data into sd card

  float env_humid = 0.0;
  float light_lux = 0.0;
  float temp = 0.0;
  float soil_moist = 0.0;

  DynamicJsonDocument doc(1024);

  doc["data"][0]["pot_id"] = POT;
  doc["data"][0]["sesnsor_id"] = 1;
  doc["data"][0]["measurement_type"] = "env_humid";
  doc["data"][0]["value"] = env_humid;

  doc["data"][0]["pot_id"] = POT;
  doc["data"][0]["sesnsor_id"] = 2;
  doc["data"][0]["measurement_type"] = "light_lux";
  doc["data"][0]["value"] = light_lux;

  doc["data"][0]["pot_id"] = POT;
  doc["data"][0]["sesnsor_id"] = 3;
  doc["data"][0]["measurement_type"] = "temp";
  doc["data"][0]["value"] = temp;

  doc["data"][0]["pot_id"] = POT;
  doc["data"][0]["sesnsor_id"] = 4;
  doc["data"][0]["measurement_type"] = "soil_moist";
  doc["data"][0]["value"] = soil_moist;

  char buffer[1024];
  serializeJson(doc, buffer);

  client.publish(mqttTopic, buffer);

  delay(30000);

  // TODO read light again
  //  TODO store data into sd card

  light_lux = 0.0;

  DynamicJsonDocument doc2(1024);

  doc2["data"][0]["pot_id"] = POT;
  doc2["data"][0]["sesnsor_id"] = 1;
  doc2["data"][0]["measurement_type"] = "light_lux";
  doc2["data"][0]["value"] = env_humid;

  char buffer2[1024];
  serializeJson(doc2, buffer2);

  client.publish(mqttTopic, buffer2);

  delay(30000);

}

