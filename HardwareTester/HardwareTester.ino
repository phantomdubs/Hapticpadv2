#include <Arduino.h>
#include <U8g2lib.h>
#include <SimpleFOC.h>
#include <SPI.h>
#include <SdFat.h>
#include <FastLED.h>

#define File FsFile
SdFat SD;

// --- HARDWARE PINOUTS ---
#define NUM_LEDS 20
#define DATA_PIN 15
const int _MISO = 4;
const int _MOSI = 3;
const int _CS = 5;
const int _SCK = 2;
#define buttonCount 8
byte buttonPins[buttonCount] = {9, 1, 0, 12, 11, 10, 13, 14};

// --- OBJECTS ---
CRGB leds[NUM_LEDS];

// THE FIX: Changed _1_ to _F_ for Full Screen Buffer
U8G2_SSD1309_128X64_NONAME0_F_4W_SW_SPI u8g2(U8G2_R0, 28, 22, 6, 7, 8);

BLDCMotor motor = BLDCMotor(7);
BLDCDriver6PWM driver = BLDCDriver6PWM(20, 21, 18, 19, 16, 17);
Encoder encoder = Encoder(27, 26, 1024);

void doA() { encoder.handleA(); }
void doB() { encoder.handleB(); }

// --- STATE MACHINE VARIABLES ---
volatile int testState = 0; 
volatile int lastTestState = -1;
volatile bool focReady = false;

// Sample Image for Final Screen
const unsigned char SD_Card [] PROGMEM = {
  0xff, 0x01, 0x55, 0x01, 0x55, 0x01, 0xff, 0x01, 0x01, 0x03, 0x2d, 0x02, 0x45, 0x03, 0x49, 0x01, 
  0x2d, 0x03, 0x01, 0x02, 0xfd, 0x02, 0x01, 0x02, 0xff, 0x03
};

// =========================================================
// CORE 0: MOTOR CONTROL ONLY
// =========================================================
void setup() {
  Serial.begin(115200);
  delay(2000); 
  Serial.println("\n\n--- DIAGNOSTIC BOOT START ---");
  Serial.println("[Core 0] Initializing Motor Driver...");

  driver.voltage_power_supply = 5;
  driver.voltage_limit = 5;
  driver.init();
  
  Serial.println("[Core 0] Initializing Encoder...");
  encoder.init();
  encoder.enableInterrupts(doA, doB);
  motor.linkSensor(&encoder);
  
  Serial.println("[Core 0] Linking Motor & Driver...");
  motor.linkDriver(&driver);
  motor.voltage_sensor_align = 3;
  motor.PID_velocity.output_ramp = 1000;
  motor.LPF_velocity.Tf = 0.025f;
  motor.velocity_limit = 4;
  motor.voltage_limit = 3;
  
  Serial.println("[Core 0] Initializing FOC...");
  motor.init();
  motor.initFOC();
  Serial.println("[Core 0] FOC Ready! Handing over to Core 1...");
  focReady = true;
}

void loop() {
  motor.loopFOC();
  
  if (testState != lastTestState) {
    if (testState == 4) { 
      motor.controller = MotionControlType::angle;
      motor.PID_velocity.P = 0.5f;
      motor.PID_velocity.I = 0.0f;
      motor.enable();
    } 
    else if (testState == 5) { 
      motor.controller = MotionControlType::angle;
      motor.PID_velocity.P = 0.65f;
      motor.PID_velocity.I = 0.2f;
      motor.enable();
    } 
    else if (testState == 6) { 
      motor.disable(); 
    }
    else {
      motor.disable(); 
    }
    lastTestState = testState;
  }

  if (testState == 4) { 
    float angle_step = radians(360.0 / 40.0);
    float currentAngle = encoder.getAngle();
    float target = round(currentAngle / angle_step) * angle_step;
    motor.move(target);
  } 
  else if (testState == 5) { 
    motor.move(0); 
  }
}

// =========================================================
// CORE 1: SCREEN, LEDS, SD CARD, & LOGIC
// =========================================================

void waitForNext() {
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(0, 60, "Press Btn 1 -> Next");
  u8g2.sendBuffer();
  
  Serial.println("[Core 1] Waiting for Button 1 (Pin 9) to be pressed...");
  delay(200); 
  while (digitalRead(buttonPins[0]) == HIGH) {
    delay(10); 
  }
  Serial.println("[Core 1] Button 1 Pressed! Moving to next state.");
  delay(200); 
  testState++;
}

