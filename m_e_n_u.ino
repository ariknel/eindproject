#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust I2C address if needed
#define A0 10 // Analog pin A0 for voltage measurement
const float R1 = 10000.0; // 10k Ohms
const float R2 = 5000.0;  // 5k Ohms
int battpc;
int percentage;
void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  // Nothing here – static display
  display();
}


void display() {
batteryIcon(); //battery icon + %
adidas(); //tracks line
controlsConnected(); //displays MQTT connection
dpSpeed();
battery();
}


void batteryIcon() {
  // Battery icons for 0% to 100%
byte batteryLevels[6][8] = {
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}, // 0%
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x1F}, // 20%
  {0x0E, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x1F, 0x1F}, // 40%
  {0x0E, 0x11, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F}, // 60%
  {0x0E, 0x11, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}, // 80%
  {0x0E, 0x11, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F}  // 100%
};
for (int i = 0; i < 6; i++) {
    lcd.createChar(i, batteryLevels[i]);
  }
  int index = map(battpc, 0, 100, 0, 5);
  lcd.setCursor(0, 0);
  lcd.write(byte(index)); // Display battery icon
  lcd.setCursor(1, 0);
  lcd.print(battpc);
  lcd.print("%");
  }
void adidas() {
///////////////IR Sensor Readings///////////////
    //port information from sensor reading into these integers
     static int left = 0;
     static int center = 0;
     static int right = 0;
///////////////////////////////////////////////
     if(left == true || center == true || right == true) {
      lcd.setCursor(0, 1);
      lcd.print("On track:       ");
      lcd.print(" ");
      lcd.print(left);
      lcd.print(center);
      lcd.print(right);
     } else {
      lcd.setCursor(0, 1);
      lcd.print("Off track       ");
     }
///////////////////////////////////////////////
}
void controlsConnected() {
///////////////////////////////////////////////
     static int broker = 0; //default not connected
     if(broker == true) {
      lcd.setCursor(12, 0);  //connected
      lcd.print("MQTT");
     }
     else {
      lcd.setCursor(11, 0);  //not connected
      lcd.print("MQTT!");
     }
}
void dpSpeed() {
  static int msft = 1; //1234 - Drive - Overdrive - Reverse
  lcd.setCursor(6, 0); 
  //default = drive
  if(msft = 1) {
   lcd.print("D");
  }
  else if(msft = 2) {
   lcd.print("S");
  }
  else if(msft = 3) {
   lcd.print("O");
  }
  else if(msft = 4) {
   lcd.print("R");
  }
  static int speed = 33; // demo speed
  lcd.setCursor(7, 0); 
  lcd.print(speed);
  lcd.print("%");
}
void battery() {
  int rawValue = analogRead(A0); // Read the raw analog value from pin A0
  float voltage = rawValue * (5.0 / 1023.0); // Convert the raw value to voltage (0-5V)
  float actualVoltage = voltage * ((R1 + R2) / R2);
  battpc = map(actualVoltage * 100, 300, 420, 0, 100); // lithiun
  battpc = constrain(battpc, 0, 100); // Clamp within 0–100%
}
 