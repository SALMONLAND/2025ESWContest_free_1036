// 라이브러리 목록
#include <DS1302.h>             // RTC 모듈 제어용
#include <Wire.h>               // I2C 통신용
#include <LiquidCrystal_I2C.h>  // LCD 제어용
#include <Servo.h>              // 서보모터 제어용

// 핀 정의 (Wemos D1 R32 ESP8266 핀 할당)
#define LCD_SCL_PIN    D3    // LCD SCL  
#define LCD_SDA_PIN    D4    // LCD SDA
#define RTC_RST        D12   // DS1302 RST
#define RTC_DAT        D13  // DS1302 DAT
#define RTC_CLK        D11   // DS1302 CLK
#define SERVO_PIN      D7    // 서보모터
#define BUTTON_1       D9    // 시간 증가 버튼
#define BUTTON_2       D0    // 횟수 버튼
#define BUTTON_3       D8    // 상호작용 버튼
#define BUZZER_PIN     D2    // 부저 핀

// 버튼 핀 재정의 (가독성을 위해)
#define TIME_BTN       BUTTON_1  // 시간 증가 버튼
#define VALUE_BTN      BUTTON_2  // 횟수 받는 버튼
#define TINTERATION_BTN BUTTON_3  // 상호작용 버튼

// LCD 객체 생성 (I2C 주소는 ESP8266에서 보통 0x27 또는 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 객체 생성
DS1302 rtc(RTC_RST, RTC_DAT, RTC_CLK);  // RTC 객체
Servo servo;  // 서보모터 객체

// 변수들
int count = 1;  // 횟수 저장 변수
int currentHour = 0;   // 현재 설정 중인 시간
int currentMinute = 0; // 현재 설정 중인 분
String timeList[10];   // 시간 데이터 리스트 (최대 10개) - "HH:MM" 형식
int timeCount = 0;     // 리스트에 저장된 시간 개수

// 버튼 상태 변수들
bool lastValueBtn = HIGH;
bool lastTimeBtn = HIGH;
bool lastInteractionBtn = HIGH;

// 부저 함수들
void beepShort() {
  tone(BUZZER_PIN, 1000, 100);  // 1000Hz 주파수로 100ms 동안 울림
  delay(150);
}

void beepLong() {
  tone(BUZZER_PIN, 800, 300);   // 800Hz 주파수로 300ms 동안 울림
  delay(350);
}

void beepSuccess() {
  tone(BUZZER_PIN, 1200, 200);  // 높은 음으로 성공 알림
  delay(250);
  tone(BUZZER_PIN, 1500, 200);
  delay(250);
}

void beepAlert() {
  // 약 복용 시간 알림음 (3번 반복)
  for(int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000, 200);
    delay(250);
    tone(BUZZER_PIN, 1500, 200);
    delay(250);
  }
}

// 시간 배열 정렬 함수 (중복 제거 포함)
void sortAndRemoveDuplicates() {
  // 버블 정렬로 시간 배열 정렬
  for(int i = 0; i < timeCount - 1; i++) {
    for(int j = 0; j < timeCount - i - 1; j++) {
      if(timeList[j] > timeList[j + 1]) {
        // 위치 바꾸기
        String temp = timeList[j];
        timeList[j] = timeList[j + 1];
        timeList[j + 1] = temp;
      }
    }
  }
  
  // 중복 제거
  int newCount = 0;
  for(int i = 0; i < timeCount; i++) {
    bool isDuplicate = false;
    // 이미 새 배열에 있는지 확인
    for(int j = 0; j < newCount; j++) {
      if(timeList[i] == timeList[j]) {
        isDuplicate = true;
        break;
      }
    }
    // 중복이 아니면 앞쪽으로 이동
    if(!isDuplicate) {
      timeList[newCount] = timeList[i];
      newCount++;
    }
  }
  timeCount = newCount;  // 새로운 개수로 업데이트
  
  // 정렬된 시간 출력
  Serial.println("=== Sorted Times ===");
  for(int i = 0; i < timeCount; i++) {
    Serial.print(timeList[i]);
    if(i < timeCount-1) Serial.print(", ");
  }
  Serial.println();
}

// 설정된 시간인지 확인하는 함수
bool isScheduledTime(String currentTime) {
  for(int i = 0; i < timeCount; i++) {
    if(timeList[i] == currentTime) {
      return true;
    }
  }
  return false;
}

