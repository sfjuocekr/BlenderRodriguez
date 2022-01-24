
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
String currentTime; // dateTime

extDcMotor *motors[4]; // hier stop ik alle motoren in, dat bespaard vier keer dezelfde code clusterfucken
extDcMotor motor1(0, 1);
extDcMotor motor2(2, 3);
extDcMotor motor3(4, 5);
extDcMotor motor4(6, 7);

String byteToString(byte *_byte, unsigned _len)
{
  String _ret = "";

  for (unsigned int i = 0; i < _len; i++)
  {
    _ret += (char)_byte[i];
  }

  return _ret;
}

void callback(char *topic, byte *payload, unsigned int length) // de payload die je krijgt moet zijn in de form van XY X = 1 of 0 en Y = motor nummer
{
  String _payload = byteToString(payload, length);
  unsigned _motorNumber = String(_payload[1]).toInt();

  Serial.println("Message arrived [" + (String)topic + "]: " + _payload);

  switch (_payload[0])
  {
  case '0':                         // off
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    Serial.print(F("\tChannel OFF: ") + String(_motorNumber));
    MQTTclient.publish(String(F("actuators/") + clientId + F("/Ch") + String(_motorNumber+1) + F("/state")).c_str(), "OFF");

    motors[_motorNumber]->stop();
    break;
  case '1':                          // on
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off (Note that LOW is the voltage level
    Serial.print(F("\tChannel ON: ") + String(_motorNumber));
    MQTTclient.publish(String(F("actuators/") + clientId + F("/Ch") + String(_motorNumber+1) + F("/state")).c_str(), "OM");

    motors[_motorNumber]->forward();
    break;
  }
}

void threadPublishCallback()
{
  float _CurrentPh = 0;

  Serial.print(F("threadPublishCallback: ") + String(millis() / 1000));

  Serial.print(F("\tCurrentPh: ") + String(_CurrentPh));
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

  Serial.println(F("\ttime: ") + String(timeClient.getEpochTime()));
  MQTTclient.publish(String(F("sensors/") + clientId + F("/debug/connected")).c_str(), String(timeClient.getEpochTime()).c_str());

  _payload += "}";

  MQTTclient.publish(String(F("sensors/") + clientId + F("/json")).c_str(), _payload.c_str());
}

boolean reconnect() // Called when client is disconnected from the MQTT server
{
  if (MQTTclient.connect(clientId.c_str()))
  {
    timeClient.update(); // Is this the right time to update the time, when we lost connection?

    Serial.print(F("reconnect: ") + String(millis() / 1000));
    Serial.println(F("\ttime: ") + String(timeClient.getEpochTime()));
    MQTTclient.publish(String(F("sensors/") + clientId + F("/debug/connected")).c_str(), String(timeClient.getEpochTime()).c_str());

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
    motors[0] = &motor1;
    motors[1] = &motor2;
    motors[2] = &motor3;
    motors[3] = &motor4;

    for (unsigned _i = 0; _i < 4; _i++)
    {
      Serial.println("Setup channel: " + String(_i));
      motors[_i]->begin(&mcp1);
      motors[_i]->stop();

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
    unsigned long _now = millis();

    if (_now - lastReconnectAttempt > 5000) // try to reconnect every 5000 milliseconds
    {
      lastReconnectAttempt = _now;

      if (reconnect())
      {
        lastReconnectAttempt = 0;
        for (unsigned _i = 0; _i < 4; _i++)
        {
          MQTTclient.subscribe(String(F("actuators/") + clientId + F("/Ch") + String(_i + 1) + F("/command")).c_str());
          // debugging subscrition topics...
          Serial.print(F("Subscribing to: "));Serial.println(String(F("actuators/") + clientId + F("/Ch") + String(_i) + F("/command")).c_str());
        }
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
