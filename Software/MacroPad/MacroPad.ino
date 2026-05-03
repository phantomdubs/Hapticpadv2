#include <Arduino.h>
#include <U8g2lib.h>
#include <SimpleFOC.h>
#include <Adafruit_TinyUSB.h>
#include <SPI.h>
#include <SdFat.h>
// This trick makes your old code and xmlData.ino compatible automatically
#define File FsFile
SdFat SD;
#include <FastLED.h>

const unsigned char SD_Card [] PROGMEM = {
	0xff, 0x01, 0x55, 0x01, 0x55, 0x01, 0xff, 0x01, 0x01, 0x03, 0x2d, 0x02, 0x45, 0x03, 0x49, 0x01, 
	0x2d, 0x03, 0x01, 0x02, 0xfd, 0x02, 0x01, 0x02, 0xff, 0x03
};

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define WHEEL_CLICKY    0
#define WHEEL_TWIST     1
#define WHEEL_MOMENTUM  2

#define KEY_LEFT_CTRL 17

#define NUM_LEDS 20
#define DATA_PIN 15

CRGB leds[NUM_LEDS];

// --- SLEEP TIMER VARIABLES ---
unsigned long lastActivityTime = 0;
const unsigned long SLEEP_TIMEOUT = 300000; // 5 minutes (300,000 ms)
bool isAsleep = false; // <--- ADD THIS LINE

//User Configurable LED Settings
byte primaryColour[3] = {0,50,50}; //Red, Green, Blue
byte secondaryColour[3] = {0,0,0};
int ledBrightness = 100;
int ledSpeed = 50;

//LED Sequence Variables
float redScale;
float greenScale;
float blueScale;
int sequenceStep = 0;
int ledMode = 3;
unsigned long ledTimer;

//LED Halo Variables
int haloCount = 0; //Which is the brightest LED in the sequence

//LED Breath Variables
bool breathIncrease = true;

//LED Bands
bool evenNumber = false;
int loopCounter;

bool debug = false;

//SD Pins
const int _MISO = 4;
const int _MOSI = 3;
const int _CS = 5;
const int _SCK = 2;

bool sdDetected = false;

U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI u8g2(U8G2_R0, 28, 22, 6, 7, 8);

BLDCMotor motor = BLDCMotor(7);
BLDCDriver6PWM driver = BLDCDriver6PWM(20, 21, 18,19, 16, 17);

//float target_velocity = 6;
Commander command = Commander(Serial);

Encoder encoder = Encoder(27, 26, 1024);
void doA(){encoder.handleA(); lastActivityTime = millis();}
void doB(){encoder.handleB(); lastActivityTime = millis();}

// angle set point variable
float target_angle = 0; //Radians. 1 Rad = 57.2958 Deg
float new_target_angle;

float target_velocity = 0;
float last_velocity = 0;
float testFactor;

float angle_step = radians(360/40);
float encoderAngle;
float lastEncoderAngle;
bool keyPressed = false;
bool wheelKeyPressed = false;

//PID Values

float Clicky_P;
float Clicky_I;
float Twist_P;
float Twist_I;
float Momentum_P;
float Momentum_I;
float motorP = 0.5;
float motorI = 0;
float motorD = 0;

long timer;
long aceltimer;
long mouseTimer;
float interval;
unsigned long wheelKeyTimer;
unsigned long keyTimer;

long debounceTimer;
bool decelDetected = false;
bool decelerating = false;

int wheelMode = 0;
int lastWheelMode = 0;
bool wheelModeChanged = true;

bool FOC_Ready = false;

#define buttonCount 8 //Number of buttons connected
byte buttonPins[buttonCount] = {9, 1, 0, 12, 11, 10, 13, 14}; //1,2,3,4,5,6
int lastButtonState[buttonCount];

//Eeprom Memory
int activeProfile = 0;
//int activePage = 1;

uint8_t icon1[24][3]; //Used to store Icon 1 Data
uint8_t icon2[24][3]; //Used to store Icon 2 Data
uint8_t icon3[24][3]; //Used to store Icon 3 Data
uint8_t icon4[24][3]; //Used to store Icon 4 Data
uint8_t icon5[24][3]; //Used to store Icon 5 Data
uint8_t icon6[24][3]; //Used to store Icon 6 Data

// ---- Profile data ----
#define maxProfileNameLength 32
#define maxProfiles 256
char profileName[maxProfileNameLength];
char profileNames[maxProfileNameLength][maxProfiles];
char buttonLabel[6][32];
unsigned long profileChangeTimer;
bool profilePlusStarted = false;
bool profileMinusStarted = false;
bool profileSelectMenu = false;

