#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

// Ganti Jenegan Wifi ro Password
#define WIFI_SSID "test"
#define WIFI_PASSWORD "12345678"

// Inisialisasi I2C LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Inisialisasi sensor ultrasonik
const int trigPin = D8;
const int echoPin = D7;

// Inisialisasi servo motor
const int servoPin = D6;
Servo myServo;


// Inisialisasi RTC
RTC_DS3231 rtc;

// Variabel untuk status memberi makanan
bool makananDiberikan = false;
bool dispenserTerbuka = false;

// Variabel untuk melacak waktu terakhir makan dan waktu pemberian makan berikutnya
int jamPemberianMakan[4] = { 8, 12, 16, 20 };  // Waktu pemberian makan (jam) dalam sehari
int jamTerakhirMakan = -1;
int indeksPemberianMakan = 0;

void setup() {
  Serial.begin(9600);

  myServo.write(90);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.print("Pet Feeder");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  Wire.begin();
  rtc.begin();
  myServo.attach(servoPin);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  delay(3000);
  lcd.clear();

  // Connect to Wi-Fi
  // attempt to connect to Wifi network:
  Serial.println("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting Wifi");
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(WIFI_SSID);
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
}

float measureDistance() {
  Serial.println("HCSR04 run...");
  long duration;
  float distance;

  // Send ultrasonic pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // HCsr04  duration of the pulse from the echo pin
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance in centimeters
  distance = duration * 0.034 / 2.0;

  return distance;
}

void loop() {
  float distance = measureDistance();

  Serial.print(distance);
  Serial.println(" CM");

  // Mengambil waktu dari modul RTC
  DateTime now = rtc.now();
  Serial.print("Waktu sekarang: ");
  Serial.print(now.day(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.year(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.println(now.second(), DEC);

  // Mengecek waktu pemberian makan berikutnya
  if (now.hour() == jamPemberianMakan[indeksPemberianMakan] && now.minute() == 0 && !makananDiberikan) {
    if (now.hour() != jamTerakhirMakan) {
      // Beri makan pada waktu pemberian makan berikutnya
      jamTerakhirMakan = now.hour();
      beriMakan();
    }
  }

  // Fungsi makan menggunkan ultrasonic
  // Jika kucing mendekat dalam jarak 10-20 cm dan dispenser belum dibuka sebelumnya
  if (distance >= 10 && distance <= 20 && !dispenserTerbuka) {
    myServo.write(180);  // Menggerakkan servo ke posisi 180 derajat
    Serial.println("Object Terdeteksi");
    Serial.println("Membuka tempat makan");
    lcd.clear();
    lcd.print("Object Terdeteksi");
    lcd.setCursor(0, 1);
    lcd.print("Memberi makan");

    delay(3000);       // Beri makanan selama 3 detik
    myServo.write(90);  // Mengembalikan servo ke posisi 90 derajat
    Serial.println("Menutup");
    delay(1000);
    lcd.clear();
    lcd.print("Menutup");
    makananDiberikan = true;  // Set status memberi makanan menjadi true
    dispenserTerbuka = true;  // Set status dispenser terbuka menjadi true
  }

  // Jika kucing masih dalam jarak 10-20 cm tapi dispenser sudah dibuka sebelumnya
  else if (distance >= 10 && distance <= 20 && dispenserTerbuka) {
    // Tidak melakukan apa pun, biarkan dispenser terbuka selama 5 detik dan kemudian akan ditutup
  }

  // Reset status dispenser jika kucing sudah menjauh
  else if (distance > 20) {
    dispenserTerbuka = false;
  }

  delay(500);
  TS();
}

//fungsi makan dengan RTC
// Fungsi untuk memberi makan
void beriMakan() {
  Serial.println("Eksekusi beriMakan.... ");
  myServo.write(180);  // Menggerakkan servo ke posisi 180 derajat
  Serial.println("Memberi makan");
  lcd.clear();
  lcd.print("Memberi makan");

  delay(3000);       // Beri makanan selama 3 detik
  myServo.write(90);  // Mengembalikan servo ke posisi 90 derajat
  Serial.println("Menutup");
  delay(1000);
  lcd.clear();
  lcd.print("Menutup");
  makananDiberikan = true;                                // Set status memberi makanan menjadi true
  delay(1000);                                            // Tunggu 1 detik (untuk menghindari loop makanan terlalu cepat)
  makananDiberikan = false;                               // Set status memberi makanan kembali menjadi false
  indeksPemberianMakan = (indeksPemberianMakan + 1) % 4;  // Pindah ke waktu pemberian makan berikutnya
}

// thingspeak
void TS() {
  Serial.println("Eksekusi TS.... ");
  float distance_cm = measureDistance();

  const char* host = "api.thingspeak.com";
  const String apiKey = "K0PA6S55MXWVN677";

  // Buat URL untuk mengirim data ke Thingspeak
  String url = "/update?api_key=" + apiKey + "&field1=" + String(distance_cm);

  // Buat koneksi ke server Thingspeak
  WiFiClient client;
  if (client.connect(host, 80)) {
    // Kirim permintaan HTTP POST ke server Thingspeak
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
    delay(500);

    // Tunggu respon dari server
    while (client.available()) {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    }
    Serial.println();
    client.stop();
  }
}