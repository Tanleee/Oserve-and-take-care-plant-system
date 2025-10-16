#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <DHT.h>
#include <string>
#include <EEPROM.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

#include <time.h>
#include "qrcode.h"

bool pumper = false, fan = false, led = false;
float t = 0, h = 0, brightness = 0, soil = 0;
bool automatic = true;
uint8_t amountUser = 0;

float limTemp = 30;
float limSoil = 50;

const char* ssid = "C427";
const char* password = "64546743";
const char* hostname = "greenops";

IPAddress local_IP(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// HTML Template với tính năng đầy đủ
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Điều Khiển Thiết Bị IoT</title>
    <style>
      * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
      }

      body {
        font-family: "Segoe UI", Tahoma, Geneva, Verdana, sans-serif;
        background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%);
        min-height: 100vh;
        padding: 20px;
      }

      .container {
        max-width: 1200px;
        margin: 0 auto;
      }

      .header {
        text-align: center;
        color: white;
        margin-bottom: 30px;
      }

      .header h1 {
        font-size: 2.5em;
        margin-bottom: 10px;
        text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
      }

      .status-bar {
        background: rgba(255, 255, 255, 0.95);
        border-radius: 15px;
        padding: 20px;
        margin-bottom: 30px;
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        backdrop-filter: blur(10px);
      }

      .connection-status {
        display: flex;
        align-items: center;
        justify-content: center;
        gap: 10px;
        margin-bottom: 15px;
      }

      .status-indicator {
        width: 12px;
        height: 12px;
        border-radius: 50%%;
        background: #4caf50;
        animation: pulse 2s infinite;
      }

      @keyframes pulse {
        0%% {
          opacity: 1;
        }
        50%% {
          opacity: 0.5;
        }
        100%% {
          opacity: 1;
        }
      }

      .disconnected {
        background: #f44336;
      }

      .dashboard {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
        gap: 25px;
        margin-bottom: 30px;
      }

      .card {
        background: rgba(255, 255, 255, 0.95);
        border-radius: 20px;
        padding: 25px;
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        backdrop-filter: blur(10px);
        transition: transform 0.3s ease, box-shadow 0.3s ease;
      }

      .card:hover {
        transform: translateY(-5px);
        box-shadow: 0 12px 40px rgba(0, 0, 0, 0.15);
      }

      .card-title {
        font-size: 1.3em;
        font-weight: 600;
        margin-bottom: 20px;
        color: #333;
        display: flex;
        align-items: center;
        gap: 10px;
      }

      .sensor-value {
        font-size: 2.5em;
        font-weight: 700;
        color: #667eea;
        text-align: center;
        margin: 15px 0;
      }

      .sensor-unit {
        font-size: 0.4em;
        color: #666;
        font-weight: normal;
      }

      .control-group {
        display: flex;
        flex-direction: column;
        gap: 15px;
      }

      .switch {
        position: relative;
        display: inline-block;
        width: 60px;
        height: 34px;
      }

      .switch input {
        opacity: 0;
        width: 0;
        height: 0;
      }

      .switch input:disabled + .slider {
        opacity: 0.5;
        cursor: not-allowed;
      }

      .slider {
        position: absolute;
        cursor: pointer;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #ccc;
        transition: 0.4s;
        border-radius: 34px;
      }

      .slider:before {
        position: absolute;
        content: "";
        height: 26px;
        width: 26px;
        left: 4px;
        bottom: 4px;
        transition: 0.4s;
        border-radius: 50%%;
        background-color: white;
      }

      input:checked + .slider {
        background-color: #4caf50;
      }

      input:checked + .slider:before {
        transform: translateX(26px);
      }

      .control-item {
        display: flex;
        justify-content: space-between;
        align-items: center;
        padding: 15px;
        background: rgba(103, 126, 234, 0.1);
        border-radius: 10px;
      }

      .control-item.disabled {
        opacity: 0.6;
      }

      .control-label {
        font-weight: 500;
        color: #333;
      }

      .device-info {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
        gap: 15px;
        margin-top: 15px;
      }

      .info-item {
        text-align: center;
        padding: 15px;
        background: rgba(103, 126, 234, 0.1);
        border-radius: 10px;
      }

      .info-label {
        font-size: 0.9em;
        color: #666;
        margin-bottom: 5px;
      }

      .info-value {
        font-size: 1.2em;
        font-weight: 600;
        color: #333;
      }

      .progress-bar {
        width: 100%%;
        height: 20px;
        background: #e0e0e0;
        border-radius: 10px;
        overflow: hidden;
        margin-top: 10px;
      }

      .progress-fill {
        height: 100%%;
        background: linear-gradient(90deg, #4caf50, #8bc34a);
        transition: width 0.3s ease;
        border-radius: 10px;
      }

      .log-container {
        background: rgba(255, 255, 255, 0.95);
        border-radius: 15px;
        padding: 20px;
        box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
        backdrop-filter: blur(10px);
      }

      .log-title {
        font-size: 1.2em;
        font-weight: 600;
        margin-bottom: 15px;
        color: #333;
      }

      .log-entries {
        max-height: 200px;
        overflow-y: auto;
        background: #f8f9fa;
        border-radius: 8px;
        padding: 15px;
      }

      .log-entry {
        padding: 8px 0;
        border-bottom: 1px solid #e9ecef;
        font-family: "Courier New", monospace;
        font-size: 0.9em;
      }

      .log-entry:last-child {
        border-bottom: none;
      }

      .timestamp {
        color: #666;
        margin-right: 10px;
      }

      @media (max-width: 768px) {
        .dashboard {
          grid-template-columns: 1fr;
        }

        .header h1 {
          font-size: 2em;
        }

        .sensor-value {
          font-size: 2em;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <header class="header">
        <h1>🏠 Hệ Thống Điều Khiển IoT</h1>
        <p>Quản lý và giám sát các thiết bị thông minh</p>
      </header>

      <div class="status-bar">
        <div class="connection-status">
          <div class="status-indicator" id="statusIndicator"></div>
          <span id="connectionStatus">Đang kết nối...</span>
        </div>
        <div class="device-info">
          <div class="info-item">
            <div class="info-label">IP Address</div>
            <div class="info-value" id="deviceIP">%DEVICEIP%</div>
          </div>
          <div class="info-item">
            <div class="info-label">Thời gian hoạt động</div>
            <div class="info-value" id="uptime">00:00:00</div>
          </div>
          <div class="info-item">
            <div class="info-label">Tín hiệu WiFi</div>
            <div class="info-value" id="wifiSignal">%WIFISIGNAL% dBm</div>
          </div>
        </div>
      </div>

      <div class="dashboard">
        <div class="card">
          <div class="card-title">🌡️ Nhiệt độ</div>
          <div class="sensor-value" id="temperature">%TEMPERATURE%<span class="sensor-unit">°C</span></div>
          <div class="progress-bar">
            <div class="progress-fill" id="tempProgress"></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">💧 Độ ẩm không khí</div>
          <div class="sensor-value" id="humidity">%HUMIDITY%<span class="sensor-unit">%%</span></div>
          <div class="progress-bar">
            <div class="progress-fill" id="humidityProgress"></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">☀️ Cường độ sáng</div>
          <div class="sensor-value" id="lightIntensity">%BRIGHTNESS%<span class="sensor-unit">lux</span></div>
          <div class="progress-bar">
            <div class="progress-fill" id="lightProgress"></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🌱 Độ ẩm đất</div>
          <div class="sensor-value" id="soilMoisture">%SOIL%<span class="sensor-unit">%%</span></div>
          <div class="progress-bar">
            <div class="progress-fill" id="soilProgress"></div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">💡 Điều khiển đèn</div>
          <div class="control-group">
            <div class="control-item" id="lightControl">
              <span class="control-label">Đèn LED chính</span>
              <label class="switch">
                <input type="checkbox" id="mainLight" onchange="toggleDevice('light', this.checked)" %LEDCHECK% />
                <span class="slider"></span>
              </label>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🌬️ Điều khiển quạt</div>
          <div class="control-group">
            <div class="control-item" id="fanControl">
              <span class="control-label">Quạt thông gió</span>
              <label class="switch">
                <input type="checkbox" id="ventilationFan" onchange="toggleDevice('fan', this.checked)" %FANCHECK% />
                <span class="slider"></span>
              </label>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🚿 Điều khiển máy bơm</div>
          <div class="control-group">
            <div class="control-item" id="pumpControl">
              <span class="control-label">Máy bơm nước</span>
              <label class="switch">
                <input type="checkbox" id="waterPump" onchange="toggleDevice('pump', this.checked)" %PUMPCHECK% />
                <span class="slider"></span>
              </label>
            </div>
          </div>
        </div>

        <div class="card">
          <div class="card-title">🤖 Hệ thống tự động</div>
          <div class="control-group">
            <div class="control-item">
              <span class="control-label">Chế độ tự động</span>
              <label class="switch">
                <input type="checkbox" id="autoMode" %AUTOCHECK% onchange="toggleAutoMode(this.checked)" />
                <span class="slider"></span>
              </label>
            </div>
          </div>
        </div>
      </div>

      <div class="log-container">
        <div class="log-title">📋 Nhật ký hoạt động</div>
        <div class="log-entries" id="logEntries">
          <div class="log-entry">
            <span class="timestamp">--:--:--</span>
            <span>Đang khởi tạo hệ thống...</span>
          </div>
        </div>
      </div>
    </div>

    <script>
      let isConnected = false;
      let isAutoMode = %AUTOMODE%;
      let updateInterval = 5000;
      var gateway = `ws://${window.location.hostname}/ws`;
      var websocket;

      function initWebSocket() {
        console.log("Trying to open a WebSocket connection...");
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onmessage = onMessage;
      }
      
      function onOpen(event) {
        console.log("Connection opened");
        setConnectionStatus(true);
        addLogEntry("WebSocket kết nối thành công");
      }
      
      function onClose(event) {
        console.log("Connection closed");
        setConnectionStatus(false);
        addLogEntry("WebSocket mất kết nối, đang thử kết nối lại...");
        setTimeout(initWebSocket, 2000);
      }
      
      function onMessage(event) {
        console.log("Received:", event.data);
        
        if (event.data == "auto") {
          isAutoMode = true;
          document.getElementById("autoMode").checked = true;
          updateControlStates();
          return;
        } else if (event.data == "manual") {
          isAutoMode = false;
          document.getElementById("autoMode").checked = false;
          updateControlStates();
          return;
        }

        let deviceId = event.data.slice(0, -1);
        let state = event.data.endsWith("1");
        const mapId = {
          pump: "waterPump",
          light: "mainLight",
          fan: "ventilationFan"
        };

        if (mapId[deviceId]) {
          document.getElementById(mapId[deviceId]).checked = state;
        }
      }

      function updateControlStates() {
        const controls = ['mainLight', 'ventilationFan', 'waterPump'];
        const controlDivs = ['lightControl', 'fanControl', 'pumpControl'];
        
        controls.forEach((id, index) => {
          const element = document.getElementById(id);
          const controlDiv = document.getElementById(controlDivs[index]);
          
          if (isAutoMode) {
            element.disabled = true;
            controlDiv.classList.add('disabled');
          } else {
            element.disabled = false;
            controlDiv.classList.remove('disabled');
          }
        });
      }

      document.addEventListener("DOMContentLoaded", function () {
        initWebSocket();
        initializeSystem();
        updateUptime();
        updateWiFiSignal();
        updateControlStates();
        
        // Cập nhật WiFi signal mỗi 5 giây
        setInterval(updateWiFiSignal, 5000);
      });

      function initializeSystem() {
        addLogEntry("Hệ thống khởi động thành công");
        addLogEntry("Đang kết nối đến server...");
      }

      function setConnectionStatus(connected) {
        isConnected = connected;
        const indicator = document.getElementById("statusIndicator");
        const status = document.getElementById("connectionStatus");

        if (connected) {
          indicator.classList.remove("disconnected");
          status.textContent = "Đã kết nối";
          status.style.color = "#4caf50";
        } else {
          indicator.classList.add("disconnected");
          status.textContent = "Mất kết nối";
          status.style.color = "#f44336";
        }
      }

      function toggleAutoMode(state) {
        isAutoMode = state;
        const action = state ? "BẬT" : "TẮT";
        addLogEntry(`${action} chế độ tự động`);
        websocket.send(state ? "auto" : "manual");
        updateControlStates();
      }

      function toggleDevice(deviceId, state) {
        if (isAutoMode) {
          addLogEntry("Không thể điều khiển thủ công khi đang ở chế độ tự động");
          return;
        }
        
        const deviceNames = {
          light: "Đèn LED chính",
          fan: "Quạt thông gió",
          pump: "Máy bơm nước"
        };
        const action = state ? "BẬT" : "TẮT";
        addLogEntry(`${action} ${deviceNames[deviceId]}`);  
        websocket.send(deviceId + (state ? "1" : "0"));
      }

      function updateUptime() {
        let startTime = Date.now();
        setInterval(() => {
          const elapsed = Date.now() - startTime;
          const hours = Math.floor(elapsed / (1000 * 60 * 60));
          const minutes = Math.floor((elapsed %% (1000 * 60 * 60)) / (1000 * 60));
          const seconds = Math.floor((elapsed %% (1000 * 60)) / 1000);

          document.getElementById("uptime").textContent = 
            `${hours.toString().padStart(2, "0")}:${minutes.toString().padStart(2, "0")}:${seconds.toString().padStart(2, "0")}`;
        }, 1000);
      }

      function updateWiFiSignal() {
        fetch("/wifiSignal")
          .then(response => response.text())
          .then(data => {
            const rssi = parseInt(data);
            let quality = "Yếu";
            let color = "#f44336";
            
            if (rssi > -50) {
              quality = "Xuất sắc";
              color = "#4caf50";
            } else if (rssi > -60) {
              quality = "Tốt";
              color = "#8bc34a";
            } else if (rssi > -70) {
              quality = "Khá";
              color = "#ffc107";
            }
            
            document.getElementById("wifiSignal").innerHTML = 
              `<span style="color: ${color}">${data} dBm (${quality})</span>`;
          })
          .catch(err => {
            console.error("Error fetching WiFi signal:", err);
          });
      }

      function updateProgressBar(elementId, value, maxValue = 100) {
        const progressBar = document.getElementById(elementId);
        const percentage = (value / maxValue) * 100;
        progressBar.style.width = percentage + '%%';
      }

      function addLogEntry(message) {
        const now = new Date();
        const timestamp = now.toLocaleTimeString("vi-VN");
        const logEntries = document.getElementById("logEntries");
        const entry = document.createElement("div");
        entry.className = "log-entry";
        entry.innerHTML = `<span class="timestamp">${timestamp}</span><span>${message}</span>`;
        logEntries.insertBefore(entry, logEntries.firstChild);

        if (logEntries.children.length > 50) {
          logEntries.removeChild(logEntries.lastChild);
        }
        logEntries.scrollTop = 0;
      }

      // Cập nhật dữ liệu cảm biến
      setInterval(function () {
        fetch("/temperature")
          .then(response => response.text())
          .then(data => {
            const temp = parseFloat(data);
            document.getElementById("temperature").innerHTML = 
              data + '<span class="sensor-unit">°C</span>';
            updateProgressBar('tempProgress', temp, 50);
          });
      }, updateInterval );

      setInterval(function () {
        fetch("/humidity")
          .then(response => response.text())
          .then(data => {
            const hum = parseFloat(data);
            document.getElementById("humidity").innerHTML = 
              data + '<span class="sensor-unit">%%</span>';
            updateProgressBar('humidityProgress', hum, 100);
          });
      }, updateInterval );

      setInterval(function () {
        fetch("/lightIntensity")
          .then(response => response.text())
          .then(data => {
            const light = parseFloat(data);
            document.getElementById("lightIntensity").innerHTML = 
              data + '<span class="sensor-unit">lux</span>';
            updateProgressBar('lightProgress', light, 30000);
          });
      }, updateInterval );

      setInterval(function () {
        fetch("/soilMoisture")
          .then(response => response.text())
          .then(data => {
            const soil = parseFloat(data);
            document.getElementById("soilMoisture").innerHTML = 
              data + '<span class="sensor-unit">%%</span>';
            updateProgressBar('soilProgress', soil, 100);
          });
      }, updateInterval );
    </script>
  </body>
</html>
)rawliteral";

void notifyClients(String message) {
  ws.textAll(message);
}

void handleWebSocketMessage(void* arg, uint8_t* data, size_t len) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "auto") == 0) {
      automatic = true;
    } else if (strcmp((char*)data, "manual") == 0) {
      automatic = false;
    } else {
      automatic = false;
      bool state = strstr((char*)data, "1");

      if (strstr((char*)data, "light")) {
        led = state;
      } else if (strstr((char*)data, "pump")) {
        pumper = state;
      } else if (strstr((char*)data, "fan")) {
        fan = state;
      }
    }
    notifyClients(String((char*)data));
  }
}

void onEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {

  switch (type) {
    case WS_EVT_CONNECT:
      amountUser++;
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      amountUser--;
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String& var) {
  if (var == "TEMPERATURE") {
    return String(t);
  } else if (var == "HUMIDITY") {
    return String(h);
  } else if (var == "SOIL") {
    return String(soil);
  } else if (var == "BRIGHTNESS") {
    return String(brightness);
  } else if (var == "DEVICEIP") {
    return WiFi.localIP().toString();
  } else if (var == "WIFISIGNAL") {
    return String(WiFi.RSSI());
  } else if (var == "AUTOMODE") {
    return automatic ? "true" : "false";
  } else if (var == "PUMPCHECK") {
    return pumper ? "checked" : "";
  } else if (var == "FANCHECK") {
    return fan ? "checked" : "";
  } else if (var == "LEDCHECK") {
    return led ? "checked" : "";
  } else if (var == "AUTOCHECK") {
    return automatic ? "checked" : "";
  }

  return String();
}

////////////////////////////////
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 8
#define btnPin 1
#define pumperPin 7
#define fanPin 6
#define ledPin 18
#define potenPin 2
#define soilPin 0

#define NUMDEVICES 3
#define EEPROM_SIZE 32
// 32 bytes
#define DHTTYPE DHT11

#define SENSOR_UPDATE_INTERVAL 5
#define FREQ 1000000

const char* ntpServer = "time.google.com";
const unsigned long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

DHT dht(DHTPIN, DHTTYPE);

BH1750 lightMeter;

const uint8_t bounceDuration = 200;
bool btnPressed = false,
     isSettings = false,
     isLoading = false;

ICACHE_RAM_ATTR void debounceBtn() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  if (interruptTime - lastInterruptTime >= bounceDuration) {
    if (isSettings) { btnPressed = true; }
    lastInterruptTime = interruptTime;
  }
  isSettings = true;
}

