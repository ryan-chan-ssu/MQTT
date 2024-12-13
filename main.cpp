///// Part A /////

// #include <Arduino.h>
// #include "hardware_control.h"

// void setup() {
//   setupHardware();  // Initialize hardware components
// }

// void loop() {
//   readPotentiometer();  // Read and display potentiometer value
//   readSwitch();         // Monitor button presses

//   // Handle Serial commands to control the LED
//   if (Serial.available()) {
//     char command = Serial.read();
//     controlLED(command);
//   }

//   delay(100);  // Small delay for stability
// }

///// Part B /////

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Wi-Fi Credentials
const char* ssid = "Loading...";
const char* password = "Finexes0621";

// HiveMQ Cloud MQTT Broker Details
const char* mqtt_server = "7fe480575924421cb99c407d2f8fb733.s1.eu.hivemq.cloud";
const int mqtt_port = 8883; // Use secure port
const char* mqtt_user = "iot_ryan";
const char* mqtt_password = "Wasd1234";

// MQTT Topics
const char* potTopic = "iot/ryan/potentiometer";
const char* switchTopic = "iot/ryan/switch";
const char* ledControlTopic = "iot/ryan/led/control";

// Constants
#define POT_PIN A0                 // Potentiometer pin
#define SWITCH_PIN D1              // Switch pin
#define LED_PIN 2                  // Built-in LED pin
#define MSG_BUFFER_SIZE 50

// Variables
char msg[MSG_BUFFER_SIZE];
unsigned long lastPotPublishTime = 0;
unsigned long switchPressTime = 0;
bool switchReleasedFlag = false;

// Wi-Fi and MQTT clients
WiFiClientSecure espClient; // Secure client for TLS
PubSubClient client(espClient);

//------------------------------------------
// Wi-Fi Connection
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

//------------------------------------------
// MQTT Callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("[MQTT] Message received on topic ");
  Serial.print(topic);
  Serial.print(": ");

  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, ledControlTopic) == 0) {
    if (payload[0] == '1') {
      digitalWrite(LED_PIN, LOW);  // Turn LED ON (active LOW)
      Serial.println("[MQTT] LED turned ON.");
    } else if (payload[0] == '0') {
      digitalWrite(LED_PIN, HIGH); // Turn LED OFF
      Serial.println("[MQTT] LED turned OFF.");
    }
  }
}

//------------------------------------------
// MQTT Reconnect
void reconnectToMQTT() {
  while (!client.connected()) {
    Serial.print("[MQTT] Attempting connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("Connected!");
      client.subscribe(ledControlTopic);
      Serial.println("[MQTT] Subscribed to LED Control Topic.");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

//------------------------------------------
// Setup Function
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP); // Use internal pull-up resistor
  digitalWrite(LED_PIN, HIGH);      // Ensure LED is OFF initially
  Serial.begin(9600);
  connectToWiFi();
  
  espClient.setInsecure(); // Disable certificate validation (simpler setup)
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  Serial.println("[INFO] Setup Complete.");
}

//------------------------------------------
// Main Loop
void loop() {
  if (!client.connected()) {
    reconnectToMQTT();
  }
  client.loop();

  // Publish potentiometer data every 15 seconds
  unsigned long now = millis();
  if (now - lastPotPublishTime > 15000) {
    lastPotPublishTime = now;
    int potValue = analogRead(POT_PIN);
    float voltage = (potValue / 1023.0) * 3.3;
    snprintf(msg, MSG_BUFFER_SIZE, "Voltage: %.2f V", voltage);
    client.publish(potTopic, msg);
    Serial.print("[MQTT] Published Potentiometer Data: ");
    Serial.println(msg);
  }

  // Publish switch state when pressed
  int switchState = digitalRead(SWITCH_PIN);
  if (switchState == LOW && !switchReleasedFlag) {
    client.publish(switchTopic, "1");
    Serial.println("[MQTT] Switch Pressed: Published '1'.");
    switchPressTime = millis();
    switchReleasedFlag = true;
  }

  // Publish "0" after 5 seconds of release
  if (switchReleasedFlag && millis() - switchPressTime > 5000) {
    client.publish(switchTopic, "0");
    Serial.println("[MQTT] Switch Released: Published '0'.");
    switchReleasedFlag = false;
  }
}
