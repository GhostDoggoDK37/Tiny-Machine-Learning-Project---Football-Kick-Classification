/**
 * @file ADXL343_to_TCP_server.cpp
 * @author morten opprud
 * @brief
 * @version 0.1
 * @date 2024-12-16
 */

#include "Particle.h"
#include "adxl343.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler;

// --- Konstanter ---
const unsigned long MAX_RECORDING_LENGTH_MS = 8000;
const int SAMPLING_INTERVAL_MS = 2;
const int BUFFER_SIZE = 1024;
const int TRANSMIT_THRESHOLD = 800;

// --- Serverkonfiguration ---
IPAddress serverAddr = IPAddress(192, 168, 185, 25);
int serverPort = 7123;
TCPClient client;

// --- Buffere ---
struct Sample {
    uint32_t timestamp;
    float x, y, z;
};

Sample buffer1[BUFFER_SIZE];
Sample buffer2[BUFFER_SIZE];
Sample *samplingBuffer = buffer1;
Sample *transmitBuffer = buffer2;
volatile int samplingIndex = 0;
volatile bool bufferReady = false;

// --- Accelerometer ---
ADXL343 accelerometer;

// --- Prototyper ---
void sampleAccelerometer();
void transmitBufferData(int samples);
void buttonHandler(system_event_t event, int data);

// --- Timer ---
Timer samplingTimer(SAMPLING_INTERVAL_MS, sampleAccelerometer);

// --- State machine ---
enum State {
    STATE_WAITING,
    STATE_CONNECT,
    STATE_RUNNING,
    STATE_FINISH
};
State state = STATE_WAITING;

unsigned long recordingStart = 0;
unsigned long lastSampleTime = 0;

#define LED_PIN D7

// --- Setup ---
void setup() {
    Log.info("System initializing...");
    Particle.connect();
    System.on(button_click, buttonHandler);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Log.info("Initializing ADXL343...");
    if (!accelerometer.begin()) {
        Log.error("Failed to initialize ADXL343! Resetting system.");
        System.reset();
    }
    Log.info("ADXL343 initialized successfully.");
    Log.info("Waiting for button press to start data collection...");
}

// --- Loop ---
void loop() {
    switch (state) {

    case STATE_WAITING:
        Log.trace("STATE_WAITING");
        break;

    case STATE_CONNECT:
        Log.error("STATE_CONNECT: Attempting connection to server...");
        if (client.connect(serverAddr, serverPort)) {
            Log.info("Connected to server %s:%d", serverAddr.toString().c_str(), serverPort);
            Log.info("Starting data collection at %d Hz", 1000 / SAMPLING_INTERVAL_MS);
            recordingStart = millis();
            samplingIndex = 0;
            samplingTimer.start();
            digitalWrite(LED_PIN, HIGH);
            state = STATE_RUNNING;
        } else {
            Log.error("Failed to connect to server %s:%d", serverAddr.toString().c_str(), serverPort);
            state = STATE_WAITING;
        }
        break;

    case STATE_RUNNING: {
        Log.trace("STATE_RUNNING");
        static unsigned long lastReconnectAttempt = 0;
        const unsigned long reconnectInterval = 2000;

        // --- Wi-Fi failsafe ---
        if (!WiFi.ready()) {
            Log.warn("Wi-Fi lost, attempting reconnect...");
            WiFi.connect();
        }

        // --- TCP failsafe ---
        if (!client.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt >= reconnectInterval) {
                lastReconnectAttempt = now;
                Log.warn("TCP disconnected. Reconnecting...");
                client.stop();
                if (client.connect(serverAddr, serverPort)) {
                    Log.info("TCP reconnected successfully.");
                } else {
                    Log.error("TCP reconnect attempt failed.");
                }
            }
            break; // spring resten over mens vi prøver at genetablere
        }

        // --- Normal drift ---
        // Log.info("Running. Samples collected: %d", samplingIndex);
        if (bufferReady) {
            Log.trace("Buffer ready. Preparing to transmit.");
            int samplesToTransmit = samplingIndex;
            samplingIndex = 0;
            bufferReady = false;

            Sample *temp = samplingBuffer;
            samplingBuffer = transmitBuffer;
            transmitBuffer = temp;

            transmitBufferData(samplesToTransmit);
            Log.info("Transmitted %d samples.", samplesToTransmit);
        }

        // --- Timeout check ---
        if (millis() - recordingStart >= MAX_RECORDING_LENGTH_MS) {
            Log.info("Maximum recording length reached (%lu ms). Switching to STATE_FINISH.",
                     millis() - recordingStart);
            state = STATE_FINISH;
        }

        break;
    }

    case STATE_FINISH:
        Log.trace("STATE_FINISH");
        samplingTimer.stop();
        Log.info("Finishing state reached. Stopping data collection.");
        if (samplingIndex > 0) {
            Log.info("Transmitting remaining %d samples before shutdown.", samplingIndex);
            bufferReady = true;
            transmitBufferData(samplingIndex);
        }
        client.stop();
        digitalWrite(LED_PIN, LOW);
        Log.info("Data collection complete. Returning to STATE_WAITING.");
        state = STATE_WAITING;
        break;
    }
}