void dispLoading(uint8_t percent, int delayTime) {
  static const unsigned char PROGMEM image_Alert_bits[] = { 0x08, 0x00, 0x1c, 0x00, 0x14, 0x00, 0x36, 0x00, 0x36, 0x00, 0x7f, 0x00, 0x77, 0x00, 0xff, 0x80 };

  display.drawRect(9, 19, 104, 20, 1);

  display.fillRect(11, 21, percent, 16, 1);

  char buffer[16];

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(28, 43);
  snprintf(buffer, sizeof(buffer), "Progress: %hhu%%", percent);
  display.print(buffer);

  display.setCursor(13, 1);
  display.print(F("Please waiting..."));

  display.drawLine(0, 9, 126, 9, 1);

  display.drawBitmap(12, 42, image_Alert_bits, 9, 8, 1);

  display.display();
  delay(delayTime);
}

void saveToEEPROM(int address, float value) {
  EEPROM.write(address, value);
  EEPROM.commit();
}

float readFromEEPROM(int address) {
  return EEPROM.read(address);
}

volatile bool shouldUpdateSensors = false;
void IRAM_ATTR onTimer() {
  if (automatic) {
    shouldUpdateSensors = true;
    return;
  }
  shouldUpdateSensors = false;
}
hw_timer_t* myTimer = NULL;

