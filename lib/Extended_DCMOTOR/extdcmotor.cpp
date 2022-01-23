#include "extdcmotor.h"
#include "Arduino.h"

extDcMotor::extDcMotor(unsigned int pinA, unsigned int pinB)
{
  _pin[0] = pinA;
  _pin[1] = pinB;
  _useMcp = false;
}

void extDcMotor::begin()
{
  begin(nullptr);
}

void extDcMotor::begin(Adafruit_MCP23X17 *mcpDevice)
{
  if (mcpDevice != nullptr)
  {
    _useMcp = true;
    _mcp = mcpDevice;
  }

  _enableOutputs();
  stop();
}

void extDcMotor::forward()
{
  _digitalWrite(_pin[0], 255);
  _digitalWrite(_pin[1], 0);
}

void extDcMotor::stop()
{
  _digitalWrite(_pin[0], 0);
  _digitalWrite(_pin[1], 0);
}

void extDcMotor::_digitalWrite(unsigned int pin, unsigned int value)
{
  if (_useMcp == true)
  {
    _mcp->digitalWrite(pin, value);
  }
  else
  {
    digitalWrite(pin, value);
  }
}

void extDcMotor::_pinMode(unsigned int pin, unsigned int value)
{
  if (_useMcp == true)
  {
    _mcp->pinMode(pin, value);
  }
  else
  {
    pinMode(pin, value);
  }
}

void extDcMotor::_enableOutputs()
{
  _pinMode(_pin[0], OUTPUT);
  _pinMode(_pin[1], OUTPUT);
}