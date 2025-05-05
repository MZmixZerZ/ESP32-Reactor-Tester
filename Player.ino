#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// จอ LCD 16×2 I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ปุ่มส่งสี
#define RED_BUTTON_PIN     33
#define YELLOW_BUTTON_PIN  25
#define GREEN_BUTTON_PIN   26

// ปุ่ม Stop สำหรับ Buzzer timing
#define STOP_BUTTON_PIN    23

// LED แสดงผลจาก Controller
#define RED_LED_PIN        18
#define YELLOW_LED_PIN     19
#define GREEN_LED_PIN      21

// Buzzer ต่อที่ pin 22 (active-LOW)
#define BUZZER_PIN         22  

// แม็ปปุ่มกับชื่อคำสั่งสี
const uint8_t buttonPins[3] = { RED_BUTTON_PIN, YELLOW_BUTTON_PIN, GREEN_BUTTON_PIN };
const char*   cmdNames[3]   = { "RED", "YELLOW", "GREEN" };

// MAC ของ Controller
uint8_t controllerAddress[] = { 0x78, 0x42, 0x1C, 0x66, 0x76, 0x10 };

typedef struct { char command[16]; } message_t;
message_t outgoingMessage, incomingMessage;
esp_now_peer_info_t peerInfo;

// สถานะ LED & Buzzer timing
bool ledState[3]      = { false, false, false };
bool buzzerOn         = false;
unsigned long bzStart = 0;

void sendCommand(const char* cmd) {
  strcpy(outgoingMessage.command, cmd);
  esp_now_send(controllerAddress, (uint8_t*)&outgoingMessage, sizeof(outgoingMessage));
}

void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  memcpy(&incomingMessage, data, sizeof(incomingMessage));
  String cmd = String(incomingMessage.command);
  cmd.trim();

  // --- จัดการ LED ---
  // ปิดทุกดวงก่อน
  for (int i = 0; i < 3; i++) {
    ledState[i] = false;
    digitalWrite((int[]){RED_LED_PIN, YELLOW_LED_PIN, GREEN_LED_PIN}[i], LOW);
  }
  if      (cmd == "LED_RED")    ledState[0] = true;
  else if (cmd == "LED_YELLOW") ledState[1] = true;
  else if (cmd == "LED_GREEN")  ledState[2] = true;
  // ถ้ามี LED_ON ให้เปิด
  for (int i = 0; i < 3; i++) {
    if (ledState[i])
      digitalWrite((int[]){RED_LED_PIN, YELLOW_LED_PIN, GREEN_LED_PIN}[i], HIGH);
  }

  // --- จัดการ Buzzer timing ---
  if (cmd == "BUZZER_ON") {
    // เปิด buzzer และเริ่มจับเวลา
    digitalWrite(BUZZER_PIN, LOW);
    buzzerOn = true;
    bzStart  = millis();

    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Buzzer Timing");
    lcd.setCursor(0, 1); lcd.print("0.00 sec");
  }
}

void OnDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  // ไม่แสดงอะไร หรือใช้ Serial.print สำหรับ debug ก็ได้
}

void setup() {
  Serial.begin(115200);

  // จอ LCD
  lcd.begin();
  lcd.backlight();

  // ปุ่มสี
  for (int i = 0; i < 3; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  // ปุ่ม Stop
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);

  // LED
  pinMode(RED_LED_PIN,    OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN,  OUTPUT);
  digitalWrite(RED_LED_PIN,    LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN,  LOW);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH); // ปิด buzzer (active-LOW)

  // เริ่ม ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) { delay(1000); }
  }
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, controllerAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  // 1) ส่งปุ่มสี (edge detect)
  static bool lastBtnState[3] = { HIGH, HIGH, HIGH };
  for (int i = 0; i < 3; i++) {
    bool cur = digitalRead(buttonPins[i]);
    if (cur == LOW && lastBtnState[i] == HIGH) {
      sendCommand(cmdNames[i]);
    }
    lastBtnState[i] = cur;
  }

  // 2) ถ้า buzzerOn ให้แสดงเวลาเรียลไทม์
  if (buzzerOn) {
    float t = (millis() - bzStart) / 1000.0;
    lcd.setCursor(0, 1);
    lcd.print(t, 2);
    lcd.print(" sec");
  }

  // 3) ปุ่ม Stop (edge detect) เพื่อหยุด buzzer
  static bool lastStopState = HIGH;
  bool curStop = digitalRead(STOP_BUTTON_PIN);
  if (curStop == LOW && lastStopState == HIGH && buzzerOn) {
    // หยุด buzzer
    digitalWrite(BUZZER_PIN, HIGH);
    buzzerOn = false;

    // คำนวณเวลาและส่งกลับ
    unsigned long elapsed = millis() - bzStart;
    sendCommand("BUZZER_OFF");

    // แสดงผลสุดท้าย 2 วินาที แล้วเคลียร์จอ
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time:");
    lcd.print(elapsed / 1000.0, 2);
    lcd.print(" sec");
    delay(2000);
    lcd.clear();
  }
  lastStopState = curStop;
}
