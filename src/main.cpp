
#include "main.h"

String clientId = "BlenderRodriguez";

unsigned long lastReconnectAttempt = 0;

Adafruit_MCP23X17 mcp1;

// mqtt vars
float waterCurrentPH;

bool channel[4]; // stop je waarden in een array

unsigned int dose[5]; // stop je waarden in een array

String whenDose;    // dateTime (when to dose?)
String lastDose;    // dateTime
String currentTime; // dateTime*/

extDcMotor *motors[4]; // hier stop ik alle motoren in, dat bespaard vier keer dezelfde code clusterfucken

String partTopic;

String byteToString(byte *_byte, unsigned _len)
{
  String _ret = "";

  for (unsigned int i = 0; i < _len; i++)
  {
    _ret += (char)_byte[i];
  }

  return _ret;
}

void callback(char *topic, byte *payload, unsigned int length) // payload is 1 of 0
{
  String _payload = byteToString(payload, length);
  Serial.println("MOTOR from topic: " + String(topic[partTopic.length() ]));
  unsigned _motorNumber = String(topic[partTopic.length()]).toInt();

  Serial.println("Message arrived [" + String(topic) + "]: " + _payload);
  Serial.println("MOTOR actuated: " + String(_motorNumber ));

  switch (_payload[0])
  {
  case '0':                         // off
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    Serial.println(F("\tChannel OFF: ") + String(_motorNumber ));
    MQTTclient.publish(String(partTopic + String(_motorNumber) + F("/state")).c_str(), "OFF");
    channel[_motorNumber] = false;
    motors[_motorNumber]->stop();
    break;
  case '1':                          // on
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off (Note that LOW is the voltage level
    Serial.println(F("\tChannel ON: ") + String(_motorNumber ));
    MQTTclient.publish(String(partTopic + String(_motorNumber) + F("/state")).c_str(), "ON");
    channel[_motorNumber] = true;
    motors[_motorNumber]->forward();
    break;
  }
}

void threadPublishCallback()
{
  float _CurrentPh = 0;

  // Serial.print(F("threadPublishCallback: ") + String(millis() / 1000));
  //
  // Serial.print(F("\tCurrentPh: ") + String(_CurrentPh));
  MQTTclient.publish(String(F("sensors/") + clientId + F("/CurrentPh")).c_str(), String(_CurrentPh).c_str());

  // JSON Payload

  String _payload = "{\"clientId\":";
  _payload += (String)clientId;

  _payload += ",\"CurrentPh\":";
  _payload += String(_CurrentPh);

  _payload += ",\"Ch1\":";
  _payload += String(channel[0]);

  _payload += ",\"Ch2\":";
  _payload += String(channel[1]);

  _payload += ",\"Ch3\":";
  _payload += String(channel[2]);

  _payload += ",\"Ch4\":";
  _payload += String(channel[3]);

  // Serial.println(F("\ttime: ") + String(timeClient.getEpochTime()));
  MQTTclient.publish(String(F("sensors/") + clientId + F("/debug/connected")).c_str(), String(timeClient.getEpochTime()).c_str());

  _payload += "}";

  MQTTclient.publish(String(F("sensors/") + clientId + F("/json")).c_str(), _payload.c_str());
}

void subscribeMQTT()  // Doe hier je subscriptions
{
  for (unsigned _i = 0; _i < 4; _i++)
  {
    MQTTclient.subscribe(String(partTopic + String(_i ) + F("/command")).c_str());
    // debugging subscrition topics...
    Serial.print(F("Subscribing to: "));
    Serial.println(String(partTopic + String(_i) + F("/command")).c_str());
  }
}

boolean reconnect() // Called when client is disconnected from the MQTT server
{
  if (MQTTclient.connect(clientId.c_str()))
  {
    timeClient.update(); // Is this the right time to update the time, when we lost connection?

    // Serial.print(F("reconnect: ") + String(millis() / 1000));
    // Serial.println(F("\ttime: ") + String(timeClient.getEpochTime()));
    MQTTclient.publish(String(F("sensors/") + clientId + F("/debug/connected")).c_str(), String(timeClient.getEpochTime()).c_str());

    subscribeMQTT();

    threadPublishCallback(); // Publish after reconnecting, probably not necesary with small LOGPERIOD
  }

  return MQTTclient.connected();
}

String generateClientIdFromMac() // Convert the WiFi MAC address to String
{
  byte _mac[6];
  String _output = "";

  WiFi.macAddress(_mac);

  for (int _i = 5; _i > 0; _i--)
  {
    _output += String(_mac[_i], HEX);
  }

  return _output;
}

void setup()
{
  Serial.begin(115200);
  // Serial.print("SDA: ");Serial.print(SDA);
  // Serial.print(" SCL: ");Serial.println(SCL);
  Serial.println("Connecting to: " + (String)ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  clientId += F("-") + generateClientIdFromMac();
  partTopic = String(F("actuators/") + clientId + F("/Ch"));

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.println("WiFi connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.setCallback(callback);

  Wire.begin(SDA, SCL);

  if (!mcp1.begin_I2C(mcp1Address))
  {
    Serial.println(F("MCPERR"));

    while (1)
    {
    }
  }
  else
  {
    for (unsigned _i = 0; _i < 4; _i++)
    {
      Serial.println("Setup channel: " + String(_i));

      motors[_i] = new extDcMotor(_i * 2, _i * 2 + 1 ); // dit zijn je motoren!!!
      motors[_i]->begin(&mcp1);
    }
  }

  threadPublish.enabled = true;
  threadPublish.setInterval(LOGPERIOD);
  threadPublish.onRun(threadPublishCallback);
}

void loop()
{
  if (!MQTTclient.connected())
  {
    if (millis() - lastReconnectAttempt >= 5000) // try to reconnect every 5000 milliseconds
    {
      lastReconnectAttempt = millis();

      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    MQTTclient.loop();
  }
  // Run threads, this makes it all work on time!
  threadController.run();
}
