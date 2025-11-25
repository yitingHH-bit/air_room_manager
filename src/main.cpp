#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ESP8266mDNS.h>  // optional mDNS (http://esp8266.local)


// ====== Firebase config (EDIT ME) ======
// Your Realtime Database host (no https://, no path)
const char* FIREBASE_HOST = "air-manager-ccdf4-default-rtdb.firebaseio.com";
// Firebase path where readings will be stored (must end with .json)
const char* FIREBASE_PATH = "/sensor_readings.json";

// If you later use auth rules, put your DB secret / token here.
// With test rules (read/write true), you can leave this empty.
const char* FIREBASE_AUTH = "";

const uint32_t CLOUD_PERIOD_MS = 30000;   // periodic upload interval (ms)

// ====== Device / Wi-Fi ======
const char* DEVICE_ID  = "esp8266-001";
// Current Wi-Fi
const char* WIFI_SSID = "yours";
const char* WIFI_PASS = "yours";

// (Optional) Static IP — uncomment and adjust if you want a fixed IP
// IPAddress local(192,168,0,50);
// IPAddress gateway(192,168,0,1);
// IPAddress subnet(255,255,255,0);
// IPAddress dns(192,168,0,1);

// ====== DHT sensor on D2 (GPIO4) ======
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

ESP8266WebServer server(80);

// ====== Time helpers (UTC ISO-8601) ======
static time_t nowUTC() { return time(nullptr); }
static String iso8601UTC() {
  time_t t = nowUTC();
  if (t <= 100000) return String("");  // not synced yet
  struct tm *tm = gmtime(&t);
  char buf[24];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm);
  return String(buf);
}

// ====== JSON builder (stack buffer, avoids String fragmentation) ======
static String makeMetricsJson(const char* deviceId, const String& ts, float t, float h, const char* aqiLiteral /* "null" or number as string */) {
  char buf[192];
  if (ts.length()) {
    snprintf(buf, sizeof(buf),
      "{\"device_id\":\"%s\",\"ts\":\"%s\",\"temp_c\":%.2f,\"rh\":%.1f,\"aqi\":%s}",
      deviceId, ts.c_str(), t, h, aqiLiteral);
  } else {
    snprintf(buf, sizeof(buf),
      "{\"device_id\":\"%s\",\"ts\":null,\"temp_c\":%.2f,\"rh\":%.1f,\"aqi\":%s}",
      deviceId, t, h, aqiLiteral);
  }
  return String(buf);
}

// ====== Sensors ======
static float readTempC()    { float t = dht.readTemperature(); return isnan(t)?NAN:t; }
static float readHumidity() { float h = dht.readHumidity();    return isnan(h)?NAN:h; }