uint8_t wheelAction;
uint8_t macroAction[6][3];   // decimal values
uint16_t macroDelay[6][3];
float targetAngle;

// ---- Global ----
uint16_t totalProfiles = 0;

File root;

uint8_t const desc_keyboard_report[] =
{
  TUD_HID_REPORT_DESC_KEYBOARD()
};

uint8_t const desc_mouse_report[] =
{
  TUD_HID_REPORT_DESC_MOUSE()
};

Adafruit_USBD_HID usb_keyboard(desc_keyboard_report, sizeof(desc_keyboard_report), HID_ITF_PROTOCOL_KEYBOARD, 2, false);
Adafruit_USBD_HID usb_mouse(desc_mouse_report, sizeof(desc_mouse_report), HID_ITF_PROTOCOL_MOUSE, 2, false);


// --- USB FLASH DRIVE LOGIC ---
Adafruit_USBD_MSC usb_msc;
volatile bool disk_mode_active = false;

int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize) {
  bool rc;
#if SD_FAT_VERSION >= 20000 // Automatically handles the vocabulary difference!
  rc = SD.card()->readSectors(lba, (uint8_t*) buffer, bufsize/512);
#else
  rc = SD.card()->readBlocks(lba, (uint8_t*) buffer, bufsize/512);
#endif
  return rc ? bufsize : -1;
}

int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  bool rc;
#if SD_FAT_VERSION >= 20000
  rc = SD.card()->writeSectors(lba, (uint8_t*) buffer, bufsize/512);
#else
  rc = SD.card()->writeBlocks(lba, (uint8_t*) buffer, bufsize/512);
#endif
  return rc ? bufsize : -1;
}

void msc_flush_cb (void) {
  SD.card()->syncDevice();
}
// -----------------------------



void managePowerState() {
  // Sleep if USB suspends OR if the 5-minute timer runs out
  bool shouldSleep = TinyUSBDevice.suspended() || (millis() - lastActivityTime > SLEEP_TIMEOUT);

  if (shouldSleep) {
    if (!isAsleep) {
      u8g2.setPowerSave(1); // Turn off screen
      usb_keyboard.keyboardRelease(0); // Release all keys
      fill_solid(leds, NUM_LEDS, CRGB::Black); // Turn off LEDs
      FastLED.show();
      isAsleep = true; // <--- Set global flag
    }
  } else {
    if (isAsleep) {
      u8g2.setPowerSave(0); // Wake screen
      fill_solid(leds, NUM_LEDS, CRGB(primaryColour[0], primaryColour[1], primaryColour[2])); 
      FastLED.show();
      isAsleep = false; // <--- Clear global flag
    }
  }
}

void setup() { // Core 0
  // --- THE BOOT BOUNCER ---
  pinMode(11, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  delay(50); 

  if (digitalRead(11) == LOW) {
    disk_mode_active = true;

    // 1. TEMPORARILY UNPLUG FROM WINDOWS
    TinyUSBDevice.detach();
    delay(200); // Give Windows time to register the disconnect

    // 2. SET CORRECT HARDWARE SPI PINS FOR SD CARD
    SPI.setRX(_MISO); 
    SPI.setTX(_MOSI); 
    SPI.setSCK(_SCK);

    // 3. INITIALIZE DISPLAY
    u8g2.begin();
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(0, 15, "USB DRIVE MODE");
      u8g2.drawStr(0, 30, "Mounting SD...");
    } while ( u8g2.nextPage() );

    // 4. MOUNT SD CARD BEFORE STARTING USB MSC
    if (SD.begin(_CS, SD_SCK_MHZ(40))) {
      
      // Setup the Flash Drive parameters now that SD is ready
      usb_msc.setID("Pico", "SD Card", "1.0");
      usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
      usb_msc.setCapacity(SD.card()->sectorCount(), 512);
      usb_msc.setUnitReady(true);
      usb_msc.begin(); 

      // 5. RE-PLUG INTO WINDOWS
      TinyUSBDevice.attach();

      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(0, 10, "USB DRIVE ACTIVE");
        u8g2.drawStr(0, 25, "1. Eject on PC");
        u8g2.drawStr(0, 35, "2. Press Button 1");
        u8g2.drawStr(0, 45, "   to Reboot");
      } while ( u8g2.nextPage() );

    } else {
      // Re-attach even on fail so the port isn't left completely dead
      TinyUSBDevice.attach(); 
      
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_5x7_tf);
        u8g2.drawStr(0, 15, "SD MOUNT FAILED!");
        u8g2.drawStr(0, 30, "Check Card");
      } while ( u8g2.nextPage() );
    }

    return; // Stay in USB mode, bypass normal boot
  }
  // --- END BOOT BOUNCER ---


  driver.voltage_power_supply = 5;
  driver.voltage_limit = 5;
  driver.init();

  encoder.init();
  encoder.enableInterrupts(doA, doB);
  motor.linkSensor(&encoder);

  driver.voltage_power_supply = 5;
  driver.init();

  motor.linkDriver(&driver);
  motor.voltage_sensor_align = 3;

  motor.PID_velocity.D = 0;

  motor.voltage_limit = 3;

  motor.PID_velocity.output_ramp = 1000;
  motor.LPF_velocity.Tf = 0.025f;//0.01f;
  motor.P_angle.P = 20;
  motor.velocity_limit = 4;

  motor.init();
  motor.initFOC();

  Serial.begin(115200);
  if(debug){
    while(!Serial){}
  }
  Serial.println("Motor ready!");
  FOC_Ready = true;
}

