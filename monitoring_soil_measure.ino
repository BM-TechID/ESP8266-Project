#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// Ganti Jenegan Wifi ro Password
#define WIFI_SSID "test"
#define WIFI_PASSWORD "12345678"


// Ganti Token BOT.e
#define BOT_TOKEN "6593729375:AAHXXb-YATKYvOpiR7NlgFiwCE7YfznjoaE"
// Ganti ID TELEGRAMU
#define CHAT_ID "6046131425"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);


LiquidCrystal_I2C lcd(0x27, 16, 2);
int pompa = 12;


void setup() {
  lcd.clear();
  lcd.init();
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DMP PROJECT");
  lcd.setCursor(0, 1);
  lcd.print("Alat siram otomatis");
  delay(2000);

  pinMode(pompa, OUTPUT);


  // attempt to connect to Wifi network:
  Serial.println("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting Wifi");
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setTrustAnchors(&cert);  // Add root certificate for api.telegram.org
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




  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org");  // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);

  bot.sendMessage(CHAT_ID, "Bot started up", "");
}


// Variabel untuk menyimpan pesan terakhir yang dikirim
String lastMessage = "";

void TS() {
  int sensorValue = analogRead(A0);

  const char* host = "api.thingspeak.com";
  const String apiKey = "PVB66K2RO2A0FL9O";

  // Buat URL untuk mengirim data ke Thingspeak
  String url = "/update?api_key=" + apiKey + "&field1=" + String(sensorValue);

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

void loop() {
  int sv = analogRead(A0);
  Serial.println(sv);

  String currentMessage;  // Variabel untuk menyimpan pesan yang akan dikirim

  if (sv > 800 && sv < 1100) {
    digitalWrite(pompa, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tanaman Kering");
    lcd.setCursor(0, 1);
    lcd.print("Menyiram....");
    Serial.println("Tanaman Kering");

    currentMessage = "Tanaman Kering\nPompa ON";
  } else if (sv > 100 && sv < 800) {
    digitalWrite(pompa, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tanaman Basah");
    lcd.setCursor(0, 1);
    lcd.print("Matikan pompa...");
    Serial.println("Tanaman Basah");

    currentMessage = "Tanaman Basah\nPompa OFF";
  } else {
    digitalWrite(pompa, LOW);
    currentMessage = "";  // Jika tidak ada kondisi yang cocok, pesan dikosongkan
  }

  // Cek apakah pesan yang akan dikirim sama dengan pesan terakhir
  // Jika sama, maka tidak perlu mengirim notifikasi
  if (currentMessage != lastMessage) {
    lastMessage = currentMessage;                  // Simpan pesan terakhir yang dikirim
    bot.sendMessage(CHAT_ID, currentMessage, "");  // Kirim notifikasi
  }
  TS();
}
