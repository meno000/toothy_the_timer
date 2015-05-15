;// Toothy Driver ver 2
// by Christian M. Restifo
// 4/23/15

// This original code in this software is released under the CC 1.0 License
// Note that this software may use libraries released under other licenses which apply to those libraries only

// Pin configuration for ATTINY85. Configure as necessary for your system if different

// First we include some stuff we'll need for later

#include <avr/power.h>
#include <Bounce2.h>

// Pin definitions for ATTiny85. Alter as necessary if you use another chip.

// Pin connected to ST_CP of 74HC595
int latchPin = 1;

// Pin connected to SH_CP of 74HC595
int clockPin = 2;

// Pin connected to DS of 74HC595
int dataPin = 0;

int startPin = 3;
int modePin = 4;

// Debouncing stuff for input switches

Bounce startPinbouncer = Bounce();
Bounce modePinbouncer = Bounce();

// Various variables we'll need for later

int numRegisters = 3;       // number of shift registers we're using 
int registerArray[] = {0, 1, 2}; // for the 3 595 shift registers. 0 is the mode, 1 is bottom teeth, 2 is top.
// the first 595 is top teeth so we shift to that one *last*.
int brushTime = 30000; // brush time for each section in milliseconds
int delayTime; // delay while flashing
int left, left1, right, right1; // used to hold the patterns we'll flash
int numFlashCycles; // each cycle is two delay times (on and off)
int countdown_delay = 500; // countdown speed
int z;
int mode = 1; // start off with first mode
int start;
boolean mode_change; // keeps track if mode has changed
int mode_select; // holds value of mode pin status

void setup() {
    if (F_CPU==16000000) clock_prescale_set(clock_div_1); // used for running at 16 MHz
  //set pins to output so you can control the shift register
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(startPin, INPUT);
  pinMode(modePin, INPUT);
  startPinbouncer.attach(startPin);
  startPinbouncer.interval(100);
  modePinbouncer.attach(modePin);
  modePinbouncer.interval(100);
  // clear registers and start up
  resetData();
  flushData(registerArray);
  delay(100);
  startup_smile();
  delay(1000);
  resetData();
  registerArray[0] = mode; // start with mode 1
  flushData(registerArray);
}

void loop() {

  start = digitalRead(startPin);

  if (start == LOW) {    // if button has been pressed, brush teeth
    countdown();
    brush(mode);
    while (mode_change == false) {      // randomly flash lights to indicate time is up until mode button is pressed
      registerArray[2] = random(0, 255);
      registerArray[1] = random(0, 255);
      flushData(registerArray);
      delay(100);
      if (digitalRead(modePin) == LOW){
        mode_change = true;  
      }
    }
    resetData();
    registerArray[0] = mode;
    flushData(registerArray);
  }

  mode_change = modePinbouncer.update();
  int mode_select = modePinbouncer.read();
  if (mode_select == LOW && mode_change == true) { // advance the mode if the button is pressed
    if (mode == 128) { // loop back to beginning if at end
      mode = 1;
    }
    else {
      mode = mode << 1; // shift left 1 or multiply by 2
    }
    registerArray[0] = mode;
    flushData(registerArray);
  }

}