// RTC에서 현재 시간을 "HH:MM" 형식으로 가져오는 함수
String getCurrentTimeString() {
  String timeStr = rtc.getTimeStr();
  // "HH:MM:SS" 형식에서 "HH:MM"만 추출
  return timeStr.substring(0, 5);
}

// 약 배출 함수 (간소화된 버전 - 대기 과정 제거)
bool dispenseMedicine() {
  Serial.println("=== DISPENSING MEDICINE ===");
  
  // LCD로 진행 상황 표시
  lcd.setCursor(0, 1);
  lcd.print("DISPENSING...   ");
  
  // 서보 재초기화 (확실하게!)
  servo.detach();
  delay(100);
  servo.attach(SERVO_PIN);
  delay(500);
  
  Serial.println("Step 1: Moving to 0 degrees (확실한 시작점)");
  servo.write(0);
  delay(2000);  // 2초 대기
  
  Serial.println("Step 2: Moving to 180 degrees (최대한 돌리기!)");
  lcd.setCursor(0, 1);
  lcd.print("MOVING 180 DEG  ");
  servo.write(180);  // 0도에서 180도로 크게 이동!
  delay(3000);  // 3초 대기 (충분히 기다림)
  
  Serial.println("Step 3: Moving to 90 degrees (중간)");
  lcd.setCursor(0, 1);
  lcd.print("MOVING 90 DEG   ");
  servo.write(90);
  delay(2000);  // 2초 대기
  
  Serial.println("Step 4: Back to 0 degrees (원위치)");
  lcd.setCursor(0, 1);
  lcd.print("RETURNING...    ");
  servo.write(0);
  delay(3000);  // 3초 대기 (완전 복귀)
  
  // 배출 완료 표시
  lcd.setCursor(0, 1);
  lcd.print("DISPENSED!      ");
  
  Serial.println("=== MEDICINE DISPENSED! ===");
  
  // 성공 부저음
  beepSuccess();
  delay(2000);
  
  // 완료 화면
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Medicine Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Please take it");
  
  // 완료 부저음
  beepSuccess();
  delay(3000);
  
  return true;  // 성공 반환
}

void setup() {
  Serial.begin(115200);  // ESP8266은 보통 115200 baud rate 사용
  
  // ESP8266 안정화를 위한 약간의 지연
  delay(1000);
  
  Serial.println("Starting ESP8266 Drug Dispenser with RTC...");

  // 버튼 핀 설정 (ESP8266은 내부 풀업 저항 사용)
  pinMode(TIME_BTN, INPUT_PULLUP);
  pinMode(VALUE_BTN, INPUT_PULLUP);
  pinMode(TINTERATION_BTN, INPUT_PULLUP);
  
  // 부저 핀 설정
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);  // 초기에는 꺼진 상태

  // I2C 초기화 (ESP8266에서는 명시적으로 SDA, SCL 핀 지정)
  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  
  // LCD 초기화
  lcd.init();
  lcd.backlight();
  
  // 부팅 메시지
  lcd.clear();
  lcd.print("Drug Dispenser");
  lcd.setCursor(0, 1);
  lcd.print("ESP8266 + RTC");
  delay(2000);
  
  // 서보모터 초기화
  servo.attach(SERVO_PIN);
  servo.write(0);
  delay(500);  // 서보 안정화 시간

  // RTC 초기화
  Serial.println("Initializing RTC...");
  lcd.clear();
  lcd.print("Init RTC");
  
  // RTC를 실행 모드로 설정하고 쓰기 보호 해제
  rtc.halt(false);
  rtc.writeProtect(false);
  
  // RTC 연결 테스트 및 현재 시간 표시
  Serial.println("RTC initialized!");
  Serial.print("Current time: ");
  Serial.println(rtc.getTimeStr());
  Serial.print("Current date: ");
  Serial.println(rtc.getDateStr());
  
  lcd.setCursor(0, 1);
  lcd.print("RTC: ");
  lcd.print(getCurrentTimeString());
  delay(2000);
  
  Serial.println("Setup complete!");
  
  // 부팅 완료 부저음
  beepSuccess();
}

