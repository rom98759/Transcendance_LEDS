#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#define LED_PIN D4
#define LED_COUNT 10
#define WIN_SCORE 5

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);

int scoreA = 0;
int scoreB = 0;
int prevScoreA = -1;
int prevScoreB = -1;

// Couleurs personnalisées (RGB)
uint8_t colorA_R = 0, colorA_G = 0, colorA_B = 255; // Bleu par défaut
uint8_t colorB_R = 255, colorB_G = 0, colorB_B = 0; // Rouge par défaut

// Luminosité (0-255)
uint8_t ledBrightness = 128; // 50% par défaut

const char* webhook_url = "https://discord.com/api/webhooks/XXXX";
// 2.4GHz wifi ESP wemos D1 Mini
const char* ssid = "XXX";
const char* password = "XXX";

unsigned long lastAnim = 0;
uint8_t animPos = 0;
int8_t animDir = 1;
const unsigned long animInterval = 120;

bool victoryActive = false;
uint8_t victoryWinner = 0; // 1 = A (bleu), 2 = B (rouge)
unsigned long victoryStart = 0;
unsigned long victoryLastFrame = 0;
const unsigned long victoryDuration = 3000;
const unsigned long victoryInterval = 140;

void animateIdle();
bool sendToWebhook(const char* webhook_url, const char* message);
void setAllColor(uint32_t color);

void updateStrip() {
  if (scoreA == prevScoreA && scoreB == prevScoreB) return;

  strip.clear();
  for (int i = 0; i < scoreA && i < 5; i++) strip.setPixelColor(i, strip.Color(colorA_R, colorA_G, colorA_B));
  for (int i = 0; i < scoreB && i < 5; i++) strip.setPixelColor(9 - i, strip.Color(colorB_R, colorB_G, colorB_B));
  strip.show();

  prevScoreA = scoreA;
  prevScoreB = scoreB;
  lastAnim = millis();

  if (scoreA >= WIN_SCORE || scoreB >= WIN_SCORE) {
    victoryActive = true;
    victoryWinner = (scoreA >= WIN_SCORE) ? 1 : 2;
    victoryStart = millis();
    victoryLastFrame = 0;
  } else {
    victoryActive = false;
    victoryWinner = 0;
  }

  Serial.printf("Score mis à jour : A=%d | B=%d\n", scoreA, scoreB);
}

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(ledBrightness);
  strip.show();

  Serial.print("Connexion Wi-Fi...");
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    if (millis() - start > 10000) { // timeout 10s
      Serial.println("\nImpossible de se connecter au Wi-Fi");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
      Serial.print("\nConnecté au Wi-Fi, IP: ");
      Serial.println(WiFi.localIP());
      char ipMsg[48];
      snprintf(ipMsg, sizeof(ipMsg), "%s", WiFi.localIP().toString().c_str());
      if (sendToWebhook(webhook_url, ipMsg)) Serial.println("[OK] IP envoyée au webhook");
  }

  server.on("/health", []() {
    server.send(200, "text/plain", "OK");
  });

  server.on("/score", []() {
    if (server.method() == HTTP_POST || server.hasArg("a") || server.hasArg("b")) {
      // Mode SET: met à jour le score
      if (server.hasArg("a")) scoreA = server.arg("a").toInt();
      if (server.hasArg("b")) scoreB = server.arg("b").toInt();
      updateStrip();
      server.send(200, "text/plain", "OK");
    } else {
      // Mode GET: renvoie le score actuel en JSON
      String json = "{\"scoreA\":" + String(scoreA) + ",\"scoreB\":" + String(scoreB) + "}";
      server.send(200, "application/json", json);
    }
  });

  // Route pour changer les couleurs
  server.on("/color", []() {
    bool updated = false;

    // Couleur du joueur A
    if (server.hasArg("ar")) { colorA_R = server.arg("ar").toInt(); updated = true; }
    if (server.hasArg("ag")) { colorA_G = server.arg("ag").toInt(); updated = true; }
    if (server.hasArg("ab")) { colorA_B = server.arg("ab").toInt(); updated = true; }

    // Couleur du joueur B
    if (server.hasArg("br")) { colorB_R = server.arg("br").toInt(); updated = true; }
    if (server.hasArg("bg")) { colorB_G = server.arg("bg").toInt(); updated = true; }
    if (server.hasArg("bb")) { colorB_B = server.arg("bb").toInt(); updated = true; }

    if (updated) {
      prevScoreA = -1; // Force l'update
      prevScoreB = -1;
      updateStrip();
      String json = "{\"colorA\":[" + String(colorA_R) + "," + String(colorA_G) + "," + String(colorA_B) + "],";
      json += "\"colorB\":[" + String(colorB_R) + "," + String(colorB_G) + "," + String(colorB_B) + "]}";
      server.send(200, "application/json", json);
    } else {
      // GET: renvoie les couleurs actuelles
      String json = "{\"colorA\":[" + String(colorA_R) + "," + String(colorA_G) + "," + String(colorA_B) + "],";
      json += "\"colorB\":[" + String(colorB_R) + "," + String(colorB_G) + "," + String(colorB_B) + "]}";
      server.send(200, "application/json", json);
    }
  });

  // Route pour changer la luminosité
  server.on("/brightness", []() {
    if (server.hasArg("value")) {
      ledBrightness = constrain(server.arg("value").toInt(), 0, 255);
      strip.setBrightness(ledBrightness);
      prevScoreA = -1; // Force l'update
      prevScoreB = -1;
      updateStrip();
      String json = "{\"brightness\":" + String(ledBrightness) + "}";
      server.send(200, "application/json", json);
    } else {
      // GET: renvoie la luminosité actuelle
      String json = "{\"brightness\":" + String(ledBrightness) + "}";
      server.send(200, "application/json", json);
    }
  });

  // reset endpoint: remet l'état à l'état initial (idle)
  server.on("/reset", []() {
    scoreA = 0;
    scoreB = 0;
    prevScoreA = -1;
    prevScoreB = -1;
    victoryActive = false;
    victoryWinner = 0;
    animPos = 0;
    animDir = 1;
    lastAnim = millis();
    colorA_R = 0; colorA_G = 0; colorA_B = 255;
    colorB_R = 255; colorB_G = 0; colorB_B = 0;
    ledBrightness = 128;
    strip.setBrightness(ledBrightness);
    strip.clear();
    strip.show();
    server.send(200, "text/plain", "RESET");
  });

  server.begin();
  Serial.println("Serveur HTTP démarré.");
}

