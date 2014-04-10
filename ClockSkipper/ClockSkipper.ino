#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Namespace.h>
#include <midi_Settings.h>

#define KNOB1 0
#define STAT1 7
#define STAT2 6
#define CHANNEL 1

int clockCount;

void setup()
{
  pinMode(STAT1,OUTPUT); 
  pinMode(STAT2,OUTPUT);
  MIDI.begin(CHANNEL);
  MIDI.turnThruOff();
  MIDI.setHandleClock(HandleClock);
  MIDI.setHandleStop(HandleStop);
  MIDI.setHandleStart(HandleStart);
  MIDI.setHandleContinue(HandleContinue);
  
  clockCount = 0;
}

void loop()
{
  MIDI.read();
}

void HandleClock(void)
{
  // Get the level from knob 1 of the MIDI shield
  int knobValue = analogRead(KNOB1);
  // We want 4 levels, so we need to divide the 1024
  // resolution result by that amount
  int level = knobValue / 256;
  
  // calculate our "divider"
  int onlyAccept = pow(2, level);
  
  // update the led so the user can check if turning
  // the knob changed anything
  setLevelLed(level);
  
  // Check if we want to send a clock signal or ignore it
  if (clockCount % onlyAccept == 0) {
    sendClock();
    clockCount = 0;
  } else {
    ignoreClock();
  }
  
  // we have handled a clock signal,
  // so increase the counter
  clockCount++;
}

void sendClock()
{
  MIDI.send(midi::Clock, 0, 0, CHANNEL);
  digitalWrite(STAT2, HIGH);
}

void ignoreClock()
{
  digitalWrite(STAT2, LOW);
}

void setLevelLed(int level)
{
  if (level % 2 == 0) {
    digitalWrite(STAT1, HIGH);
  } else {
    digitalWrite(STAT1, LOW);
  }
}

void HandleStop(void)
{
  MIDI.send(midi::Stop, 0, 0, 1);
}

void HandleStart(void)
{
  MIDI.send(midi::Start, 0, 0, 1);
}

void HandleContinue(void)
{
  MIDI.send(midi::Continue, 0, 0, 1);
}