void initTimerInterrupt() {
  myTimer = timerBegin(FREQ);
  timerAttachInterrupt(myTimer, &onTimer);
  timerAlarm(myTimer, SENSOR_UPDATE_INTERVAL * FREQ, true, 0);
}

void initRoutes() {
  // Route chính
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/ws", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (ws.canHandle(request)) {
      ws.handleRequest(request);
    } else {
      request->send(404);
    }
  });

  // Routes cho sensor data
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(t).c_str());
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(h).c_str());
  });

  server.on("/lightIntensity", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(brightness).c_str());
  });

  server.on("/soilMoisture", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(soil).c_str());
  });

  server.on("/wifiSignal", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", String(WiFi.RSSI()).c_str());
  });
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println(F("IP configure fail!"));
  }

  Serial.print(F("Connecting to WiFi "));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(1000);
  }

  Serial.println(F(""));
  Serial.println("IP address: " + WiFi.localIP().toString());
}

void initDNS() {
  if (!MDNS.begin(hostname)) {
    Serial.println(F("Setting up MDNS error!"));
    while (1) {
      delay(1000);
    }
  }
  Serial.println("MDNS response start at: http://" + String(hostname) + ".local");
}

void initPin() {
  pinMode(btnPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(pumperPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(potenPin, INPUT);
  pinMode(soilPin, INPUT);
}
//////////////////////////////////////////////////////////////////////
esp_qrcode_handle_t qrcode_handle = NULL;
esp_qrcode_config_t qr_config;

void display_qrcode(esp_qrcode_handle_t qrcode) {
  qrcode_handle = qrcode;

  display.clearDisplay();

  // Lấy kích thước QR code
  int qr_size = esp_qrcode_get_size(qrcode);

  // Tính toán scale để vừa màn hình
  int scale = min(SCREEN_WIDTH, SCREEN_HEIGHT) / qr_size;
  if (scale < 1) scale = 1;

  // Căn giữa QR code
  int offsetX = (SCREEN_WIDTH - (qr_size * scale)) / 2;
  int offsetY = (SCREEN_HEIGHT - (qr_size * scale)) / 2;

  // Vẽ QR code
  for (int y = 0; y < qr_size; y++) {
    for (int x = 0; x < qr_size; x++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        display.fillRect(
          offsetX + (x * scale),
          offsetY + (y * scale),
          scale,
          scale,
          SSD1306_WHITE);
      }
    }
  }

  display.display();
}

// Hàm khởi tạo QR code system
void initQRCode() {
  // Thiết lập cấu hình QR code một lần
  qr_config = ESP_QRCODE_CONFIG_DEFAULT();
  qr_config.display_func = display_qrcode;
  qr_config.max_qrcode_version = 10;
  qr_config.qrcode_ecc_level = ESP_QRCODE_ECC_LOW;

  Serial.println(F("QR Code system đã sẵn sàng!"));
}

// Hàm để cập nhật và hiển thị QR code mới
void updateQRCode(const char* text) {

  esp_err_t err = esp_qrcode_generate(&qr_config, text);

  if (err != ESP_OK) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(1);
    display.setCursor(8, 29);
    display.println(F("Error generating QR"));
    display.display();
  }
}

/////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(4, 5);

  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  Serial.println(F("BH1750 begin"));

  EEPROM.begin(EEPROM_SIZE);

  float val1 = readFromEEPROM(0);
  float val2 = readFromEEPROM(1);
  if (val1 != 255) {
    limTemp = val1;
  }
  if (val2 != 255) {
    limSoil = val2;
  }

  initWiFi();
  initDNS();

  initWebSocket();
  initRoutes();
  server.begin();

  dht.begin();
  t = dht.readTemperature();
  h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    t = 0;
    h = 0;
  }

  initPin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  initQRCode();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000);

  for (uint8_t i = 0; i <= 100; i++) {
    dispLoading(i, 50);
    display.clearDisplay();
  }

  attachInterrupt(digitalPinToInterrupt(btnPin), debounceBtn, FALLING);
  initTimerInterrupt();
}


