#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>

// Set the LCD address to 0x3F/0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Sensor IR
int IRValue;
int count = 0;
int cond = 0;

// RTC
RTC_DS3231 rtc;
DateTime now;
int dataSeconds = 0;
int dataMinutes = 0;
int dataHours = 0;
int dataSendCounter;
// Setting tanggal menjadi nama hari
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
const long utcOffsetInSeconds = 25200; //NTP Time Offset GMT+7
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String formattedTime;

// Setup Wifi
const char* ssid3 = "STTB1";
const char* password3 = "Si4nt4r321";

const char* ssid2 = "MT1";
const char* password2 = "siantar321";

const char* ssid = "MT3";
const char* password = "siantar321";

// Set your Static IP address
IPAddress local_IP(192, 168, 15, 218);
// Set your Gateway IP address
IPAddress gateway(192, 168, 15, 250);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// Get Data
String serverNameGet = "http://192.168.15.221/counter_hit_api/getDataLastCounter.php?kode_product=";
String getData;
const char* counterFromDB;
int counterValueDB;
bool sendStatus;

const unsigned long interval = 10;
unsigned long lastMillis = 0;

String dateTime;
String dateTimeLCD;
String postData;
String ip_address;
String deviceName = "Counter";
String dataSD;

// Kode Produk
String produk1 = "P-0722-00239";
String produk2 = "produk2";
String produk3 = "produk3";
String produk4 = "produk4";

String nama1 = "Tic Tic Bwg 2000";


// Tombol
#define resetButton D8

void hitungBarang() {
  // Mendapatkan waktu saat ini dalam milidetik
  unsigned long currentMillis = millis();

  // Periksa apakah sudah saatnya mengubah status LED
  if (currentMillis - lastMillis >= interval) {
    // Menyimpan waktu terakhir perubahan status LED
    lastMillis = currentMillis;
    IRValue = analogRead(A0);
    if (IRValue > 500) {
      count = count;
      cond = 0;
    } else if(IRValue < 500 && cond == 0) {
      count += 1;
      cond = 1;
    } else if(IRValue < 500 && cond == 1) {
      count = count;
      cond = 1;
    }
    lcd.setCursor(1, 2);
    lcd.print("Total : ");
    lcd.setCursor(9, 2);
    lcd.print(count);
  }
}

void connectToWiFi()  {
  int flag = 1; 

  // Configures static IP address
  if(!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  int network = WiFi.scanNetworks();
  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) {
    for (int i = 0 ; i < network; i++) {
      switch (flag) {
        case 1: 
          flag = 2;
          if (WiFi.SSID(i) == ssid) {
            WiFi.begin(ssid, password);
            lcd.setCursor(1, 0);
            lcd.print("Connecting to WiFi");
            delay(80);
            break;
          }
          case 2: 
            flag = 3;
            if (WiFi.SSID(i) == ssid2) {
            WiFi.begin(ssid2, password2);
            lcd.setCursor(1, 0);
            lcd.print("Connecting to WiFi");
            delay(80);
            break;
          }
          case 3:
            flag = 1;
            if (WiFi.SSID(i) == ssid3) {
            WiFi.begin(ssid3, password3);
            lcd.setCursor(1, 0);
            lcd.print("Connecting to WiFi");
            delay(80);
            break;
          }
      }
    }

    retries++;
    delay(500);
    Serial.print(".");
  }
  if (retries > 14) {
      Serial.println(F("WiFi connection FAILED"));
  }
  if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("WiFi connected!"));
      Serial.println("IP address : ");
      Serial.println(WiFi.localIP());
  }else if(WiFi.status() != WL_CONNECTED){
      Serial.println(F("WiFi disconnected!"));
  }
}