// --- Sampling ---
void sampleAccelerometer() {
    if (samplingIndex < BUFFER_SIZE) {
        float x, y, z;
        accelerometer.readAccelerationG(&x, &y, &z);

        samplingBuffer[samplingIndex].timestamp = micros();
        samplingBuffer[samplingIndex].x = x;
        samplingBuffer[samplingIndex].y = y;
        samplingBuffer[samplingIndex].z = z;

        samplingIndex++;
        lastSampleTime = millis();

        if (samplingIndex >= TRANSMIT_THRESHOLD) {
            bufferReady = true;
            Log.trace("Buffer threshold reached (%d samples).", samplingIndex);
        }
    }
}

// --- Transmission ---
void transmitBufferData(int samples) {
    if (samples == 0) {
        Log.warn("Transmit called with empty buffer. Skipping transmission.");
        return;
    }

    Log.trace("Transmitting %d samples...", samples);
    for (int i = 0; i < samples; i++) {
        String data = String::format(
            "%lu,%.2f,%.2f,%.2f\n",
            transmitBuffer[i].timestamp,
            transmitBuffer[i].x,
            transmitBuffer[i].y,
            transmitBuffer[i].z);

        int written = client.write((const uint8_t *)data.c_str(), data.length());
        if (written != data.length()) {
            Log.warn("Partial write: %d of %d bytes (sample %d).", written, data.length(), i);
        }
    }
    Log.info("Buffer transmission complete.");
}

// --- Button handler ---
void buttonHandler(system_event_t event, int data) {
    Log.error("Button pressed. Current state: %d", state);
    switch (state) {
    case STATE_WAITING:
        if (WiFi.ready()) {
            Log.error("Wi-Fi ready. Moving to STATE_CONNECT.");
            state = STATE_CONNECT;
        } else {
            Log.error("Wi-Fi not ready. Cannot start data collection.");
        }
        break;

    case STATE_RUNNING:
        Log.error("Stopping measurement manually. Moving to STATE_FINISH.");
        state = STATE_FINISH;
        break;
    }
}


#if 0
#include "Particle.h"
#include "adxl343.h" // Include the ADXL343 driver

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler;

// Constants
const unsigned long MAX_RECORDING_LENGTH_MS = 8000; // 8000;
const int SAMPLING_INTERVAL_MS = 2;                 // Sampling interval in milliseconds
const int BUFFER_SIZE = 25;                          // Buffer size
const int TRANSMIT_THRESHOLD = 20;                   // Transmit buffer after 20 samples

// Server configuration
IPAddress serverAddr = IPAddress(192, 168, 35, 25);
int serverPort = 7123;

TCPClient client;

// Buffers for double buffering
struct Sample
{
  uint32_t timestamp; // Timestamp in microseconds
  float x, y, z;      // Accelerometer data
};

Sample buffer1[BUFFER_SIZE];
Sample buffer2[BUFFER_SIZE];
Sample *samplingBuffer = buffer1;
Sample *transmitBuffer = buffer2;
volatile int samplingIndex = 0;
volatile bool bufferReady = false;

// ADXL343 accelerometer
ADXL343 accelerometer;

void sampleAccelerometer();
void transmitBufferData();

// Timer for sampling
Timer samplingTimer(SAMPLING_INTERVAL_MS, sampleAccelerometer);

// State machine
enum State
{
  STATE_WAITING,
  STATE_CONNECT,
  STATE_RUNNING,
  STATE_FINISH
};
State state = STATE_WAITING;

unsigned long recordingStart = 0;
unsigned long lastSampleTime = 0;

#define LED_PIN D7

void buttonHandler(system_event_t event, int data);
void transmitBufferData(int samples);

