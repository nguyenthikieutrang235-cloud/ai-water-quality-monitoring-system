/********* BLYNK CONFIG *********/
#define BLYNK_TEMPLATE_ID "TMPL6i_p9yMNc"
#define BLYNK_TEMPLATE_NAME "Giám sát nước sinh hoạt"
#define BLYNK_AUTH_TOKEN "EWykw8JwlstTkurtzo6rB-GRePd7RCTi"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Jjj";
char pass[] = "01012009";
HardwareSerial mySerial(2);
BlynkTimer timer;

/********* BIẾN DỮ LIỆU *********/
String rxData = "";

// Thông số Cảm biến
int tdsPPM = 0;
float turbNTU = 0.0;
float distanceValue = 0.0;
String waterText = "Đang kết nối...";

// Thông số Hệ thống mới (Theo STM32 Update)
String simTime = "00:00";
String simSpeed = "x1";
String pumpStatus = "TẮT";

// --- GỬI DỮ LIỆU LÊN BLYNK ĐỊNH KỲ ---
void sendDataToBlynk() {
  Blynk.virtualWrite(V0, tdsPPM);        // V0: TDS (Gauge)
  Blynk.virtualWrite(V1, distanceValue); // V1: Khoảng cách (Level/Label)
  Blynk.virtualWrite(V2, waterText);     // V2: Trạng thái nước (Label)
  Blynk.virtualWrite(V3, turbNTU);       // V3: Độ đục (Gauge)
  
  // Các chân ảo mới cho Hệ thống thời gian & Bơm
  Blynk.virtualWrite(V4, simTime);       // V4: Đồng hồ ảo (Label)
  Blynk.virtualWrite(V5, simSpeed);      // V5: Tốc độ tua (Label)
  Blynk.virtualWrite(V6, pumpStatus);    // V6: Trạng thái Bơm (Label/LED)
}

// --- HÀM BÓC TÁCH DỮ LIỆU CHUẨN STM32 ---
void parseData(String s) {
  // Chuỗi mẫu: TIME:05:58,SPEED:30.0,PPM:250,NTU:45.5,D:15.5,Q:0,F:1,H:0,S:0,P:1,PULSE:4500

  // 1. Lấy Thời gian ảo (TIME)
  int p_time = s.indexOf("TIME:");
  if (p_time != -1) {
    simTime = s.substring(p_time + 5, p_time + 10); // Lấy đúng 5 ký tự "HH:MM"
  }

  // 2. Lấy Tốc độ mô phỏng (SPEED)
  int p_speed = s.indexOf(",SPEED:");
  if (p_speed != -1) {
    int comma = s.indexOf(',', p_speed + 1);
    simSpeed = "x" + s.substring(p_speed + 7, comma); // Gắn thêm chữ "x" cho đẹp
  }

  // 3. Lấy TDS (PPM)
  int p_ppm = s.indexOf(",PPM:");
  if (p_ppm != -1) {
    int comma = s.indexOf(',', p_ppm + 1);
    tdsPPM = s.substring(p_ppm + 5, comma).toInt();
  }

  // 4. Lấy Độ đục (NTU)
  int p_ntu = s.indexOf(",NTU:");
  if (p_ntu != -1) {
    int comma = s.indexOf(',', p_ntu + 1);
    turbNTU = s.substring(p_ntu + 5, comma).toFloat();
  }

  // 5. Lấy Khoảng cách (D)
  int p_d = s.indexOf(",D:");
  if (p_d != -1) {
    int comma = s.indexOf(',', p_d + 1);
    String dStr = s.substring(p_d + 3, comma);
    if (dStr == "ERR") distanceValue = -1.0;
    else distanceValue = dStr.toFloat();
  }

  // 6. Lấy Chất lượng nước (Q) - Tin tưởng hoàn toàn vào STM32
  int p_q = s.indexOf(",Q:");
  if (p_q != -1) {
    int qVal = s.substring(p_q + 3, p_q + 4).toInt();
    if (qVal == 0)      waterText = "SẠCH";
    else if (qVal == 1) waterText = "CẢNH BÁO";
    else if (qVal == 2) waterText = "NGUY HIỂM";
  }

  // 7. Lấy Trạng thái Máy Bơm (P)
  int p_p = s.indexOf(",P:");
  if (p_p != -1) {
    int pumpVal = s.substring(p_p + 3, p_p + 4).toInt();
    if (pumpVal == 1) pumpStatus = "ĐANG CHẠY";
    else pumpStatus = "TẮT";
  }

  // In ra Serial để giám sát
  Serial.printf("[%s %s] Bơm:%s | PPM:%d | NTU:%.1f | D:%.1f | %s\n", 
                simTime.c_str(), simSpeed.c_str(), pumpStatus.c_str(), 
                tdsPPM, turbNTU, distanceValue, waterText.c_str());
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 16, 17);
  
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  timer.setInterval(1000L, sendDataToBlynk); // Cập nhật Blynk 1 giây/lần
  Serial.println("ESP32: Đã đồng bộ với STM32 Version Mới!");
}

void loop() {
  Blynk.run();
  timer.run();

  while (mySerial.available()) {
    char c = mySerial.read();
    
    if (c == '\n') {
      rxData.trim();
      if (rxData.length() > 0) {
        parseData(rxData);
      }
      rxData = "";
    } else if (c != '\r') {
      rxData += c;
    }
  }
}