void sendLogData() {
  HTTPClient http;    // http object of clas HTTPClient
  WiFiClient wclient; // wclient object of clas HTTPClient
  now = rtc.now();
  dateTime = String(now.year()) + '-' + String(now.month()) + '-' + String(now.day()) + ' ' + String(timeClient.getFormattedTime());
  postData = "kode_product=" + String(produk1) + "&counter=" + String(count) + "&date=" + dateTime + "&ip_address=" + ip_address;
  http.begin(wclient, "http://192.168.15.221/counter_hit_api/saveCounter.php"); // Connect to host where MySQL databse is hosted
  http.addHeader("Content-Type", "application/x-www-form-urlencoded"); //Specify content-type header

  int httpCode = http.POST(postData);
  // If connection eatablished then do this
  if (httpCode == 200) {
    String webpage = http.getString();
    Serial.println(webpage + "\n");
    return;
  } else { 
    Serial.println(httpCode); 
    Serial.println("Failed to upload values. \n"); 
  }
  http.end();
}

void getLogData() {
  HTTPClient http;    // http object of clas HTTPClient
  WiFiClient wclient; // wclient object of clas HTTPClient

  getData = serverNameGet + String(produk1);
  http.begin(wclient, getData);

  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String payload = http.getString();
    JSONVar myArray = JSON.parse(payload);
    counterFromDB = myArray["counter"];
    counterValueDB = atoi(counterFromDB);
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}

void setup() {
  Serial.begin(9600);

  sendStatus = false;

//  pinMode(resetButton, INPUT_PULLUP);
  
  lcd.init();
  lcd.clear();         
  lcd.backlight();
  
  // Get NTP
  timeClient.begin();
  timeClient.setTimeOffset(25200);
  rtc.begin();
  
  connectToWiFi();
  lcd.clear();
}

void loop() {
  ip_address = WiFi.localIP().toString();
  now = rtc.now();
  timeClient.update();
  dateTime = String(now.year()) + '-' + String(now.month()) + '-' + String(now.day()) + ' ' + String(timeClient.getFormattedTime());
  dateTimeLCD = String(timeClient.getFormattedTime());

  lcd.setCursor(1, 0);
  lcd.print(produk1);
  lcd.setCursor(1, 1);
  lcd.print(nama1);
  lcd.setCursor(11, 3);
  lcd.print(dateTimeLCD);
  hitungBarang();
      
  if(WiFi.status() == WL_CONNECTED) {
    now = rtc.now();
    timeClient.update();
    
    dataSeconds = timeClient.getSeconds();
    if(count == 0) {
      getLogData();
      if(counterValueDB != 0){
        count = counterValueDB;
      }
    }
    
    dataMinutes = timeClient.getMinutes();
    dataHours = timeClient.getHours();
    if(dataHours == 7 && dataMinutes == 50 && dataSeconds == 0) {
      count = 0;
      delay(1000);
      sendLogData();
      delay(1000);
      lcd.clear();
      delay(100);
    }
    
    if(dataHours == 19 && dataMinutes == 50 && dataSeconds == 0) {
      count = 0;
      delay(1000);
      sendLogData();
      delay(1000);
      lcd.clear();
      delay(100);
    }

    if(dataSeconds == 10 || dataSeconds == 25 || dataSeconds == 40 || dataSeconds == 55) {
      if(sendStatus == true) {
        sendStatus = false;
      }
    }
    
    if(dataSeconds == 00 || dataSeconds == 15 || dataSeconds == 30 || dataSeconds == 45) {
      if(sendStatus == false) {
        sendLogData();
        sendStatus = true;
      }
    }
    
    lcd.setCursor(1, 3);
    lcd.print("WiFi OK ");
    
  } else {
    lcd.setCursor(1, 3);
    lcd.print("WiFi Dis");
  }

//  Serial.println(digitalRead(resetButton));

//  if(digitalRead(resetButton) == HIGH) {
//    count = 0;
//    sendLogData();
//    Serial.println(digitalRead(resetButton));
//    Serial.println("Reset");
//    delay(100);
//    ESP.restart();
//  }
}
