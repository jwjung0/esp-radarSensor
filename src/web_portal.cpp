#include "web_portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#include "monitor_html.h"
#include "radar_sensor.h"

static const char *kApSsid = "RadarSensor";
static const char *kMdnsHost = "radarsensor";
static const IPAddress kApIp(192, 168, 4, 1);
static const IPAddress kApMask(255, 255, 255, 0);
static const uint16_t kDnsPort = 53;

static WebServer server(80);
static DNSServer dnsServer;
static bool s_started = false;

static const char *modeStr(RadarMode m) {
  switch (m) {
  case RADAR_MODE_MOV:
    return "mov";
  case RADAR_MODE_OCC:
    return "occ";
  default:
    return "none";
  }
}

static void sendLiveJson() {
  RadarLive live;
  RadarConfig cfg;
  radarSensorGetLive(&live);
  radarSensorGetConfig(&cfg);

  const uint32_t age =
      (live.last_ms == 0) ? 0 : (millis() - live.last_ms);

  char buf[480];
  snprintf(buf, sizeof(buf),
           "{\"mode\":\"%s\",\"dis\":%.2f,\"dis_raw\":%.2f,\"age_ms\":%lu,"
           "\"rmax\":%.2f,\"mth1\":%.0f,\"mth2\":%.0f,\"mth3\":%.0f,"
           "\"clear_s\":%.1f,\"mov_s\":%.1f,"
           "\"filter_on\":%s,\"filt_q\":%.2f,\"filt_r\":%.1f,"
           "\"cfg_ok\":%s,\"cfg_seq\":%lu}",
           modeStr(live.mode), live.distance_m, live.distance_raw_m,
           (unsigned long)age, cfg.rmax, cfg.mth1, cfg.mth2, cfg.mth3,
           cfg.clear_s, cfg.mov_s, cfg.filter_enabled ? "true" : "false",
           cfg.filt_q, cfg.filt_r, cfg.valid ? "true" : "false",
           (unsigned long)cfg.seq);
  server.send(200, "application/json", buf);
}

static void handleRoot() {
  server.send_P(200, "text/html", MONITOR_HTML);
}

static void handleLive() { sendLiveJson(); }

static void handleConfig() {
  if (!server.hasArg("rmax") || !server.hasArg("mth1") ||
      !server.hasArg("mth2") || !server.hasArg("mth3") ||
      !server.hasArg("clear_s") || !server.hasArg("mov_s")) {
    server.send(400, "application/json",
                "{\"ok\":false,\"err\":\"missing fields\"}");
    return;
  }

  const float rmax = server.arg("rmax").toFloat();
  const float mth1 = server.arg("mth1").toFloat();
  const float mth2 = server.arg("mth2").toFloat();
  const float mth3 = server.arg("mth3").toFloat();
  const float clear_s = server.arg("clear_s").toFloat();
  const float mov_s = server.arg("mov_s").toFloat();

  // Apply always persists: ESP NVS + LD1125H save.
  radarSensorApplyConfig(rmax, mth1, mth2, mth3, clear_s, mov_s, true);

  RadarConfig cfg;
  radarSensorGetConfig(&cfg);
  char buf[192];
  snprintf(buf, sizeof(buf),
           "{\"ok\":true,\"rmax\":%.2f,\"mth1\":%.0f,\"mth2\":%.0f,\"mth3\":%.0f,"
           "\"clear_s\":%.1f,\"mov_s\":%.1f,\"cfg_seq\":%lu}",
           cfg.rmax, cfg.mth1, cfg.mth2, cfg.mth3, cfg.clear_s, cfg.mov_s,
           (unsigned long)cfg.seq);
  server.send(200, "application/json", buf);
}

static void handleFilter() {
  if (!server.hasArg("filter_on") || !server.hasArg("filt_q") ||
      !server.hasArg("filt_r")) {
    server.send(400, "application/json",
                "{\"ok\":false,\"err\":\"missing fields\"}");
    return;
  }

  const String onArg = server.arg("filter_on");
  const bool enabled =
      (onArg == "1" || onArg == "true" || onArg == "TRUE" || onArg == "on");
  const float q = server.arg("filt_q").toFloat();
  const float r = server.arg("filt_r").toFloat();

  radarSensorApplyFilterConfig(enabled, q, r);

  RadarConfig cfg;
  radarSensorGetConfig(&cfg);
  char buf[192];
  snprintf(buf, sizeof(buf),
           "{\"ok\":true,\"filter_on\":%s,\"filt_q\":%.2f,\"filt_r\":%.1f,"
           "\"cfg_seq\":%lu}",
           cfg.filter_enabled ? "true" : "false", cfg.filt_q, cfg.filt_r,
           (unsigned long)cfg.seq);
  server.send(200, "application/json", buf);
}

static void handleRefresh() {
  radarSensorRequestGetAll();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleCaptive() {
  server.sendHeader("Location", String("http://") + kApIp.toString() + "/",
                    true);
  server.send(302, "text/plain", "");
}

static void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/live", HTTP_GET, handleLive);
  server.on("/api/config", HTTP_POST, handleConfig);
  server.on("/api/filter", HTTP_POST, handleFilter);
  server.on("/api/refresh", HTTP_POST, handleRefresh);
  server.on("/generate_204", HTTP_GET, handleCaptive);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptive);
  server.on("/connecttest.txt", HTTP_GET, handleCaptive);
  server.on("/ncsi.txt", HTTP_GET, handleCaptive);
  server.onNotFound(handleRoot);
}

void webPortalBegin(void) {
  if (s_started) {
    return;
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(kApIp, kApIp, kApMask);
  WiFi.softAP(kApSsid);
  dnsServer.start(kDnsPort, "*", kApIp);

  if (!MDNS.begin(kMdnsHost)) {
    Serial.println("mDNS start failed");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("mDNS: http://%s.local/\n", kMdnsHost);
  }

  setupRoutes();
  server.begin();
  s_started = true;

  Serial.printf("SoftAP \"%s\"  http://%s/\n", kApSsid,
                kApIp.toString().c_str());
  Serial.println("  also http://radarsensor.monitor/ (SoftAP DNS)");
}

void webPortalLoop(void) {
  if (!s_started) {
    return;
  }
  dnsServer.processNextRequest();
  server.handleClient();
}
