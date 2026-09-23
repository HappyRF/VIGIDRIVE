#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <UniversalTelegramBot.h>

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID = "Resyvex";
const char* WIFI_PASSWORD = "12345678";

// ============================================================
// TELEGRAM
// ============================================================

// Replace these with your actual values

const char* BOT_TOKEN = "8766021776:AAES4fk44qM5OJIp1_jxomm59SztrT4SKxA";
const char* CHAT_ID   = "1402993867";

// ============================================================
// PIN DEFINITIONS
// ============================================================

#define SDA_PIN 21
#define SCL_PIN 22

#define BUZZER_PIN 25

#define RESET_BUTTON_PIN 27

#define GPS_RX_PIN 16
#define GPS_TX_PIN 17

// ============================================================
// MPU6050
// ============================================================

#define MPU_ADDR 0x68

#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define WHO_AM_I     0x75

// ============================================================
// ACCIDENT SETTINGS
// ============================================================

// Your current stationary reading is approximately 1.2-1.3G.
//
// Start with 2.0G for testing.
// Tune this later based on real readings.

const float ACCIDENT_THRESHOLD = 2.0;

// Warning period

const unsigned long WARNING_TIME = 10000;

// ============================================================
// OBJECTS
// ============================================================

TinyGPSPlus gps;

HardwareSerial GPSSerial(2);

WiFiClientSecure secureClient;

UniversalTelegramBot bot(
  BOT_TOKEN,
  secureClient
);

WebServer server(80);

// ============================================================
// STATUS VARIABLES
// ============================================================

bool mpuOK = false;

bool wifiConnected = false;

bool telegramConfigured = false;

bool telegramWorking = false;

bool gpsHasFix = false;

bool accidentWarning = false;

bool accidentConfirmed = false;

bool accidentCancelled = false;

bool gpsWaiting = false;

// ============================================================
// SENSOR VALUES
// ============================================================

float ax = 0;

float ay = 0;

float az = 0;

float totalG = 0;

// ============================================================
// GPS VALUES
// ============================================================

double latitude = 0;

double longitude = 0;

float gpsAltitude = 0;

float gpsSpeed = 0;

int satelliteCount = 0;

// ============================================================
// TIMERS
// ============================================================

unsigned long accidentStartTime = 0;

unsigned long lastSensorRead = 0;

unsigned long lastGPSDisplay = 0;

unsigned long lastWiFiCheck = 0;

unsigned long lastTelegramTest = 0;

unsigned long lastStatusPrint = 0;

unsigned long lastWarningPrint = 0;

// ============================================================
// COUNTERS
// ============================================================

unsigned long accidentCount = 0;

// ============================================================
// DIRECT MPU6050 FUNCTIONS
// ============================================================

void mpuWriteRegister(byte reg, byte value)
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);

  Wire.write(value);

  byte error = Wire.endTransmission();

  if (error != 0)
  {
    Serial.print("MPU write error: ");
    Serial.println(error);
  }
}


// ------------------------------------------------------------

bool mpuReadRegister(byte reg, byte &value)
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);

  if (Wire.endTransmission(false) != 0)
  {
    return false;
  }

  if (Wire.requestFrom(MPU_ADDR, 1) != 1)
  {
    return false;
  }

  value = Wire.read();

  return true;
}


// ------------------------------------------------------------

bool initializeMPU()
{
  byte whoAmI = 0;

  if (!mpuReadRegister(WHO_AM_I, whoAmI))
  {
    Serial.println("❌ MPU6050 communication failed.");

    return false;
  }

  Serial.print("MPU WHO_AM_I = 0x");

  if (whoAmI < 16)
  {
    Serial.print("0");
  }

  Serial.println(whoAmI, HEX);

  /*
    Your module previously returned 0x70 but still produced
    correct acceleration data.

    Therefore we don't reject the sensor based only on WHO_AM_I.
  */

  mpuWriteRegister(PWR_MGMT_1, 0x00);

  delay(100);

  Serial.println("MPU6050 initialized.");

  return true;
}


// ------------------------------------------------------------