void setup1() {
  delay(2500); 
  Serial.println("[Core 1] Initializing Buttons...");
  for (int i = 0; i < buttonCount; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  Serial.println("[Core 1] Setting SPI Pins...");
  SPI.setRX(_MISO);
  SPI.setTX(_MOSI);
  SPI.setSCK(_SCK);
  
  Serial.println("[Core 1] Initializing OLED Screen...");
  u8g2.begin();
  
  Serial.println("[Core 1] Initializing FastLED...");
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();
  
  Serial.println("[Core 1] Waiting for Motor Alignment to finish...");
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(0, 20, "Aligning Motor...");
  u8g2.drawStr(0, 35, "Do not touch knob.");
  u8g2.sendBuffer();
  
  while (!focReady) { delay(10); }
  Serial.println("[Core 1] Alignment complete. Starting tests!");
}

void loop1() {
  u8g2.clearBuffer();
  
  switch (testState) {
    
    // --- STATE 0: SD Card & XML Check ---
    case 0: {
      Serial.println("\n--- TEST 1: SD CARD MOUNT ---");
      u8g2.drawStr(0, 10, "1. SD Card Check");
      u8g2.sendBuffer();

      Serial.println("[Core 1] Attempting SD.begin()... (If it freezes here, check SD pins/card)");
      if (SD.begin(_CS, SD_SCK_MHZ(40))) { 
        Serial.println("[Core 1] SUCCESS: SD Card Mounted!");
        u8g2.drawStr(0, 25, "SD Mount: SUCCESS");
        u8g2.sendBuffer();

        Serial.println("[Core 1] Attempting to open /config.xml...");
        File file = SD.open("/config.xml");
        if (file) {
          Serial.println("[Core 1] SUCCESS: config.xml opened.");
          u8g2.drawStr(0, 40, "config.xml: FOUND");
          u8g2.sendBuffer();

          Serial.println("[Core 1] Searching for <Clicky_P> in file...");
          if (file.find("<Clicky_P>")) {
             float p = file.parseFloat();
             Serial.print("[Core 1] SUCCESS: Found <Clicky_P> = ");
             Serial.println(p);
             u8g2.drawStr(0, 50, "XML Read: SUCCESS");
          } else {
             Serial.println("[Core 1] FAILED: Could not find <Clicky_P> in file.");
             u8g2.drawStr(0, 50, "XML Read: FAILED");
          }
          Serial.println("[Core 1] Closing config.xml");
          file.close();
        } else {
          Serial.println("[Core 1] FAILED: config.xml not found on card.");
          u8g2.drawStr(0, 40, "config.xml: MISSING");
        }
      } else {
        Serial.println("[Core 1] FATAL: SD.begin() Failed.");
        u8g2.drawStr(0, 25, "SD Mount: FAILED");
      }
      waitForNext();
      break;
    }

    // --- STATE 1: Button Check ---
    case 1: {
      Serial.println("\n--- TEST 2: BUTTON CHECK ---");
      for (int i = 0; i < buttonCount; i++) {
        bool buttonPressed = false;
        Serial.print("[Core 1] Waiting for user to press Button ");
        Serial.println(i + 1);
        
        while (!buttonPressed) {
          u8g2.clearBuffer();
          u8g2.drawStr(0, 10, "2. Button Check");
          u8g2.setCursor(0, 30);
          u8g2.print("Press Button: ");
          u8g2.print(i + 1);
          u8g2.sendBuffer();
          
          if (digitalRead(buttonPins[i]) == LOW) {
            Serial.print("[Core 1] Button ");
            Serial.print(i + 1);
            Serial.println(" registered!");
            
            buttonPressed = true;
            fill_solid(leds, NUM_LEDS, CRGB::Green);
            FastLED.show();
            delay(200); 
            FastLED.clear();
            FastLED.show();
          }
        }
      }
      testState++; 
      break;
    }

    // --- STATE 2: LED Check ---
    case 2: {
      Serial.println("\n--- TEST 3: LED RAINBOW ---");
      u8g2.drawStr(0, 10, "3. LED Rainbow Test");
      u8g2.drawStr(0, 25, "Check for dead pixels");
      
      u8g2.drawStr(0, 60, "Press Btn 1 -> Next");
      u8g2.sendBuffer();
      
      uint8_t hue = 0;
      delay(200); 
      Serial.println("[Core 1] Running FastLED Rainbow... Press Button 1 to continue.");
      while (digitalRead(buttonPins[0]) == HIGH) {
        fill_rainbow(leds, NUM_LEDS, hue++, 10);
        FastLED.show();
        delay(10);
      }
      FastLED.clear();
      FastLED.show();
      Serial.println("[Core 1] Rainbow stopped.");
      delay(200); 
      testState++;
      break;
    }

    // --- STATE 3: Knob Check (Clicky) ---
    case 3: {
      Serial.println("\n--- TEST 4: KNOB PHYSICS (CLICKY) ---");
      u8g2.drawStr(0, 10, "4. Knob Test (1/3)");
      u8g2.drawStr(0, 30, "Mode: CLICKY");
      u8g2.drawStr(0, 45, "Spin it to feel detents");
      waitForNext();
      break;
    }

    // --- STATE 4: Knob Check (Twist) ---
    case 4: {
      Serial.println("\n--- TEST 5: KNOB PHYSICS (TWIST) ---");
      u8g2.drawStr(0, 10, "4. Knob Test (2/3)");
      u8g2.drawStr(0, 30, "Mode: TWIST");
      u8g2.drawStr(0, 45, "Should spring back");
      waitForNext();
      break;
    }

    // --- STATE 5: Knob Check (Momentum) ---
    case 5: {
      Serial.println("\n--- TEST 6: KNOB PHYSICS (MOMENTUM) ---");
      u8g2.drawStr(0, 10, "4. Knob Test (3/3)");
      u8g2.drawStr(0, 30, "Mode: MOMENTUM");
      u8g2.drawStr(0, 45, "Should spin freely");
      waitForNext();
      break;
    }

    // --- STATE 6: Screen Rendering & Done ---
    case 6: {
      Serial.println("\n--- TEST 7: FINAL BITMAP TEST ---");
      u8g2.drawStr(0, 10, "5. Bitmap Test");
      u8g2.drawStr(0, 25, "All Hardware Working!");
      u8g2.drawXBMP(59, 40, 10, 13, SD_Card);
      u8g2.sendBuffer();
      
      Serial.println("[Core 1] Diagnostic complete. Halting.");
      
      for(int i = 0; i < 3; i++){
        fill_solid(leds, NUM_LEDS, CRGB::Green); FastLED.show(); delay(200);
        FastLED.clear(); FastLED.show(); delay(200);
      }
      
      while(true) { delay(100); } 
      break;
    }
  }
}