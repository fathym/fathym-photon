#include "Fathym.h"

// Handler that receives MQTT messages for this device
Fathym * instance = NULL;

void mqttReceiveHandler(char * topic, byte * payload, unsigned int length) {
  if (instance != NULL) {
      instance->receive(topic, payload, length);
  }
}

// Constructor
Fathym::Fathym() {
  init("", FATHYM_DEFAULT_PORT, "", "");
}

// Global initialization
void Fathym::init(char * server, uint16_t port, char * username, char * password) {
  instance = this;

  // Connection
  _server = server;
  _port = port;
  _username = username;
  _password = password;
  _lastTimeSync = 0;
  _keepAlive = MQTT_KEEPALIVE;
  _subscribed = false;
  _error = ERROR_NONE;

  // Publishing rate
  setPublishRate(FATHYM_PUBLISH_RATE); // this makes sure the keep alive is greater than publish rate

  // Setup DEBUG LED pin if configured
  if (FATHYM_USE_DEBUG_LED) {
    pinMode(FATHYM_DEBUG_LED_PIN, OUTPUT);
  }

  // Setup internal JSON buffer
  _json = &_jsonBuffer.createObject();
  JsonObject & json = *_json;

  // Assign the Photon's device ID for the ID in published messages
  _id = System.deviceID();
  json[FATHYM_ID_PROPERTY] = _id.c_str();

  // Set the send/receive topics using the device ID
  _sendTopic = String("fathym.devices.from.") + _id;
  _receiveTopic = String("fathym.devices.to.") + _id;

  // If adding a time stamp, setup time zone
  if (FATHYM_ADD_TIMESTAMP) {
    Time.zone(FATHYM_TIMEZONE_OFFSET);
    time_t time = Time.now();
    _timeStamp = Time.format(time, TIME_FORMAT_ISO8601_FULL);
  }

  // If configured to add the device's name make an entry into the message
  if (FATHYM_ADD_DEVICE_NAME) {
    json[FATHYM_DEVICE_NAME_PROPERTY] = _name.c_str();
    _name == NULL;
  }

  // Initialize storage
  //_flash = Devices::createDefaultStore();
}

// Handler that retrieves the device's name from the cloud
void Fathym::nameHandler(const char * topic, const char * data) {
  _name = String(data);
}

// Starts the Fathym library (should be called in setup() function of your main project file)
void Fathym::setup(void) {
  // If configured to use batteries, set it up
  #ifdef FATHYM_USE_BATTERY_POWER
  lipo.begin(); // start up the battery monitor
  lipo.quickStart(); // recalibrate for battery SoC
  #endif
}

// Begins a Fathym message update cycle; performs connection maintenance and prepares the connection for publishing.
void Fathym::beginUpdate(void) {
  // If the error state exists is non-critical, clear it
  if (_error != ERROR_NONE && _error < ERROR_CRITICAL) {
    _error = ERROR_NONE;
  }

  // Get the current device uptime
  unsigned long uptime = millis();

  // Record the time at which the update began
  _lastBeginUpdate = uptime;

  // If it is time to resync device time to cloud network time, do so...
  if ((uptime - _lastTimeSync) / 60000 >= FATHYM_RESYNC_TIME_MINS) {
    Particle.syncTime();
    _lastTimeSync = uptime;
  }

  // If we're using the device name, but don't have it yet, attempt to retrive the name
  if (FATHYM_ADD_DEVICE_NAME && _name == NULL) {
    while (_name == NULL) {
      flash(3, 33);
      // Subscribe to receive device name from Particle cloud
      Particle.subscribe("spark/", &Fathym::nameHandler, this);
      Particle.publish("spark/device/name");
      delay(2000); // wait a little bit to receive the name
    }
  }

  // If connected, update the underlying MQTT client message processing
  if (isConnected()) {
    // Subscribe to receive messages
    if (!_subscribed) {
        _subscribed = _mqtt->subscribe(_receiveTopic);
    }
  }
  // Otherwise attempt to reconnect
  else {
    reconnect();
  }
}