void loop(){
  // 첫 번째 루프: 횟수 설정
  lcd.clear();
  lcd.print("Set Count");
  
  while(1) {
    // 현재 버튼 상태 읽기
    bool currentValueBtn = digitalRead(VALUE_BTN);
    bool currentInteractionBtn = digitalRead(TINTERATION_BTN);
    
    // 횟수 버튼이 눌렸을 때
    if (lastValueBtn == HIGH && currentValueBtn == LOW) {
      count++;
      
      // LCD에 표시
      lcd.clear();
      lcd.print("Set Count: ");
      lcd.print(count);
      
      Serial.print("Count: ");
      Serial.println(count);
      
      // 버튼 눌림 부저음
      beepShort();
      
      delay(200);
    }
    
    // 상호작용 버튼이 눌렸을 때 (다음 단계로)
    if (lastInteractionBtn == HIGH && currentInteractionBtn == LOW) {
      Serial.println("Moving to time setting...");
      
      // 단계 이동 부저음
      beepLong();
      
      delay(200);
      break;  // 첫 번째 while 루프 종료
    }
    
    // 이전 상태 저장
    lastValueBtn = currentValueBtn;
    lastInteractionBtn = currentInteractionBtn;
    
    delay(10);
    yield();  // ESP8266 watchdog reset 방지
  }
  
  // 두 번째 루프: 시간 설정 (30분 단위, 00:00부터 시작)
  timeCount = 0;  // 저장된 시간 개수 초기화
  currentHour = 0;    // 00:00부터 시작
  currentMinute = 0;  // 00:00부터 시작
  
  lcd.clear();
  lcd.print("Set Times");
  lcd.setCursor(0, 1);
  lcd.print("30min steps");
  delay(2000);
  
  while(1) {
    // 현재 버튼 상태 읽기
    bool currentTimeBtn = digitalRead(TIME_BTN);
    bool currentInteractionBtn = digitalRead(TINTERATION_BTN);
    
    // 시간 버튼이 눌렸을 때 (30분씩 증가)
    if (lastTimeBtn == HIGH && currentTimeBtn == LOW) {
      currentMinute += 30;  // 30분씩 증가
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour++;
        if (currentHour >= 24) {
          currentHour = 0;
        }
      }
      
      // LCD에 현재 시간 표시
      lcd.clear();
      lcd.print("Time: ");
      if(currentHour < 10) lcd.print("0");
      lcd.print(currentHour);
      lcd.print(":");
      if(currentMinute < 10) lcd.print("0");
      lcd.print(currentMinute);
      lcd.setCursor(0, 1);
      lcd.print("Times: ");
      lcd.print(timeCount);
      lcd.print("/");
      lcd.print(count);
      
      Serial.print("Current Time: ");
      if(currentHour < 10) Serial.print("0");
      Serial.print(currentHour);
      Serial.print(":");
      if(currentMinute < 10) Serial.print("0");
      Serial.println(currentMinute);
      
      // 시간 변경 부저음
      beepShort();
      
      delay(200);
    }
    
    // 상호작용 버튼이 눌렸을 때 (시간을 리스트에 추가)
    if (lastInteractionBtn == HIGH && currentInteractionBtn == LOW) {
      if (timeCount < count) {  // 아직 더 입력받을 수 있으면
        // "HH:MM" 형식으로 시간 문자열 생성
        String timeStr = "";
        if(currentHour < 10) timeStr += "0";
        timeStr += String(currentHour);
        timeStr += ":";
        if(currentMinute < 10) timeStr += "0";
        timeStr += String(currentMinute);
        
        timeList[timeCount] = timeStr;  // 리스트에 추가
        timeCount++;
        
        Serial.print("Added time: ");
        Serial.print(timeStr);
        Serial.print(" (");
        Serial.print(timeCount);
        Serial.print("/");
        Serial.print(count);
        Serial.println(")");
        
        // LCD 업데이트
        lcd.setCursor(0, 1);
        lcd.print("Times: ");
        lcd.print(timeCount);
        lcd.print("/");
        lcd.print(count);
        
        // 시간 추가 부저음
        beepLong();
        
        // 모든 시간을 입력받았으면 종료
        if (timeCount >= count) {
          Serial.println("All times set! Breaking...");
          delay(1000);
          break;  // 두 번째 while 루프 종료
        }
      }
      
      delay(200);
    }
    
    // 이전 상태 저장
    lastTimeBtn = currentTimeBtn;
    lastInteractionBtn = currentInteractionBtn;
    
    delay(10);
    yield();  // ESP8266 watchdog reset 방지
  }
  
  // 시간 정렬 및 중복 제거
  sortAndRemoveDuplicates();
  
  // 설정 완료 후 결과 표시
  lcd.clear();
  lcd.print("Setup Complete!");
  
  // 설정 완료 부저음
  beepSuccess();
  
  // 설정된 시간들 시리얼에 출력
  Serial.println("=== Setup Complete ===");
  Serial.print("Count: ");
  Serial.println(count);
  Serial.print("Times: ");
  for(int i = 0; i < timeCount; i++) {
    Serial.print(timeList[i]);
    if(i < timeCount-1) Serial.print(", ");
  }
  Serial.println();
  
  // 세 번째 루프: RTC 시간 모니터링 및 약 배출
  bool waitingForDispense = false;  // 배출 대기 상태
  String lastCheckedTime = "";      // 마지막으로 체크한 시간
  
  lcd.clear();
  lcd.print("System Running");
  delay(1000);
  
  while(1) {
    // ESP8266 watchdog reset 방지
    yield();
    
    // RTC에서 현재 시간 가져오기
    String currentRTCTime = getCurrentTimeString();
    
    // LCD에 현재 시간 표시
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    lcd.print(currentRTCTime);
    
    // 설정된 시간인지 확인 (시간이 바뀌었을 때만 체크)
    if(currentRTCTime != lastCheckedTime && isScheduledTime(currentRTCTime) && !waitingForDispense) {
      waitingForDispense = true;
      lastCheckedTime = currentRTCTime;
      
      // LCD로 알림 표시
      lcd.setCursor(0, 1);
      lcd.print("*** MEDICINE ***");
      
      // 약 복용 시간 알림 부저음
      beepAlert();
      
      lcd.setCursor(0, 1);
      lcd.print("Press INTERACTION");
      
      Serial.print("Medicine time! ");
      Serial.print(currentRTCTime);
      Serial.println(" - Waiting for user action");
    }
    
    // 시간이 바뀌면 lastCheckedTime 업데이트
    if(currentRTCTime != lastCheckedTime && !waitingForDispense) {
      lastCheckedTime = currentRTCTime;
    }
    
    // 배출 대기 중일 때 상호작용 버튼 확인
    if(waitingForDispense) {
      bool currentInteractionBtn = digitalRead(TINTERATION_BTN);
      
      if(lastInteractionBtn == HIGH && currentInteractionBtn == LOW) {
        // LCD 피드백
        lcd.setCursor(0, 1);
        lcd.print("PROCESSING...   ");
        
        // 약 배출 확인 부저음
        beepShort();
        
        // 약 배출 시도
        bool dispenseSuccess = dispenseMedicine();
        
        if(dispenseSuccess) {
          // 배출 성공시
          lcd.setCursor(0, 1);
          lcd.print("Dispensed OK!   ");
          
          Serial.println("Medicine dispensed successfully!");
          
          // 3초 대기 후 정상 모니터링 재개
          delay(3000);
          
          waitingForDispense = false;
          Serial.println("System resumed normal monitoring");
          
        } else {
          // 배출 실패시 - 다시 시도할 수 있도록 대기 상태 유지
          lcd.setCursor(0, 1);
          lcd.print("RETRY? Press BTN");
          
          Serial.println("Dispensing failed! Please try again...");
          
          // 실패시에는 waitingForDispense를 true로 유지해서 다시 시도 가능
          delay(2000);
        }
      }
      
      lastInteractionBtn = currentInteractionBtn;
      
      // 배출 대기 중에는 짧은 딜레이로 버튼 반응성 향상
      delay(50);
      
    } else {
      // 정상 모니터링 중
      
      // 다른 버튼들도 부저 피드백
      bool currentTimeBtn = digitalRead(TIME_BTN);
      bool currentValueBtn = digitalRead(VALUE_BTN);
      
      if(lastTimeBtn == HIGH && currentTimeBtn == LOW) {
        // 시간 버튼 부저 피드백
        lcd.setCursor(15, 1);
        lcd.print("T");
        beepShort();
        delay(200);
      }
      
      if(lastValueBtn == HIGH && currentValueBtn == LOW) {
        // 값 버튼 부저 피드백
        lcd.setCursor(15, 1);
        lcd.print("V");
        beepShort();
        delay(200);
      }
      
      lastTimeBtn = currentTimeBtn;
      lastValueBtn = currentValueBtn;
      
      lcd.setCursor(0, 1);
      lcd.print("Next: ");
      
      // 다음 예정 시간 표시
      bool foundNext = false;
      for(int i = 0; i < timeCount; i++) {
        if(timeList[i] > currentRTCTime) {
          lcd.print(timeList[i]);
          foundNext = true;
          break;
        }
      }
      if(!foundNext && timeCount > 0) { // 마지막이면 다음날 첫 시간
        lcd.print(timeList[0]);
      }
      
      // 1초마다 RTC 시간 체크
      delay(1000);
    }
  }
}