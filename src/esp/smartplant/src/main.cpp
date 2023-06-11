#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Credentials.h>
#include <DHT.h>
#include <BH1750.h>
#include <Wire.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <Adafruit_NeoPixel.h>

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


// GLOBALS
const int POT = POT_ID;



WiFiClientSecure espClient;
PubSubClient client(espClient);
// PubSubClient * client;
DHT dht(ENV_SENSOR_PIN, DHT11);
BH1750 GY302;
BearSSL::CertStore certStore;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4, D4, NEO_GRB + NEO_KHZ800);


void setup_wifi() 
{
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(0, 0, 100));
  pixels.setPixelColor(1, pixels.Color(0, 0, 100));
  pixels.setPixelColor(2, pixels.Color(0, 0, 100));
  pixels.setPixelColor(3, pixels.Color(0, 0, 100));
  pixels.show();

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  String type = incommingMessage.substring(0, incommingMessage.indexOf("-"));
  String exceed = incommingMessage.substring(incommingMessage.indexOf("-")+1, incommingMessage.length());

  if (type == "light_lux") {
    if (exceed == "min") {
      pixels.setPixelColor(0, pixels.Color(100, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(0, pixels.Color(100, 100, 0));
    }
  } else if (type == "soil_moist") {
    if (exceed == "min") {
      pixels.setPixelColor(1, pixels.Color(100, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(1, pixels.Color(100, 100, 0));
    }
  } else if (type == "temp") {
    if (exceed == "min") {
      pixels.setPixelColor(2, pixels.Color(100, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(2, pixels.Color(100, 100, 0));
    }
  } else if (type == "env_humid") {
    if (exceed == "min") {
      pixels.setPixelColor(3, pixels.Color(100, 0, 0));
    } else if (exceed == "max") {
      pixels.setPixelColor(3, pixels.Color(100, 100, 0));
    }
  }
  pixels.show();
}

// Check WiFi Connectio
void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  setup_wifi();
  setDateTime();
  

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  // BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  // bear.setCertStore(&certStore);

  // client = new PubSubClient(*bear);
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
      client.subscribe("alertings/%d", POT);

    } else {
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
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

  float light_lux = GY302.readLightLevel();
  float temp = dht.readTemperature();
  float env_humid = dht.readHumidity();
  float soil_moist = analogRead(SOIL_SENSOR_PIN);;

  DynamicJsonDocument doc(256);

  Serial.println("env_humid: " + String(env_humid));
  Serial.println("light_lux: " + String(light_lux));
  Serial.println("temp: " + String(temp));
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
  Serial.println(buffer);
  client.publish(mqtt_topic, buffer);

  doc.clear();

  doc["data"][0]["pot_id"] = POT_ID;
  doc["data"][0]["sensor_id"] = SOIL_SENSOR;
  doc["data"][0]["measurement_type"] = "soil_moist";
  doc["data"][0]["value"] = soil_moist;

  serializeJson(doc, buffer);
  Serial.println(buffer);
  client.publish(mqtt_topic, buffer);
  

  delay(30000); // Wait for 1 second to allow the sensor to be read

}
