#include "config.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// #include <Thread.h>
// #include <StaticThreadController.h>
#include <Adafruit_MCP23X17.h>
#include <Thread.h>
#include <StaticThreadController.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "extdcmotor.h"

WiFiClient dhcpClient;
PubSubClient MQTTclient(dhcpClient);
Thread threadPublish = Thread();
StaticThreadController<1> threadController(&threadPublish);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, time_server, TIMEZONE * 3600, 60000);

unsigned long lastReconnectAttempt = 0;
