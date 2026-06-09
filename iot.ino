/********* BLYNK CONFIG *********/
#define BLYNK_TEMPLATE_ID "TMPL6i_p9yMNc"
#define BLYNK_TEMPLATE_NAME "Giám sát nước sinh hoạt"
#define BLYNK_AUTH_TOKEN "EWykw8JwlstTkurtzo6rB-GRePd7RCTi"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

/********* WIFI *********/
char ssid[] = "Jjj";
char pass[] = "01012009";

/********* UART *********/
HardwareSerial mySerial(2);

/********* BLYNK *********/
char auth[] = BLYNK_AUTH_TOKEN;

/********* DATA *********/
String rxData = "";

int tdsPPM = 0;        // V0
int distanceValue = 0; // V1
String waterText = ""; // V2

/********* PARSE DATA *********/
void parseData(String s) {

  int p_adc = s.indexOf("ADC:");
  int p_v   = s.indexOf(",V:");
  int p_ppm = s.indexOf(",PPM:");
  int p_d   = s.indexOf(",D:");
  int p_p   = s.indexOf(",P:");
  int p_w   = s.indexOf(",W:");
  int p_l   = s.indexOf(",L:");

  // Kiểm tra chuỗi hợp lệ
  if (p_adc >= 0 && p_v >= 0 && p_ppm >= 0 && p_d >= 0 && p_p >= 0 && p_w >= 0 && p_l >= 0) {

    /********* LẤY DỮ LIỆU *********/
    tdsPPM = s.substring(p_ppm + 5, p_d).toInt();

    String distanceStr = s.substring(p_d + 3, p_p);
    int waterQuality   = s.substring(p_w + 3, p_l).toInt();

    /********* XỬ LÝ DISTANCE *********/
    if (distanceStr == "ERR") {
      distanceValue = -1;
    } else {
      distanceValue = distanceStr.toFloat();
    }

    /********* CHUYỂN TEXT *********/
    if (waterQuality == 0)      waterText = "SACH";
    else if (waterQuality == 1) waterText = "CANH BAO";
    else if (waterQuality == 2) waterText = "NGUY HIEM";
    else                        waterText = "UNKNOWN";

    /********* DEBUG SERIAL *********/
    Serial.println("===== DATA TU STM32 =====");
    Serial.print("PPM: "); Serial.println(tdsPPM);
    Serial.print("Distance: "); Serial.println(distanceValue);
    Serial.print("Water: "); Serial.println(waterText);
    Serial.println();

    /********* GỬI BLYNK *********/
    Blynk.virtualWrite(V0, tdsPPM);
    Blynk.virtualWrite(V1, distanceValue);
    Blynk.virtualWrite(V2, waterText);

  } else {
    Serial.println("Sai dinh dang:");
    Serial.println(s);
  }
}

/********* SETUP *********/
void setup() {
  Serial.begin(115200);

  // UART2: RX = GPIO16, TX = GPIO17
  mySerial.begin(115200, SERIAL_8N1, 16, 17);

  Blynk.begin(auth, ssid, pass);

  Serial.println("ESP32 + Blynk Ready");
}

/********* LOOP *********/
void loop() {
  Blynk.run();

  while (mySerial.available()) {
    char c = mySerial.read();

    if (c == '\n') {
      rxData.trim();

      if (rxData.length() > 0) {
        Serial.print("Raw: ");
        Serial.println(rxData);
        parseData(rxData);
      }

      rxData = "";
    }
    else if (c != '\r') {
      rxData += c;
    }
  }
}