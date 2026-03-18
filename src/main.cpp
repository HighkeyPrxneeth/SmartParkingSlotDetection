#include <WiFi.h>
#include <PubSubClient.h>

// ESP32 Control Pins for MUX
const int pinS0 = 2;
const int pinS1 = 4;
const int pinS2 = 5;
const int pinS3 = 18;

// Master ESP32 pins
const int trigMaster = 19;
const int echoMaster = 21;

const int NUM_PAIRS = 5;

// LED pins
const int ledPins[NUM_PAIRS] = {27, 26, 25, 33, 32};

const int THRESHOLD_CM = 50; // Distance to count as "High" (vehicle detected)

// Sensor pairs
const int pairs[NUM_PAIRS][2] = {
  {0, 1}, // Pair 0
  {2, 3}, // Pair 1
  {4, 5}, // Pair 2
  {6, 7}, // Pair 3
  {8, 9}  // Pair 4
};

bool turnedOn[NUM_PAIRS] = { 0 };

// Network and MQTT variables
const char* mqtt_server = "YOUR_MQTT_BROKER_IP"; // Change this to your MQTT broker's IP address
const char* mqtt_topic = "parking/slots";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200); // Baud rate for ESP32

  // Connect to WiFi
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Trying to connect to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  pinMode(pinS0, OUTPUT);
  pinMode(pinS1, OUTPUT);
  pinMode(pinS2, OUTPUT);
  pinMode(pinS3, OUTPUT);

  pinMode(trigMaster, OUTPUT);
  pinMode(echoMaster, INPUT);

  for (int i = 0; i < NUM_PAIRS; i++) {
	pinMode(ledPins[i], OUTPUT);
	digitalWrite(ledPins[i], LOW); // Start with LED off
  }

  client.setServer(mqtt_server, 1883); // Define the server
}

// Changes the sensor in focus
void selectMuxChannel(int channel) {
  // The mux used has 16 usable channels from which we are using 10
  // Each channel is has a 4 digit binary assigned to it
  // For example, 5 in binary is 0101
  // So we need to set S0 to 0, S1 to 1, S2 to 0 and S3 to 1
  // Then the pulses sent to the pins are switched to the sensor in that channel

  // bitRead(binary, position) will give the bit in a binary variable
  // at the mentioned position

  digitalWrite(pinS0, bitRead(channel, 0)); // 0th bit
  digitalWrite(pinS1, bitRead(channel, 1)); // 1st bit
  digitalWrite(pinS2, bitRead(channel, 2)); // 2nd bit
  digitalWrite(pinS3, bitRead(channel, 3)); // 3rd bit
  delayMicroseconds(10);
}

// Get the distance from the pairs of sensors
int getDistance(int sensorIndex) {
  selectMuxChannel(sensorIndex);

  digitalWrite(trigMaster, LOW); // LOW before starting the scan
  delayMicroseconds(2);
  digitalWrite(trigMaster, HIGH); // Send a beam for 10μs
  delayMicroseconds(10);
  digitalWrite(trigMaster, LOW); // Turn off the beam

  long duration = pulseIn(echoMaster, HIGH, 30000); // Checking if the beam reflected back

  if (duration == 0) return 999; // Return a huge number if nothing is detected

  return duration * 0.034 / 2; // Duration * Speed of sound / 2 (it is a bounce)
}

// Reconnection function for when the connection is broken
void reconnect() {
  while (!client.connected()) {
    Serial.println("Trying to reconnect...");

    if (client.connect("ESP32")) { // Any name works it seems
      Serial.println("Connected!");
    } else {
      Serial.println("Failed to reconnected, retrying in 5 seconds.");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) { // Ensure connection is alive
    reconnect();
  }

  client.loop(); // Run the connection loop in background

  // Run a loop to check state of every sensor pair
  for (int i = 0; i < NUM_PAIRS; i++) {
    int sensorA = pairs[i][0];
    int sensorB = pairs[i][1];

    // Read the first sensor
    int distA = getDistance(sensorA);
    delay(40); // Wait for acoustic echoes to die down

    // Read the opposite sensor
    int distB = getDistance(sensorB);
    delay(40); // Wait before moving to the next pair

    // Check if both sensors detect something closer than the threshold
    if (distA < THRESHOLD_CM && distB < THRESHOLD_CM) {
      if (!turnedOn[i]) {
      Serial.print("Vehicle parked at slot ");
      Serial.println(i + 1);
      turnedOn[i] = true;

      // Send occupied state payload  
      String payload = "{\"slot_" + String(i+1) + "\": \"occupied\"}";
      client.publish(mqtt_topic, payload.c_str());
      }
      digitalWrite(ledPins[i], HIGH);
    } else {
      if (turnedOn[i]) {
        Serial.print("Vehicle left the slot ");
        Serial.println(i + 1);
        turnedOn[i] = false;

        // Send free state payload
        String payload = "{\"slot_" + String(i+1) + "\": \"free\"}";
        client.publish(mqtt_topic, payload.c_str());
      }
      digitalWrite(ledPins[i], LOW);
    }
  }
}