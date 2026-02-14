/*
 * ESP32-S3 CAM - Détecteur Frelon Asiatique
 * Deep Sleep + Capture autonome + Relève WiFi
 */

#include "esp_camera.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include <WebServer.h>
#include <FFat.h>
#include <FS.h>

// CONFIGURATION
#define GPIO_PIR 13
#define GPIO_BUTTON 14
#define PHOTO_RESOLUTION FRAMESIZE_VGA
#define JPEG_QUALITY 12
#define MAX_PHOTOS 20
#define AP_SSID "Frelon-Cam"
#define AP_PASSWORD "frelon2026"
#define WEB_TIMEOUT_MS 180000

// Pins Caméra OV3660
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 10
#define SIOD_GPIO_NUM 40
#define SIOC_GPIO_NUM 39
#define Y9_GPIO_NUM 48
#define Y8_GPIO_NUM 11
#define Y7_GPIO_NUM 12
#define Y6_GPIO_NUM 14
#define Y5_GPIO_NUM 16
#define Y4_GPIO_NUM 18
#define Y3_GPIO_NUM 17
#define Y2_GPIO_NUM 15
#define VSYNC_GPIO_NUM 38
#define HREF_GPIO_NUM 47
#define PCLK_GPIO_NUM 13

WebServer server(80);
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int photoCount = 0;
camera_fb_t * fb = NULL;
bool webServerActive = false;
unsigned long webServerStartTime = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  bootCount++;
  
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║  ESP32-S3 CAM - Détecteur Frelon        ║");
  Serial.println("╚═══════════════════════════════════════════╝");
  Serial.printf("Boot #%d | Photos: %d/%d\n", bootCount, photoCount, MAX_PHOTOS);
  
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("📸 PIR - Mode Capture");
      modeCapturePhoto();
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("📱 BOUTON - Mode Relève");
      modeReleveTerrain();
      break;
    default:
      Serial.println("⚡ PREMIER BOOT");
      if (!initFFat()) Serial.println("❌ Erreur FFat");
      else Serial.println("✅ OK");
      break;
  }
  
  configurerDeepSleep();
}

void loop() {
  if (webServerActive) {
    server.handleClient();
    if (millis() - webServerStartTime > WEB_TIMEOUT_MS) {
      Serial.println("⏱️ Timeout");
      arreterServeurWeb();
      configurerDeepSleep();
    }
  }
  delay(10);
}

void modeCapturePhoto() {
  if (!initFFat() || !initCamera()) return;
  
  Serial.println("📸 Capture...");
  fb = esp_camera_fb_get();
  
  if (fb) {
    Serial.printf("✅ %d bytes\n", fb->len);
    if (sauvegarderPhoto(fb)) {
      photoCount++;
      gererLimitePhotos();
    }
    esp_camera_fb_return(fb);
  }
  
  deinitCamera();
  FFat.end();
}

void modeReleveTerrain() {
  if (!initFFat()) return;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.printf("WiFi: %s\nIP: http://192.168.4.1\n", AP_SSID);
  
  configurerServeurWeb();
  server.begin();
  webServerActive = true;
  webServerStartTime = millis();
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  if (psramFound()) {
    config.frame_size = PHOTO_RESOLUTION;
    config.jpeg_quality = JPEG_QUALITY;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 15;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  
  if (esp_camera_init(&config) != ESP_OK) return false;
  
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_exposure_ctrl(s, 1);
  }
  return true;
}

void deinitCamera() {
  esp_camera_deinit();
}

bool initFFat() {
  if (!FFat.begin(true)) return false;
  if (!FFat.exists("/photos")) FFat.mkdir("/photos");
  return true;
}

bool sauvegarderPhoto(camera_fb_t * fb) {
  if (!fb) return false;
  char filename[32];
  snprintf(filename, sizeof(filename), "/photos/%lu.jpg", millis());
  File file = FFat.open(filename, FILE_WRITE);
  if (!file) return false;
  size_t written = file.write(fb->buf, fb->len);
  file.close();
  return (written == fb->len);
}

void gererLimitePhotos() {
  File dir = FFat.open("/photos");
  if (!dir) return;
  
  int count = 0;
  String oldestFile = "";
  unsigned long oldestTime = ULONG_MAX;
  
  File file = dir.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      count++;
      String name = file.name();
      int lastSlash = name.lastIndexOf('/');
      int lastDot = name.lastIndexOf('.');
      if (lastSlash >= 0 && lastDot > lastSlash) {
        unsigned long ts = name.substring(lastSlash + 1, lastDot).toInt();
        if (ts < oldestTime) {
          oldestTime = ts;
          oldestFile = name;
        }
      }
    }
    file = dir.openNextFile();
  }
  dir.close();
  
  if (count >= MAX_PHOTOS && oldestFile.length() > 0) {
    FFat.remove(oldestFile);
  }
}

