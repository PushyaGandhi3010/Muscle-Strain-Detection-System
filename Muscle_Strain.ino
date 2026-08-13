MUSCLE STRAIN DETECTION SYSTEM

#include <WiFi.h>
#include <HTTPClient.h>

// ===================== WIFI CONFIG =====================
const char* ssid = " ";
const char* password = " ";
String apiKey = " ";
String server = "http://api.thingspeak.com/update";

// ===================== PIN CONFIG =====================
#define EMG_PIN   34
#define BUZZER    25
#define MOTOR     27

// ===================== SETTINGS ======================
#define SAMPLE_COUNT   30
#define NORMAL_DELTA    10
#define MILD_DELTA      35
#define ABNORMAL_TIME   1000

// ===================== VARIABLES =====================
float samples[SAMPLE_COUNT];
int sampleIndex = 0;

float lastEMG = 0;
unsigned long abnormalStart = 0;

// ===================== FILTER ========================
float getFilteredEMG_mV() {
  samples[sampleIndex] = analogRead(EMG_PIN);
  sampleIndex = (sampleIndex + 1) % SAMPLE_COUNT;

  float sum = 0;
  for (int i = 0; i < SAMPLE_COUNT; i++)
    sum += samples[i];

  float avgADC = sum / SAMPLE_COUNT;
  return (avgADC * 3300.0) / 4095.0;
}

// ===================== WIFI CONNECT ===================
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
}

// ===================== SETUP =========================
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  pinMode(MOTOR, OUTPUT);

  digitalWrite(BUZZER, LOW);
  digitalWrite(MOTOR, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(EMG_PIN, ADC_11db);

  connectWiFi();

  Serial.println("====================================");
  Serial.println("EMG Muscle Strain Monitor + Cloud");
  Serial.println("====================================");
}

// ===================== LOOP ==========================
void loop() {

  float emg_mV = getFilteredEMG_mV();
  float delta = abs(emg_mV - lastEMG);
  lastEMG = emg_mV;

  int activity = 0;

  // -------- ACTIVITY DETECTION --------
  if (delta < NORMAL_DELTA) {
    activity = 0;
    abnormalStart = 0;
  }
  else if (delta < MILD_DELTA) {
    activity = 1;
    abnormalStart = 0;
  }
  else {
    if (abnormalStart == 0)
      abnormalStart = millis();

    if (millis() - abnormalStart >= ABNORMAL_TIME)
      activity = 2;
  }

  // -------- OUTPUT CONTROL --------
  digitalWrite(MOTOR, activity == 2 ? HIGH : LOW);
  digitalWrite(BUZZER, activity == 2 ? HIGH : LOW);

  // -------- SERIAL OUTPUT --------
  Serial.print("EMG: ");
  Serial.print(emg_mV, 1);
  Serial.print(" mV | Δ: ");
  Serial.print(delta, 1);
  Serial.print(" | Status: ");

  if (activity == 0) Serial.println("NORMAL");
  else if (activity == 1) Serial.println("MILD");
  else Serial.println("ABNORMAL ⚠️");

  // -------- SEND DATA TO THINGSPEAK --------
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = server + "?api_key=" + apiKey +
                 "&field1=" + String(emg_mV, 1) +
                 "&field2=" + String(delta, 1) +
                 "&field3=" + String(activity);

    http.begin(url);
    int httpCode = http.GET();
    http.end();

    Serial.print("Cloud Update: ");
    Serial.println(httpCode);
  }

  Serial.println("--------------------------------");

  delay(15000);  // ThingSpeak safe delay
}

