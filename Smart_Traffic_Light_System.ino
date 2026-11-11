#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================================
// WIFI ACCESS POINT
// =====================================================

const char* WIFI_NAME = "ESP32_MUSIC";
const char* WIFI_PASSWORD = "12345678";

WebServer server(80);

// =====================================================
// TRAFFIC LIGHT 1
// =====================================================

#define RED1     25
#define YELLOW1  26
#define GREEN1   27

// =====================================================
// TRAFFIC LIGHT 2
// =====================================================

#define RED2     18
#define YELLOW2  19
#define GREEN2   23

// =====================================================
// ULTRASONIC SENSORS
// =====================================================

#define TRIG1 16
#define ECHO1 35

#define TRIG2 17
#define ECHO2 34

const float DETECTION_DISTANCE = 20.0;

// =====================================================
// OLED 1 - HARDWARE I2C
// GPIO 21 SDA, GPIO 22 SCL
// =====================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled1(
  U8G2_R0,
  U8X8_PIN_NONE
);

// =====================================================
// OLED 2 - SOFTWARE I2C
// GPIO 13 SDA, GPIO 14 SCL
// =====================================================

U8G2_SH1106_128X64_NONAME_F_SW_I2C oled2(
  U8G2_R0,
  14,
  13,
  U8X8_PIN_NONE
);

// =====================================================
// TM1650 DISPLAYS
// =====================================================

TwoWire I2C_2 = TwoWire(1);

#define DISPLAY_ADDR 0x34
#define CONTROL_ADDR 0x24

byte digits[] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66,
  0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

// =====================================================
// TRAFFIC TIMING
// =====================================================

const unsigned long durations[4] = {
  5000, 3000, 5000, 3000
};

int phase = 0;
unsigned long phaseStart = 0;

// =====================================================
// ULTRASONIC DETECTION
// =====================================================

float distance1 = 0;
float distance2 = 0;

bool previousDetected1 = false;
bool previousDetected2 = false;

bool violation1 = false;
bool violation2 = false;

unsigned long lastAlert1 = 0;
unsigned long lastAlert2 = 0;

const unsigned long ALERT_COOLDOWN = 3000;

String latestAlert = "No violations detected";
unsigned long alertNumber = 0;

// =====================================================
// OLED UPDATE TIMING
// =====================================================

unsigned long lastOLED = 0;
unsigned long lastSensor = 0;
unsigned long lastSegment = 0;

// =====================================================
// TRAFFIC STATUS FUNCTIONS
// =====================================================

const char* getStatus1() {

  if (phase == 0) return "GREEN";
  if (phase == 1) return "YELLOW";

  return "RED";
}

const char* getStatus2() {

  if (phase == 2) return "GREEN";
  if (phase == 3) return "YELLOW";

  return "RED";
}

int getCountdown1(unsigned long now) {

  unsigned long elapsed = now - phaseStart;

  int remaining =
    (durations[phase] - min(elapsed, durations[phase]) + 999) / 1000;

  if (phase == 2) return remaining + 3;

  return remaining;
}

int getCountdown2(unsigned long now) {

  unsigned long elapsed = now - phaseStart;

  int remaining =
    (durations[phase] - min(elapsed, durations[phase]) + 999) / 1000;

  if (phase == 0) return remaining + 3;

  return remaining;
}

// =====================================================
// SET TRAFFIC LIGHT OUTPUTS
// =====================================================

void updateTrafficLights() {

  // Traffic 1
  digitalWrite(RED1, LOW);
  digitalWrite(YELLOW1, LOW);
  digitalWrite(GREEN1, LOW);

  // Traffic 2
  digitalWrite(RED2, LOW);
  digitalWrite(YELLOW2, LOW);
  digitalWrite(GREEN2, LOW);

  if (phase == 0) {
    digitalWrite(GREEN1, HIGH);
    digitalWrite(RED2, HIGH);
  }

  else if (phase == 1) {
    digitalWrite(YELLOW1, HIGH);
    digitalWrite(RED2, HIGH);
  }

  else if (phase == 2) {
    digitalWrite(RED1, HIGH);
    digitalWrite(GREEN2, HIGH);
  }

  else if (phase == 3) {
    digitalWrite(RED1, HIGH);
    digitalWrite(YELLOW2, HIGH);
  }
}

