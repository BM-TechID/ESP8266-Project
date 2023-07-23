/*
Name Project: Water MonSys
Kelompok:
21102116 ASYAFA DITRA AL-HAUNA
21102121 MANSUR JULIANTO
21102122 M.AMAR IZZUDDIN
21102145 DIMAS TEGUH RAMADHANI
*/

#include <DHT.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define DHT_PIN 27
#define TRIG_PIN 19
#define ECHO_PIN 18
#define RELAY_PIN 14
#define BUZZER_PIN 12

// Inisialisasi sensor DHT11
DHT dht(DHT_PIN, DHT11);

// Tinggi sensor dari permukaan tanah (misalnya dalam cm)
const int SENSOR_HEIGHT = 27;

// Inisialisasi objek LCD I2C dengan alamat I2C (sesuaikan dengan alamat modul LCD I2C Anda)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "#MafKon_Project";
const char* password = "tempikmumambu";
const char* thingspeakApiKey = "SXV6Q38V58XSGFIJ";  // Ganti dengan API Key Thingspeak Anda
const char* thingspeakUrl = "http://api.thingspeak.com/update";

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Mulai sensor DHT11
  dht.begin();

  // Inisialisasi LCD I2C
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("UAS DMP JHON...");
  lcd.setCursor(0, 1);
  lcd.print("WATER MON SYS");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");

  // Hubungkan ke WiFi
  connectToWiFi();
}

void connectToWiFi() {
  // Menunggu koneksi ke WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("Connected to WiFi!");
}

void uploadToThingspeak(float temperature, float humidity, int distance) {
  // Buat objek JSON untuk menyimpan data
  StaticJsonDocument<200> doc;
  doc["api_key"] = thingspeakApiKey;
  doc["field1"] = distance;
  doc["field2"] = temperature;
  doc["field3"] = humidity;

  // Serialisasi objek JSON ke dalam bentuk string
  String jsonString;
  serializeJson(doc, jsonString);

  // Buat request HTTP POST ke server Thingspeak
  HTTPClient http;
  http.begin("http://api.thingspeak.com/update.json");
  http.addHeader("Content-Type", "application/json");

  // Kirim data JSON melalui POST request
  int httpCode = http.POST(jsonString);

  // Cek status response HTTP
  if (httpCode == 200) {
    Serial.println("Data sent to Thingspeak successfully!");
  } else {
    Serial.print("Failed to send data to Thingspeak. HTTP error code: ");
    Serial.println(httpCode);
  }

  // Akhiri koneksi HTTP
  http.end();
}




int getDistanceWithHeight() {
  // Melepas sinyal trigger selama 2 mikrodetik
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Membaca durasi dari sinyal echo
  unsigned long duration = pulseIn(ECHO_PIN, HIGH);

  // Menghitung jarak berdasarkan kecepatan suara (340 m/s pada suhu kamar)
  float distance_cm = duration * 0.034 / 2;

  // Sisipkan kembali perhitungan tinggi aktual dari permukaan tanah
  int actual_distance_cm = SENSOR_HEIGHT - distance_cm;

  // Pastikan jarak tidak negatif
  if (actual_distance_cm < 0) {
    actual_distance_cm = 0;
  }

  return actual_distance_cm;
}

void fillWater() {
  // Fungsi untuk menghidupkan relay untuk mengisi air sampai tinggi 17 cm
  digitalWrite(RELAY_PIN, HIGH);
}

void stopWater() {
  // Fungsi untuk mematikan relay dan buzzer
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

void monAIR() {
  int distance = getDistanceWithHeight();
  Serial.print("Tinggi Air: ");
  Serial.print(distance);
  Serial.println(" cm");

  lcd.setCursor(0, 0);
  lcd.print("            ");  // Membersihkan tampilan sebelumnya
  lcd.setCursor(0, 0);
  lcd.print("T.AIR: ");
  lcd.print(distance);
  lcd.print(" CM");

  if (distance < 3) {
    // Kondisi 1: Ketika air kurang dari 3 cm, relay dihidupkan untuk mengisi air sampai tinggi 17 cm
    fillWater();
  } else if (distance >= 3 && distance <= 5) {
    // Kondisi 3: Ketika air di antara 3-5 cm, buzzer berbunyi setelah 5 cm buzzer mati
    fillWater();
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(900);

  } else if (distance > 17) {
    // Kondisi 2: Ketika air mencapai atau melebihi 17 cm, semua perangkat dimatikan
    stopWater();
  }
}

void readDHT() {
  // Baca kelembaban dan suhu dari sensor DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Cek apakah bacaan dari sensor DHT11 berhasil
  if (!isnan(humidity) && !isnan(temperature)) {
    Serial.print("Kelembaban: ");
    Serial.print(humidity);
    Serial.print(" % | ");
    Serial.print("Suhu: ");
    Serial.print(temperature);
    Serial.println(" °C");

    lcd.setCursor(0, 1);
    lcd.print("H: ");
    lcd.print(int(humidity));
    lcd.print("% | ");
    lcd.print("T: ");
    lcd.print(int(temperature));
    lcd.print("C");
  } else {
    Serial.println("Gagal membaca kelembaban atau suhu dari sensor DHT11");

    lcd.setCursor(0, 1);
    lcd.print("Gagal membaca DHT");
  }
}

void loop() {


  // Read temperature, humidity, and distance
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int distance = getDistanceWithHeight();
  uploadToThingspeak(temperature, humidity, distance);
  // Tunggu beberapa saat sebelum mengukur lagi
  monAIR();
  readDHT();
  delay(1000);
}
