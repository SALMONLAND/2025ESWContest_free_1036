// DS1302_Serial_Easy for WEMOS D1 R1
// DS1302 RTC module example adapted for WEMOS D1 R1 (ESP8266)
//
// WEMOS D1 R1 연결:
// DS1302: RST pin -> WEMOS D4 (GPIO2)
//         DAT pin -> WEMOS D5 (GPIO14)
//         CLK pin -> WEMOS D6 (GPIO12)
//         VCC -> 3.3V
//         GND -> GND

#include <DS1302.h>

// DS1302 초기화 (RST, DAT, CLK 핀 순서)
DS1302 rtc(D12, D13, D11);

void setup() {
  // RTC를 실행 모드로 설정하고 쓰기 보호 해제
  rtc.halt(false);
  rtc.writeProtect(false);

  // 시리얼 통신 설정
  Serial.begin(115200); // ESP8266은 보통 115200 baud rate 사용

  Serial.println("WEMOS D1 R1 + DS1302 RTC 예제");
  Serial.println("==============================");

  // 다음 라인들은 이미 DS1302에 저장된 값을 사용하려면 주석 처리하세요
  rtc.setDOW(FRIDAY);        // 요일을 금요일로 설정
  rtc.setTime(13,35, 0);     // 시간을 12:00:00으로 설정 (24시간 형식)
  rtc.setDate(14, 8, 2025);  // 날짜를 2025년 8월 14일로 설정
}

void loop() {
  // 요일 출력
  Serial.print(rtc.getDOWStr());
  Serial.print(" ");

  // 날짜 출력
  Serial.print(rtc.getDateStr());
  Serial.print(" -- ");

  // 시간 출력
  Serial.println(rtc.getTimeStr());

  // 1초 대기
  delay(1000);
}