// =====================================================
// TRAFFIC PHASE CONTROL
// =====================================================

void updateTrafficCycle(unsigned long now) {

  if (now - phaseStart >= durations[phase]) {

    phase = (phase + 1) % 4;
    phaseStart = now;

    updateTrafficLights();
  }
}

// =====================================================
// TM1650 FUNCTIONS
// PRESERVING YOUR WORKING DISPLAY METHOD
// =====================================================

void writeTM1650(TwoWire &bus, byte address, byte data) {

  bus.beginTransmission(address);
  bus.write(data);
  bus.endTransmission();
}

void initSegment(TwoWire &bus) {

  writeTM1650(bus, CONTROL_ADDR, 0x01);
  delay(100);
}

void showNumber(TwoWire &bus, int number) {

  number = constrain(number, 0, 9999);

  int d[4];

  d[0] = number / 1000;
  d[1] = (number / 100) % 10;
  d[2] = (number / 10) % 10;
  d[3] = number % 10;

  for (int i = 0; i < 4; i++) {

    writeTM1650(
      bus,
      DISPLAY_ADDR + i,
      digits[d[i]]
    );
  }
}

// =====================================================
// ULTRASONIC DISTANCE
// =====================================================

float readDistance(int trigPin, int echoPin) {

  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(
    echoPin,
    HIGH,
    25000
  );

  if (duration == 0) return -1;

  return duration * 0.0343 / 2.0;
}

// =====================================================
// VIOLATION ALERT
// =====================================================

void createAlert(String message) {

  latestAlert = message;
  alertNumber++;

  Serial.println("================================");
  Serial.println("TRAFFIC VIOLATION DETECTED!");
  Serial.println(message);
  Serial.println("================================");
}

void checkUltrasonicSensors(unsigned long now) {

  distance1 = readDistance(TRIG1, ECHO1);
  distance2 = readDistance(TRIG2, ECHO2);

  bool detected1 =
    distance1 > 0 && distance1 <= DETECTION_DISTANCE;

  bool detected2 =
    distance2 > 0 && distance2 <= DETECTION_DISTANCE;

  // Sensor 1 belongs to Traffic 1
  if (detected1 && !previousDetected1) {

    if (phase == 2 || phase == 3) {

      if (now - lastAlert1 >= ALERT_COOLDOWN) {

        violation1 = true;
        lastAlert1 = now;

        createAlert(
          "RED LIGHT VIOLATION - TRAFFIC 1"
        );
      }
    }
  }

  // Sensor 2 belongs to Traffic 2
  if (detected2 && !previousDetected2) {

    if (phase == 0 || phase == 1) {

      if (now - lastAlert2 >= ALERT_COOLDOWN) {

        violation2 = true;
        lastAlert2 = now;

        createAlert(
          "RED LIGHT VIOLATION - TRAFFIC 2"
        );
      }
    }
  }

  previousDetected1 = detected1;
  previousDetected2 = detected2;
}

// =====================================================
// PEDESTRIAN DRAWING
// =====================================================

void drawPedestrian(U8G2 &display, bool walking) {

  int x = 112;

  display.drawCircle(x, 21, 5);

  display.drawLine(x, 26, x, 40);

  display.drawLine(x, 30, x - 7, 35);
  display.drawLine(x, 30, x + 7, 35);

  if (walking) {

    if ((millis() / 300) % 2 == 0) {

      display.drawLine(x, 40, x - 7, 52);
      display.drawLine(x, 40, x + 7, 49);

    } else {

      display.drawLine(x, 40, x - 7, 49);
      display.drawLine(x, 40, x + 7, 52);
    }

  } else {

    display.drawLine(x, 40, x - 5, 53);
    display.drawLine(x, 40, x + 5, 53);
  }
}

// =====================================================
// OLED DISPLAY UPDATE
// =====================================================

