#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <BH1750.h>
#include <DHT.h>
#include <string>

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

bool pumper = false, fan = false, led = false;
float t = 0, h = 0, brightness = 0, soil = 0;
bool automatic = true;
uint8_t amountUser = 0;

float limTemp = 30;
float limSoil = 50;

unsigned long previousMillis = 0;  // will store last time DHT was updated
const long interval = 10000;       // Updates DHT readings every 10 seconds

const char* ssid = "C427";
const char* password = "64546743";

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
            updateProgressBar('lightProgress', light, 65535);
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

#define DHTPIN 14
#define btnPin 2
#define pumperPin 12
#define fanPin 13
#define ledPin 15

#define NUMDEVICES 3
#define EEPROM_SIZE 32
// 32 bytes

// Uncomment the type of sensor in use:
#define DHTTYPE DHT11  // DHT 11

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
  display.print("Please waiting...");

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

void setup() {
  Serial.begin(115200);

  Wire.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
  Serial.println("BH1750 begin");

  EEPROM.begin(EEPROM_SIZE);

  float val1 = readFromEEPROM(0);
  float val2 = readFromEEPROM(1);
  if (val1 != 255) {
    limTemp = val1;
  }
  if (val2 != 255) {
    limSoil = val2;
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println(WiFi.localIP());

  // Khởi tạo WebSocket
  initWebSocket();

  // Route chính
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
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

  server.begin();

  dht.begin();
  t = dht.readTemperature();
  h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    t = 0;
    h = 0;
  }

  pinMode(btnPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPin), debounceBtn, FALLING);

  pinMode(ledPin, OUTPUT);
  pinMode(pumperPin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);

  for (uint8_t i = 0; i <= 100; i++) {
    dispLoading(i, 50);
    display.clearDisplay();
  }
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

#define SENSOR_UPDATE_INTERVAL 5000
void updateData() {
  static unsigned long lastUpdate = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastUpdate < SENSOR_UPDATE_INTERVAL) {
    return;
  }

  lastUpdate = currentTime;

  float tempReading = dht.readTemperature();
  float humidityReading = dht.readHumidity();

  if (!isnan(tempReading)) {
    t = tempReading;
  } else {
    Serial.println("Lỗi đọc nhiệt độ từ DHT11");
  }

  if (!isnan(humidityReading)) {
    h = humidityReading;
  } else {
    Serial.println("Lỗi đọc độ ẩm từ DHT11");
  }

  float lightReading = lightMeter.readLightLevel();

  if (!isnan(lightReading) && lightReading >= 0) {
    brightness = lightReading;
  } else {
    Serial.println("Lỗi đọc cảm biến ánh sáng BH1750");
  }
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

void dispTempHumi(bool detail = false) {

  display.clearDisplay();

  // display temperature
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");

  // display humidity
  if (detail) {
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Humidity: ");
    display.setTextSize(2);
    display.setCursor(0, 45);
    display.print(h);
    display.print(" %");
  }
  display.display();
}

uint8_t numSelected(uint8_t numOptions, uint8_t offset = 0) {
  float val = analogRead(A0);
  uint8_t num = floor((val * numOptions / 1023));
  if (num == numOptions) { --num; }
  return (num + offset);
}

uint8_t selectedOption = 255;

