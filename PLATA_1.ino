#include <Arduino.h>
#include <Ethernet.h>
#include <PubSubClient.h>

byte MAC[] = { 0x2C, 0xAF, 0xE4, 0x8D, 0x1B, 0x4F };
const char* mqttServer = "mqtt.justtask.uz";
const int mqttPort = 1883;
const char* mqttReciveTopic = "ESP32/PLATA1TR";
const char* mqttSenderTopic = "ESP32/PLATA1RX";

EthernetClient ethClient;
PubSubClient client(ethClient);

const uint8_t DIGITAL_OUTPUTS[] = {PB12, PB13, PB14, PB15};
const uint8_t ANALOG_PINS[] = {PB0, PB1};

uint8_t currentPinStates[sizeof(DIGITAL_OUTPUTS)] = {0};
uint8_t savedPinStates[sizeof(DIGITAL_OUTPUTS)] = {0};
uint16_t currentAnalogStates[sizeof(ANALOG_PINS)] = {0};
uint16_t savedAnalogStates[sizeof(ANALOG_PINS)] = {0};

void setup() {
  Serial.begin(9600);
  pinModeSetter();
  ethernetConnector();
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  checkAndUpdatePinStates(currentPinStates, savedPinStates);
  checkAndUpdateAnalogPinStates(currentAnalogStates, savedAnalogStates);
}

void pinModeSetter() {
  for (int i = 0; i < sizeof(DIGITAL_OUTPUTS); i++) {
    pinMode(DIGITAL_OUTPUTS[i], OUTPUT);
  }

  for (int i = 0; i < sizeof(ANALOG_PINS); i++) {
    pinMode(ANALOG_PINS[i], INPUT);
  }
}

void checkAndUpdatePinStates(uint8_t currentStates[], uint8_t savedStates[]) {
  for (int i = 0; i < sizeof(DIGITAL_OUTPUTS); ++i) {
    currentStates[i] = digitalRead(DIGITAL_OUTPUTS[i]);

    if (currentStates[i] != savedStates[i]) {
      savedStates[i] = currentStates[i];

      String json = "{\"pin" + String(i) + "\":" + String(currentStates[i] == HIGH ? 0 : 1) + "}";
      sendDataToMQTT(json);
    }
  }
}

void checkAndUpdateAnalogPinStates(uint16_t currentStates[], uint16_t savedStates[]) {
  for (int i = 0; i < sizeof(ANALOG_PINS); ++i) {
    long analogPinValue = analogRead(ANALOG_PINS[i]);
    currentStates[i] = map(analogPinValue, 0, 1022, 0, 360);

    if (abs(currentStates[i] - savedStates[i]) > 2) {
      savedStates[i] = currentStates[i];

      String json = "{\"pinA" + String(i) + "\":" + String(currentStates[i]) + "}";
      sendDataToMQTT(json);
      delay(5);
    }
  }
}

void sendDataToMQTT(String stringData) {
  client.publish(mqttSenderTopic, stringData.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  String pin1 = extractValue(message, "pin1");
  String pin2 = extractValue(message, "pin2");
  String pin3 = extractValue(message, "pin3");
  String pin4 = extractValue(message, "pin4");

  if (pin1.length() > 0) {
    digitalWrite(PB12, pin1.toInt());
  }

  if (pin2.length() > 0) {
    digitalWrite(PB13, pin2.toInt());
  }

  if (pin3.length() > 0) {
    digitalWrite(PB14, pin3.toInt());
  }

  if (pin4.length() > 0) {
    digitalWrite(PB15, pin4.toInt());
  }
}

String extractValue(const String& jsonString, const String& key) {
  String keyWithQuotes = "\"" + key + "\":";
  int keyIndex = jsonString.indexOf(keyWithQuotes);

  if (keyIndex == -1) {
    return "";
  }

  int valueStart = keyIndex + keyWithQuotes.length();
  int commaIndex = jsonString.indexOf(',', valueStart);

  if (commaIndex == -1) {
    commaIndex = jsonString.length() - 1;
  }

  String value = jsonString.substring(valueStart, commaIndex);
  value.trim();

  // Remove quotes if present
  if (value.startsWith("\"") && value.endsWith("\"")) {
    value = value.substring(1, value.length() - 1);
  }

  return value;
}

void reconnect() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("")) {
      Serial.println("Connected to MQTT server");
      client.subscribe(mqttReciveTopic);
    } else {
      Serial.print("Failed to connect to MQTT server, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void ethernetConnector() {
  if (Ethernet.begin(const_cast<uint8_t*>(MAC)) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while (true);
  }

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("Connected to Ethernet");
}