// Ends a Fathym message update cycle.
void Fathym::endUpdate(void) {
  if (FATHYM_AUTO_PUBLISH) {
    // Publish the current message data
    publish();

    // Start with the ideal update delay in milliseconds
    unsigned long updateDelay = _publishRate * 1000; // convert from seconds to milliseconds

    // Get current time
    unsigned long now = millis();

    // Get the delta time between the beginning of the update and now
    long deltaTime = now - _lastBeginUpdate;

    // Adjust the ideal delay by the delta time compensated amount
    updateDelay -= deltaTime;
    if (updateDelay < 0) updateDelay = 0;

    // Set the target time to delay until before next publish
    unsigned long targetTime = now + updateDelay;
    unsigned long lastTime = now;

    // Wait until the next publish time but split it into chunks
    // so MQTT communications can update throughout.
    while (lastTime <= targetTime) {
      // Update MQTT communications
      int mqttMsgPerUpdate = MQTT_MESSAGES_PER_UPDATE;
      if (mqttMsgPerUpdate < 0) mqttMsgPerUpdate = 1; // make sure there is a positive number

      // Update communications and consume messages
      for (int i = 0; i < mqttMsgPerUpdate; i++) {
          _mqtt->loop();
      }

      // Get the current time
      now = millis();

      // Use whichever delay has the smallest increment
      long mqttUpdateRate = MQTT_UPDATE_RATE;
      if (mqttUpdateRate < 0) mqttUpdateRate = 1000; // make sure there is a positive update rate and if not, default to 1 second
      long pubDelay = updateDelay < mqttUpdateRate ? updateDelay : mqttUpdateRate;

      // Compensate for overdelay past the target delay time
      unsigned long nextTime = now + pubDelay;

      // If the time target given the next delay time overshoots...
      if (nextTime > targetTime) {
        // Substract by the overage to even it back out
        pubDelay -= nextTime - targetTime;
        if (pubDelay < 0) pubDelay = 0;
      }

      // Delay until next loop
      if (pubDelay > 0) delay((unsigned long)pubDelay);

      // Update the last time to the current time
      lastTime = millis();
    }
  }
}

// Connects to the given message broker/server on the given port using the provided username and password.
bool Fathym::connect(char * server, char * username, char * password) {
  return connect(server, FATHYM_DEFAULT_PORT, username, password);
}

// Connects to the given message broker/server on the given port using the provided username and password.
bool Fathym::connect(char * server, uint16_t port, char * username, char * password)
{
  _server = server;
  _port = port;
  _username = username;
  _password = password;

  // If the MQTT client hasn't been created yet, create it
  if (_mqtt == NULL) {
    _mqtt = new MQTT(_server, _port, mqttReceiveHandler);
    _mqtt->setKeepAlive(_keepAlive);
  }

  // Connect using the MQTT client
  _mqtt->connect(_server, _username, _password);

  // Check for a valid connection state and report accordingly
  if (_mqtt->isConnected()) {
    // Subscribe to receive messages
    if (!_subscribed) {
        _subscribed = _mqtt->subscribe(_receiveTopic);
    }

    flash(8, 50);
    return true;
  }
  else {
    _subscribed = false;
    flash(8, 500);
    return false;
  }
}

// Reconnects to the last known connection.
bool Fathym::reconnect(void) {
  _subscribed = false;
  flash(4, 250);
  return connect(_server, _port, _username, _password);
}

// Determines whether or not Fathym is currently connected to the configured message broker.
bool Fathym::isConnected(void) {
  if (_mqtt == NULL) return false;
  return _mqtt->isConnected();
}

// Sets the MQTT connection keep alive time in seconds
void Fathym::setKeepAlive(uint16_t seconds) {
  // No change, nothing to do
  if (_keepAlive == seconds) return;

  // Set local keep alive value
  _keepAlive = seconds;

  // Enforce a minimum value for keep alive
  if (_keepAlive < 5) _keepAlive = 5;

  // MQTT object is not initialized yet
  if (_mqtt == NULL) return;

  // Set keep alive on underlying MQTT instance
  _mqtt->setKeepAlive(_keepAlive);

  // Reconnect after keep alive update to initiate new keep alive with broker
  reconnect();
}

// Sets the publishing rate in seconds when auto-publishing is enabled.
void Fathym::setPublishRate(uint16_t seconds) {
  if (_publishRate == seconds) return; // no change, nothing to do

  // Update the rate
  _publishRate = seconds;

  // Make sure that the MQTT keep alive time is greater than the update cycle
  // otherwise the connection will continuously time out after one publish
  if (_keepAlive <= _publishRate) {
    setKeepAlive(_publishRate + (_publishRate / 2)); // keep alive is 1.5 times the publish rate
  }
}

// Publishes a raw mesage payload to the connected message broker/server on the given topic.
bool Fathym::publishRaw(const char * topic, const char * payload) {
  if (!isConnected()) {
    return false;
  }

  bool success = _mqtt->publish(topic, payload);

  // If we're using LED pin debugging and the publish was successful flash the LED to indicate a publish
  if (FATHYM_DEBUG_SHOW_PUBLISH && success) {
    digitalWrite(FATHYM_DEBUG_LED_PIN, HIGH);
    delay(FATHYM_DEBUG_PUBLISH_DELAY);
    digitalWrite(FATHYM_DEBUG_LED_PIN, LOW);
  }

  return success;
}