bool readMPU()
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(ACCEL_XOUT_H);

  if (Wire.endTransmission(false) != 0)
  {
    return false;
  }

  if (Wire.requestFrom(MPU_ADDR, 6) != 6)
  {
    return false;
  }

  int16_t rawX =
    (Wire.read() << 8) |
    Wire.read();

  int16_t rawY =
    (Wire.read() << 8) |
    Wire.read();

  int16_t rawZ =
    (Wire.read() << 8) |
    Wire.read();


  /*
    ±2G scale

    16384 LSB = 1G
  */

  ax = rawX / 16384.0;

  ay = rawY / 16384.0;

  az = rawZ / 16384.0;


  totalG = sqrt(
    ax * ax +
    ay * ay +
    az * az
  );

  return true;
}


// ============================================================
// BUZZER
// ============================================================

void buzzerOn()
{
  digitalWrite(
    BUZZER_PIN,
    HIGH
  );
}


// ------------------------------------------------------------

void buzzerOff()
{
  digitalWrite(
    BUZZER_PIN,
    LOW
  );
}


// ------------------------------------------------------------

void buzzerTest()
{
  Serial.println("Testing buzzer...");

  buzzerOn();

  delay(1000);

  buzzerOff();

  Serial.println("Buzzer test complete.");
}


// ============================================================
// GPS
// ============================================================

void readGPS()
{
  while (GPSSerial.available())
  {
    gps.encode(
      GPSSerial.read()
    );
  }


  if (gps.location.isValid())
  {
    gpsHasFix = true;

    latitude =
      gps.location.lat();

    longitude =
      gps.location.lng();
  }
  else
  {
    gpsHasFix = false;
  }


  if (gps.satellites.isValid())
  {
    satelliteCount =
      gps.satellites.value();
  }


  if (gps.altitude.isValid())
  {
    gpsAltitude =
      gps.altitude.meters();
  }


  if (gps.speed.isValid())
  {
    gpsSpeed =
      gps.speed.kmph();
  }
}


// ============================================================
// WIFI
// ============================================================

void connectWiFi()
{
  Serial.println();
  Serial.println("==============================");
  Serial.println("CONNECTING TO MOBILE HOTSPOT");
  Serial.println("==============================");

  WiFi.mode(WIFI_STA);

  WiFi.setSleep(false);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print("SSID: ");

  Serial.println(
    WIFI_SSID
  );


  int attempts = 0;


  while (
    WiFi.status() != WL_CONNECTED &&
    attempts < 30
  )
  {
    delay(500);

    Serial.print(".");

    attempts++;
  }


  Serial.println();


  if (WiFi.status() == WL_CONNECTED)
  {
    wifiConnected = true;

    Serial.println("✅ WIFI CONNECTED!");

    Serial.print("SSID: ");

    Serial.println(
      WiFi.SSID()
    );


    Serial.print("IP Address: ");

    Serial.println(
      WiFi.localIP()
    );


    Serial.print("RSSI: ");

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(" dBm");
  }
  else
  {
    wifiConnected = false;

    Serial.println(
      "❌ WIFI CONNECTION FAILED"
    );
  }
}


// ============================================================
// TELEGRAM
// ============================================================

bool telegramIsConfigured()
{
  if (
    strlen(BOT_TOKEN) < 20 ||
    strlen(CHAT_ID) < 1
  )
  {
    return false;
  }

  if (
    String(BOT_TOKEN) ==
    "YOUR_BOT_TOKEN"
  )
  {
    return false;
  }

  if (
    String(CHAT_ID) ==
    "YOUR_CHAT_ID"
  )
  {
    return false;
  }

  return true;
}


// ------------------------------------------------------------

void sendTelegram(String message)
{
  if (!wifiConnected)
  {
    Serial.println(
      "Telegram skipped: WiFi not connected."
    );

    return;
  }


  if (!telegramConfigured)
  {
    Serial.println(
      "Telegram skipped: Bot not configured."
    );

    return;
  }


  Serial.println(
    "Sending Telegram message..."
  );


  int result =
    bot.sendMessage(
      CHAT_ID,
      message,
      "HTML"
    );


  if (result)
  {
    telegramWorking = true;

    Serial.println(
      "✅ Telegram message sent."
    );
  }
  else
  {
    telegramWorking = false;

    Serial.println(
      "❌ Telegram message failed."
    );
  }
}


// ------------------------------------------------------------

