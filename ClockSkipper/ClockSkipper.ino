/*

AUTHOR is Thomas Bucsics aka nexxyz (http://www.nexxyz.com)

This little tool was made for Arduino and is to be used with
a SparkFun Midi Shield.

Its primary intention was to enable properly timed MIDI sync with a
MIDI modded Korg MonoTribe, but this probably has many other uses - be creative!

For the MonoTribe, specifically, it is needed because the MonoTribe
values a beat at 12 clock messages instead of the standard 24. This means that
if you sync it via the MIDI mod to something else, you will only get half a
beat that you can program. This will not do at all, and I could not find a quick
solution without going via the sync input. So I decided to create something :)

How to use (after uploading to your Arduino):

* Connect your machine you want as clock master to MIDI in
* Connect the machine you want as slave to MIDI out
* Set your MidiShield to "Run"
* Start your master

What you can do with it:
* Knob 1 (A0) controls the amount the clock is "slowed"
** values are factor 1 (no change), 2, 4, 8
** the red LED will change each time you cross a border between values
** the green led will flash with each "outgoing"/slave beat
* Button 1 (D2) triggers a "resync" of the slave
** on the next "incoming"/master beat, the slave will be stopped and immediately started
** this is useful if you change between values or get your slave out of sync somehow
* Button 2 (D3) sends a start message to the slave
* Button 3 (D4) sends a stop message to the slave

That's it.

There are probably a few bugs in here - if you find any, let me know!

Debugging it is quite tedious, since the MIDI Library/Shield uses the serial 
for MIDI communication. I did some mocking and checked the behaviour of the
calculation and a bit of checking of the basic algorithm, but nothing major. Most
testing I did with a x0xb0x and my MonoTribe.

The resync mechanism was added later so it had a chance of being useful in a live
setting.

Have fun!

*/

#include <MIDI.h>

#define KNOB1 0
#define STAT1 7
#define STAT2 6
#define CHANNEL 1
#define BUTTON1 2
#define BUTTON2 3
#define BUTTON3 4
#define CLOCKS_IN_QUARTER 24

int clockCount;
int onlyAccept;
int inClockCount;
int outClockCount;
boolean resyncScheduled;
boolean button1lastState;
boolean button2lastState;
boolean button3lastState;
boolean knob1lastValue;

void setup()
{
  pinMode(STAT1,OUTPUT); 
  pinMode(STAT2,OUTPUT);
  pinMode(BUTTON1,INPUT);
  pinMode(BUTTON2,INPUT);
  pinMode(BUTTON3,INPUT);

  digitalWrite(BUTTON1,HIGH);
  digitalWrite(BUTTON2,HIGH);
  digitalWrite(BUTTON3,HIGH);

  MIDI.begin(CHANNEL);
  MIDI.turnThruOff();
  MIDI.setHandleClock(HandleClock);
  MIDI.setHandleStop(HandleStop);
  MIDI.setHandleStart(HandleStart);
  MIDI.setHandleContinue(HandleContinue);

  onlyAccept = 1;
  initializeClockCounts();  
  resyncScheduled = false;

  button1lastState = false;
  button2lastState = false;
  button3lastState = false;
  knob1lastValue = -1;
}

void loop()
{
  processInputs();
  MIDI.read();
}

void processInputs()
{
  processKnobs();
  processButtons();
}

void processButtons()
{
  // Read the button states
  boolean button1state = button(BUTTON1);
  boolean button2state = button(BUTTON2);
  boolean button3state = button(BUTTON3);

  // Buttons should only trigger once
  // First button schedules a Resync
  if(button1state != button1lastState)
  { 
    if (button1state) scheduleResync();
    button1lastState = button1state;
  }

  // Second button sends a Start message
  if(button2state != button2lastState)
  { 
    if (button2state) sendStart();
    button2lastState = button2state;
  }

  // Third button sends a Stop message
  if(button3state != button3lastState)
  { 
    if (button3state) sendStop();
    button3lastState = button3state;
  }
}

void processKnobs()
{
  // Get the level from knob 1 of the MIDI shield
  int knob1Value = analogRead(KNOB1);
  
  if (knob1Value != knob1lastValue)
  {
    // We want 4 levels, so we need to divide the 1024
    // resolution result by that amount
    int level = knob1Value / 256;
  
    // calculate our "divider"
    onlyAccept = 1 << level;
  
    // update the led so the user can check if turning
    // the knob changed anything
    setLevelLed(level);
    knob1lastValue = knob1Value;
  }
}

void executeResync()
{
  sendStop();
  sendStart();
}

void scheduleResync()
{
  resyncScheduled = true;
}

void HandleClock(void)
{ 
  if (inClockCount % CLOCKS_IN_QUARTER == 0)
  {
    handleInQuarter();
    inClockCount = 0;
  }

  // Check if we want to send a clock signal or ignore it
  if (clockCount % onlyAccept == 0) {
    sendClock();
    clockCount = 0;

    if (outClockCount % CLOCKS_IN_QUARTER == 0)
    {
      // Turn the led on upon a quarter/beat of the slave machine
      digitalWrite(STAT2, LOW);
      outClockCount = 0;
    }
    else
    {
      // Otherwise turn it off
      digitalWrite(STAT2, HIGH);
    }
    outClockCount++;
  }
  // we have received a clock signal,
  // so increase the counter
  inClockCount++;
  clockCount++;
}

void handleInQuarter()
{
  // If a resync was scheduled, we execute it and deschedule it
  if (resyncScheduled)
  {
    executeResync();
    resyncScheduled = false;
  }
}

void sendClock()
{
  MIDI.send(midi::Clock, 0, 0, CHANNEL);
}


// Set the level LED so the user can see if they are changing anything
// It basically turns on or off alternatingly once you reach a value threshhold
void setLevelLed(int level)
{
  if (level % 2 == 0) {
    digitalWrite(STAT1, HIGH);
  } 
  else {
    digitalWrite(STAT1, LOW);
  }
}

void initializeClockCounts()
{
  inClockCount = 0;
  outClockCount = 0;
  clockCount = 0;
}

// Forward stop
void HandleStop(void)
{
  sendStop();
}

// Forward start
void HandleStart(void)
{
  sendStart();
}

// Forward continue
void HandleContinue(void)
{
  sendContinue();
}

boolean button(int button_num)
{
  return (digitalRead(button_num) == LOW);
}

// When we send a start, we also reset our clock counts
void sendStart()
{
  MIDI.send(midi::Start, 0, 0, 1);
  initializeClockCounts();
}

void sendStop()
{
  MIDI.send(midi::Stop, 0, 0, 1);
  initializeClockCounts();
}

void sendContinue()
{
  MIDI.send(midi::Continue, 0, 0, 1);
}