void updateOLED(
  U8G2 &display,
  int trafficNumber,
  const char* status,
  int countdown,
  bool walking,
  bool violation
) {

  char line[24];

  display.clearBuffer();

  display.setFont(u8g2_font_6x10_tf);

  display.drawStr(0, 9, "SMART TRAFFIC");
  display.drawHLine(0, 12, 128);

  snprintf(
    line,
    sizeof(line),
    "TRAFFIC %d",
    trafficNumber
  );

  display.drawStr(0, 25, line);

  display.drawStr(0, 37, status);

  snprintf(
    line,
    sizeof(line),
    "TIME: %02d SEC",
    countdown
  );

  display.drawStr(0, 49, line);

  drawPedestrian(display, walking);

  display.drawStr(
    88,
    63,
    walking ? "WALK" : "STOP"
  );

  if (violation) {

    display.drawBox(78, 0, 50, 11);

    display.setDrawColor(0);
    display.drawStr(82, 9, "ALERT");
    display.setDrawColor(1);
  }

  display.sendBuffer();
}

// =====================================================
// PHONE WEBPAGE
// =====================================================

const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Smart Traffic System</title>

<style>
body {
  background: #101820;
  color: white;
  font-family: Arial;
  text-align: center;
  margin: 0;
  padding: 15px;
}