uint8_t popup(String header) {
  bool ans = numSelected(2);

  display.drawRect(17, 15, 94, 34, 1);

  //49,17
  alignText(17, header + " ?");

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
    uint8_t ans = popup("Save");
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

void dispMenu(String options[], const unsigned char* const image_arrays[], void (*function[])(void), uint8_t numOptions, uint8_t* placeHolder) {

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
      display.drawRect(0, y, 128, 20, WHITE);  // 20 = 2+16+2
    }
    alignText(y + 6, options[i], 30);
    display.drawBitmap(2, y + 2, image_arrays[i], 16, 16, 1);

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

void diagnose() {
  static unsigned long prevTime = millis();
  unsigned long currentTime = millis();
  static uint8_t currentDevice = 1;
  String deviceNames[] = { "LED", "FAN", "PUMPER" };

  if (currentTime - prevTime >= 5000) {
    display.clearDisplay();
    uint8_t ans = popup(deviceNames[currentDevice] + " working ?");
    currentDevice == NUMDEVICES ? currentDevice = 1 : currentDevice++;
  }

  selectedOption = 255;
}

void exit() {
  isSettings = false;
  btnPressed = false;
  selectedOption = 255;
}

void loop() {
  Serial.println(automatic);

  display.clearDisplay();
  updateData();
  updateDeviceState();
  
  if (amountUser != 0) {
    Serial.println(amountUser);
    display.Adafruit_SSD1306::ssd1306_command(SSD1306_DISPLAYOFF);
    return;
  }
  display.Adafruit_SSD1306::ssd1306_command(SSD1306_DISPLAYON);

  if (!isSettings) {
    dispTempHumi(false);
  } else {
    static const unsigned char PROGMEM image_device_sleep_mode_white_bits[] = { 0x04, 0x00, 0x1c, 0x0e, 0x28, 0x02, 0x48, 0x04, 0x51, 0xee, 0x90, 0x40, 0x90, 0x80, 0x91, 0xe0, 0x88, 0x00, 0x88, 0x06, 0x46, 0x1c, 0x41, 0xe4, 0x20, 0x08, 0x18, 0x30, 0x07, 0xc0, 0x00, 0x00 };

    static const unsigned char PROGMEM image_Temperature_bits[] = { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x02, 0x80, 0x04, 0x40, 0x07, 0xc0, 0x07, 0xc0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_weather_humidity_white_bits[] = { 0x04, 0x00, 0x04, 0x00, 0x0c, 0x00, 0x0a, 0x00, 0x12, 0x00, 0x11, 0x00, 0x20, 0x80, 0x20, 0x80, 0x41, 0x40, 0x40, 0xc0, 0x80, 0xa0, 0x80, 0x20, 0x40, 0x40, 0x40, 0x40, 0x30, 0x80, 0x0f, 0x00 };

    static const unsigned char PROGMEM image_crossed_bits[] = { 0x00, 0x00, 0x00, 0x00, 0xc0, 0x60, 0xe0, 0xe0, 0x71, 0xc0, 0x3b, 0x80, 0x1f, 0x00, 0x0e, 0x00, 0x1f, 0x00, 0x3b, 0x80, 0x71, 0xc0, 0xe0, 0xe0, 0xc0, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_menu_tool_wrench_bits[] = { 0x00, 0x00, 0x00, 0xe0, 0x01, 0x60, 0x02, 0x80, 0x02, 0x8c, 0x03, 0x0c, 0x02, 0xb4, 0x02, 0x48, 0x05, 0xf0, 0x0a, 0x00, 0x14, 0x00, 0x28, 0x00, 0x50, 0x00, 0xa0, 0x00, 0xc0, 0x00, 0x00, 0x00 };

    static const unsigned char PROGMEM image_menu_settings_sliders_bits[] = { 0x38, 0x00, 0x44, 0x00, 0xc7, 0xfc, 0x44, 0x00, 0x38, 0x00, 0x00, 0x70, 0x00, 0x88, 0xff, 0x8c, 0x00, 0x88, 0x00, 0x70, 0x38, 0x00, 0x44, 0x00, 0xc7, 0xfc, 0x44, 0x00, 0x38, 0x00, 0x00, 0x00 };

    static const unsigned char* const image_arrays[] = {
      image_Temperature_bits,
      image_weather_humidity_white_bits,
      image_device_sleep_mode_white_bits,
      image_menu_settings_sliders_bits,
      image_menu_tool_wrench_bits,
      image_crossed_bits
    };

    String options[] = {
      "Temp: " + String(limTemp) + "C",
      "Moisture: " + String(limSoil) + "%",
      "Sleep: " + String(isSleep ? "ON" : "OFF"),
      "Manual",
      "System diagnose",
      "Exit"
    };

    void (*function[6])(void) = { setTemp, setSoil, setSleep, manual, diagnose, exit };

    dispMenu(options, image_arrays, function, 6, &selectedOption);
  }
}