// Publish the current message data to the connected message broker/server
bool Fathym::publish(void) {
  return publish(_sendTopic); // publish to the default send topic for the device
}

// Publish the current message data to the connected message broker/server
bool Fathym::publish(const char * topic) {
  if (!isConnected()) {
    return false;
  }

  JsonObject & json = *_json;
  const char * payload;

  // If there is no current error state, publish data
  if (_error == ERROR_NONE) {
    // If configured to add the device's cloud name, include it
    if (FATHYM_ADD_DEVICE_NAME) {
      json[FATHYM_DEVICE_NAME_PROPERTY] = _name.c_str();
    }

    // If set to include the device uptime, include it
    if (FATHYM_ADD_UPTIME) {
      json[FATHYM_UPTIME_PROPERTY] = millis();
      //set(FATHYM_UPTIME_PROPERTY, (long)millis(), "ms");
    }

    // If set to include device free memory, include it
    if (FATHYM_ADD_FREE_MEMORY) {
      json[FATHYM_FREE_MEMORY_PROPERTY] = System.freeMemory();
      //set(FATHYM_FREE_MEMORY_PROPERTY, (int)System.freeMemory(), "bytes");
    }

    // Include battery information as configured
    #ifdef FATHYM_USE_BATTERY_POWER

    if (FATHYM_USE_BATTERY_POWER && FATHYM_MONITOR_BATTERY) {
        // If set to include battery voltage, include it
        if (FATHYM_ADD_BATTERY_VOLTAGE) {
          json[FATHYM_BATTERY_VOLTAGE_PROPERTY] = lipo.getVoltage();
          //set(FATHYM_BATTERY_VOLTAGE_PROPERTY, (float)lipo.getVoltage(), "v");
        }

        // If set to include battery charge level, include it
        if (FATHYM_ADD_BATTERY_CHARGE) {
          json[FATHYM_BATTERY_CHARGE_PROPERTY] = lipo.getSOC();
          //set(FATHYM_BATTERY_CHARGE_PROPERTY, (float)lipo.getSOC(), "%");
        }
    }

    #endif // FATHYM_USE_BATTERY_POWER

    // If set to use time stamp, add the current time stamp
    if (FATHYM_ADD_TIMESTAMP) {
      time_t time = Time.now();
      _timeStamp = Time.format(time, TIME_FORMAT_ISO8601_FULL);
      json[FATHYM_TIMESTAMP_PROPERTY] = _timeStamp.c_str();
    }

    // Serialize current message values
    size_t maxDataSize = MQTT_MAX_PACKET_SIZE - MQTT_MAX_HEADER_SIZE; // leave some size for the MQTT header
    char buffer[maxDataSize]; // create a buffer of the max payload size
    payload = buffer;
    size_t written = json.printTo(buffer, maxDataSize);

    // Check to see that the JSON object is properly terminated
    if (buffer[written - 1] != '}') {
      _error = ERROR_JSON_BUFFER_MAX;
    }
  }

  // If there is an error, send an error message payload instead with the error code
  if (_error != ERROR_NONE) {
    _errorJson = String("{\"" + _idProp + "\":\"" + _id + "\",\"error\":");
    _errorJson.concat(_error);
    _errorJson.concat("}");
    payload = _errorJson.c_str();
  }

  // Publish to the given topic on the connected message broker/server
  bool success = _mqtt->publish(topic, payload);

  // If we're using LED pin debugging and the publish was successful flash the LED to indicate a publish
  if (FATHYM_DEBUG_SHOW_PUBLISH && success) {
    digitalWrite(FATHYM_DEBUG_LED_PIN, HIGH);
    delay(FATHYM_DEBUG_PUBLISH_DELAY);
    digitalWrite(FATHYM_DEBUG_LED_PIN, LOW);

    // If there is an error state, flash twice to indicate that
    if (_error != ERROR_NONE) {
      flash(4, 75);
    }
  }

  return success;
}

// Receives an MQTT message
void Fathym::receive(char * topic, byte * payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  DynamicJsonBuffer rxBuffer;
  JsonObject & msg = rxBuffer.parseObject(p);

  if (msg.success()) {
    // TODO Process commands from server
  }
}