void sendStartupTelegram()
{
  String message = "";

  message +=
    "<b>🚗 RESYVEX ACCIDENT SYSTEM</b>\n\n";

  message +=
    "✅ ESP32 Started\n";

  message +=
    "📶 WiFi: CONNECTED\n";

  message +=
    "🧠 MPU6050: ";

  message +=
    mpuOK ? "OK\n" : "ERROR\n";


  message +=
    "📡 GPS: ";

  message +=
    gpsHasFix ? "FIXED\n" : "WAITING FOR FIX\n";


  message +=
    "📍 Satellites: ";

  message +=
    String(satelliteCount);

  message += "\n\n";

  message +=
    "System is ready.";

  sendTelegram(message);
}


// ============================================================
// ACCIDENT ALERT
// ============================================================

void startAccidentWarning()
{
  if (accidentWarning)
  {
    return;
  }


  accidentWarning = true;

  accidentConfirmed = false;

  accidentCancelled = false;

  gpsWaiting = false;


  accidentStartTime =
    millis();


  buzzerOn();


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "🚨 ACCIDENT / IMPACT DETECTED!"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "Impact: "
  );

  Serial.print(
    totalG,
    2
  );

  Serial.println(
    " G"
  );

  Serial.println(
    "Buzzer ON"
  );

  Serial.println(
    "Press RESET within 10 seconds."
  );


  if (wifiConnected)
  {
    String message = "";

    message +=
      "<b>🚨 POSSIBLE ACCIDENT DETECTED</b>\n\n";

    message +=
      "Impact: ";

    message +=
      String(totalG, 2);

    message +=
      " G\n\n";

    message +=
      "🔊 Buzzer activated\n";

    message +=
      "⏱ 10-second cancellation period\n\n";

    message +=
      "Press the physical RESET button if this is a false alarm.";

    sendTelegram(message);
  }
}


// ============================================================
// CANCEL ACCIDENT
// ============================================================

void cancelAccident()
{
  accidentWarning = false;

  accidentCancelled = true;

  accidentConfirmed = false;

  gpsWaiting = false;

  buzzerOff();


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "✅ ACCIDENT CANCELLED"
  );

  Serial.println(
    "RESET BUTTON PRESSED"
  );

  Serial.println(
    "================================"
  );


  if (wifiConnected)
  {
    String message = "";

    message +=
      "<b>✅ ACCIDENT ALERT CANCELLED</b>\n\n";

    message +=
      "Reset button was pressed within 10 seconds.\n\n";

    message +=
      "No accident location was sent.";

    sendTelegram(message);
  }
}


// ============================================================
// CONFIRM ACCIDENT
// ============================================================

void confirmAccident()
{
  accidentWarning = false;

  accidentConfirmed = true;

  accidentCancelled = false;

  accidentCount++;

  buzzerOff();

  gpsWaiting = true;


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "🚨 ACCIDENT CONFIRMED!"
  );

  Serial.println(
    "================================"
  );

  Serial.println(
    "Getting GPS location..."
  );


  if (gpsHasFix)
  {
    sendAccidentLocation();
  }
  else
  {
    Serial.println(
      "⚠️ GPS FIX NOT AVAILABLE."
    );

    if (wifiConnected)
    {
      String message = "";

      message +=
        "<b>🚨 ACCIDENT CONFIRMED</b>\n\n";

      message +=
        "⚠️ GPS location is currently unavailable.\n\n";

      message +=
        "Satellites: ";

      message +=
        String(satelliteCount);

      message += "\n\n";

      message +=
        "Please check the GPS module.";

      sendTelegram(message);
    }
  }
}


// ============================================================
// SEND ACCIDENT LOCATION
// ============================================================

