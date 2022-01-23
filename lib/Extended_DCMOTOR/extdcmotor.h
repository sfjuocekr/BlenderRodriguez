#include "Adafruit_MCP23X17.h"

class extDcMotor
{
public:
  extDcMotor(unsigned int pinA, unsigned int pinB);
  void begin();
  void begin(Adafruit_MCP23X17 *mcpDevice);
  void forward();
  void stop();

private:
  uint8_t _pin[2];
  Adafruit_MCP23X17 *_mcp;
  bool _useMcp;

  void _digitalWrite(unsigned int pin, unsigned int value);
  void _pinMode(unsigned int pin, unsigned int value);
  void _enableOutputs();
};