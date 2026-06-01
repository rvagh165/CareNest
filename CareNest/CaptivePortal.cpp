#include "CaptivePortal.h"

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#include "RTC.h"

static const uint16_t DNS_PORT = 53;
static const char *AP_SSID = "CareNest";

static DNSServer dnsServer;
static WebServer webServer(80);
static bool portalRunning = false;
static bool stopRequested = false;
static bool timeSet = false;
static unsigned long portalStartedMs = 0;
static const unsigned long PORTAL_TIMEOUT_MS = 5UL * 60UL * 1000UL; // 5 minutes

static const char kIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>CareNest - Time Setup</title>
  <style>
    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:22px;background:#0b1220;color:#e5e7eb}
    .card{max-width:520px;margin:0 auto;background:#111a2e;border:1px solid rgba(255,255,255,.10);border-radius:14px;padding:18px}
    h1{margin:0 0 10px 0;font-size:20px}
    p{margin:0 0 14px 0;color:#9ca3af;line-height:1.35}
    label{display:block;font-size:12px;color:#9ca3af;margin:10px 0 6px}
    select,button{width:100%;padding:12px;border-radius:12px;border:1px solid rgba(255,255,255,.14);background:#0f172a;color:#e5e7eb;font:inherit}
    button{margin-top:12px;background:#3b82f6;border-color:#60a5fa;font-weight:650;cursor:pointer}
    .note{margin-top:12px;padding:10px 12px;border-radius:12px;background:rgba(15,23,42,.75);border:1px solid rgba(255,255,255,.10);color:#cbd5e1;font-size:13px}
  </style>
</head>
<body>
  <div class="card">
    <h1>CareNest - Time Setup</h1>
    <p>Select your time zone and press "Set Time". This uses your phone's current time (no internet needed).</p>

    <label for="tz">Time zone</label>
    <select id="tz"></select>

    <button id="btnSet">Set Time</button>
    <div class="note" id="status">Waiting...</div>
  </div>

  <script>
    const tzSel = document.getElementById('tz');
    const statusEl = document.getElementById('status');

    function pad2(n){ return String(n).padStart(2,'0'); }
    function fmtOffset(mins){
      const sign = mins >= 0 ? '+' : '-';
      const a = Math.abs(mins);
      const hh = Math.floor(a/60), mm = a%60;
      return `UTC${sign}${pad2(hh)}:${pad2(mm)}`;
    }

    function buildTzOptions(){
      const offsets = [];
      for(let m=-12*60; m<=14*60; m+=30) offsets.push(m);
      for(const m of offsets){
        const opt = document.createElement('option');
        opt.value = String(m);
        opt.textContent = fmtOffset(m);
        tzSel.appendChild(opt);
      }
      tzSel.value = '330'; // IST default
    }

    function tzEpochFromUtc(utcEpochSec, offsetMins){
      return utcEpochSec + (offsetMins * 60);
    }

    function setStatus(text){
      statusEl.textContent = text;
    }

    async function onSet(){
      const offsetMins = parseInt(tzSel.value, 10) || 0;
      const utcEpoch = Math.floor(Date.now()/1000);
      const tzEpoch = tzEpochFromUtc(utcEpoch, offsetMins);

      setStatus('Setting time...');
      const body = new URLSearchParams();
      body.set('epoch', String(tzEpoch));
      body.set('offsetMins', String(offsetMins));

      try{
        const r = await fetch('/api/time', { method:'POST', body, headers: {'Content-Type':'application/x-www-form-urlencoded'} });
        const t = await r.text();
        if(!r.ok) throw new Error(t || 'request failed');
        setStatus('Time set successfully. You can close this page.');
      }catch(e){
        setStatus('Failed to set time. Try again.');
      }
    }

    buildTzOptions();
    document.getElementById('btnSet').addEventListener('click', onSet);
    setStatus('Waiting...');
  </script>
</body>
</html>
)HTML";

static void sendRedirectToRoot()
{
    webServer.sendHeader("Location", "/", true);
    webServer.send(302, "text/plain", "");
}

static void handleRoot()
{
    webServer.send_P(200, "text/html", kIndexHtml);
}

static bool parseUint32Arg(const String &value, uint32_t *out)
{
    if (!out) {
        return false;
    }
    if (value.length() == 0) {
        return false;
    }
    const char *s = value.c_str();
    char *end = nullptr;
    unsigned long v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') {
        return false;
    }
    *out = (uint32_t)v;
    return true;
}

static void handleApiTime()
{
    if (!webServer.hasArg("epoch")) {
        webServer.send(400, "text/plain", "missing epoch");
        return;
    }

    uint32_t epoch = 0;
    if (!parseUint32Arg(webServer.arg("epoch"), &epoch)) {
        webServer.send(400, "text/plain", "bad epoch");
        return;
    }

    rtcSetTime(epoch);
    timeSet = true;
    webServer.send(200, "text/plain", "ok");

    // Stop AP mode after successful set.
    stopRequested = true;
}

void captivePortalStart(void)
{
    if (portalRunning) {
        return;
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID);

    IPAddress apIP = WiFi.softAPIP();

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);

    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/api/time", HTTP_POST, handleApiTime);

    // Common captive portal probe endpoints
    webServer.on("/generate_204", HTTP_ANY, sendRedirectToRoot); // Android
    webServer.on("/gen_204", HTTP_ANY, sendRedirectToRoot);
    webServer.on("/fwlink", HTTP_ANY, sendRedirectToRoot);       // Windows
    webServer.on("/hotspot-detect.html", HTTP_ANY, sendRedirectToRoot); // iOS
    webServer.on("/library/test/success.html", HTTP_ANY, sendRedirectToRoot); // iOS

    webServer.onNotFound(sendRedirectToRoot);
    webServer.begin();

    portalRunning = true;
    stopRequested = false;
    timeSet = false;
    portalStartedMs = millis();

    Serial.print(F("[Portal] AP started: "));
    Serial.print(AP_SSID);
    Serial.print(F(" @ "));
    Serial.println(apIP);
}

void captivePortalStop(void)
{
    if (!portalRunning) {
        return;
    }

    dnsServer.stop();
    webServer.stop();

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    portalRunning = false;
    stopRequested = false;
    portalStartedMs = 0;
    Serial.println(F("[Portal] stopped"));
}

void captivePortalUpdate(void)
{
    if (!portalRunning) {
        return;
    }

    dnsServer.processNextRequest();
    webServer.handleClient();

    if (stopRequested) {
        captivePortalStop();
        return;
    }

    if (PORTAL_TIMEOUT_MS > 0 && (millis() - portalStartedMs) >= PORTAL_TIMEOUT_MS) {
        Serial.println(F("[Portal] timeout"));
        captivePortalStop();
    }
}

bool captivePortalIsRunning(void)
{
    return portalRunning;
}

uint32_t captivePortalRemainingMs(void)
{
    if (!portalRunning) {
        return 0;
    }
    unsigned long elapsed = millis() - portalStartedMs;
    if (elapsed >= PORTAL_TIMEOUT_MS) {
        return 0;
    }
    return (uint32_t)(PORTAL_TIMEOUT_MS - elapsed);
}

bool captivePortalWasTimeSet(void)
{
    return timeSet;
}