void sendAccidentLocation()
{
  if (!gpsHasFix)
  {
    Serial.println(
      "GPS location unavailable."
    );

    return;
  }


  Serial.println();
  Serial.println(
    "GPS LOCATION FOUND!"
  );


  Serial.print(
    "Latitude: "
  );

  Serial.println(
    latitude,
    6
  );


  Serial.print(
    "Longitude: "
  );

  Serial.println(
    longitude,
    6
  );


  Serial.print(
    "Satellites: "
  );

  Serial.println(
    satelliteCount
  );


  String mapsLink =
    "https://maps.google.com/?q=";

  mapsLink +=
    String(latitude, 6);

  mapsLink += ",";

  mapsLink +=
    String(longitude, 6);


  String message = "";

  message +=
    "<b>🚨 ACCIDENT DETECTED!</b>\n\n";

  message +=
    "⚠️ Accident confirmed.\n\n";


  message +=
    "📍 <b>Location</b>\n";

  message +=
    "Latitude: ";

  message +=
    String(latitude, 6);

  message += "\n";

  message +=
    "Longitude: ";

  message +=
    String(longitude, 6);

  message += "\n\n";


  message +=
    "🛰 Satellites: ";

  message +=
    String(satelliteCount);

  message += "\n";


  message +=
    "🌐 <a href=\"";

  message +=
    mapsLink;

  message +=
    "\">OPEN LOCATION IN GOOGLE MAPS</a>\n\n";


  message +=
    "📶 WiFi: Connected\n";

  message +=
    "🧠 MPU6050: OK\n";

  message +=
    "🚨 Impact: ";

  message +=
    String(totalG, 2);

  message +=
    " G";


  sendTelegram(message);


  Serial.println();
  Serial.println(
    "✅ ACCIDENT LOCATION SENT TO TELEGRAM"
  );
}


// ============================================================
// WEB SERVER
// ============================================================

String htmlPage()
{
  String html = "";

  html += "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";

  html +=
    "<meta name='viewport' content='width=device-width,initial-scale=1'>";

  html +=
    "<meta http-equiv='refresh' content='5'>";

  html += "<title>Resyvex Accident System</title>";

  html += "<style>";

  html +=
    "body{font-family:Arial;background:#111;color:white;padding:20px;}";

  html +=
    ".card{background:#222;padding:18px;margin:10px 0;border-radius:12px;}";

  html +=
    ".ok{color:#00ff88;}";

  html +=
    ".bad{color:#ff5555;}";

  html +=
    "h1{color:#00d9ff;}";

  html += "</style>";

  html += "</head>";

  html += "<body>";

  html +=
    "<h1>🚗 RESYVEX ACCIDENT DETECTION</h1>";


  // WIFI

  html += "<div class='card'>";

  html += "<h2>📶 WiFi</h2>";

  html += "Status: ";

  html +=
    wifiConnected
    ? "<span class='ok'>CONNECTED</span>"
    : "<span class='bad'>DISCONNECTED</span>";

  html += "<br>SSID: ";

  html += WIFI_SSID;

  html += "<br>IP: ";

  html +=
    WiFi.localIP().toString();

  html += "<br>RSSI: ";

  html +=
    String(WiFi.RSSI());

  html += " dBm";

  html += "</div>";


  // MPU

  html += "<div class='card'>";

  html += "<h2>🧠 MPU6050</h2>";

  html += "Status: ";

  html +=
    mpuOK
    ? "<span class='ok'>OK</span>"
    : "<span class='bad'>ERROR</span>";

  html += "<br>X: ";

  html += String(ax, 2);

  html += " G";

  html += "<br>Y: ";

  html += String(ay, 2);

  html += " G";

  html += "<br>Z: ";

  html += String(az, 2);

  html += " G";

  html += "<br>Total: ";

  html += String(totalG, 2);

  html += " G";

  html += "</div>";


  // GPS

  html += "<div class='card'>";

  html += "<h2>📡 GPS</h2>";

  html += "Status: ";

  html +=
    gpsHasFix
    ? "<span class='ok'>GPS FIX</span>"
    : "<span class='bad'>WAITING FOR FIX</span>";

  html += "<br>Satellites: ";

  html +=
    String(satelliteCount);

  html += "<br>Latitude: ";

  html +=
    String(latitude, 6);

  html += "<br>Longitude: ";

  html +=
    String(longitude, 6);

  html += "</div>";


  // TELEGRAM

  html += "<div class='card'>";

  html += "<h2>🤖 Telegram</h2>";

  html += "Bot configured: ";

  html +=
    telegramConfigured
    ? "<span class='ok'>YES</span>"
    : "<span class='bad'>NO</span>";

  html += "<br>Last status: ";

  html +=
    telegramWorking
    ? "<span class='ok'>WORKING</span>"
    : "<span class='bad'>NOT TESTED / ERROR</span>";

  html += "</div>";


  // SYSTEM

  html += "<div class='card'>";

  html += "<h2>⚙️ System</h2>";

  html += "Accident warning: ";

  html +=
    accidentWarning
    ? "<span class='bad'>ACTIVE</span>"
    : "<span class='ok'>NO</span>";

  html += "<br>Accident count: ";

  html +=
    String(accidentCount);

  html += "<br>Threshold: ";

  html +=
    String(ACCIDENT_THRESHOLD, 2);

  html += " G";

  html += "</div>";


  html +=
    "<p>Page automatically refreshes every 5 seconds.</p>";

  html += "</body>";

  html += "</html>";

  return html;
}