void loop() {
  server.handleClient();

  if (victoryActive) {
    unsigned long now = millis();
    if (now - victoryStart <= victoryDuration) {
      unsigned long n = now;
      if (n - victoryLastFrame >= victoryInterval) {
        victoryLastFrame = n;
        bool on = (((n - victoryStart) / victoryInterval) % 2) == 0;
        uint32_t color = (victoryWinner == 1) ? strip.Color(colorA_R, colorA_G, colorA_B) : strip.Color(colorB_R, colorB_G, colorB_B);
        if (on) for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color);
        else strip.clear();
        strip.show();
      }
    } else {
      uint32_t color = (victoryWinner == 1) ? strip.Color(colorA_R, colorA_G, colorA_B) : strip.Color(colorB_R, colorB_G, colorB_B);
      setAllColor(color);
      victoryActive = false;
    }
  } else {
    if (prevScoreA == -1 && prevScoreB == -1) animateIdle();
  }
}

// animation idle non bloquante : point bleu à gauche et point rouge à droite
void animateIdle() {
  unsigned long now = millis();
  if (now - lastAnim < animInterval) return;
  lastAnim = now;
  strip.clear();
  strip.setPixelColor(animPos % 5, strip.Color(colorA_R / 3, colorA_G / 3, colorA_B / 3));
  uint8_t rightPos = 9 - (animPos % 5);
  strip.setPixelColor(rightPos, strip.Color(colorB_R / 3, colorB_G / 3, colorB_B / 3));
  strip.show();
  if (animDir > 0) { animPos++; if (animPos >= 5) { animPos = 4; animDir = -1; } }
  else { if (animPos == 0) animDir = 1; else animPos--; }
}

// helper: fill strip with a color and show
void setAllColor(uint32_t color) { for (int i = 0; i < LED_COUNT; i++) strip.setPixelColor(i, color); strip.show(); }

// Envoi minimal vers webhook Discord (payload JSON simple)
bool sendToWebhook(const char* webhook_url, const char* message) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, webhook_url)) return false;

  http.addHeader("Content-Type", "application/json");

  char payload[192];
  int len = snprintf(payload, sizeof(payload), "{\"content\":\"%s\"}", message);
  int httpCode = http.POST((uint8_t*)payload, len);

  if (httpCode <= 0) {
    http.end();
    return false;
  }

  String resp = http.getString();
  (void)resp; // pas nécessaire mais utile pour debug si besoin

  http.end();
  return true;
}
