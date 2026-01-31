#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <time.h>
#include <ArduinoJson.h>

#define DHTPIN 10
#define DHTTYPE DHT11
#define LED_PIN 8

const char* ssid = "theinfoflux";
const char* password = "12345678";

WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

bool ledState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.println(WiFi.localIP());

  configTime(5 * 3600, 0, "pool.ntp.org");

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/toggle", handleToggle);

  server.begin();
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  String page = R"====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32-C3 Dashboard</title>

<style>
body{
  margin:0;
  font-family:'Segoe UI', sans-serif;

  /* 🔵➡🟡 Dark blue to yellow gradient */
  background: linear-gradient(135deg, #0b1c2d, #FFD700);

  height:100vh;
  display:flex;
  justify-content:center;
  align-items:center;
}

.card{
  width:380px;
  padding:30px;
  border-radius:25px;
  background:#132f4c; /* dark blue card */
  box-shadow:0 20px 50px rgba(0,0,0,0.6);
  text-align:center;
  color:#fff;
}

h1{
  font-size:26px;
  margin-bottom:20px;
}

/* 🟡 Yellow data */
.data{
  font-size:24px;
  margin:12px 0;
  font-weight:bold;
  background:#FFD700;
  color:#000;
  border-radius:16px;
  padding:12px;
}

.time{
  font-size:20px;
  margin:15px 0;
  font-weight:600;
  background:#FFD700;
  color:#000;
  padding:10px;
  border-radius:14px;
}

/* Button */
.toggle{
  margin-top:20px;
  padding:16px;
  border-radius:50px;
  border:none;
  width:100%;
  font-size:18px;
  cursor:pointer;
  font-weight:700;
  transition:0.3s;
}

.toggle-on{
  background:linear-gradient(135deg,#00c853,#b2ff59);
  color:#000;
}

.toggle-off{
  background:linear-gradient(135deg,#d50000,#ff8a80);
  color:#fff;
}

.status{
  margin-top:12px;
  font-size:18px;
  font-weight:bold;
}

.footer{
  margin-top:18px;
  font-size:14px;
  opacity:0.85;
}
</style>
</head>

<body>
<div class="card">
  <h1>ESP32-C3 SMART PANEL</h1>

  <div class="data" id="tempData">🌡 Temperature: -- °C</div>
  <div class="data" id="humData">💧 Humidity: -- %</div>
  <div class="time" id="time">Loading...</div>

  <button class="toggle toggle-off" id="ledButton" onclick="toggleLED()">Toggle LED</button>
  <div class="status" id="ledStatus">LED: --</div>

  <div class="footer">Powered by XIAO ESP32-C3</div>
</div>

<script>
function loadData(){
  fetch('/data')
  .then(r => r.json())
  .then(d => {
    document.getElementById('tempData').innerHTML =
      "🌡 Temperature: " + d.temp + " °C";
    document.getElementById('humData').innerHTML =
      "💧 Humidity: " + d.hum + " %";
    document.getElementById('time').innerHTML = d.time;

    let btn = document.getElementById('ledButton');
    let status = document.getElementById('ledStatus');

    if(d.led){
      btn.className = "toggle toggle-on";
      status.innerHTML = "LED: ON 🔥";
    } else {
      btn.className = "toggle toggle-off";
      status.innerHTML = "LED: OFF ❄️";
    }
  });
}

function toggleLED(){
  fetch('/toggle').then(()=>loadData());
}

setInterval(loadData,1000);
loadData();
</script>
</body>
</html>
)====";

  server.send(200, "text/html", page);
}

void handleData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[30];
  strftime(timeStr, sizeof(timeStr), "%d %b %Y | %H:%M:%S", &timeinfo);

  StaticJsonDocument<200> doc;
  doc["temp"] = t;
  doc["hum"] = h;
  doc["time"] = timeStr;
  doc["led"] = ledState;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json; charset=UTF-8", json);
}

void handleToggle() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState);
  server.send(200, "text/plain", "OK");
}