void setup1(){ //core 1
  delay(200); // Give Core 0 a tiny head start to check the button
  
  if(disk_mode_active) return; // If flash drive mode is on, SHUT DOWN this core entirely.

  // --- NORMAL MACRO PAD MODE ---
  usb_keyboard.begin();
  usb_mouse.begin();
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);

  SPI.setRX(_MISO);
  SPI.setTX(_MOSI);
  SPI.setSCK(_SCK);

  initialiseSD();
  loadSettings("/config.xml");
  calculateColourMultiplier();

  u8g2.begin();
  for(int i = 0; i < buttonCount; i++){
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void buttonRead(){ //Read button inputs and set state arrays.
for (int i = 0; i < buttonCount; i++){
    int input = !digitalRead(buttonPins[i]);
    if (input != lastButtonState[i]){
      lastButtonState[i] = input;
      
      lastActivityTime = millis(); // <--- WAKES UP THE DEVICE ON BUTTON PRESS
    }
  }
  if(sdDetected && !profileSelectMenu){
    for(int i = 0; i < 6; i++){
      if(lastButtonState[i]){
        macroOutput(i);
        keyPressed = true;
        keyTimer = millis();
      }
    }
    if(lastButtonState[6]){ //Next Profile
      if(!profilePlusStarted){
        profilePlusStarted = true;
        profileChangeTimer = millis();
      }
      if(profilePlusStarted && profileChangeTimer + 100 < millis()){
        profileSelectMenu = true;
        profileChangeTimer = millis();
      }
    } else {
      if(profilePlusStarted){
        if(activeProfile < totalProfiles - 1){
          activeProfile++;
          loadProfile("/config.xml", activeProfile);
          calculateColourMultiplier();
          storeLastProfile();
          Serial.print("Stored last profile = ");
          Serial.println(activeProfile);
          loadButtonIcons();
          delay(100);
        }
      }
    }
    if(lastButtonState[7]){ //Previous Profile
      if(!profileMinusStarted){
        profileMinusStarted = true;
        profileChangeTimer = millis();
      } else if(profileMinusStarted && profileChangeTimer + 100 < millis()){
        profileSelectMenu = true;
        profileChangeTimer = millis();
        //delay(500);
        //Serial.println("Trigger Menu");
      }
    } else {
      if(profileMinusStarted){
        if(activeProfile > 0 ){
          activeProfile--;
          loadProfile("/config.xml", activeProfile);
          calculateColourMultiplier();
          storeLastProfile();
          Serial.print("Stored last profile = ");
          Serial.println(activeProfile);
          loadButtonIcons();
          delay(100);
        }
      }
    }
    if(!lastButtonState[7] && !lastButtonState[6]){
      profileMinusStarted = false;
      profilePlusStarted = false;
    }
      
    if(keyPressed && keyTimer + 50 < millis()){
      usb_keyboard.keyboardRelease(0);
      keyPressed = false;
    }
  } else if(!sdDetected && !profileSelectMenu){
    if(lastButtonState[4]){
      initialiseSD();
      delay(100);
    }
  } else {
    if(profileChangeTimer + 500 < millis()){
      if(lastButtonState[7] || lastButtonState[6]){
        profileSelectMenu = false;
        profileMinusStarted = false;
        profilePlusStarted = false;
        loadProfile("/config.xml", activeProfile);
        calculateColourMultiplier();
        storeLastProfile();
        Serial.print("Stored last profile = ");
        Serial.println(activeProfile);
        loadButtonIcons();
        delay(200);
      }
    }
  }
}

void initialiseSD(){
SD.end(); //Attempt to close any previous SD sessions

  if (!SD.begin(_CS)) {
    Serial.println("initialization failed!");
    return;
  } else {
    sdDetected = true;
    Serial.println("SD Initialised!");
  }

  totalProfiles = countProfiles("/config.xml");
  if(debug){
    Serial.print("Profiles found: ");
    Serial.println(totalProfiles);
  }

  activeProfile = readLastProfile();
  Serial.print("Active Profile = ");
  Serial.println(activeProfile);

  loadProfile("/config.xml", activeProfile);
  calculateColourMultiplier();
  loadButtonIcons();
}

void loop() {
  if (disk_mode_active) { 
    tud_task(); // Keep the USB traffic flowing!
    
    // Check if Button 1 (Pin 9) is pressed to exit Flash Drive Mode
    if (digitalRead(9) == LOW) {
      delay(100); // Quick debounce
rp2040.reboot(); // The correct RP2040-specific restart command!
    }
    
    return; // Freeze the motors
  }

 
  motor.loopFOC();
  // ... rest of the original code ...

  if(lastWheelMode != wheelMode){
    wheelModeChanged = true;
  }
  lastWheelMode = wheelMode;

  if(!profileSelectMenu){
    if(wheelMode == 0){
      notchyWheel();
    } else if(wheelMode == 1){
      twistScroll();
    } else if(wheelMode == 2){
      freeSpinning();
    }
  } else {
    notchyWheel();
  }
}

void macroOutput(int button){
  uint8_t keycode[6] = { 0 };
  int modifier = 0;
        
  for(int i = 0; i < 3; i++){
    delay(macroDelay[button][i]);
    if(macroAction[button][i] != 0){
      keycode[0] = convertKeycode(macroAction[button][i]);
      if(checkModifiers(macroAction[button][i]) != 0){
        modifier = checkModifiers(macroAction[button][i]);
        Serial.println("Modifier Detected");
      } else {
        usb_keyboard.keyboardReport(0, modifier, keycode);
      }
    }
  }
}

void loop1() {

  
if (disk_mode_active) return; // Freeze the OLED and Buttons completely

managePowerState(); // <--- Check power state every loop

  u8g2.firstPage();
  // ... rest of the original code ...

  u8g2.firstPage();
  buttonRead();
 if(ledTimer + ledSpeed < millis() && !isAsleep){ // <--- Add && !isAsleep
    if(ledMode == 0){
      haloLED();
    } else if(ledMode == 1){
      breathLED();
    } else if(ledMode == 2){
      ledBand();
    } else if(ledMode == 3){
      rainbowLED();
    } else if(ledMode == 4){
      solidLED();
    } else {
      offLED();
    }
    ledTimer = millis();
  }
  
  do {
    if(sdDetected){
      if(!profileSelectMenu){
        drawGrid();
        drawActiveProfile();
      } else {
        drawProfileMenu();
      }

      encoderAngle = encoder.getAngle();

      if(wheelMode == 2){
        if(abs(lastEncoderAngle - encoderAngle) > 0.1){
          wheelActionCheck();
          usb_mouse.mouseScroll(0, (lastEncoderAngle - encoderAngle) * 10, 0);
          lastEncoderAngle = encoderAngle;
        } else {
          cancelWheelAction();
        }
      }
    } else {
      u8g2.drawXBMP(59, 10, 10, 13, SD_Card);
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(17, 36, "No SD Card Detected!");
      u8g2.drawStr(53, 60, "Retry");
    }

  } while ( u8g2.nextPage() );
}

bool storeLastProfile(){
  Serial.println("--- ATTEMPTING TO SAVE PROFILE ---");
  
  // 1. Try to remove the file (using a safe 8.3 filename)
  bool removed = SD.remove("/last.txt");
  if(removed) {
    Serial.println("Success: Old last.txt deleted.");
  } else {
    Serial.println("Notice: Could not delete last.txt (might not exist yet).");
  }

  // 2. Try to create the new file
  File file = SD.open("/last.txt", FILE_WRITE);
  if (file) {
    file.print(activeProfile);
    file.close();
    Serial.println("Success: New profile number saved to last.txt!");
    return true;
  } else {
    Serial.println("FATAL ERROR: SD.open failed. Cannot write to card!");
    return false;
  }
}

int readLastProfile(){
  // Look for the new safe filename
  File file = SD.open("/last.txt"); 
  
  // If it doesn't exist yet, fallback to the old one so it doesn't break
  if (!file){
    file = SD.open("/lastProfile"); 
    if (!file) return 0; // If neither exist, return profile 0
  }

  int savedProfile = file.parseInt(); 
  file.close(); 
  return (uint8_t)savedProfile; 
}