// indent: 255-> center, otherwise: indent
void alignText(uint8_t y, String text, uint8_t indent = 255, uint8_t textSize = 1, bool textColour = 1) {
  uint8_t totalWidth = (textSize == 1) ? 6 * text.length() - 1 : 13 * text.length() - 3;
  uint8_t x;

  if (indent != 255) {
    x = indent;
  } else {
    x = (SCREEN_WIDTH - totalWidth) / 2;
  }

  display.setTextSize(textSize);
  display.setTextWrap(false);
  display.setTextColor(textColour);
  display.setCursor(x, y);
  display.print(text);
}

void updateData() {
  float tempReading = dht.readTemperature();
  float humidityReading = dht.readHumidity();

  if (!isnan(tempReading)) {
    t = tempReading;
  } else {
    Serial.println(F("Lỗi đọc nhiệt độ từ DHT11"));
  }

  if (!isnan(humidityReading)) {
    h = humidityReading;
  } else {
    Serial.println(F("Lỗi đọc độ ẩm từ DHT11"));
  }

  float lightReading = lightMeter.readLightLevel();

  if (!isnan(lightReading) && lightReading >= 0) {
    brightness = lightReading;
  } else {
    Serial.println(F("Lỗi đọc cảm biến ánh sáng BH1750"));
  }

  soil = 100 - ((float)analogRead(soilPin)) * 100 / 4096;
}

#define MAX_ENV_BRIGHTNESS 20000

void updateDeviceState() {
  int dutyCycle;
  if (automatic) {
    t > limTemp ? fan = true : fan = false;
    soil < limSoil ? pumper = true : pumper = false;
    float ratio = (MAX_ENV_BRIGHTNESS - brightness) / MAX_ENV_BRIGHTNESS;
    dutyCycle = (int)(ratio * 255.0);
  }

  digitalWrite(fanPin, fan);
  digitalWrite(pumperPin, pumper);
  analogWrite(ledPin, automatic ? dutyCycle : led * 255);
}