h1 { color: #00d4ff; }

.card {
  background: #202e3a;
  padding: 18px;
  margin: 12px auto;
  border-radius: 14px;
  max-width: 420px;
}

.status {
  font-size: 25px;
  font-weight: bold;
}

.counter {
  font-size: 32px;
  color: #00d4ff;
}

.green { color: #00e676; }
.yellow { color: #ffeb3b; }
.red { color: #ff5252; }

.alert {
  background: #b71c1c;
  padding: 15px;
  border-radius: 10px;
  margin-top: 10px;
}

button {
  background: #00bcd4;
  border: none;
  border-radius: 8px;
  padding: 12px;
  color: black;
  font-size: 16px;
  margin: 10px;
}
</style>
</head>

<body>

<h1>SMART TRAFFIC SYSTEM</h1>

<div class="card">
<h2>TRAFFIC 1</h2>
<div id="status1" class="status">Loading...</div>
<div id="count1" class="counter">--</div>
<div>Ultrasonic distance:</div>
<div id="distance1">-- cm</div>
</div>

<div class="card">
<h2>TRAFFIC 2</h2>
<div id="status2" class="status">Loading...</div>
<div id="count2" class="counter">--</div>
<div>Ultrasonic distance:</div>
<div id="distance2">-- cm</div>
</div>

<div class="card">
<h2>VIOLATION MONITOR</h2>
<div id="alert">No violations detected</div>
<button onclick="enableNotifications()">
Enable Phone Notifications
</button>
</div>

<script>

let previousAlert = 0;

function enableNotifications() {

  if ("Notification" in window) {
    Notification.requestPermission();
  } else {
    alert("Browser notifications are not supported.");
  }
}

function setStatus(id, status) {

  let element = document.getElementById(id);

  element.innerText = status;
  element.className = "status";

  if (status === "GREEN") element.classList.add("green");
  if (status === "YELLOW") element.classList.add("yellow");
  if (status === "RED") element.classList.add("red");
}

async function updateDashboard() {

  try {

    const response = await fetch("/data");
    const data = await response.json();

    setStatus("status1", data.status1);
    setStatus("status2", data.status2);

    document.getElementById("count1").innerText =
      data.count1 + " SEC";

    document.getElementById("count2").innerText =
      data.count2 + " SEC";

    document.getElementById("distance1").innerText =
      data.distance1 + " cm";

    document.getElementById("distance2").innerText =
      data.distance2 + " cm";

    document.getElementById("alert").innerText =
      data.alert;

    if (data.alertNumber !== previousAlert) {

      previousAlert = data.alertNumber;

      if ("Notification" in window &&
          Notification.permission === "granted") {

        new Notification("Smart Traffic Alert", {
          body: data.alert
        });
      }

      if (navigator.vibrate) {
        navigator.vibrate([300, 150, 300]);
      }
    }

  } catch (error) {
    console.log("ESP32 connection lost");
  }
}

setInterval(updateDashboard, 500);
updateDashboard();

</script>

</body>
</html>
)rawliteral";

// =====================================================
// WEB SERVER ROUTES
// =====================================================

void setupWebServer() {

  server.on("/", HTTP_GET, []() {

    server.send_P(
      200,
      "text/html",
      webpage
    );
  });

  server.on("/data", HTTP_GET, []() {

    unsigned long now = millis();

    int count1 = getCountdown1(now);
    int count2 = getCountdown2(now);

    String json = "{";

    json += "\"status1\":\"";
    json += getStatus1();
    json += "\",";

    json += "\"status2\":\"";
    json += getStatus2();
    json += "\",";

    json += "\"count1\":";
    json += String(count1);
    json += ",";

    json += "\"count2\":";
    json += String(count2);
    json += ",";

    json += "\"distance1\":";
    json += distance1 < 0 ? "\"No echo\"" : String(distance1, 1);
    json += ",";

    json += "\"distance2\":";
    json += distance2 < 0 ? "\"No echo\"" : String(distance2, 1);
    json += ",";

    json += "\"alert\":\"";
    json += latestAlert;
    json += "\",";

    json += "\"alertNumber\":";
    json += String(alertNumber);

    json += "}";

    server.send(
      200,
      "application/json",
      json
    );
  });

  server.begin();
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Traffic pins
  pinMode(RED1, OUTPUT);
  pinMode(YELLOW1, OUTPUT);
  pinMode(GREEN1, OUTPUT);

  pinMode(RED2, OUTPUT);
  pinMode(YELLOW2, OUTPUT);
  pinMode(GREEN2, OUTPUT);

  // Ultrasonic pins
  pinMode(TRIG1, OUTPUT);
  pinMode(ECHO1, INPUT);

  pinMode(TRIG2, OUTPUT);
  pinMode(ECHO2, INPUT);

  // I2C bus 1
  Wire.begin(21, 22);

  // I2C bus 2
  I2C_2.begin(32, 33);

  // OLED 1
  oled1.setI2CAddress(0x3C * 2);
  oled1.begin();

  // OLED 2
  oled2.setI2CAddress(0x3C * 2);
  oled2.begin();

  // TM1650 displays
  initSegment(Wire);
  initSegment(I2C_2);

  // Start traffic cycle
  phase = 0;
  phaseStart = millis();

  updateTrafficLights();

  // Start Wi-Fi access point
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);

  Serial.println();
  Serial.println("SMART TRAFFIC SYSTEM STARTED");
  Serial.print("WiFi name: ");
  Serial.println(WIFI_NAME);
  Serial.print("Password: ");
  Serial.println(WIFI_PASSWORD);
  Serial.print("Website: http://");
  Serial.println(WiFi.softAPIP());

  // Start phone webpage
  setupWebServer();

  Serial.println("System ready!");
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  unsigned long now = millis();

  // Traffic cycle
  updateTrafficCycle(now);

  // Phone webpage
  server.handleClient();

  // Ultrasonic sensors
  if (now - lastSensor >= 150) {

    lastSensor = now;

    checkUltrasonicSensors(now);
  }

  // Countdown displays
  if (now - lastSegment >= 200) {

    lastSegment = now;

    showNumber(
      Wire,
      getCountdown1(now)
    );

    showNumber(
      I2C_2,
      getCountdown2(now)
    );
  }

  // OLED displays
  if (now - lastOLED >= 200) {

    lastOLED = now;

    int count1 = getCountdown1(now);
    int count2 = getCountdown2(now);

    bool walking1 = (phase == 2 || phase == 3);
    bool walking2 = (phase == 0 || phase == 1);

    updateOLED(
      oled1,
      1,
      getStatus1(),
      count1,
      walking1,
      violation1
    );

    updateOLED(
      oled2,
      2,
      getStatus2(),
      count2,
      walking2,
      violation2
    );
  }
}
