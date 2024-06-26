// Written by: Kyle Adler, Scott Russel, Josh Liu
//
// 06/17/2024
//
// Purpose: Keep time with RTC, log flight data and GPS to SD card and trasmit over LoRa.
// Board: Adafruit ESP32 Feather, serial at 115200.
//
//
// !!! When powering from LiPo, ensure the polarity is correct per Adafruit's docs,
// don't fry your charging circuit like I did.
// https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/power-management


// Feather9x_RX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (receiver)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_TX

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// Radio Set Up
#define RFM95_INT 14 // E
#define RFM95_CS 32 // D
#define RFM95_RST 15 // C
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// oled
Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
#define BUTTON_A 15
#define BUTTON_B 32
#define BUTTON_C 14

float gndVoltage;



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  while (!Serial) delay(1);
  delay(100);

  Serial.println("Feather LoRa RX Test!");

  // Initialize OLED
  if (!display.begin(0x3C, true)) {
    Serial.println("Couldn't find SH1107");
    while (1);
  }

   // Flip the display 180 degrees
  display.setRotation(1);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Jus hangin around...");
  display.display();

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}

void loop() {
  // stuff
  gndVoltage = 2.0*analogRead(A13)/4098.0*3.30;

  // Serial.println(gndVoltage);

  // radio stuff
  if (rf95.available()) {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);


    if (rf95.recv(buf, &len)) {
      digitalWrite(LED_BUILTIN, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      Serial.print((char*)buf);
       Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      display.clearDisplay();
      display.setRotation(1);
      display.setCursor(0, 0);

      display.print("TX# RCVD: ");
      char txnum[7]; // Buffer for string (5 characters + null terminator)
      strncpy(txnum, (char*)buf+57, 6);
      txnum[6] = '\0'; // Null-terminate the string      
      display.println(txnum);
      
      display.print("LAT: ");
      char lat[11]; strncpy(lat, (char*)buf, 10); lat[10] = '\0';
      display.println(lat);

      display.print("LONG: ");
      char lon[11]; strncpy(lon, (char*)buf+11, 10); lon[10] = '\0';
      display.println(lon);

      display.print("ACC: ");
      char acc[6]; strncpy(acc, (char*)buf+22, 5); acc[5] = '\0';
      display.println(acc);

      //display.setCursor(64, 0);

      display.print("TEMP: ");
      char temp[7]; strncpy(temp, (char*)buf+28, 6);temp[6] = '\0';
      //display.setCursor(64, 8);
      display.println(temp);

      //display.setCursor(64, 16);
      display.print("ALT: ");
      char alt[9]; strncpy(alt, (char*)buf+35, 8); alt[8] = '\0';
      //display.setCursor(64, 24);
      display.println(alt);

      //display.setCursor(64, 32);
      display.print("PRES: ");
      char pres[8]; strncpy(pres, (char*)buf+44, 7); pres[7] = '\0';
      //display.setCursor(64, 40);
      display.println(pres);

      //display.setCursor(64, 48);
      display.print("RSSI: ");
      //display.setCursor(64, 56);
      display.println(rf95.lastRssi(), DEC);


      display.setRotation(4);


      display.setCursor(0,112);
      display.print("FBAT: ");
      char fltVoltage[5]; strncpy(fltVoltage, (char*)buf+52, 4); fltVoltage[4] = '\0';
      display.println(fltVoltage);

      display.setCursor(0,120);
      display.print("GBAT: ");
      display.println(gndVoltage);


      display.display();

      // Send a reply
      // uint8_t data[] = "And hello back to you";
      // rf95.send(data, sizeof(data));
      // rf95.waitPacketSent();
      // Serial.println("Sent a reply");
      // digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.println("Receive failed");
    } 
  }
}


