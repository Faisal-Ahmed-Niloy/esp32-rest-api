//need to change the wifi config &
// server ip
// run "server.py" with this.

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "time.h"

// ---------wifi config ----------
const char* ssid = "Faisal";
const char* password = "987654321";

// ip & port
const char* SERVER_IP = "192.168.30.31"; // change as per pc ip 
const int SERVER_PORT = 8000;

String userName = "A123"; // current user
int targetNumber = 0;
int doneCount = 0;
bool maintenanceActive = false;   // lock device when maintenance triggered
bool targetAchieved = false;      // track if target reached

//tft pins
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Buttons
const int BTN1 = 27; // increment
const int BTN2 = 26; // maintainance (long press for 2s)
unsigned long btn1_last = 0;
unsigned long btn2_press_time = 0;
bool btn2_pressed = false;
const unsigned long debounce_ms = 50;
const unsigned long longpress_ms = 2000;

// Timing
unsigned long lastTargetPoll = 0;
const unsigned long pollIntervalMs = 5000; // poll every 5s for updated target

// NTP - date time update
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// helpers
bool serverReachable = false;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 starting...");

  // Buttons
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);

  // TFT init
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

  // connect WiFi
  connectWiFi();

  // NTP setup to get date/time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // initial target fetch
  fetchTargetFromServer();

  // initial screen
  drawDisplay();
}

void loop() {
  unsigned long now = millis();

  // Poll target periodically
  if (now - lastTargetPoll > pollIntervalMs) {
    lastTargetPoll = now;
    fetchTargetFromServer();
  }

 if (!maintenanceActive) {
  // handle button1 - increment 
  if (digitalRead(BTN1) == LOW) {
    if (millis() - btn1_last > 300) { 
      btn1_last = millis();
      onButton1Press();
    }
  }

  // handle button2 long press
  if (digitalRead(BTN2) == LOW) {
    if (!btn2_pressed) {
      btn2_pressed = true;
      btn2_press_time = millis();
    } else {
      if (millis() - btn2_press_time >= longpress_ms) {
        onButton2LongPress();
        while (digitalRead(BTN2) == LOW) { delay(10); }
        btn2_pressed = false;
      }
    }
  } else {
    btn2_pressed = false;
  }
 }

  // update display status area every loop
  if (!maintenanceActive) {
    drawStatusOnly(); // display will not update if maintainance is on 
  }
  delay(10);
}

// ---------------------------
// BUTTON HANDLERS
// ---------------------------
void onButton1Press() {
  doneCount++;
  Serial.printf("[BTN1] %s doneCount -> %d (target %d)\n", userName.c_str(), doneCount, targetNumber);
  // send JSON to server
  sendDoneToServer();
  if (!maintenanceActive) {
    drawDisplay();  // if maintainance is on, the display will not fetch
  }


  // if target reached show congrats
  if (targetNumber > 0 && doneCount >= targetNumber && !targetAchieved) {
  showTargetCompleteMessage();   // congrats overlay
  delay(3000);                   // show for 3s
  targetAchieved = true;         // mark achieved
  }
}

void onButton2LongPress() {
  Serial.printf("[BTN2 LONG] Maintenance requested by %s\n", userName.c_str());
  maintenanceActive = true;   // lock device
  showMaintenanceMessage();   // stays until restart
  sendMaintenanceToServer();
  //drawDisplay();
}

// ---------------------------
// wifi & server
// ---------------------------
void connectWiFi() {
  Serial.printf("Connecting to WiFi '%s' ...\n", ssid);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: ");
    Serial.println(WiFi.localIP());
    serverReachable = true; // initially assume reachable; we'll verify on request
  } else {
    Serial.println("\nWiFi connection failed.");
    serverReachable = false;
  }
}

bool checkServerReachable() {
  if (WiFi.status() != WL_CONNECTED) return false;
  HTTPClient http;
  String url = String("http://") + SERVER_IP + ":" + String(SERVER_PORT) + "/"; // root
  http.begin(url);
  http.setTimeout(3000);
  int code = http.GET();
  http.end();
  bool ok = (code == 200);
  serverReachable = ok;
  return ok;
}

void fetchTargetFromServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[fetchTarget] WiFi not connected");
    serverReachable = false;
    return;
  }
  String url = String("http://") + SERVER_IP + ":" + String(SERVER_PORT) + "/get_target?user=" + userName;
  Serial.println("[fetchTarget] GET " + url);
  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code > 0 && code == 200) {
    String payload = http.getString();
    Serial.println("[fetchTarget] payload: " + payload);
    // parse JSON
    DynamicJsonDocument doc(256);
    DeserializationError err = deserializeJson(doc, payload);
    if (!err) {
      targetNumber = doc["target"] | 0;
      if (doneCount < targetNumber) {
        targetAchieved = false; //reset flag after new target arrives
      }
      serverReachable = true;
    } else {
      Serial.println("[fetchTarget] JSON parse error");
      serverReachable = false;
    }
  } else {
    Serial.printf("[fetchTarget] Error code: %d\n", code);
    serverReachable = false;
  }
  http.end();
  if (!maintenanceActive) {
    drawDisplay();
  }
}

// Send /data on every BTN1 press with date/time/user/target/done
void sendDoneToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[sendDone] WiFi disconnected");
    return;
  }
  // Get date/time
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("[sendDone] Failed to obtain time");
  }
  char datestr[32], timestr[32];
  if (getLocalTime(&timeinfo)) {
    strftime(datestr, sizeof(datestr), "%Y-%m-%d", &timeinfo);
    strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
  } else {
    strcpy(datestr, "n/a");
    strcpy(timestr, "n/a");
  }

  // build JSON
  DynamicJsonDocument doc(512);
  doc["date"] = String(datestr);
  doc["time"] = String(timestr);
  doc["user"] = userName;
  doc["target"] = targetNumber;
  doc["done"] = doneCount;

  String json;
  serializeJson(doc, json);

  String url = String("http://") + SERVER_IP + ":" + String(SERVER_PORT) + "/data";
  Serial.println("[sendDone] POST " + url + " -> " + json);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(json);
  if (code > 0) {
    String payload = http.getString();
    Serial.printf("[sendDone] response %d: %s\n", code, payload.c_str());
    serverReachable = (code == 200);
  } else {
    Serial.printf("[sendDone] error code %d\n", code);
    serverReachable = false;
  }
  http.end();
}

// Send maintenance notify (btn2 long prs)
void sendMaintenanceToServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[sendMaint] WiFi disconnected");
    return;
  }

  // build JSON
  DynamicJsonDocument doc(256);
  doc["user"] = userName;
  doc["message"] = String("Maintenance needed for user: ") + userName;

  String json;
  serializeJson(doc, json);

  String url = String("http://") + SERVER_IP + ":" + String(SERVER_PORT) + "/notify";
  Serial.println("[sendMaint] POST " + url + " -> " + json);

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(json);
  if (code > 0) {
    String payload = http.getString();
    Serial.printf("[sendMaint] response %d: %s\n", code, payload.c_str());
    serverReachable = (code == 200);
  } else {
    Serial.printf("[sendMaint] error code %d\n", code);
    serverReachable = false;
  }
  http.end();
}

// ---------------------------
// display
// ---------------------------

