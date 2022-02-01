#include <Adafruit_MCP23X17.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "extdcmotor.h"
#include <NTPClient.h>
#include <PubSubClient.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <WiFiUdp.h>
#include <Wire.h>

#include "config.h"


WiFiClient dhcpClient;
PubSubClient MQTTclient(dhcpClient);
Thread threadPublish = Thread();
StaticThreadController<1> threadController(&threadPublish);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, time_server, TIMEZONE * 3600, 60000);
