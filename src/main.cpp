#include <Arduino.h>
#include <SoftwareSerial.h>

#define RELAY_PIN 7
#define simRX 3
#define simTX 2

SoftwareSerial sim800l(simRX, simTX); // RX, TX pins for SoftwareSerial
// put function declarations here:

String smsMessage = "";
bool smsReceived = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  sim800l.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially

  Serial.println("Memulai inisialisasi modul SIM...");
  delay(1000);

  sendATCommand("AT", 1000); // Cek koneksi
  sendATCommand("AT+CMGF=1", 1000); // Set mode teks SMS
  sendATCommand("AT+CNMI=2,1", 1000); // Notifikasi SMS ke port serial
  Serial.println("Modul SIM siap.");
}

void loop() {
  if (sim800l.available()) {
    char c = sim800l.read();
    smsMessage += c;
    if (smsMessage.endsWith("\r\n")) { // Akhir dari baris SMS
      Serial.print("SMS Diterima: ");
      Serial.println(smsMessage);
      smsReceived = true;
      processSMS(smsMessage);
      smsMessage = ""; // Reset pesan
    }
  }
}

String sendATCommand(String command, unsigned long timeout) {
  sim800l.println(command);
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (sim800l.available()) {
      char c = sim800l.read();
      response += c;
    }
  }
  Serial.print("AT Command: ");
  Serial.println(command);
  Serial.print("Response: ");
  Serial.println(response);
  return response;
}

// Fungsi untuk memproses pesan SMS
void processSMS(String message) {
  // Hapus header SMS yang tidak relevan (misalnya "+CMT:")
  int index = message.indexOf("+CMT:");
  if (index != -1) {
    message = message.substring(index); // Ambil dari +CMT: dan seterusnya

    // Ekstrak isi pesan (biasanya setelah nomor telepon dan tanggal/waktu)
    // Ini butuh parsing yang lebih robust tergantung format +CMT:
    // Untuk sederhana, kita akan cari keyword langsung.
    // Misal: +CMT: "+628123456789",,"24/06/20,10:30:15+28"\r\nISI PESAN\r\n

    // Contoh sederhana: Cek apakah pesan mengandung "ON" atau "OFF"
    // Konversi pesan ke huruf besar untuk case-insensitivity
    message.toUpperCase();

    if (message.indexOf("HIDUPKAN") != -1 || message.indexOf("ON") != -1) {
      digitalWrite(RELAY_PIN, HIGH); // Nyalakan relai (sesuaikan dengan modul Anda, HIGH/LOW = ON)
      Serial.println("Perintah HIDUPKAN diterima. Relai ON.");
      sendSMS("Relai berhasil dihidupkan.", "NOMOR_PENERIMA_FEEDBACK"); // Ganti dengan nomor pengirim
    } else if (message.indexOf("MATIKAN") != -1 || message.indexOf("OFF") != -1) {
      digitalWrite(RELAY_PIN, LOW); // Matikan relai (sesuaikan dengan modul Anda, HIGH/LOW = OFF)
      Serial.println("Perintah MATIKAN diterima. Relai OFF.");
      sendSMS("Relai berhasil dimatikan.", "NOMOR_PENERIMA_FEEDBACK"); // Ganti dengan nomor pengirim
    } else if (message.indexOf("STATUS") != -1) {
      String status = (digitalRead(RELAY_PIN) == HIGH) ? "ON" : "OFF";
      sendSMS("Status relai saat ini: " + status, "NOMOR_PENERIMA_FEEDBACK");
    } else {
      sendSMS("Perintah tidak dikenal. Gunakan HIDUPKAN, MATIKAN, atau STATUS.", "NOMOR_PENERIMA_FEEDBACK");
    }

    // Setelah memproses, hapus SMS dari SIM (penting agar tidak memori penuh)
    sendATCommand("AT+CMGD=1,4", 1000); // Hapus semua pesan yang sudah dibaca/terkirim
  }
}

void sendSMS(String message, String phoneNumber) {
  sim800l.print("AT+CMGS=\"");
  sim800l.print(phoneNumber);
  sim800l.println("\"");
  delay(100);
  sim800l.print(message);
  delay(100);
  sim800l.write(26); // ASCII code untuk CTRL+Z (EOF)
  Serial.print("Mengirim SMS: ");
  Serial.println(message);
  delay(3000); // Beri waktu modul SIM untuk mengirim
  // Anda bisa menambahkan logika untuk mengecek respons "OK" dari AT command
}