void overview() {
  static unsigned long prevTime = millis();
  unsigned long currentTime = millis();

  static char timeString[10];
  static char dateString[20];
  struct tm timeInfo;

  if (currentTime - prevTime >= SENSOR_UPDATE_INTERVAL * 1000) {
    if (!getLocalTime(&timeInfo)) {
      return;
    }
    snprintf(dateString, sizeof(dateString), "%02d/%02d/%04d", timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
    snprintf(timeString, sizeof(timeString), "%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min);
    prevTime = currentTime;
  }

  static const unsigned char PROGMEM image_download_bits[] = { 0x01, 0x00, 0x21, 0x08, 0x10, 0x10, 0x03, 0x80, 0x8c, 0x62, 0x48, 0x24, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x48, 0x24, 0x8c, 0x62, 0x03, 0x80, 0x10, 0x10, 0x21, 0x08, 0x01, 0x00, 0x00, 0x00 };

  static const unsigned char PROGMEM image_tree_bits[] = { 0x05, 0x40, 0x2a, 0xd4, 0x57, 0x26, 0x31, 0xb8, 0x5d, 0xea, 0x87, 0x95, 0x6b, 0x36, 0xbb, 0x68, 0x4f, 0xfd, 0x37, 0xc6, 0x51, 0xa9, 0x29, 0x82, 0x01, 0x80, 0x01, 0x80, 0x03, 0xc0, 0x0f, 0xf0 };

  static const unsigned char PROGMEM image_weather_humidity_white_bits[] = { 0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x12, 0x00, 0x11, 0x00, 0x20, 0x80, 0x20, 0x80, 0x41, 0x40, 0x40, 0xc0, 0x80, 0xa0, 0x80, 0x20, 0x40, 0x40, 0x40, 0x40, 0x30, 0x80, 0x0f, 0x00 };

  static const unsigned char PROGMEM image_weather_temperature_bits[] = { 0x1c, 0x00, 0x22, 0x02, 0x2b, 0x05, 0x2a, 0x02, 0x2b, 0x38, 0x2a, 0x60, 0x2b, 0x40, 0x2a, 0x40, 0x2a, 0x60, 0x49, 0x38, 0x9c, 0x80, 0xae, 0x80, 0xbe, 0x80, 0x9c, 0x80, 0x41, 0x00, 0x3e, 0x00 };


  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(1, 3);
  display.print(dateString);

  display.drawLine(0, 12, 127, 12, 1);

  display.setCursor(74, 3);
  display.print(timeString);

  display.drawLine(63, 12, 63, 63, 1);

  display.drawBitmap(2, 45, image_weather_humidity_white_bits, 11, 16, 1);

  display.drawBitmap(2, 17, image_weather_temperature_bits, 16, 16, 1);

  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%.1f C", t);
  display.setCursor(22, 22);
  display.print(buffer);

  display.drawBitmap(66, 46, image_download_bits, 15, 16, 1);

  display.drawBitmap(66, 17, image_tree_bits, 16, 16, 1);

  snprintf(buffer, sizeof(buffer), "%.1f %%", h);
  display.setCursor(22, 51);
  display.print(buffer);

  snprintf(buffer, sizeof(buffer), "%.1f %%", soil);
  display.setCursor(93, 23);
  display.print(buffer);

  snprintf(buffer, sizeof(buffer), "%.1f lx", brightness);
  display.setCursor(87, 51);
  display.print(buffer);

  display.display();
}

uint8_t numSelected(uint8_t numOptions, uint8_t offset = 0) {
  float val = analogRead(potenPin);
  uint8_t num = floor((val * numOptions / 4096));
  if (num == numOptions) { --num; }
  return (num + offset);
}

uint8_t selectedOption = 255;

uint8_t popup(String header) {
  bool ans = numSelected(2);

  display.drawRect(0, 15, 128, 34, 1);

  alignText(17, header);

  if (ans) {
    display.drawRect(21, 33, 39, 13, 1);
    display.fillRect(69, 33, 39, 13, 1);
  } else {
    display.fillRect(21, 33, 39, 13, 1);
    display.drawRect(69, 33, 39, 13, 1);
  }

  alignText(36, "Yes", 80, 1, !ans);
  alignText(36, "No", 35, 1, ans);

  display.display();

  if (btnPressed) {
    btnPressed = false;
    return ans;
  }
  return 255;
}

bool isSleep = false;

bool savedWindow = false;

void setVal(String header, int start, int stop, char unit, float* target) {
  static bool savedTemp = false;
  static uint8_t temp;

  uint8_t hover;
  static uint8_t numPressed = 0;
  bool item1 = 1, item2 = 0, item3 = 0;
  static bool focus = true;

  if (savedTemp) {
    savedTemp = false;
    selectedOption = 255;  //Get out of this option forever
    *target = temp;
    return;
  }

  if (savedWindow) {
    uint8_t ans = popup("Save ?");
    if (ans == 1) {
      savedTemp = true;
      savedWindow = false;
      numPressed = 0;
      if (target == &limTemp) {
        saveToEEPROM(0, temp);
      } else if (target == &limSoil) {
        saveToEEPROM(1, temp);
      }
    } else if (ans == 0) {
      savedTemp = false;
      savedWindow = false;
      numPressed = 1;
    }
    return;
  }

  if (btnPressed) {
    ++numPressed;
    btnPressed = false;
  }

  if (focus) {
    temp = numSelected(stop - start + 1, start);
  } else {
    hover = numSelected(3);
  }

  alignText(2, header);
  display.drawLine(0, 11, 127, 11, 1);

  if (unit == 'C') {
    display.drawCircle(72, 23, 2, 1);
    alignText(25, String(temp) + " C", 41, 2);
  } else {
    alignText(25, String(temp) + " " + unit, 41, 2);
  }

  static const unsigned char PROGMEM image_crosshairs_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x15, 0x00, 0x24, 0x80, 0xfb, 0xe0, 0x24, 0x80, 0x15, 0x00, 0x0e, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  // 0: focus
  if (numPressed == 0) {
    focus = true;
    display.fillRect(115, 16, 13, 13, 1);
    display.drawRoundRect(0, 48, 63, 16, 7, 1);
    display.drawRoundRect(65, 48, 63, 16, 7, 1);
  } else if (numPressed == 1) {
    // 1: exit focus and go to hover
    focus = false;
    switch (hover) {
      case 0:
        item1 = 1, item2 = 0, item3 = 0;
        display.fillRect(115, 16, 13, 13, 1);
        display.drawRoundRect(0, 48, 63, 16, 7, 1);
        display.drawRoundRect(65, 48, 63, 16, 7, 1);
        break;
      case 1:
        item1 = 0, item2 = 1, item3 = 0;
        display.fillRoundRect(0, 48, 63, 16, 7, 1);
        display.drawRoundRect(65, 48, 63, 16, 7, 1);
        break;
      case 2:
        item1 = 0, item2 = 0, item3 = 1;
        display.drawRoundRect(0, 48, 63, 16, 7, 1);
        display.fillRoundRect(65, 48, 63, 16, 7, 1);
        break;
      default:
        break;
    }
  } else if (numPressed == 2) {
    numPressed = 0;
    switch (hover) {
      case 0:
        focus = true;
        break;
      case 1:
        selectedOption = 255;
        break;
      case 2:
        savedWindow = true;
        return;
      default:
        break;
    }
  }

  display.drawBitmap(116, 15, image_crosshairs_bits, 11, 16, !item1);
  alignText(52, "Back", 21, 1, !item2);
  alignText(52, "Save", 85, 1, !item3);

  display.display();
}

void setTemp() {
  setVal("Temperature limit", 28, 50, 'C', &limTemp);
}

void setSoil() {
  setVal("Soil moisture", 0, 100, '%', &limSoil);
}

void setSleep() {
  isSleep = !isSleep;
  selectedOption = 255;
  btnPressed = false;
}

void dispMenu(String options[], const unsigned char* const image_arrays[], void (*function[])(void), uint8_t numOptions, uint8_t* placeHolder, bool checkBox = false, bool checkedPosition[NUMDEVICES + 1] = {}) {

  static const unsigned char PROGMEM image_choice_bullet_off_bits[] = { 0x07, 0xc0, 0x1c, 0x70, 0x30, 0x18, 0x60, 0x0c, 0x40, 0x04, 0xc0, 0x06, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0xc0, 0x06, 0x40, 0x04, 0x60, 0x0c, 0x30, 0x18, 0x1c, 0x70, 0x07, 0xc0, 0x00, 0x00 };

  uint8_t hover = numSelected(numOptions);

  if (*placeHolder != 255) {
    function[*placeHolder]();
    return;
  }

  if (btnPressed) {
    *placeHolder = hover;
    btnPressed = false;
  }

  uint8_t y = 0;
  const uint8_t dispAmount = 3;
  uint8_t start = hover - hover % dispAmount;

  for (uint8_t i = start; i < start + dispAmount && i < numOptions; i++) {
    if (i == hover) {
      if (checkBox) {
        if (hover == numOptions - 2) {
          display.fillRoundRect(0, y, 63, 20, 7, 1);
          alignText(y + 6, options[i], 20, 1, 0);
          continue;
        } else if (hover == numOptions - 1) {
          display.fillRoundRect(65, y, 63, 20, 7, 1);
          alignText(y + 6, options[i], 78, 1, 0);
          continue;
        }
      }
      display.drawRect(0, y, 128, 20, WHITE);  // 20 = 2+16+2
    }

    if (checkBox) {
      if (i == numOptions - 2 && hover != i) {
        display.drawRoundRect(0, y, 63, 20, 7, 1);
        alignText(y + 6, options[i], 20, 1, 1);
        continue;
      } else if (i == numOptions - 1 && hover != i) {
        display.drawRoundRect(65, y, 63, 20, 7, 1);
        alignText(y + 6, options[i], 78, 1, 1);
        continue;
      } else {
        display.drawBitmap(2, y + 2, image_choice_bullet_off_bits, 16, 16, 1);
        display.fillCircle(9, y + 9, 5, checkedPosition[i]);
        alignText(y + 6, options[i], 30);
      }
    } else {
      display.drawBitmap(2, y + 2, image_arrays[i], 16, 16, 1);
      alignText(y + 6, options[i], 30);
    }
    y += 21;
  }
  display.display();
}

uint8_t manualSelectedOption = 255;

void setPumper() {
  pumper = !pumper;
  manualSelectedOption = 255;
};

void setFan() {
  fan = !fan;
  manualSelectedOption = 255;
};

void setLed() {
  led = !led;
  manualSelectedOption = 255;
};

void exitManual() {
  pumper = false;
  fan = false;
  led = false;
  manualSelectedOption = 255;
  selectedOption = 255;
  automatic = true;
}

void manual() {
  automatic = false;

  static const unsigned char PROGMEM image_display_brightness_bits[] = { 0x01, 0x00, 0x21, 0x08, 0x10, 0x10, 0x03, 0x80, 0x8c, 0x62, 0x48, 0x24, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x48, 0x24, 0x8c, 0x62, 0x03, 0x80, 0x10, 0x10, 0x21, 0x08, 0x01, 0x00, 0x00, 0x00 };

  static const unsigned char PROGMEM image_weather_humidity_bits[] = { 0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0e, 0x00, 0x1e, 0x00, 0x1f, 0x00, 0x3f, 0x80, 0x3f, 0x80, 0x7e, 0xc0, 0x7f, 0x40, 0xff, 0x60, 0xff, 0xe0, 0x7f, 0xc0, 0x7f, 0xc0, 0x3f, 0x80, 0x0f, 0x00 };

  static const unsigned char PROGMEM image_weather_wind_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x03, 0x88, 0x04, 0x44, 0x04, 0x44, 0x00, 0x44, 0x00, 0x88, 0xff, 0x32, 0x00, 0x00, 0xad, 0x82, 0x00, 0x60, 0x00, 0x10, 0x00, 0x10, 0x01, 0x20, 0x00, 0xc0 };

  static const unsigned char PROGMEM image_crossed_bits[] = { 0x00, 0x00, 0x00, 0x00, 0xc0, 0x60, 0xe0, 0xe0, 0x71, 0xc0, 0x3b, 0x80, 0x1f, 0x00, 0x0e, 0x00, 0x1f, 0x00, 0x3b, 0x80, 0x71, 0xc0, 0xe0, 0xe0, 0xc0, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  static const unsigned char* const image_arrays[] = {
    image_weather_humidity_bits,
    image_weather_wind_bits,
    image_display_brightness_bits,
    image_crossed_bits
  };

  String options[] = {
    "Pumper: " + String(pumper ? "ON" : "OFF"),
    "FAN: " + String(fan ? "ON" : "OFF"),
    "LED: " + String(led ? "ON" : "OFF"),
    "Exit"
  };

  void (*function[4])(void) = { setPumper, setFan, setLed, exitManual };
  dispMenu(options, image_arrays, function, 4, &manualSelectedOption);
}
///////////////////////////////////////
uint8_t diagnoseSelectedOption = 255;
bool checkList[NUMDEVICES + 1] = {};

void isCheckAll() {
  checkList[0] = !checkList[0];
  for (int i = 1; i <= NUMDEVICES; i++) {
    checkList[i] = checkList[0];
  }
  diagnoseSelectedOption = 255;
}

void isCheckPumper() {
  checkList[1] = !checkList[1];
  if (!checkList[1]) {
    checkList[0] = false;
  }
  diagnoseSelectedOption = 255;
}

void isCheckFan() {
  checkList[2] = !checkList[2];
  if (!checkList[2]) {
    checkList[0] = false;
  }
  diagnoseSelectedOption = 255;
}

void isCheckLed() {
  checkList[3] = !checkList[3];
  if (!checkList[3]) {
    checkList[0] = false;
  }
  diagnoseSelectedOption = 255;
}

void isBack() {
  diagnoseSelectedOption = 255;
  selectedOption = 255;
}

void checkedCompleted(uint8_t* currentDevice, bool* stillWorking, const char** deviceNames, bool* firstTime) {
  pumper = false, led = false, fan = false;
  static const unsigned char PROGMEM image_choice_right_bits[] = { 0x03, 0xc0, 0x0c, 0x30, 0x11, 0x88, 0x26, 0x64, 0x48, 0x12, 0x50, 0x0a, 0x90, 0x29, 0xa4, 0x45, 0xa2, 0x85, 0x91, 0x09, 0x50, 0x0a, 0x48, 0x12, 0x26, 0x64, 0x11, 0x88, 0x0c, 0x30, 0x03, 0xc0 };

  static const unsigned char PROGMEM image_choice_wrong_bits[] = { 0x0f, 0xe0, 0x10, 0x10, 0x27, 0xc8, 0x48, 0x24, 0x90, 0x12, 0xa4, 0x4a, 0xa2, 0x8a, 0xa1, 0x0a, 0xa2, 0x8a, 0xa4, 0x4a, 0x90, 0x12, 0x48, 0x24, 0x27, 0xc8, 0x10, 0x10, 0x0f, 0xe0, 0x00, 0x00 };

  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(11, 0);
  display.print("Diagnostic results");
  display.drawLine(0, 9, 127, 9, 1);

  uint8_t x = 1, y = 15;
  for (int i = 0; i < NUMDEVICES; i++) {
    if (!checkList[i + 1]) { continue; }

    display.drawBitmap(x, y, stillWorking[i] ? image_choice_right_bits : image_choice_wrong_bits, 16, 16, 1);
    display.setCursor(x + 20, y + 5);
    display.print(deviceNames[i]);

    if (x == 1 && y == 15) {
      x = 64;
    } else if (x == 64 && y == 15) {
      x = 1, y = 40;
    }
  }

  display.display();
  //////////////////////
  if (btnPressed) {
    btnPressed = false;
    *currentDevice = 0;
    diagnoseSelectedOption = 255;
    memset(checkList, false, sizeof(checkList));
    memset(stillWorking, false, sizeof(stillWorking));
    selectedOption = 255;
    *firstTime = true;
  }
}

#define CHECK_DURATION 5000

void isCheckup() {
  static unsigned long prevTime = millis();
  static bool firstTime = true;
  if (firstTime) {
    prevTime = millis();
    firstTime = false;
  }

  unsigned long currentTime = millis();

  static uint8_t currentDevice = 0;
  const char* deviceNames[NUMDEVICES] = { "Pumper", "Fan", "Led" };
  static bool stillWorking[NUMDEVICES] = {};
  static bool confirmPopup = false;

  char buffer[20];
  if (currentDevice != NUMDEVICES) {
    if (currentTime - prevTime >= CHECK_DURATION) {
      pumper = false, led = false, fan = false;

      if (confirmPopup) {
        confirmPopup = false;
        prevTime = currentTime;
      } else {
        display.clearDisplay();
        snprintf(buffer, sizeof(buffer), "%s working ?", deviceNames[currentDevice]);
        uint8_t ans = popup(buffer);

        if (ans != 255) {
          stillWorking[currentDevice] = ans;
          currentDevice++;
          confirmPopup = true;
        }
      }
    } else {
      if (!checkList[currentDevice + 1]) {
        currentDevice++;
        return;
      }

      display.clearDisplay();

      snprintf(buffer, sizeof(buffer), "%s turning on...", deviceNames[currentDevice]);
      alignText(29, buffer);
      display.display();

      pumper = (currentDevice == 0);
      fan = (currentDevice == 1);
      led = (currentDevice == 2);
    }
  } else {
    checkedCompleted(&currentDevice, stillWorking, deviceNames, &firstTime);
  }
}

void diagnose() {
  automatic = false;

  String options[] = {
    "All",
    "Pumper",
    "Fan",
    "Led",
    "Back",
    "Checkup"
  };

  void (*function[6])(void) = { isCheckAll, isCheckPumper, isCheckFan, isCheckLed, isBack, isCheckup };

  dispMenu(options, NULL, function, 6, &diagnoseSelectedOption, true, checkList);
}

void about() {
  String qrContent = "=== HE THONG CHAM SOC CAY ===\n";
  qrContent += "ID: ESP32-" + String((uint32_t)ESP.getEfuseMac(), HEX).substring(0, 6) + "\n";
  qrContent += "Model: ESP32-C3\n";
  qrContent += "Firmware: v1.2.3\n";
  qrContent += "IP: " + WiFi.localIP().toString() + "\n";
  qrContent += ("Web: http://" + String(hostname) + ".local\n");
  qrContent += "Hotline: 0339507429";

  updateQRCode(qrContent.c_str());

  if (btnPressed) {
    selectedOption = 255;
    btnPressed = false;
  }
}

void exit() {
  isSettings = false;
  btnPressed = false;
  selectedOption = 255;
}

void loop() {
  display.clearDisplay();

  if (shouldUpdateSensors) {
    updateData();
    shouldUpdateSensors = false;
  }
  updateDeviceState();

  if (amountUser != 0) {
    display.Adafruit_SSD1306::ssd1306_command(SSD1306_DISPLAYOFF);
    return;
  }
  display.Adafruit_SSD1306::ssd1306_command(SSD1306_DISPLAYON);

  if (!isSettings) {
    overview();
  } else {
    static const unsigned char PROGMEM image_device_sleep_mode_white_bits[] = { 0x04, 0x00, 0x1c, 0x0e, 0x28, 0x02, 0x48, 0x04, 0x51, 0xee, 0x90, 0x40, 0x90, 0x80, 0x91, 0xe0, 0x88, 0x00, 0x88, 0x06, 0x46, 0x1c, 0x41, 0xe4, 0x20, 0x08, 0x18, 0x30, 0x07, 0xc0, 0x00, 0x00 };

    static const unsigned char PROGMEM image_Temperature_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x04, 0x40, 0x07, 0xc0, 0x07, 0xc0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_weather_humidity_white_bits[] = { 0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x12, 0x00, 0x11, 0x00, 0x20, 0x80, 0x20, 0x80, 0x41, 0x40, 0x40, 0xc0, 0x80, 0xa0, 0x80, 0x20, 0x40, 0x40, 0x40, 0x40, 0x30, 0x80, 0x0f, 0x00 };

    static const unsigned char PROGMEM image_crossed_bits[] = { 0x00, 0x00, 0x00, 0x00, 0xc0, 0x60, 0xe0, 0xe0, 0x71, 0xc0, 0x3b, 0x80, 0x1f, 0x00, 0x0e, 0x00, 0x1f, 0x00, 0x3b, 0x80, 0x71, 0xc0, 0xe0, 0xe0, 0xc0, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_menu_tool_wrench_bits[] = { 0x00, 0x00, 0x00, 0xe0, 0x01, 0x60, 0x02, 0x80, 0x02, 0x8c, 0x03, 0x0c, 0x02, 0xb4, 0x02, 0x48, 0x05, 0xf0, 0x0a, 0x00, 0x14, 0x00, 0x28, 0x00, 0x50, 0x00, 0xa0, 0x00, 0xc0, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_menu_settings_sliders_bits[] = { 0x38, 0x00, 0x44, 0x00, 0xc7, 0xfc, 0x44, 0x00, 0x38, 0x00, 0x00, 0x70, 0x00, 0x88, 0xff, 0x8c, 0x00, 0x88, 0x00, 0x70, 0x38, 0x00, 0x44, 0x00, 0xc7, 0xfc, 0x44, 0x00, 0x38, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_menu_information_sign_white_bits[] = { 0x07, 0xc0, 0x18, 0x30, 0x23, 0x08, 0x42, 0x84, 0x43, 0x04, 0x80, 0x02, 0x83, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x42, 0x84, 0x43, 0x84, 0x20, 0x08, 0x18, 0x30, 0x07, 0xc0, 0x00, 0x00 };

    static const unsigned char* const image_arrays[] = {
      image_Temperature_bits,
      image_weather_humidity_white_bits,
      image_device_sleep_mode_white_bits,
      image_menu_settings_sliders_bits,
      image_menu_tool_wrench_bits,
      image_menu_information_sign_white_bits,
      image_crossed_bits
    };

    String options[] = {
      "Temp: " + String(limTemp) + "C",
      "Moisture: " + String(limSoil) + "%",
      "Sleep: " + String(isSleep ? "ON" : "OFF"),
      "Manual",
      "System diagnose",
      "About",
      "Exit"
    };

    void (*function[7])(void) = { setTemp, setSoil, setSleep, manual, diagnose, about, exit };

    dispMenu(options, image_arrays, function, 7, &selectedOption);
  }
}