// Flashes the debug LED pin a given number of flashes with a given millisecond delay between high/low
void Fathym::flash(uint8_t numFlashes, uint8_t delayMs) {
  if (!FATHYM_USE_DEBUG_LED) return;

  for (int i = 0; i < numFlashes; i++) {
    digitalWrite(FATHYM_DEBUG_LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(FATHYM_DEBUG_LED_PIN, LOW);
    delay(delayMs);
  }
}

// Remove a value entry from the message
void Fathym::remove(const char * name) {
  JsonObject & json = *_json;
  json.remove(name);
}

// Sets a boolean message value
void Fathym::set(const char * name, bool value) {
  JsonObject & json = *_json;
  json[name] = value;
}

// Sets a string message value
void Fathym::set(const char * name, const char * value) {
  JsonObject & json = *_json;
  json[name] = value;
}

// Sets a float message value
void Fathym::set(const char * name, float value) {
  set(name, value, FATHYM_DEFAULT_DECIMAL_PLACES);
}

// Sets a float message value and determines the number of decimal places to include
void Fathym::set(const char * name, float value, uint8_t decimals) {
  JsonObject & json = *_json;
  json[name].set(value, decimals);
}

// Sets a double message value
void Fathym::set(const char * name, double value) {
  set(name, value, FATHYM_DEFAULT_DECIMAL_PLACES);
}

// Sets a double message value and determines the number of decimal places to include
void Fathym::set(const char * name, double value, uint8_t decimals) {
  JsonObject & json = *_json;
  json[name].set(value, decimals);
}

// Sets an int message value
void Fathym::set(const char * name, int value) {
  JsonObject & json = *_json;
  json[name] = value;
}

// Sets a long message value
void Fathym::set(const char * name, long value) {
  JsonObject & json = *_json;
  json[name] = value;
}

// Sets a float message value with the associated units
void Fathym::set(const char * name, float value, const char * units) {
  set(name, value, units, FATHYM_DEFAULT_DECIMAL_PLACES);
}

// Sets a float message value with the associated units and determines the number of decimal places to include
void Fathym::set(const char * name, float value, const char * units, uint8_t decimals) {
  JsonObject & json = *_json;

  // Get the nested object for the value/units
  JsonObject & nestedObj =  json[name];

  // Create it if it doesn't exist
  if (nestedObj == JsonObject::invalid()) {
    JsonObject & newNested = json.createNestedObject(name);
    newNested["value"].set(value, decimals);
    newNested["units"] = units;
  }
  // Otherwise update the current object
  else {
    nestedObj["value"].set(value, decimals);
    nestedObj["units"] = units;
  }
}

// Sets a double message value with the associated units
void Fathym::set(const char * name, double value, const char * units) {
  set(name, value, units, FATHYM_DEFAULT_DECIMAL_PLACES);
}

// Sets a double message value with the associated unit sand determines the number of decimal places to include
void Fathym::set(const char * name, double value, const char * units, uint8_t decimals) {
  JsonObject & json = *_json;

  // Get the nested object for the value/units
  JsonObject & nestedObj =  json[name];

  // Create it if it doesn't exist
  if (nestedObj == JsonObject::invalid()) {
    JsonObject & newNested = json.createNestedObject(name);
    newNested["value"].set(value, decimals);
    newNested["units"] = units;
  }
  // Otherwise update the current object
  else {
    nestedObj["value"].set(value, decimals);
    nestedObj["units"] = units;
  }
}

// Sets a int message value with the associated units
void Fathym::set(const char * name, int value, const char * units) {
  JsonObject & json = *_json;

  // Get the nested object for the value/units
  JsonObject & nestedObj =  json[name];

  // Create it if it doesn't exist
  if (nestedObj == JsonObject::invalid()) {
    JsonObject & newNested = json.createNestedObject(name);
    newNested["value"] = value;
    newNested["units"] = units;
  }
  // Otherwise update the current object
  else {
    nestedObj["value"] = value;
    nestedObj["units"] = units;
  }
}

// Sets a long message value with the associated units
void Fathym::set(const char * name, long value, const char * units) {
  JsonObject & json = *_json;

  // Get the nested object for the value/units
  JsonObject & nestedObj =  json[name];

  // Create it if it doesn't exist
  if (nestedObj == JsonObject::invalid()) {
    JsonObject & newNested = json.createNestedObject(name);
    newNested["value"] = value;
    newNested["units"] = units;
  }
  // Otherwise update the current object
  else {
    nestedObj["value"] = value;
    nestedObj["units"] = units;
  }
}

// Prints the current fathym JSON data to the serial port for debugging
void Fathym::printJson(void) {
  JsonObject & json = *_json;
  json.printTo(Serial);
  Serial.println();
}