void brush(int mode) {

    // Constructs the flashing patterns. Note that for the first four, we call a subroutine to send the data out since it's fairly simple
    // The other patterns are sent out here due to their complexity
    // How the first three (left, left1, right, right1) work:
    
    // left and left1 is a full integer constructed such that only the 4 left (ie, MSB leds) are lit.
    // right and right1 are set up so that the first four are on and the 4 right LEDs flash in the proper manner
    // Example:
    // Bits => 0    1    2    3    4    5    6    7
    //         1    1    1    1    0    0    0    0  => evalutes to 15. So if left (or left1) is set to 15, those four will turn on.
    //         1    1    1    1    0    0    1    1  => evaluates to 207. This will turn on all left LEDs and the two far right LEDs
    
    // The patterns below are called from here. This is for stuff like the Larson scanner effect
    
    int pattern[] = {7, 14, 13, 11};        // snake pattern for left
    int pattern2[] = {123, 239, 223, 177};  // snake pattern for right
    int pattern3[] = {11, 13, 14, 7};       // reverse snake pattern for left
    int pattern4[] = {177, 239, 223, 177};  // reverse snake pattern for right
    int pattern5[] = {1, 2, 4, 8, 4, 2};        // larson 
    int pattern6[] = {31, 47, 79, 143, 79, 47}; // larson
  
  switch (mode) {

    case 1:  // flashes each section while counting down
      delayTime = 25; // delay while flashing
      numFlashCycles = brushTime / (delayTime * 2); // each cycle is two delay times (on and off)
      left = 15;
      left1 = 0;
      right = 255;
      right1 = 15;
      createPattern(left, left1, right, right1);
      break;

    case 2:   // alternately flashes 2 LEDs out of the 4
      delayTime = 250; // delay while flashing
      numFlashCycles = brushTime / (delayTime * 2); // each cycle is two delay times (on and off)
      left = 3;
      left1 = 12;
      right = 63;
      right1 = 207;
      createPattern(left, left1, right, right1);
      break;

    case 4:   // alternately flashes 2 LEDs out of the 4. Similar to case 2 but with different pattern
      brushTime = 30000; // brush time for each section in seconds
      delayTime = 250; // delay while flashing
      numFlashCycles = brushTime / (delayTime * 2); // each cycle is two delay times (on and off)
      left = 9;
      left1 = 6;
      right = 159;
      right1 = 111;
      createPattern(left, left1, right, right1);
      break;

    case 8:   // Does a "run" of the 4 LEDs
      delayTime = 50; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one LED at a time
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          flushData(registerArray);
          delay(delayTime);
          registerArray[x] = registerArray[x] << 1; // left shift to multiply by 2
          if (registerArray[x] == 16) {
            registerArray[x] = 1;
          }
        }
        int temp = 16;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = 15 | temp;
          flushData(registerArray);
          delay(delayTime);
          temp = temp << 1; // left shift to multiply by 2
          if (temp == 256) {
            temp = 16;
          }
        }
        registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
      }
      break;

    case 16:   // Random illumination of 4 LEDs
      delayTime = 25; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one number at a time
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = random(0, 15);  // flash random 4 LEDS
          flushData(registerArray);
          delay(delayTime);
        }
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = 15 | random(16, 255); // we use | with 15 to make sure first 4 LEDs stay on
          flushData(registerArray);
          delay(delayTime);
        }
        registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
      }
      break;


    case 32:   // Snake pattern
      delayTime = 200; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one number at a time
      z = 0;
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 4) {
            z = 0;
          }
        }
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern2[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 4) {
            z = 0;
          }
        }
        registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
      }
      break;

    case 64:   // Reverse snake
      delayTime = 200; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one number at a time

      z = 0;
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern3[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 4) {
            z = 0;
          }
        }
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern4[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 4) {
            z = 0;
          }
        }
        registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
      }
      break;

    case 128:   // Larson scanner pattern
      delayTime = 100; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one number at a time
      z = 0;
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern5[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 6) {
            z = 0;
          }
        }
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern6[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 6) {
            z = 0;
          }
        }
        registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
      }
      break;

  }


}

void createPattern(int left, int left1, int right, int right1) { // sends the pattern selected by brush procedure to LEDs
  for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
    for (int y = 0; y < numFlashCycles; y++) {
      registerArray[x] = left;   // flash first 4 LEDS
      flushData(registerArray);
      delay(delayTime);
      registerArray[x] = left1;
      flushData(registerArray);
      delay(delayTime);
    }
    for (int y = 0; y < numFlashCycles; y++) {
      registerArray[x] = right; //flash second set of LEDS with first 4 "on"
      flushData(registerArray);
      delay(delayTime);
      registerArray[x] = right1;
      flushData(registerArray);
      delay(delayTime);
    }
    registerArray[x] = 255;  // set all top teeth to "on" while processing bottom row
  }
}


void flushData(int bytesToPush[]) { // shifts out the data to the registers
  digitalWrite(latchPin, LOW);
  for (int x = 0; x < numRegisters; x++) {
    shiftOut(dataPin, clockPin, MSBFIRST, bytesToPush[x]);
  }
  digitalWrite(latchPin, HIGH);
}

void countdown() {  // counts down to start brushing. lights top teeth LEDS and does a countdown
  int x = 255;
  while (x >= 1) {
    registerArray[2] = x;
    flushData(registerArray);
    x = x >> 1; // shift one to the right or divide by two
    delay(countdown_delay);
  }
}

void resetData() { // clears all registers so that all LEDs are off
  for (int x = 0; x < numRegisters; x++) {
    registerArray[x] = 0;
  }
}

void startup_smile() { // 'smiles' with the leds on startup
  int smile_pattern[] = {0,24,60,126,255}; // starts are middle two LEDs and works outwards on both sides
  for (int z = 0; z <= 3; z++) {
    for (int y = 0; y <= 4; y++) {
      for (int x = 0; x < numRegisters; x++) {
        registerArray[x] = smile_pattern[y];
      }
      flushData(registerArray);
      delay(100);
    }  
  }
  
}

