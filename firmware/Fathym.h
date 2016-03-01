/*
Fathym library for Particle Core & Photon
This software is released under the MIT License.

Copyright (c) 2016 Michael Everett
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _FATHYM
#define _FATHYM

// Load build configuration settings
//#include "FathymBuild.h" // uncomment for local Particle Dev build
#include "../FathymBuild.h"

// Standard Photon library
#include "application.h"

// MQTT library used for underlying message broker communication
#include "MQTT.h"

// If this is a local Particle Dev build, reference dependencies/libraries differently
#ifdef LOCAL_BUILD
// Used for JSON data communications
#include "SparkJson.h"
#else
#include "SparkJson/SparkJson.h"
#endif

// Used for access to device flash storage
// #ifdef LOCAL_BUILD
// #include "flashee-eeprom.h"
// #else
// #include "flashee-eeprom/flashee-eeprom.h"
// #endif
// using namespace Flashee;

// The name of the device ID property to use
#ifndef FATHYM_ID_PROPERTY
#define FATHYM_ID_PROPERTY "id"
#endif

// Whether or not to include the device's name
#ifndef FATHYM_ADD_DEVICE_NAME
#define FATHYM_ADD_DEVICE_NAME false
#endif

// The name of the device name property to use
#ifndef FATHYM_DEVICE_NAME_PROPERTY
#define FATHYM_DEVICE_NAME_PROPERTY "name"
#endif

// The default number of decimal places to include from numbers with decimal values
#ifndef FATHYM_DEFAULT_DECIMAL_PLACES
#define FATHYM_DEFAULT_DECIMAL_PLACES 3
#endif

// The interval in minutes that the device will resynchronize device time to Particle cloud time
#ifndef FATHYM_RESYNC_TIME_MINS
#define FATHYM_RESYNC_TIME_MINS 1440 // resync every 24 hours
#endif

// Whether or not to include the current uptime of the device
#ifndef FATHYM_ADD_UPTIME
#define FATHYM_ADD_UPTIME true
#endif

// The name of the device uptime property to use
#ifndef FATHYM_UPTIME_PROPERTY
#define FATHYM_UPTIME_PROPERTY "ut"
#endif

// Whether or not to include a time stamp on each published message
#ifndef FATHYM_ADD_TIMESTAMP
#define FATHYM_ADD_TIMESTAMP true
#endif

// The time zone offset to use when adding a time stamp to the published message
#ifndef FATHYM_TIMEZONE_OFFSET
#define FATHYM_TIMEZONE_OFFSET -7 // Colorado/Mountain time!
#endif

// The name of the time stamp property to use
#ifndef FATHYM_TIMESTAMP_PROPERTY
#define FATHYM_TIMESTAMP_PROPERTY "ts"
#endif

// Whether or not to include the device's free memory in the message for detecting memory leaks
#ifndef FATHYM_ADD_FREE_MEMORY
#define FATHYM_ADD_FREE_MEMORY false
#endif

// The name of the free memory property to use
#ifndef FATHYM_FREE_MEMORY_PROPERTY
#define FATHYM_FREE_MEMORY_PROPERTY "mem"
#endif

// Whether or not the Fathym library will automatically handle publishing data or not
#ifndef FATHYM_AUTO_PUBLISH
#define FATHYM_AUTO_PUBLISH true
#endif

// The publish rate (in seconds) for message data (applies when FATHYM_AUTO_PUBLISH is set to true)
#ifndef FATHYM_PUBLISH_RATE
#define FATHYM_PUBLISH_RATE 10
#endif

// Default to standard MQTT port
#ifndef FATHYM_DEFAULT_PORT
#define FATHYM_DEFAULT_PORT 1883
#endif

// The rate at which the MQTT communication loop updates in milliseconds.
// This includes ping/keep alive/QoS/receiving messages. It runs on a
// software timer independent of the main program loop.
#ifndef MQTT_UPDATE_RATE
#define MQTT_UPDATE_RATE 1000
#endif

// The number of times to run the MQTT communications update loop per
// iteration as defined by MQTT_UPDATE_RATE. The update loop will consume
// 1 waiting message per update per update cycle.
#ifndef MQTT_MESSAGES_PER_UPDATE
#define MQTT_MESSAGES_PER_UPDATE 10
#endif

// The number of seconds to use for the MQTT connection keep alive.
// The keep alive needs to be longer than your publish rate otherwise
// the connection will continuously time out/reconnect after one publish.
#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 20
#endif

// The maximum size of the MQTT header in bytes
#ifndef MQTT_MAX_HEADER_SIZE
#define MQTT_MAX_HEADER_SIZE 160
#endif

// The reserved buffer size in bytes for MQTT data packet buffer.
// The maximum buffer size available to serialize JSON messages to string
// is determined by the MQTT_MAX_PACKET_SIZE - MQTT_MAX_HEADER_SIZE.
#ifndef MQTT_MAX_PACKET_SIZE
#define MQTT_MAX_PACKET_SIZE 640
#endif

// Whether or not to use the defined debug pin for Fathym visual status debugging
#ifndef FATHYM_USE_DEBUG_LED
#define FATHYM_USE_DEBUG_LED true
#endif

// Default LED pin to use when debugging with LED status
#ifndef FATHYM_DEBUG_LED_PIN
#define FATHYM_DEBUG_LED_PIN D7
#endif

// Whether or not to use the debug LED (if it is enabled) to indicate a successful publish
#ifndef FATHYM_DEBUG_SHOW_PUBLISH
#define FATHYM_DEBUG_SHOW_PUBLISH true
#endif

// The number of milliseconds to flash the debug LED to indicate a successful publish
#ifndef FATHYM_DEBUG_PUBLISH_DELAY
#define FATHYM_DEBUG_PUBLISH_DELAY 20
#endif

// Device error states
#define ERROR_NONE            0
#define ERROR_JSON_BUFFER_MAX 1
#define ERROR_CRITICAL        128

//==== Battery Shield ===========================================================================
/* This section is optional if you're creating a battery powered design that uses the SparkFun
 * Photon Battery Shield (https://www.sparkfun.com/products/13626). If you want to enable the
 * battery features then in your FathymBuild.h file somewhere put: #define FATHYM_USE_BATTERY_POWER
 */

