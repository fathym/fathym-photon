// Import Fathym library
#include "FathymBuild.h"
#include "Fathym/Fathym.h"

// Declare Fathym singleton instance
Fathym Fathym("server", "vhost:username", "password");

void setup() {
  // Connect to AMQP broker via underlying MQTT layer/plugin
  Fathym.connect();
}

void loop() {
  // Update Fathym connection
  Fathym.beginUpdate();

  // Set the JSON/sensor values to publish
  Fathym.setValue("lat", 39.978369);
  Fathym.setValue("long", -105.274364);
  Fathym.setValue("testInt", random(10, 4096));
  Fathym.setValue("testFloat",  (float)random(1, 100) / 1.5);
  Fathym.setValue("testBool", true);
  Fathym.setValue("intWithUnits", random(0, 120), "v");
  Fathym.setValue("floatWithUnits", (float)random(1, 10000) / 3.5, "m/s");

  // Publish the latest values to Fathym
  Fathym.endUpdate();
}
