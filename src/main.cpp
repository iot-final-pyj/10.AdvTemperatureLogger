#include <Arduino.h>
#include <WiFi.h>
#include <ConfigPortal32.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <DHTesp.h>

char*               ssid_pfix = (char*)"AmbientSensor";
String              user_config_html = ""
    "<p><input type='text' name='meta.influxIP' placeholder='InfluxDB Address'>"
    "<p><input type='text' name='meta.token' placeholder='InfluxDB Token'>"
    "<p><input type='text' name='meta.bucket' placeholder='InfluxDB Bucket'>"
    "<p><input type='text' name='meta.reqinterval' placeholder='Report Interval'>";
char                influxIP[50];
int                 reqinterval = 2000;

char                influxURL[200];

DHTesp              dht;
int                 interval = 2000;
unsigned long       lastDHTReadMillis = 0;
float               humidity = 0;
float               temperature = 0;
char                dht_buffer[10];

char                bucket[50];
char                token[200];

void readDHT22() {
    unsigned long currentMillis = millis();

    if(currentMillis - lastDHTReadMillis >= interval) {
        lastDHTReadMillis = currentMillis;

        humidity = dht.getHumidity();              // Read humidity (percent)
        temperature = dht.getTemperature();             // Read temperature as Fahrenheit
    }
}

WiFiClient client;

void setup(void){
    Serial.begin(115200);
    dht.setup(15, DHTesp::DHT22);

    loadConfig();
    // *** If no "config" is found or "config" is not "done", run configDevice ***
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    // main setup
    Serial.printf("\nIP address : "); Serial.println(WiFi.localIP());
    if (cfg.containsKey("meta")) {
        if (cfg["meta"].containsKey("influxIP")) {
            sprintf(influxIP, (const char*)cfg["meta"]["influxIP"]);
        }
        if (cfg["meta"].containsKey("token")) {
            sprintf(token, (const char*)cfg["meta"]["token"]);
        }
        if (cfg["meta"].containsKey("bucket")) {
            sprintf(bucket, (const char*)cfg["meta"]["bucket"]);
        }
        if (cfg["meta"].containsKey("reqinterval")) {
            reqinterval = cfg["meta"]["reqinterval"];
        }
    }

    sprintf(influxURL, "http://%s:8086/write?db=%s", influxIP, bucket);

    MDNS.begin("thermoSensor");
}

void loop(void){
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    char data[200];

    readDHT22();
    sprintf(data, "ambient,location=room02 temperature=%.1f,humidity=%.1f", temperature, humidity);

    if (http.begin(client, influxURL)) {  // HTTP

        http.addHeader("Authorization", String("Token ") + token);
        int httpCode = http.POST(String(data));

        if (httpCode > 0) {                // httpCode will be negative on error
            Serial.printf("[HTTP] POST... code: %d\n", httpCode);
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = http.getString();
                Serial.println(payload);
            }
        } else {
            Serial.printf("[HTTP] POST... failed, HTTP Code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
        }
        http.end();
    } else {
        Serial.printf("[HTTP} Unable to connect\n");
    }

    delay(interval);
}