// Used for battery power
#ifdef FATHYM_USE_BATTERY_POWER

#ifdef LOCAL_BUILD
// Used for JSON data communications
#include "SparkFunMAX17043.h"
#else
#include "SparkFunMAX17043/SparkFunMAX17043.h"
#endif // end include

// Whether or not to report on battery voltage and charge level
#ifndef FATHYM_MONITOR_BATTERY
#define FATHYM_MONITOR_BATTERY true
#endif

// If monitoring battery power, whether or not to include the voltage level
#ifndef FATHYM_ADD_BATTERY_VOLTAGE
#define FATHYM_ADD_BATTERY_VOLTAGE true
#endif

// The name of the battery voltage memory property to use
#ifndef FATHYM_BATTERY_VOLTAGE_PROPERTY
#define FATHYM_BATTERY_VOLTAGE_PROPERTY "batV"
#endif

// If monitoring battery power, whether or not to include the charge level
#ifndef FATHYM_ADD_BATTERY_CHARGE
#define FATHYM_ADD_BATTERY_CHARGE true
#endif

// The name of the battery voltage memory property to use
#ifndef FATHYM_BATTERY_CHARGE_PROPERTY
#define FATHYM_BATTERY_CHARGE_PROPERTY "batC"
#endif

#endif // end FATHYM_USE_BATTERY_POWER

// Fathym API class
class Fathym;
typedef void (Fathym::*MQTT_HANDLER)(const char * payload);

// Main Fathym class that provides singleton-like access to the API
class Fathym {
public:
  // Constructors
  Fathym();

  // Event Handlers
  void nameHandler(const char * topic, const char * data); // used to retrieve device name

  // Device
  void setup(void);

  // Connection
  void beginUpdate(void);
  void endUpdate(void);
  bool connect(char * server, char * username, char * password);
  bool connect(char * server, uint16_t port, char * username, char * password);
  bool isConnected(void);
  void setKeepAlive(uint16_t seconds);

  // Message
  void setPublishRate(uint16_t seconds);
  bool publishRaw(const char * topic, const char * payload);
  bool publish(void);
  bool publish(const char * topic);
  void remove(const char * name);
  void set(const char * name, bool value);
  void set(const char * name, const char * value);
  void set(const char * name, float value);
  void set(const char * name, float value, uint8_t decimals);
  void set(const char * name, double value);
  void set(const char * name, double value, uint8_t decimals);
  void set(const char * name, int value);
  void set(const char * name, long value);
  void set(const char * name, float value, const char * units);
  void set(const char * name, float value, const char * units, uint8_t decimals);
  void set(const char * name, double value, const char * units);
  void set(const char * name, double value, const char * units, uint8_t decimals);
  void set(const char * name, int value, const char * units);
  void set(const char * name, long value, const char * units);
  void printJson(void);
  void receive(char * topic, byte * payload, unsigned int length);

private:
  // Initialize
  void init(char * server, uint16_t port, char * username, char * password);

  // Device
  String _id; // stores the device's ID
  String _idProp = FATHYM_ID_PROPERTY; // the property name to use for the device ID
  String _name; // stores the device's name
  String _timeStamp; // stores the current timestamp string for the last publish
  uint16_t _publishRate; // rate (in seconds) at which auto-publishing occurs if it is enabled
  unsigned long _lastTimeSync; // used to resync to cloud network time to avoid local time drift
  unsigned long _lastBeginUpdate; // used to adjust delay compensation to attempt to regulate a more stable update/publish rate
  String _sendTopic; // the device-specific send topic
  String _receiveTopic; // the device-specific receive topic
  uint8_t _error; // used to indicate the error state of the device (if any)
  String _errorJson; // string used to send a device error message

  // Connection
  char * _server;
  uint16_t _port;
  char * _username;
  char * _password;
  MQTT * _mqtt;
  uint16_t _keepAlive;
  bool _subscribed;
  bool reconnect(void);

  // Storage
  //FlashDevice * _flash;

  // JSON
  DynamicJsonBuffer _jsonBuffer;
  JsonObject * _json;

  // Utility
  void flash(uint8_t numFlashes, uint8_t delayMs);
};

// Singleton instance to use
//extern Fathym fathym; // this is causing an error when building for some reason

#endif
