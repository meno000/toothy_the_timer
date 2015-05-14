// Toothy Driver ver 2
// by Christian M. Restifo
// 4/23/15

// Pin configuration for ATTINY85. Configure as necessary for your system if different
#include <avr/power.h>
#include <Bounce2.h>

//Pin connected to ST_CP of 74HC595
int latchPin = 1;
//Pin connected to SH_CP of 74HC595
int clockPin = 2;
////Pin connected to DS of 74HC595
int dataPin = 0;
//int enablePin = 4;

int startPin = 3;
int modePin = 4;

Bounce startPinbouncer = Bounce();
Bounce modePinbouncer = Bounce();

// values for lighting up LEDS as we want

int numRegisters = 3;       // number of shift registers we're using
int registerArray[] = {
  0, 1, 2
}; // for the 3 595 shift registers. 0 is the mode, 1 is bottom teeth, 2 is top.
// the first 595 is top teeth so we shift to that one *last*.
int brushTime = 30000; // brush time for each section in seconds
int delayTime; // delay while flashing
int left, left1, right, right1;
int pattern[] = {
  1, 8, 4, 2
};
int pattern2[] = {
  31, 143, 79, 47
};
int numFlashCycles; // each cycle is two delay times (on and off)
int countdown_delay = 500; // countdown speed
int z;
int mode = 1;
int start;
boolean mode_change;
int mode_select;

void setup() {
    if (F_CPU==16000000) clock_prescale_set(clock_div_1);
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
  resetData();
  flushData(registerArray);
  delay(500);
  startup_smile();
  delay(2000);
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
  if (mode_select == LOW && mode_change == true) {
    if (mode == 128) {
      mode = 1;
    }
    else {
      mode = mode << 1; // shift left 1 or multiply by 2
    }
    /*    if (mode == 128) {
     mode = 2 ^ (random(0,7)); // pick a random mode if last mode selected
     }*/
    registerArray[0] = mode;
    flushData(registerArray);
  }

}

void brush(int mode) {

    int pattern3[] = {1, 2, 4, 8, 4, 2};
    int pattern4[] = {31, 47, 79, 143, 79, 47};
    int pattern5[] = {1, 4, 2, 4, 8, 2};
    int pattern6[] = {47, 31, 79, 47, 143, 47};
  
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
      left = 3;
      left1 = 12;
      right = 63;
      right1 = 207;
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


    case 32:   // Another pattern
      delayTime = 50; // delay while flashing
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

    case 64:   // Larson scanner pattern
      delayTime = 100; // delay while flashing
      numFlashCycles = brushTime / (delayTime); // Each cycle is one flash since we're only illuminating one number at a time

      z = 0;
      for (int x = numRegisters - 1; x >= 1; x--) { // count through the top and bottom registers. Go in reverse because of the way we shift out
        registerArray[x] = 1;
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern3[z];   // pick LED from pattern
          flushData(registerArray);
          delay(delayTime);
          z = z + 1;
          if (z == 6) {
            z = 0;
          }
        }
        for (int y = 0; y < numFlashCycles; y++) {
          registerArray[x] = pattern4[z];   // pick LED from pattern
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

void createPattern(int left, int left1, int right, int right1) {
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

void resetData() { // clears all registers
  for (int x = 0; x < numRegisters; x++) {
    registerArray[x] = 0;
  }
}

void startup_smile() { // 'smiles' with the leds on startup
  int smile_pattern[] = {0,24,60,126,255};
  for (int z = 0; z <= 3; z++) {
    for (int y = 0; y <= 4; y++) {
      for (int x = 0; x < numRegisters; x++) {
        registerArray[x] = smile_pattern[y];
      }
      flushData(registerArray);
      delay(150);
    }  
  }
  
}

