#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD 16×2 I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// โหมด
#define LIGHT_MODE 0
#define SOUND_MODE 1
int currentMode = LIGHT_MODE;

// ปุ่มและไฟบอกสถานะ
#define MODE_BUTTON_PIN    14
#define START_BUTTON_PIN   27
const int lightIndicatorPin = 5;
const int soundIndicatorPin = 33;

// ตัวแปรจับเวลา Buzzer
bool     bzTriggered = false;
unsigned long bzStart = 0;
unsigned long bzElapsed;

// จำนวนช่องและพิน LED/ปุ่ม
#define CHANNEL_COUNT 3
const char* ledName[CHANNEL_COUNT]    = { "RED", "YELLOW", "GREEN" };
const int   led_pin[CHANNEL_COUNT]    = { 32, 13, 15 };
const int   button_pin[CHANNEL_COUNT] = { 16,  4, 26 };
bool timing[CHANNEL_COUNT]            = { false, false, false };
unsigned long startTime[CHANNEL_COUNT]= { 0, 0, 0 };
bool lastButtonState[CHANNEL_COUNT]   = { HIGH, HIGH, HIGH };
int  lastChannel                      = -1;

// ESP-NOW
typedef struct { char command[16]; } struct_message;
struct_message incomingMessage, outgoingMessage;
volatile bool commandReceived = false;

uint8_t peerAddr[] = { 0x10, 0x06, 0x1C, 0x41, 0xDC, 0xFC };
esp_now_peer_info_t peerInfo;

// รับคำสั่งจาก Player
void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  memcpy(&incomingMessage, data, sizeof(incomingMessage));
  String cmd = String(incomingMessage.command);
  cmd.trim();

  // Light Mode: รับ STOP (สีใดๆ)  
  if (currentMode == LIGHT_MODE && 
      (cmd == "RED" || cmd == "YELLOW" || cmd == "GREEN")) {
    commandReceived = true;
  }
  // Sound Mode: รับ BUZZER_OFF → หยุดจับเวลา
  else if (currentMode == SOUND_MODE && cmd == "BUZZER_OFF") {
    bzElapsed   = millis() - bzStart;
    bzTriggered = false;
    // แสดงผลสุดท้าย
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Buzzer Stopped");
    lcd.setCursor(0, 1);
    lcd.print("Time:");
    lcd.print(bzElapsed / 1000.0, 2);
    lcd.print(" sec");
    delay(2000);
    // กลับไปแสดงโหมด
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Mode:");
    lcd.setCursor(0, 1);
    lcd.print("Sound Mode");
  }
}

// ส่งคำสั่งผ่าน ESP-NOW
void sendCommand(const char* cmd) {
  strcpy(outgoingMessage.command, cmd);
  esp_now_send(peerAddr, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
}

// แสดงผลโหมดบนจอ
void updateModeDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Mode:");
  lcd.setCursor(0, 1);
  lcd.print(currentMode == LIGHT_MODE ? "Light Mode" : "Sound Mode");
  delay(200);
}

void setup() {
  Serial.begin(115200);

  // ตั้งค่า ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  memcpy(peerInfo.peer_addr, peerAddr, 6);
  peerInfo.channel = 0; peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // ตั้งค่า LCD
  lcd.begin();
  lcd.backlight();

  // ตั้งค่าปุ่มและ LED
  pinMode(MODE_BUTTON_PIN,   INPUT_PULLUP);
  pinMode(START_BUTTON_PIN,  INPUT_PULLUP);
  pinMode(lightIndicatorPin, OUTPUT);
  pinMode(soundIndicatorPin, OUTPUT);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    pinMode(led_pin[i], OUTPUT);
    pinMode(button_pin[i], INPUT_PULLUP);
    digitalWrite(led_pin[i], LOW);
  }

  lcd.clear(); 
  lcd.setCursor(0, 0); lcd.print("Rocket Switch");
  delay(1500);
  updateModeDisplay();
}

void loop() {
  // 1) สลับโหมด
  int newMode = (digitalRead(MODE_BUTTON_PIN) == LOW) ? LIGHT_MODE : SOUND_MODE;
  if (newMode != currentMode) {
    currentMode = newMode;
    // รีเซ็ตสถานะเมื่อเปลี่ยนโหมด
    bzTriggered = false;
    commandReceived = false;
    lastChannel = -1;
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      timing[i] = false;
      digitalWrite(led_pin[i], LOW);
      lastButtonState[i] = HIGH;
    }
    lcd.clear();
    updateModeDisplay();
  }

  // 2) อัปเดตไฟบอกสถานะโหมด
  digitalWrite(lightIndicatorPin, currentMode == LIGHT_MODE ? HIGH : LOW);
  digitalWrite(soundIndicatorPin, currentMode == SOUND_MODE ? HIGH : LOW);

  // 3) Sound Mode:  
  if (currentMode == SOUND_MODE) {
    // 3.1) กดปุ่ม START → ส่ง BUZZER_ON + เริ่มจับเวลา
    if (!bzTriggered && digitalRead(START_BUTTON_PIN) == LOW) {
      delay(50);
      if (digitalRead(START_BUTTON_PIN) == LOW) {
        sendCommand("BUZZER_ON");
        bzTriggered = true;
        bzStart     = millis();
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Buzzer Timing...");
        lcd.setCursor(0, 1); lcd.print("0.00 sec");
        delay(300);
      }
    }
    // 3.2) ขณะจับเวลา: อัปเดต LCD แบบเรียลไทม์
    if (bzTriggered) {
      lcd.setCursor(0, 1);
      float t = (millis() - bzStart) / 1000.0;
      lcd.print(t, 2);
      lcd.print(" sec");
    }
  }

  // 4) Light Mode: (เดิม)
  if (currentMode == LIGHT_MODE) {
    static const char* cmds[CHANNEL_COUNT] = { "LED_RED", "LED_YELLOW", "LED_GREEN" };
    // 4.1) รับ STOP จาก Player
    if (commandReceived && lastChannel >= 0 && timing[lastChannel]) {
      unsigned long elapsed = millis() - startTime[lastChannel];
      timing[lastChannel] = false;
      digitalWrite(led_pin[lastChannel], LOW);
      sendCommand("LED_OFF");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(ledName[lastChannel]); lcd.print(" Time:");
      lcd.setCursor(0, 1);
      lcd.print(elapsed / 1000.0, 2); lcd.print(" sec");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("Ready for");
      lcd.setCursor(0, 1); lcd.print("next round");
      lastChannel     = -1;
      commandReceived = false;
    }
    // 4.2) Local START
    for (int i = 0; i < CHANNEL_COUNT; i++) {
      bool btn = digitalRead(button_pin[i]);
      if (btn == LOW && lastButtonState[i] == HIGH && !timing[i]) {
        sendCommand(cmds[i]);
        timing[i]     = true;
        startTime[i]  = millis();
        lastChannel   = i;
        digitalWrite(led_pin[i], HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(ledName[i]); lcd.print(" Timing...");
        lcd.setCursor(0, 1);
        lcd.print("0.00 sec");
      }
      lastButtonState[i] = btn;
    }
    // 4.3) Real-time update
    if (lastChannel >= 0 && timing[lastChannel]) {
      lcd.setCursor(0, 1);
      lcd.print((millis() - startTime[lastChannel]) / 1000.0, 2);
      lcd.print(" sec");
    }
  }
}
