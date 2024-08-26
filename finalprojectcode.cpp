/* Code inspired by Arduino DHT Nonblocking library example on GitHub page */
#include "Timer.h"
#include <dht_nonblocking.h>

// custom defines
#define true 1
#define TOTAL_TASKS 2
#define PERIOD_GCD 10
#define PERIOD_fanTemp 1000
#define PERIOD_7SegmentDisplay 10
#define DHT_SENSOR_TYPE DHT_TYPE_11


static const int DHT11_PIN = 3;
const int motor = 2;

DHT_nonblocking dht_sensor(DHT11_PIN, DHT_SENSOR_TYPE);

// Helper Function display a number on ONE digit

// gSegPins
// An array of pins of the arduino that are connected
// to segments a, b, c, d, e... g in that order.
char gSegPins[] = {7, 5, A3, A2, A1, 6, A4};


// displayNumTo7Seg
// displays one number (between 0 and 9) "targetNum" on the digit conneceted to "digitPin"
// E.g. If I wanted to display the number 6 on the third digit of my display.
// and the third digit was connected to pin A0, then I'd write: displayNumTo7Seg(6, A0);
void displayNumTo7Seg(unsigned int targetNum, int digitPin) {


    // A map of integers to the respective values needed to display
    // a value on one 7 seg digit.
    unsigned char encodeInt[10] = {
        // 0    int 1     2     3     4     5     6     7     8     9
        0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    };

    // Make sure the target digit is off while updating the segments iteratively
    digitalWrite(digitPin, HIGH);


    // Update the segments
    for (int k = 0; k < 7; ++k) {
        digitalWrite(gSegPins[k], encodeInt[targetNum] & (1 << k));
    }

    // Turn on the digit again
    digitalWrite(digitPin, LOW);
}

// task structure
struct task {
 
  unsigned short period;
  unsigned short timeElapsed;

  void ( *tick ) ( void );

};

// global variables
static int temp = 0;
static struct task gTaskSet[ TOTAL_TASKS ];

// all possible states for each state machine
enum States_fanTemp { INIT_fanTemp, fan_on };
enum States_7SegmentDisplay { INIT_7SegmentDisplay, RIGHT, LEFT };

// task function declarations
void tick_fanTemp ( void );
void tick_7SegmentDisplay ( void );
void initializeTasks ( void );
void TimerISR ( void );


void setup(){
  pinMode(7, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(3, INPUT);
  pinMode(motor, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);

  initializeTasks();

  TimerSet(PERIOD_GCD);
  TimerOn();

  Serial.begin(115200);
}


static bool measure_room(float *temperature, float *humidity) {
  static unsigned long measurement_time = millis();

  /* Measure once every four seconds. */
  if (millis() - measurement_time > 4000ul) {
    if (dht_sensor.measure(temperature, humidity) == true) {
      measurement_time = millis();
      return true;
    }
  }

  return false;
}

void loop(){

  float temperature;
  float humidity;

  if (measure_room(&temperature, &humidity) == true) {
    temp = ((temperature) * (9.0/5.0)) + 32;
  }

  scheduleTasks();

  TimerFlag = 0;
  while (!TimerFlag) {}
}

void initializeTasks ( void ) {
  
//  // initialize task[0] to threeLEDs task
  gTaskSet[0].period = PERIOD_fanTemp;
  gTaskSet[0].timeElapsed = 0;
  gTaskSet[0].tick = tick_fanTemp;

  // initialize task[1] to blinkingLED
  gTaskSet[1].period = PERIOD_7SegmentDisplay;
  gTaskSet[1].timeElapsed = 0;
  gTaskSet[1].tick = tick_7SegmentDisplay;

  return;

}


void tick_fanTemp ( void ) {
static enum States_fanTemp state_fanTemp = INIT_fanTemp;

  switch ( state_fanTemp ) {
    case INIT_fanTemp:
    if (temp < 75) {
      state_fanTemp = INIT_fanTemp;
      Serial.println(temp);
    }
    else {
      state_fanTemp = fan_on;
    }
    break;

    case fan_on:
    if (temp >= 75) {
      state_fanTemp = fan_on;
    }
    else {
      state_fanTemp = INIT_fanTemp;
    }
    break;

  }

  switch ( state_fanTemp ) {
    case INIT_fanTemp:
    digitalWrite(motor, LOW);
    break;
    
    case fan_on:
    digitalWrite(motor, HIGH);
    break;
  }

  switch ( state_fanTemp) {
    case INIT_fanTemp:
    Serial.println("initial");
    break;

    case fan_on:
    Serial.println("fan on");
    break;
  }
  return;
}


void tick_7SegmentDisplay ( void ) {

    static enum States_7SegmentDisplay state_7SegmentDisplay = INIT_7SegmentDisplay;



  // transitions between states
  switch ( state_7SegmentDisplay ) { 

    int num;
    
    case INIT_7SegmentDisplay:
    state_7SegmentDisplay = RIGHT;
    break;

    case RIGHT:
    num = temp % 10;
    digitalWrite(4, HIGH);
    displayNumTo7Seg(num, A5);
    state_7SegmentDisplay = LEFT;
    break;

    case LEFT:
    num = temp / 10;
    digitalWrite(A5, HIGH);

    displayNumTo7Seg(num, 4);
    state_7SegmentDisplay = RIGHT;
    }


  // behavior of each state
  switch ( state_7SegmentDisplay ) { }

  return;
}



void scheduleTasks() {
  for ( int i = 0;  i < TOTAL_TASKS; i++ ) {
    gTaskSet[i].timeElapsed += PERIOD_GCD;
    
    if( gTaskSet[i].timeElapsed >= gTaskSet[i].period ) {
      gTaskSet[i].tick();
      gTaskSet[i].timeElapsed = 0;
    }
  }

  return;

}