#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DEBUG
#define BUTTON_PIN 2
#define LED1_PIN 6
#define LED2_PIN 7
#define BUZZER_PIN 9
#define LED_BUILTIN_PIN 13


#define ARR_CNT 5
#define CMD_SIZE 60

char lcdLine1[17] = "";
char lcdLine2[17] = "";
char sendBuf[CMD_SIZE];
char recvId[10] = "SSS_SQL"; 

bool buzzerActive = false;
bool eventActiveFlag = false;

SoftwareSerial BTSerial(10, 11); // RX, TX

char lastDeviceId[10] = "";

unsigned long buzzerOffMillis = 0;   // 버튼 눌러서 BUZZER OFF 화면 시작 시간
const unsigned long buzzerDisplayTime = 10000; // 10초 유지
bool buzzerOffScreen = false; // 화면이 10초 동안 유지 중인지 플래그

char lcdNormalU[17] = "SUPERVISOR  MODE";  // 평소 보여줄 윗 줄 문자열
char lcdNormalD[17] = " SECURITY SAFE!";  // 평소 보여줄 아랫 줄문자열

// ===== 함수 원형 선언 =====
void lcdDisplay(int x, int y, char * str);
void bluetoothEvent();
// ==========================

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("setup() start!");
#endif
  lcd.init();
  lcd.backlight();
  lcdDisplay(0, 0, lcdLine1);
  lcdDisplay(1, 0, lcdLine2);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);


  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  BTSerial.begin(9600); 
}

void loop()
{
  if (BTSerial.available())
        bluetoothEvent();

    // 화면 출력 처리
    if (buzzerOffScreen) {
        // BUZZER OFF 화면 10초 유지
        if (millis() - buzzerOffMillis >= buzzerDisplayTime) {
            buzzerOffScreen = false; // 10초 끝나면 화면 복귀
            lcdDisplay(1, 0, lcdNormalD); // 평소 화면으로 복귀
        }
    } else {
        // 평소 화면 출력
        lcdDisplay(0, 0, lcdNormalU);
        if (!eventActiveFlag) {
            lcdDisplay(1, 0, lcdNormalD);
        }
    }

    // ----- 버튼 눌렸을 때
    if(eventActiveFlag){
        if (digitalRead(BUTTON_PIN) == LOW && buzzerActive)  
        {
            Serial.println("button_press");
            buzzerActive = false;       
            digitalWrite(BUZZER_PIN, LOW);
            digitalWrite(LED1_PIN, LOW);    // 추후 번호에 따라 꺼지는게 다르게 수정해야함
            digitalWrite(LED2_PIN, LOW);

            sprintf(lcdLine2, "-%s-  BUZZER OFF!", lastDeviceId);
            lcdDisplay(1, 0, lcdLine2);
#ifdef DEBUG
            Serial.println(sendBuf);
#endif
            eventActiveFlag = false;

            // 10초 화면 유지 시작
            buzzerOffMillis = millis();
            buzzerOffScreen = true;
        }
    }
}


void bluetoothEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len = BTSerial.readBytesUntil('\n', recvBuf, sizeof(recvBuf) - 1);

  recvBuf[len] = '\0';   // 문자열 종료 처리

// ★ 버퍼에 남은 찌꺼기(\r, \n, 공백 등) 모두 제거
while (BTSerial.available() > 0) {
    int c = BTSerial.peek();
    if (c == '\r' || c == '\n' || c == ' ') {
        BTSerial.read();  // 버리고 다음 확인
    } else {
        break;            // 더는 찌꺼기 아님 → 중단
    }
}

#ifdef DEBUG
  Serial.print("Recv : ");
  Serial.println(recvBuf);
#endif

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }

  // SECURITY 이벤트 처리
  if (pArray[1] && !strcmp(pArray[1], "SECURITY")) {

    // 공통 동작: 부저 ON
    eventActiveFlag = true;
    buzzerActive = true;
    digitalWrite(BUZZER_PIN, HIGH);

    // ID 확인해서 해당 LED ON
    if (pArray[3]) {
        strcpy(lastDeviceId, pArray[3]);  // ex) "1" or "2"

        if (!strcmp(pArray[3], "1")) {
            digitalWrite(LED1_PIN, HIGH);
        } else if (!strcmp(pArray[3], "2")) {
            digitalWrite(LED2_PIN, HIGH);
        }
    }

    // 상황 코드별 LCD 메시지 출력
    if (pArray[2]) {
        if (!strcmp(pArray[2], "1")) {       // SYSLOCK
            sprintf(lcdLine2, "-%s- SYSTEMLOCK!!", pArray[3]);
        } else if (!strcmp(pArray[2], "2")) { // FORCE SAFE
            sprintf(lcdLine2, "-%s- FORCE SAFE!", pArray[3]);
        } else {
            sprintf(lcdLine2, " SEC NUM WRONG!");
        }
        lcdDisplay(1, 0, lcdLine2);
    }

    // 서버로 상태 전송
    sprintf(sendBuf, "[%s]SECURITY@%s@%s\n", recvId, pArray[2], pArray[0]);
    BTSerial.write(sendBuf);
#ifdef DEBUG
    Serial.println(sendBuf);
#endif
    // 서버로 상태 전송
    sprintf(sendBuf, "[%s]SECURITY@%s@%s\n", recvId, pArray[2], pArray[0]);
    BTSerial.write(sendBuf);
#ifdef DEBUG
    Serial.println(sendBuf);
#endif
    return; // SECURITY 처리 후 다른 이벤트는 무시
  }

  // 기존 BUZZER 명령 제거, 필요 없음
}

void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(y, x);
  lcd.print(str);
  for (int i = len; i > 0; i--)
    lcd.write(' ');
}