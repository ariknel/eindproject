#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
const int S0 = 4; //connects to pin 34 on PCB (reroute)
const int S1 = 33;
const int S2 = 32;
const int S3 = 25;
const int SIG = 34;
//////////////
int IN1 = 16;
int IN2 = 17;
int IN3 = 26;
int IN4 = 27;
int EN_A = 0;
int EN_B = 2;
/////new5/
int irSensors[3]; // Left, Center, Right
const float R1 = 10000.0; // 10k Ohms
const float R2 = 5000.0;  // 5k Ohms
int battpc;
int percentage;
int speedPwm = 0; //0 - 255 60 -> lowest -> default
uint16_t driveStates = 0b0000000000000000;
LiquidCrystal_I2C lcd(0x27, 16, 2); //adressen: INA226: 0x40  ,   LCD: 0x27
int speedControl = 160;
int button_virtual_1 = 0;
int button_virtual_2 = 0;
int fqp_01 = 18;
int fqp_02 = 19;
int fqp_03 = 23;
const char* ssid = "embed";
const char* password = "weareincontrol";
const char* mqtt_server = "192.168.1.120";
WiFiClient espClient;
PubSubClient client(espClient);
//Taken verdeeld op core0 en core1
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t logicTaskHandle = NULL;
//battery conversion
int getBatteryLevel() {
  analogReadResolution(12);         // 12-bit ADC: 0-4095
  analogSetAttenuation(ADC_11db);   // Allows voltage up to ~3.3V

  int rawADC = analogRead(5); //io5 ADC
  float voltageAtPin = (rawADC / 4095.0) * 3.3; // Convert ADC to voltage
  float batteryVoltage = voltageAtPin * ((R1 + R2) / R2); // Undo voltage divider

  // Map 3.0V–4.2V to 0%–100%
  int batteryPercent = map(batteryVoltage * 100, 300, 420, 0, 100);
  batteryPercent = constrain(batteryPercent, 0, 100);

  return batteryPercent;
}
// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  String topicStr = String(topic);
  if (topicStr == "esp32/control/mode") {
    if (message == "Turtle") {
      Serial.println(message);
      digitalWrite(fqp_01, HIGH);
      digitalWrite(fqp_02, LOW);
      digitalWrite(fqp_03, LOW);
    } else if (message == "Eco") {
      Serial.println(message);
      digitalWrite(fqp_01, LOW);
      digitalWrite(fqp_02, HIGH);
      digitalWrite(fqp_03, LOW);
    }
    else if (message == "Sport") {
      Serial.println(message);
      digitalWrite(fqp_01, LOW);
      digitalWrite(fqp_02, LOW);
      digitalWrite(fqp_03, HIGH);
    }
    // Mode selection logic here (can add GPIO actions)
  } else if (topicStr == "esp32/control/speed") {
    speedControl = message.toInt();
    if (speedControl < 60) speedControl = 60;
    Serial.println("Speed: " + String(speedControl));
  } else if (topicStr == "esp32/control/buttonsactive") {
    if (message == "button1") {
      button_virtual_1 = 1;
      Serial.println("gas");
    } else if (message == "button2") {
      button_virtual_2 = 1;
      Serial.println("rev");
    }
  } else if (topicStr == "esp32/control/buttonsoff") {
    if (message == "button1") button_virtual_1 = 0;
    else if (message == "button2") button_virtual_2 = 0;
    //Serial.println(button_virtual_1); //debug
    //Serial.println(button_virtual_2);  //debug
  }
}
void setupPins() {
  pinMode(fqp_01, OUTPUT);
  pinMode(fqp_02, OUTPUT);
  pinMode(fqp_03, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(EN_A, OUTPUT);
 pinMode(EN_B, OUTPUT);
   pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  // SIG should be an input
  pinMode(SIG, INPUT);
}
void setup_wifi() {
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/control/mode");
      client.subscribe("esp32/control/speed");
      client.subscribe("esp32/control/buttonsactive");
      client.subscribe("esp32/control/buttonsoff");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
// MQTT loop task for core 0
void mqttTask(void *parameter) {
  for (;;) {
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    delay(10); // brief yield
  }
}

// Application logic task for core 1
void logicTask(void *parameter) {
  unsigned long lastSend = 0;
  for (;;) {
    if (millis() - lastSend > 5000) {
      lastSend = millis();
      int batteryLevel = getBatteryLevel(); // real value from ADC
String batteryStr = String(batteryLevel);
client.publish("esp32/status/battery", batteryStr.c_str());
      Serial.print("Battery published: ");
      Serial.println(batteryStr);
    }
    drive();
    driving();
  }
}

void setup() {
  setupPins();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // Create tasks pinned to specific cores
  xTaskCreatePinnedToCore(
    mqttTask, "MQTT Task", 4096, NULL, 1, &mqttTaskHandle, 0); // Core 0

  xTaskCreatePinnedToCore(
    logicTask, "Logic Task", 4096, NULL, 1, &logicTaskHandle, 1); // Core 1
/////new5/25
 lcd.init();
  lcd.backlight();
  //new26
  // Loop through all 16 channels
  for (int i = 0; i < 3; i++) {
    selectMuxChannel(i);
    delay(5); // small delay for stable signal
    irSensors[i] = digitalRead(SIG); // HIGH = white, LOW = black
  }
    // Print values
  Serial.print("Left: "); Serial.print(irSensors[0]);
  Serial.print("  Center: "); Serial.print(irSensors[1]);
  Serial.print("  Right: "); Serial.println(irSensors[2]);

  // Example logic: car is on line if center IR is LOW (black)
  if (irSensors[1] == LOW) {
    Serial.println("Car is on the line.");
  } else {
    Serial.println("Car is off the line.");
  }
}
void loop() {
  // Nothing here — everything is in tasks
}
void drive() {
  analogWrite(EN_A, speedControl);   //left pair
  analogWrite(EN_B, speedControl);   //right pair
  //01 -> forward 10 -> reverse
  /////////////////////////////
  if (button_virtual_1 == true) { //gas
    driveStates |= (1 << 1); // bit 1 -> 1 (IN1)
    driveStates |= (2 << 0); // bit 2 -> 0 (IN2)
    driveStates |= (3 << 1); // bit 3 -> 1 (IN1)
    driveStates |= (4 << 0); // bit 4 -> 0 (IN2)
  } else if (button_virtual_1 == false) {
    driveStates |= (1 << 0); // bit 1 -> 0 (IN1)
    driveStates |= (2 << 0); // bit 2 -> 0 (IN2)
    driveStates |= (3 << 0); // bit 3 -> 0 (IN1)
    driveStates |= (4 << 0); // bit 4 -> 0 (IN2)
  }
  if (button_virtual_2 == true) {
    driveStates |= (1 << 0);
    driveStates |= (2 << 1);
    driveStates |= (3 << 0);
    driveStates |= (4 << 1);
  } else if (button_virtual_2 == false) {
    driveStates |= (1 << 0);
    driveStates |= (2 << 0);
    driveStates |= (3 << 0);
    driveStates |= (4 << 0);
  }
}
void driving() {
  //left pair
  digitalWrite(IN1, (driveStates >> 1) & 1); // bit 1 -> IN1
  digitalWrite(IN2, (driveStates >> 1) & 2); // bit 2 -> IN2
  //right pair
  digitalWrite(IN3, (driveStates >> 1) & 3); // bit 3 -> IN1
  digitalWrite(IN4, (driveStates >> 1) & 4); // bit 4 -> IN2
}
void selectMuxChannel(int channel) {
  digitalWrite(S0, channel & 1);
  digitalWrite(S1, (channel >> 1) & 1);
  digitalWrite(S2, (channel >> 2) & 1);
  digitalWrite(S3, (channel >> 3) & 1);
}