void configurerServeurWeb() {
  server.on("/", HTTP_GET, []() {
    String html = R"(<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Frelon-Cam</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px}.card{background:#fff;border-radius:15px;padding:20px;margin:15px auto;max-width:800px;box-shadow:0 10px 30px rgba(0,0,0,.2)}h1{text-align:center;margin-bottom:20px}.btn{display:block;width:100%;padding:15px;border:none;border-radius:10px;font-size:16px;font-weight:bold;cursor:pointer;margin:10px 0}.btn-success{background:#4caf50;color:#fff}.btn-danger{background:#d32f2f;color:#fff}.gallery{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px;margin-top:20px}.photo-item{border-radius:10px;overflow:hidden;aspect-ratio:4/3;cursor:pointer}.photo-item img{width:100%;height:100%;object-fit:cover}.stats{background:#f5f5f5;padding:15px;border-radius:10px;margin:10px 0}.stats p{margin:5px 0;color:#666}</style></head><body><div class="card"><h1>🐝 Frelon-Cam</h1><div class="stats"><p><strong>Photos:</strong> <span id="count">...</span></p><p><strong>Stockage:</strong> <span id="storage">...</span></p></div></div><div class="card"><button class="btn btn-success" onclick="location.reload()">🔄 Actualiser</button><button class="btn btn-danger" onclick="deleteAll()">🗑️ Supprimer</button></div><div class="card"><h2 style="margin-bottom:15px">Galerie</h2><div id="gallery" class="gallery">Chargement...</div></div><script>function load(){fetch('/list').then(r=>r.json()).then(d=>{document.getElementById('count').textContent=d.count;document.getElementById('storage').textContent=(d.total/1048576).toFixed(1)+' MB total, '+(d.used/1048576).toFixed(1)+' MB utilisés';let g=document.getElementById('gallery');g.innerHTML='';if(d.photos.length===0){g.innerHTML='Aucune photo';return}d.photos.forEach(p=>{let div=document.createElement('div');div.className='photo-item';div.innerHTML='<img src="/photo?file='+p+'">';div.onclick=()=>window.open('/photo?file='+p);g.appendChild(div)})})}function deleteAll(){if(confirm('Supprimer ?')){fetch('/delete_all').then(()=>{alert('OK');load()})}}load();setInterval(load,10000)</script></body></html>)";
    server.send(200, "text/html", html);
  });
  
  server.on("/list", HTTP_GET, []() {
    File dir = FFat.open("/photos");
    String json = "{\"count\":" + String(photoCount) + ",\"total\":" + String(FFat.totalBytes()) + ",\"used\":" + String(FFat.usedBytes()) + ",\"photos\":[";
    File file = dir.openNextFile();
    bool first = true;
    while (file) {
      if (!file.isDirectory()) {
        if (!first) json += ",";
        json += "\"" + String(file.name()) + "\"";
        first = false;
      }
      file = dir.openNextFile();
    }
    json += "]}";
    dir.close();
    server.send(200, "application/json", json);
  });
  
  server.on("/photo", HTTP_GET, []() {
    if (!server.hasArg("file")) { server.send(400, "text/plain", "Missing file"); return; }
    String filename = server.arg("file");
    if (!filename.startsWith("/photos/")) { server.send(403, "text/plain", "Forbidden"); return; }
    File file = FFat.open(filename, "r");
    if (!file) { server.send(404, "text/plain", "Not found"); return; }
    server.streamFile(file, "image/jpeg");
    file.close();
  });
  
  server.on("/delete_all", HTTP_GET, []() {
    File dir = FFat.open("/photos");
    File file = dir.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String name = file.name();
        file.close();
        FFat.remove(name);
      }
      file = dir.openNextFile();
    }
    dir.close();
    photoCount = 0;
    server.send(200, "text/plain", "OK");
  });
}

void arreterServeurWeb() {
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  webServerActive = false;
  FFat.end();
}

void configurerDeepSleep() {
  esp_sleep_enable_ext0_wakeup((gpio_num_t)GPIO_PIR, 1);
  esp_sleep_enable_ext1_wakeup((1ULL << GPIO_BUTTON), ESP_EXT1_WAKEUP_ANY_HIGH);
  if (webServerActive) arreterServeurWeb();
  Serial.println("💤 Deep Sleep\n");
  Serial.flush();
  delay(100);
  esp_deep_sleep_start();
}
