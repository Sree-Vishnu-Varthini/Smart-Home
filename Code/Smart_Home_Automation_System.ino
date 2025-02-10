#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ThingSpeak.h>

// WiFi Credentials
const char* ssid = "wifi";   // Replace with your WiFi SSID
const char* password = "wifi";  // Replace with your WiFi Password

// ThingSpeak Credentials
const unsigned long CHANNEL_ID = YOUR_CHANNEL_ID;  // Replace with your ThingSpeak Channel ID
const char* API_KEY = "YOUR_API_KEY";  // Replace with your ThingSpeak API Key

WiFiClient client;

// Pin Definitions for Sensors
#define PIR_PIN 2         // PIR sensor connected to GPIO2 (J2_7)
#define FIRE_SENSOR_PIN 4 // Fire sensor connected to GPIO4 (J2_14)
#define LDR_PIN A0        // LDR sensor connected to ADC_CH0 (J10_2)
#define MQ135_PIN A1      // MQ-135 Air Quality sensor connected to ADC_CH1 (J10_4)
#define DHTPIN 15         // DHT11 Temperature & Humidity sensor connected to GPIO15 (J3_4)

// Pin Definitions for Relays
#define RELAY_MOTION 5    // Relay IN1 controls Motion-based activation (J2_10)
#define RELAY_LIGHT 6     // Relay IN2 controls Light based on LDR (J2_12)
#define RELAY_FIRE 7      // Relay IN3 controls Fan/Exhaust for Air Quality (J2_14)

// Buzzer Pin for Fire Alarm
#define BUZZER_PIN 14     // Buzzer connected to GPIO8 (J2_6)

// DHT Sensor Type
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

void setup() {
    Serial.begin(115200);  // Start Serial Monitor

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    ThingSpeak.begin(client);  

    // Initialize Sensors as Inputs
    pinMode(PIR_PIN, INPUT);
    pinMode(FIRE_SENSOR_PIN, INPUT);
    dht.begin();  // Start DHT11 sensor

    // Initialize Relay and Buzzer Pins as Outputs
    pinMode(RELAY_MOTION, OUTPUT);
    pinMode(RELAY_LIGHT, OUTPUT);
    pinMode(RELAY_FIRE, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);

    // Set initial state: Turn off relays and buzzer
    digitalWrite(RELAY_MOTION, HIGH);
    digitalWrite(RELAY_LIGHT, HIGH);
    digitalWrite(RELAY_FIRE, HIGH);
    digitalWrite(BUZZER_PIN, LOW);

    Serial.println("All Sensors, Relays & Buzzer Initialized...\n");
}

void loop() {
    Serial.println("---------- SENSOR READINGS ----------");

    // 🚶‍♂ PIR Sensor - Motion Detection
    int motionState = digitalRead(PIR_PIN);
    Serial.print("PIR Motion: ");
    Serial.println(motionState ? "🚶‍♂ Motion Detected!" : "No Motion");

    // Control Relay for Motion Detection (Turn ON if motion is detected)
    digitalWrite(RELAY_MOTION, motionState ? LOW : HIGH);
    Serial.println(motionState ? "🔴 Motion Relay ON" : "⚪ Motion Relay OFF");

    // 🔥 Fire Sensor - Flame Detection
    int fireState = digitalRead(FIRE_SENSOR_PIN);
    Serial.print("Flame Detection: ");
    Serial.println(fireState ? "No Fire" : "🔥 Fire Detected! 🔥");

    // Control Buzzer for Fire Alarm (ON when fire is detected)
    digitalWrite(BUZZER_PIN, fireState == LOW ? HIGH : LOW);

    // ☀ LDR Sensor - Light Intensity Measurement
    int lightValue = analogRead(LDR_PIN);
    float ldrVoltage = lightValue * (3.3 / 4095.0); // Convert ADC value to voltage
    Serial.print("Light Intensity: ");
    Serial.print(lightValue);
    Serial.print(" | Voltage: ");
    Serial.println(ldrVoltage);

    // Control Relay for Light (Turn ON when it's dark)
    digitalWrite(RELAY_LIGHT, lightValue > 1500 ? LOW : HIGH);
    Serial.println(lightValue > 1500 ? "💡 Light Relay ON (Dark)" : "⚪ Light Relay OFF (Bright)");

    // 🌫 MQ-135 Sensor - Air Quality Measurement
    int airQualityValue = analogRead(MQ135_PIN);
    float airQualityVoltage = airQualityValue * (3.3 / 4095.0);
    Serial.print("Air Quality (ADC Value): ");
    Serial.print(airQualityValue);
    Serial.print(" | Voltage: ");
    Serial.println(airQualityVoltage);

    // Control Fan/Exhaust Relay based on Air Quality
    if (airQualityValue > 1400) {  // Poor Air Quality
        Serial.println("⚠ Poor Air Quality! ⚠");
        digitalWrite(RELAY_FIRE, LOW); // Turn ON fan
        Serial.println("Fan Relay ON");
    } else if (airQualityValue > 700) {  // Moderate Air Quality
        Serial.println("😐 Moderate Air Quality.");
        digitalWrite(RELAY_FIRE, LOW); // Turn ON fan
        Serial.println("Fan Relay ON");
    } else {  // Good Air Quality
        Serial.println("😊 Good Air Quality.");
        digitalWrite(RELAY_FIRE, HIGH); // Turn OFF fan
        Serial.println("Fan Relay OFF");
    }

    // 🌡 DHT11 Sensor - Temperature & Humidity Measurement
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // Check if DHT11 sensor is working correctly
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("❌ Failed to read from DHT11 sensor!");
    } else {
        Serial.print("🌡 Temperature: ");
        Serial.print(temperature);
        Serial.print("°C | 💧 Humidity: ");
        Serial.print(humidity);
        Serial.println("%");
    }

    Serial.println("-------------------------------------\n");

    // Send Data to ThingSpeak
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, lightValue);
    ThingSpeak.setField(4, airQualityValue);
    ThingSpeak.setField(5, motionState);
    ThingSpeak.setField(6, fireState);

    int status = ThingSpeak.writeFields(CHANNEL_ID, API_KEY);

    delay(3000);  // Wait 3 seconds before next reading
}