// ====== Cloud upload -> Firebase ======
bool postToCloud(float t, float h, const String& isoTs) {
  // Build URL: https://HOST/PATH[?auth=...]
  String url = String("https://") + FIREBASE_HOST + FIREBASE_PATH;
  if (FIREBASE_AUTH && strlen(FIREBASE_AUTH) > 0) {
    url += "?auth=";
    url += FIREBASE_AUTH;
  }

  WiFiClientSecure client;
  client.setTimeout(5000);   // keep UI responsive even if cloud is slow
  client.setInsecure();      // DEV ONLY (ignore TLS cert; for production load CA/fingerprint)

  HTTPClient http;
  if (!http.begin(client, url)) {
    Serial.println("[FIREBASE] http.begin() failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  // Build payload
  String body = makeMetricsJson(DEVICE_ID, isoTs, t, h, "null");

  Serial.print("[FIREBASE] POST ");
  Serial.println(url);
  Serial.print("[FIREBASE] body: ");
  Serial.println(body);

  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.printf("[FIREBASE] HTTP %d\n", code);
  Serial.println(resp);

  // Firebase usually returns 200 or 201 on success
  return code >= 200 && code < 300;
}

// ====== HTTP handlers ======

// Root: just a small JSON banner (frontend is external)
void handleRoot() {
  IPAddress ip = WiFi.localIP();
  String body = String("{\"ok\":true,\"msg\":\"ESP8266 API running (frontend is external)\",\"ip\":\"")
                + ip.toString() + "\"}";
  server.send(200, "application/json", body);
}

void handleTemp(){  // legacy quick test
  float t = readTempC();
  if (isnan(t)) {
    server.send(500,"application/json","{\"ok\":false,\"err\":\"sensor_read_failed\"}");
    return;
  }
  char buf[96];
  snprintf(buf, sizeof(buf),
           "{\"ok\":true,\"temp_c\":%.2f,\"temp_f\":%.2f}",
           t, t*9.0/5.0+32.0);
  server.send(200,"application/json", buf);
}

// Unified schema endpoint used by the page and cloud
void handleMetrics(){
  float t = readTempC(), h = readHumidity();
  if (isnan(t) || isnan(h)) {
    server.send(500,"application/json","{\"ok\":false,\"err\":\"sensor_read_failed\"}");
    return;
  }
  String ts = iso8601UTC(); // may be empty if NTP not synced yet
  String j  = makeMetricsJson(DEVICE_ID, ts, t, h, "null");
  server.send(200,"application/json", j);
}

// Manual cloud upload trigger (POST /api/push)
void handlePush() {
  float t = readTempC(), h = readHumidity();
  if (isnan(t) || isnan(h)) {
    server.send(500, "application/json", "{\"ok\":false,\"err\":\"sensor_read_failed\"}");
    return;
  }
  String ts = iso8601UTC();
  bool ok = postToCloud(t, h, ts);
  server.send(200, "application/json", String("{\"ok\":") + (ok?"true":"false") + "}");
}

// Device info endpoint (IP/SSID/device_id)
void handleInfo() {
  IPAddress ip = WiFi.localIP();
  String ssid = WiFi.SSID();
  char buf[160];
  snprintf(buf, sizeof(buf),
    "{\"device_id\":\"%s\",\"ip\":\"%u.%u.%u.%u\",\"ssid\":\"%s\"}",
    DEVICE_ID, ip[0],ip[1],ip[2],ip[3], ssid.c_str());
  server.send(200, "application/json", buf);
}

// Diagnostics: Wi-Fi status and addressing
void handleDiag(){
  IPAddress ip = WiFi.localIP(), gw = WiFi.gatewayIP(), sn = WiFi.subnetMask(), dns = WiFi.dnsIP();
  char buf[256];
  snprintf(buf, sizeof(buf),
    "{\"status\":%d,\"ssid\":\"%s\",\"ip\":\"%s\",\"gateway\":\"%s\",\"subnet\":\"%s\",\"dns\":\"%s\",\"rssi\":%d}",
    WiFi.status(), WiFi.SSID().c_str(),
    ip.toString().c_str(), gw.toString().c_str(), sn.toString().c_str(), dns.toString().c_str(), WiFi.RSSI());
  server.send(200, "application/json", buf);
}

// Health check and 404 logger
void handlePing(){ server.send(200, "text/plain", "pong"); }
void handleNotFound() {
  Serial.printf("[HTTP] 404: %s\n", server.uri().c_str());
  server.send(404, "text/plain", "Not found");
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // DHT warm-up (DHT11: keep sample rate <= ~1 Hz)
  pinMode(DHT_PIN, INPUT_PULLUP);
  dht.begin();
  for(int i=0;i<2;i++){
    dht.readTemperature();
    dht.readHumidity();
    delay(1200);
  }
  Serial.println("DHT initialized (pin=D2/GPIO4, type=DHT11)");

  // Wi-Fi: STA + AP (so you always have a back door at 192.168.4.1)
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_AP_STA);

  // (Optional) Static IP before begin()
  // WiFi.config(local, gateway, subnet, dns);

  // Connect STA
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("Connecting to %s", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    Serial.printf("\nConnected! IP: %u.%u.%u.%u\n", ip[0],ip[1],ip[2],ip[3]);
  } else {
    Serial.println("\nSTA connect timed out.");
  }

  // Bring up a fallback AP (always available)
  WiFi.softAP("esp8266-setup", "12345678");
  IPAddress apIP(192,168,4,1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  Serial.println("SoftAP up: SSID=esp8266-setup  PASS=12345678  URL: http://192.168.4.1/");

  // ---- Register routes and start HTTP server (MUST happen before NTP/mDNS) ----
  server.enableCORS(true);
  server.on("/", handleRoot);
  server.on("/api/temp", handleTemp);
  server.on("/api/metrics", handleMetrics);
  server.on("/api/info", handleInfo);
  server.on("/api/push", HTTP_POST, handlePush);
  server.on("/diag", handleDiag);
  server.on("/ping", handlePing);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  // NTP (UTC) — done after server is up to avoid blocking access
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("NTP syncing...");
  for (int i = 0; i < 20 && nowUTC() <= 100000; ++i) {
    delay(250);
  }

  // Optional mDNS (Windows may need Bonjour)
  if (MDNS.begin("esp8266")) {
    Serial.println("mDNS: http://esp8266.local/");
  }
}

void loop() {
  server.handleClient();

  // periodic serial log of sensor values
  static uint32_t lastPrint=0, lastCloud=0;
  if (millis()-lastPrint > 2500) {
    lastPrint = millis();
    float t=readTempC(), h=readHumidity();
    if (isnan(t)||isnan(h)) {
      Serial.println("[DHT] read failed");
    } else {
      Serial.printf("[DHT] T=%.2f °C  H=%.1f %%\n", t, h);
    }
    yield();
  }

  // periodic cloud upload (to Firebase)
  if (millis() - lastCloud > CLOUD_PERIOD_MS) {
    lastCloud = millis();
    float t=readTempC(), h=readHumidity();
    if (!isnan(t) && !isnan(h)) {
      String ts = iso8601UTC();
      postToCloud(t, h, ts);
    }
  }
}