void setup()
{
  Particle.connect();
  System.on(button_click, buttonHandler);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Log.info("Initializing.");

  if (!accelerometer.begin())
  {
    Log.error("Failed to initialize ADXL343!");
    System.reset();
  }

  Log.info("ADXL343 initialized. Waiting for button press...");
}

void loop()
{
  switch (state)
  {
  case STATE_WAITING:
    break;

  case STATE_CONNECT:
    if (client.connect(serverAddr, serverPort))
    {
      Log.info("Connected to server. Starting data collection...");
      Log.info("Sample rate: %d Hz", 1000 / SAMPLING_INTERVAL_MS);
      recordingStart = millis();
      samplingIndex = 0;
      samplingTimer.start();
      digitalWrite(LED_PIN, HIGH);
      state = STATE_RUNNING;
    }
    else
    {
      Log.error("Failed to connect to server.");
      state = STATE_WAITING;
    }
    break;

  case STATE_RUNNING: {   
    static unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectInterval = 2000; // prøv hvert 2. sekund

    // --- Wi-Fi failsafe ---
    if (!WiFi.ready()) {
        Log.warn("Wi-Fi lost, attempting reconnect...");
        WiFi.connect();
    }

    // --- TCP failsafe ---
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= reconnectInterval) {
            lastReconnectAttempt = now;
            Log.warn("TCP disconnected. Reconnecting...");
            client.stop();
            if (client.connect(serverAddr, serverPort)) {
                Log.info("TCP reconnected to server.");
            } else {
                Log.error("Reconnect attempt failed.");
            }
        }
        break; // spring resten over mens vi prøver at genetablere
    }

    // --- Normal drift / bufferhåndtering ---
    Log.info("Running. Samples collected: %d", samplingIndex);
    if (bufferReady) {
        int samplesToTransmit = samplingIndex;  // snapshot af aktuelt index
        samplingIndex = 0;                      // nulstil index
        bufferReady = false;

        // Byt buffere før transmission
        Sample *temp = samplingBuffer;
        samplingBuffer = transmitBuffer;
        transmitBuffer = temp;

        // Send forrige buffer
        transmitBufferData(samplesToTransmit);
        Log.info("Transmitted %d samples.", samplesToTransmit);
    }

    // --- Stop efter timeout ---
    if (millis() - recordingStart >= MAX_RECORDING_LENGTH_MS) {
        state = STATE_FINISH;
        Log.info("Maximum recording length reached. Going to finish state.");
    }

    break;  // <-- vigtigt: break inde i blokken
}           // <-- lukker blokscope korrekt

  case STATE_FINISH:
    samplingTimer.stop();
    Log.info("Finishing state reached. Stopping data collection.");
    if (samplingIndex > 0)
    {
      bufferReady = true;
      transmitBufferData(samplingIndex);
    }
    client.stop();
    digitalWrite(LED_PIN, LOW);
    Log.info("Data collection complete.");
    state = STATE_WAITING;
    break;
  }
}

void sampleAccelerometer()
{
  if (samplingIndex < BUFFER_SIZE)
  {
    // Sample accelerometer
    float x, y, z;
    accelerometer.readAccelerationG(&x, &y, &z);

    samplingBuffer[samplingIndex].timestamp = micros();
    samplingBuffer[samplingIndex].x = x;
    samplingBuffer[samplingIndex].y = y;
    samplingBuffer[samplingIndex].z = z;

    samplingIndex++;
    lastSampleTime = millis();

    // Mark buffer as ready if threshold is met
    if (samplingIndex >= TRANSMIT_THRESHOLD)
    {
      bufferReady = true;
    }
  }
}

void transmitBufferData(int samples)
{
  if (samples == 0)
  {
    Log.warn("Transmit called with empty buffer. Skipping transmission.");
    return;
  }
  //Log.info("Transmitting %d samples...", samples);
  for (int i = 0; i < samples; i++)
  {
    String data = String::format(
        "%lu,%.2f,%.2f,%.2f\n",
        transmitBuffer[i].timestamp,
        transmitBuffer[i].x,
        transmitBuffer[i].y,
        transmitBuffer[i].z);
    client.write(((const uint8_t *)data.c_str()), data.length());
  }
  //Log.info("Buffer transmission complete.");
}

void buttonHandler(system_event_t event, int data)
{
  switch (state)
  {
  case STATE_WAITING:
    if (WiFi.ready())
    {
      state = STATE_CONNECT;
    }
    else
    {
      Log.warn("Wi-Fi not ready.");
    }
    break;

  case STATE_RUNNING:
    state = STATE_FINISH;
    break;
  }
}

#endif