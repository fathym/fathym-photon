// Tells the library that the current project is building via local Particle Dev
//#define LOCAL_BUILD true

// The name of the device ID property to use
#define FATHYM_ID_PROPERTY "id"

// Whether or not to include the device's name
#define FATHYM_ADD_DEVICE_NAME false

// The name of the device name property to use
#define FATHYM_DEVICE_NAME_PROPERTY "name"

// The default number of decimal places to include from numbers with decimal values
#define FATHYM_DEFAULT_DECIMAL_PLACES 6

// The interval in minutes that the device will resynchronize device time to Particle cloud time
#define FATHYM_RESYNC_TIME_MINS 1440 // resync every 24 hours

// Whether or not to include the current uptime of the device
#define FATHYM_ADD_UPTIME true

// The name of the device uptime property to use
#define FATHYM_UPTIME_PROPERTY "ut"

// Whether or not to include a time stamp on each published message
#define FATHYM_ADD_TIMESTAMP true

// The time zone offset to use when adding a time stamp to the published message
#define FATHYM_TIMEZONE_OFFSET -7 // Colorado/Mountain time!

// The name of the time stamp property to use
#define FATHYM_TIMESTAMP_PROPERTY "ts"

// Whether or not to include the device's free memory in the message for detecting memory leaks
#define FATHYM_ADD_FREE_MEMORY false

// The name of the free memory property to use
#define FATHYM_FREE_MEMORY_PROPERTY "mem"

// Whether or not the Fathym library will automatically handle publishing data or not
#define FATHYM_AUTO_PUBLISH true

// The publish rate (in seconds) for message data (applies when FATHYM_AUTO_PUBLISH is set to true)
#define FATHYM_PUBLISH_RATE 10

// The rate at which the MQTT communication loop updates in milliseconds.
// This includes ping/keep alive/QoS/receiving messages. It runs on a
// software timer independent of the main program loop.
#define MQTT_UPDATE_RATE 1000

// Default to standard MQTT port
#define MQTT_DEFAULT_PORT 1883

// The number of seconds to use for the MQTT connection keep alive.
// The keep alive needs to be longer than your publish rate otherwise
// the connection will continuously time out/reconnect after one publish.
#define MQTT_KEEPALIVE 20

// The maximum size of the MQTT header in bytes
#define MQTT_MAX_HEADER_SIZE 160

// The reserved buffer size in bytes for MQTT data packet buffer.
// The maximum buffer size available to serialize JSON messages to string
// is determined by the MQTT_MAX_PACKET_SIZE - MQTT_MAX_HEADER_SIZE.
#define MQTT_MAX_PACKET_SIZE 640

// Default topic on the message broker to publish messages to
#define FATHYM_DEFAULT_TOPIC ""

// Whether or not to use the defined debug pin for Fathym visual status debugging
#define FATHYM_USE_DEBUG_LED true

// Default LED pin to use when debugging with LED status
#define FATHYM_DEBUG_LED_PIN D7

// Whether or not to use the debug LED (if it is enabled) to indicate a successful publish
#define FATHYM_DEBUG_SHOW_PUBLISH true

// The number of milliseconds to flash the debug LED to indicate a successful publish
#define FATHYM_DEBUG_PUBLISH_DELAY 20
