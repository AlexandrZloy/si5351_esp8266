#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <si5351.h>
#include <Wire.h>

Si5351 si5351;

bool clk_en[3] = { false, false, false };

uint32_t freq[3] = { 7000000UL, 0UL, 0UL };

// TODO do the calibration https://github.com/etherkit/Si5351Arduino/blob/master/examples/si5351_calibration/si5351_calibration.ino

const char* ssid = "Si5351_VFO_esp8266";
const char* password = "vfo12345678";

ESP8266WebServer server(80);

const uint8_t I2C_SDA = 0;
const uint8_t I2C_SCL = 2;

void updateFrequency(uint32_t f, uint8_t clkIdx) {
  si5351_clock clk;
  if (clkIdx == 0) {
    clk = SI5351_CLK0;
  } else if (clkIdx == 1) {
    clk = SI5351_CLK1;
  } else {
    clk = SI5351_CLK2;
  }
  si5351.set_freq(static_cast<uint64_t>(f) * SI5351_FREQ_MULT, clk);
  freq[clkIdx] = f;
  Serial.print("CLK");
  Serial.print(clkIdx);
  Serial.print(" freq=");
  Serial.print(f);
  if (f < SI5351_CLKOUT_MIN_FREQ) {
    if (clk_en[clkIdx]) {
      si5351.output_enable(clk, 0);
      Serial.print(" disabled");
      clk_en[clkIdx] = false;
    }
  } else {
    if (!clk_en[clkIdx]) {
      si5351.output_enable(clk, 1);
      Serial.print(" enabled");
      clk_en[clkIdx] = true;
    }
  }
  Serial.println();
}

void handleSetFrequency() {
  if (!server.hasArg("f") || !server.hasArg("c")) {
    server.send(400, "text/plain", "Missing frequency or clock parameter");
    return;
  }

  String value = server.arg("f");
  String clock = server.arg("c");

  // Convert the HTTP parameter to an unsigned long value.
  uint32_t newFrequency = strtoul(value.c_str(), nullptr, 10);
  uint8_t requestedClk = strtoul(clock.c_str(), nullptr, 10);

  if (requestedClk > 2) {
    server.send(400, "text/plain", "Invalid clock");
    return;
  }

  if (newFrequency < SI5351_CLKOUT_MIN_FREQ) {
    newFrequency = 0;
  }
  if (newFrequency > SI5351_MULTISYNTH_MAX_FREQ) {
    newFrequency = SI5351_MULTISYNTH_MAX_FREQ;
  }

  updateFrequency(newFrequency, requestedClk);

  server.send(200, "text/plain", "OK");
}

void handleSwitchClk() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "Missing clock parameter");
    return;
  }

  String value = server.arg("c");

  uint8_t requestedClk = strtoul(value.c_str(), nullptr, 10);

  if (requestedClk > 2) {
    server.send(400, "text/plain", "Invalid clock");
    return;
  }

  server.send(200, "text/plain", String(freq[requestedClk]));
}

void setup() {
  Serial.begin(115200);
  delay(100);

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
  }

  // Initialize I2C.
  Wire.begin(I2C_SDA, I2C_SCL);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("Access point started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  bool initialized = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  if (!initialized) {
    Serial.println("Si5351 initialization failed");
  } else {
    Serial.println("Si5351 initialized");
  }

  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
  si5351.set_ms_source(SI5351_CLK1, SI5351_PLLB);
  si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);

  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);

  si5351.set_clock_disable(SI5351_CLK0, SI5351_CLK_DISABLE_HI_Z);
  si5351.set_clock_disable(SI5351_CLK1, SI5351_CLK_DISABLE_HI_Z);
  si5351.set_clock_disable(SI5351_CLK2, SI5351_CLK_DISABLE_HI_Z);

  updateFrequency(freq[0], 0);

  server.serveStatic("/", LittleFS, "/index.html");
  server.on("/set", HTTP_GET, handleSetFrequency);
  server.on("/getS", HTTP_GET, handleSwitchClk);

  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