// ------------------------------------------------------------

void handleRoot()
{
  server.send(
    200,
    "text/html",
    htmlPage()
  );
}


// ------------------------------------------------------------

void startWebServer()
{
  server.on(
    "/",
    handleRoot
  );

  server.begin();

  Serial.println(
    "✅ Web server started."
  );
}


// ============================================================
// PRINT STATUS
// ============================================================

void printStatus()
{
  Serial.println();
  Serial.println(
    "========================================"
  );

  Serial.println(
    "       RESYVEX SYSTEM STATUS"
  );

  Serial.println(
    "========================================"
  );


  // WIFI

  Serial.print(
    "WiFi       : "
  );

  Serial.println(
    wifiConnected
    ? "CONNECTED"
    : "DISCONNECTED"
  );


  if (wifiConnected)
  {
    Serial.print(
      "SSID       : "
    );

    Serial.println(
      WiFi.SSID()
    );


    Serial.print(
      "IP Address : "
    );

    Serial.println(
      WiFi.localIP()
    );


    Serial.print(
      "RSSI       : "
    );

    Serial.print(
      WiFi.RSSI()
    );

    Serial.println(
      " dBm"
    );
  }


  // TELEGRAM

  Serial.print(
    "Telegram   : "
  );

  if (!telegramConfigured)
  {
    Serial.println(
      "NOT CONFIGURED"
    );
  }
  else if (telegramWorking)
  {
    Serial.println(
      "WORKING"
    );
  }
  else
  {
    Serial.println(
      "CONFIGURED / NOT TESTED"
    );
  }


  // MPU

  Serial.print(
    "MPU6050    : "
  );

  Serial.println(
    mpuOK
    ? "OK"
    : "ERROR"
  );


  // GPS

  Serial.print(
    "GPS        : "
  );

  Serial.println(
    gpsHasFix
    ? "FIXED"
    : "WAITING"
  );


  Serial.print(
    "Satellites : "
  );

  Serial.println(
    satelliteCount
  );


  if (gpsHasFix)
  {
    Serial.print(
      "Latitude   : "
    );

    Serial.println(
      latitude,
      6
    );


    Serial.print(
      "Longitude  : "
    );

    Serial.println(
      longitude,
      6
    );
  }


  // ACCELERATION

  Serial.print(
    "Impact     : "
  );

  Serial.print(
    totalG,
    2
  );

  Serial.println(
    " G"
  );


  // SYSTEM

  Serial.print(
    "Accidents  : "
  );

  Serial.println(
    accidentCount
  );


  Serial.print(
    "Uptime     : "
  );

  Serial.print(
    millis() / 1000
  );

  Serial.println(
    " seconds"
  );


  Serial.println(
    "========================================"
  );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  Serial.println();
  Serial.println(
    "################################################"
  );

  Serial.println(
    "#       RESYVEX ACCIDENT DETECTION             #"
  );

  Serial.println(
    "#             ESP32 SYSTEM                     #"
  );

  Serial.println(
    "################################################"
  );


  // ==========================================================
  // BUZZER
  // ==========================================================

  pinMode(
    BUZZER_PIN,
    OUTPUT
  );

  buzzerOff();


  // ==========================================================
  // RESET BUTTON
  // ==========================================================

  pinMode(
    RESET_BUTTON_PIN,
    INPUT_PULLUP
  );


  // ==========================================================
  // I2C
  // ==========================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // ==========================================================
  // MPU
  // ==========================================================

  Serial.println();

  Serial.println(
    "Initializing MPU6050..."
  );

  mpuOK =
    initializeMPU();


  if (mpuOK)
  {
    Serial.println(
      "✅ MPU6050 READY"
    );
  }
  else
  {
    Serial.println(
      "❌ MPU6050 ERROR"
    );
  }


  // ==========================================================
  // GPS
  // ==========================================================

  Serial.println();

  Serial.println(
    "Starting GPS..."
  );

  GPSSerial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX_PIN,
    GPS_TX_PIN
  );

  Serial.println(
    "GPS serial started."
  );


  // ==========================================================
  // BUZZER TEST
  // ==========================================================

  buzzerTest();


  // ==========================================================
  // WIFI
  // ==========================================================

  connectWiFi();


  // ==========================================================
  // TELEGRAM
  // ==========================================================

  telegramConfigured =
    telegramIsConfigured();


  if (telegramConfigured)
  {
    Serial.println(
      "Telegram bot configured."
    );

    /*
      setInsecure() is convenient for ESP32 development.
      For a production device, certificate validation is preferable.
    */

    secureClient.setInsecure();

  }
  else
  {
    Serial.println();
    Serial.println(
      "⚠️ Telegram NOT configured."
    );

    Serial.println(
      "Enter BOT_TOKEN and CHAT_ID."
    );
  }


  // ==========================================================
  // WEB SERVER
  // ==========================================================

  if (wifiConnected)
  {
    startWebServer();
  }


  // ==========================================================
  // STARTUP TELEGRAM
  // ==========================================================

  if (wifiConnected && telegramConfigured)
  {
    delay(1000);

    sendStartupTelegram();
  }


  // ==========================================================
  // FINAL STATUS
  // ==========================================================

  printStatus();


  Serial.println();

  Serial.println(
    "========================================"
  );

  Serial.println(
    "          SYSTEM READY"
  );

  Serial.println(
    "========================================"
  );


  if (wifiConnected)
  {
    Serial.print(
      "Open browser: http://"
    );

    Serial.println(
      WiFi.localIP()
    );
  }
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // WEB SERVER
  // ==========================================================

  if (wifiConnected)
  {
    server.handleClient();
  }


  // ==========================================================
  // GPS
  // ==========================================================

  readGPS();


  // ==========================================================
  // WIFI MONITOR
  // ==========================================================

  if (
    millis() -
    lastWiFiCheck >=
    5000
  )
  {
    lastWiFiCheck =
      millis();


    if (
      WiFi.status() ==
      WL_CONNECTED
    )
    {
      wifiConnected = true;
    }
    else
    {
      wifiConnected = false;

      Serial.println(
        "⚠️ WiFi disconnected."
      );
    }
  }


  // ==========================================================
  // MPU READ
  // ==========================================================

  if (
    millis() -
    lastSensorRead >=
    100
  )
  {
    lastSensorRead =
      millis();


    if (mpuOK)
    {
      if (!readMPU())
      {
        Serial.println(
          "❌ MPU reading failed."
        );
      }
    }
  }


  // ==========================================================
  // RESET BUTTON
  // ==========================================================

  bool resetPressed =
    digitalRead(
      RESET_BUTTON_PIN
    ) == LOW;


  // ==========================================================
  // ACCIDENT WARNING ACTIVE
  // ==========================================================

  if (accidentWarning)
  {
    // --------------------------------------
    // RESET PRESSED
    // --------------------------------------

    if (resetPressed)
    {
      cancelAccident();

      delay(1000);

      return;
    }


    // --------------------------------------
    // COUNTDOWN
    // --------------------------------------

    unsigned long elapsed =
      millis() -
      accidentStartTime;


    if (
      elapsed <
      WARNING_TIME
    )
    {
      if (
        millis() -
        lastWarningPrint >=
        1000
      )
      {
        lastWarningPrint =
          millis();


        int remaining =
          10 -
          (elapsed / 1000);


        Serial.print(
          "🚨 WARNING: "
        );

        Serial.print(
          remaining
        );

        Serial.println(
          " seconds remaining"
        );
      }
    }
    else
    {
      // ------------------------------------
      // 10 SECONDS COMPLETED
      // ------------------------------------

      confirmAccident();

      delay(1000);

      return;
    }
  }
  else
  {
    // ========================================================
    // NORMAL ACCIDENT DETECTION
    // ========================================================

    if (
      mpuOK &&
      totalG >=
      ACCIDENT_THRESHOLD
    )
    {
      startAccidentWarning();
    }
  }


  // ==========================================================
  // PERIODIC STATUS
  // ==========================================================

  if (
    millis() -
    lastStatusPrint >=
    5000
  )
  {
    lastStatusPrint =
      millis();

    printStatus();
  }


  delay(10);
}