void drawDisplay() {
  tft.fillScreen(ILI9341_BLACK);

  // Title
  tft.setCursor(6, 6);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_CYAN, ILI9341_BLACK);
  tft.println("  - GMS -");

  // User
  tft.setTextSize(2);
  tft.setCursor(6, 36);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.printf("User: %s\n", userName.c_str());

  // progressBar Area (shifted up slightly)
  drawProgressBar(doneCount, targetNumber);

  /*  //old layout
   //target
   tft.setCursor(6, 70);
   tft.setTextSize(3);
   tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
   tft.printf("Target - %d\n", targetNumber);
 
   // Done count
   tft.setCursor(6, 120);
   tft.setTextSize(3);
   tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
   tft.printf("Work Done: %d\n", doneCount);
  */

  // new layout of done and target - split by vertical divider
  int centerX = 160;         // center of screen
  int numberY = 95;          // Y position for large numbers
  int pcsY = numberY + 60;   // Y position for "pcs" labels

  // Prepare number strings
  char doneStr[8], targetStr[8];
  sprintf(doneStr, "%d", doneCount);
  sprintf(targetStr, "%d", targetNumber);

  // Estimate widths for TextSize 6 (≈36px per char)
  int doneWidth = strlen(doneStr) * 36;
  int targetWidth = strlen(targetStr) * 36;

  // DONE side (left half)
  int doneX = centerX - 20 - doneWidth;  // 20px gap from center
  tft.setCursor(doneX, numberY);
  tft.setTextSize(6);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print(doneStr);

  // "pcs" below Done
  tft.setCursor(doneX, pcsY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("pcs");

  // TARGET side (right half)
  int targetX = centerX + 20;  // 20px gap from center
  tft.setCursor(targetX, numberY);
  tft.setTextSize(6);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.print(targetStr);

  // "pcs" below Target
  tft.setCursor(targetX, pcsY);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("pcs");

    // Vertical divider line (4px wide)
  int dividerX = 158;  // center of screen minus half thickness
  int dividerYTop = numberY - 10;
  int dividerYBottom = pcsY + 20;
  int dividerWidth = 4;
  tft.fillRect(dividerX, dividerYTop, dividerWidth, dividerYBottom - dividerYTop, ILI9341_WHITE);


/*  // target achieve
  if (targetAchieved) {
    tft.setCursor(6, 175); // shifted down 5px to avoid overlap
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
    tft.println("Target Achieved!");
  }
*/

  // target achieve message - centered and lowered
  if (targetAchieved) {
    const char* msg = "Target Achieved!";
    int textSize = 2;
    int charWidth = 12; // TextSize 2 ≈ 6px * 2
    int msgLength = strlen(msg);
    int msgWidth = msgLength * charWidth;

    int centerX = (320 - msgWidth) / 2;
    int centerY = 178; // lowered for spacing

    tft.setCursor(centerX, centerY);
    tft.setTextSize(textSize);
    tft.setTextColor(ILI9341_MAGENTA, ILI9341_BLACK);
    tft.println(msg);
  }


  // bottom message area
  tft.setCursor(6, 200);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.println("Press & hold Button 2 for    Maintenance Help."); // oops! banan bhul hoisilo

  // status area (top-right)
  drawStatusOnly();
}


// for the progress bar on the top of done/target 
void drawProgressBar(int done, int target) {
  int barX = 6, barY = 63, barW = 300, barH = 10; // barY raised by ~7px

  // Outline
  tft.drawRect(barX, barY, barW, barH, ILI9341_WHITE);

  // Fill based on progress
  int filledW = 0;
  if (target > 0) {
    filledW = (done * barW) / target;
    if (filledW > barW) filledW = barW; // cap at 100%
  }

  // Green filled portion
  if (filledW > 0) {
    tft.fillRect(barX + 1, barY + 1, filledW - 2, barH - 2, ILI9341_GREEN);
  }

  // Red remaining portion
  if (filledW < barW) {
    tft.fillRect(barX + filledW, barY + 1, barW - filledW - 1, barH - 2, ILI9341_RED);
  }
}

//status display
void drawStatusOnly() {
  // top-right small boxes for wifi & server status
  int x = 220, y = 7;
  // wifi status
  tft.fillRect(x, y, 100, 20, ILI9341_BLACK);
  tft.setCursor(x+2, y+2);
  tft.setTextSize(1);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print("WiFi: ON");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print("WiFi: OFF");
  }

  // server status below wifi
  tft.fillRect(x, y+20, 100, 20, ILI9341_BLACK); //reduced to y+20 from y+25 for inline
  tft.setCursor(x+2, y+22);
  tft.setTextSize(1);
  if (serverReachable || checkServerReachable()) {
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
    tft.print("Server: UP");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.print("Server: DOWN");
  }
}

// show maintenance overlay
void showMaintenanceMessage() {
  // overlay rectangle center
  int cx = 40, cy = 90;
  tft.fillRect(20, 80, 280, 80, ILI9341_RED);
  tft.setCursor(40, 100);
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
  tft.println("MAINTENANCE         MODE ON");
  Serial.println("Displaying: MAINTENANCE MODE ON");
}

// show completion message
void showTargetCompleteMessage() {
  tft.fillRect(20, 80, 280, 80, ILI9341_BLUE);
  tft.setCursor(30, 100);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLUE);
  tft.println("Target Completed!"); //minor correction
  tft.setCursor(30, 130);
  tft.println("Well Done!");
  Serial.println("Displaying: Today's Target Completed! Well Done!");
}
