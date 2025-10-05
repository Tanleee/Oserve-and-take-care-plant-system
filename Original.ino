#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 14  // Digital pin connected to the DHT sensor
#define btnPin 2
#define pumperPin 12
#define fanPin 13
#define ledPin 15

// Uncomment the type of sensor in use:
#define DHTTYPE DHT11  // DHT 11
// #define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

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

void setup() {
  Serial.begin(115200);
  
  pinMode(btnPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(btnPin), debounceBtn, FALLING);

  pinMode(ledPin, OUTPUT);
  pinMode(pumperPin, OUTPUT);
  pinMode(fanPin, OUTPUT);

  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  //lowercase: (5,5)  | uppercase: (5,7)
  // space between each letter 1 pixel
}

const uint16_t duration = 5000;

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

void dispTempHumi(float* t = nullptr, float* h = nullptr, bool detail = false) {
  static unsigned long prevTime = 0;
  unsigned long currentTime = millis();

  static float tempC = -1, humidity = -1;
  if (currentTime - prevTime >= duration) {
    tempC = dht.readTemperature();
    humidity = dht.readHumidity();
    prevTime = currentTime;
  }

  if (tempC == -1 && humidity == -1) {
    for (uint8_t i = 0; i <= 100; i++) {
      dispLoading(i, 50);
      display.clearDisplay();
    }
  }

  if (isnan(tempC) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  if (h != nullptr) { *h = humidity; }
  if (t != nullptr) { *t = tempC; }
  // clear display
  display.clearDisplay();

  // display temperature
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print(tempC);
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
    display.print(humidity);
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

uint8_t save() {
  bool ans = numSelected(2);

  display.drawRect(17, 15, 94, 34, 1);

  display.setTextSize(1);
  display.setTextColor(1);
  display.setTextWrap(false);
  display.setCursor(49, 17);
  display.print("Save ?");

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

float limTemp = 30;
float limSoil = 50;
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
    uint8_t ans = save();
    if (ans == 1) {
      savedTemp = true;
      savedWindow = false;
      numPressed = 0;
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

bool automatic = true;

bool pumper = false, fan = false, led = false;
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
}

void manual() {

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

  digitalWrite(ledPin, led);
  digitalWrite(fanPin, fan);
  digitalWrite(pumperPin, pumper);
}

void diagnose() {
  selectedOption = 255;
}

void exit() {
  isSettings = false;
  btnPressed = false;
  selectedOption = 255;
}

void loop() {

  float t, h;

  display.clearDisplay();

  if (!isSettings) {
    dispTempHumi(&t, &h, false);
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