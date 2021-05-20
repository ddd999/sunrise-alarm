// Sunrise Alarm Clock v0.001
// May 19, 2021

#define DEBUG

// For onboard DotStar RGB LED
#include <Adafruit_DotStar.h>
#include <SPI.h>

// Needed for 7-segment display
#include <Wire.h>
const uint8_t DISPLAY_ADDRESS1 = 0x71; //This is the default address of the OpenSegment with both solder jumpers open

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <RTClib.h>

#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <TaskManagerIO.h>

// create both an Arduino and an IO expander based IO abstraction
IoAbstractionRef ioExpander = ioFrom8574(0x20);

// For button event handling
#include <AceButton.h>
using namespace ace_button;
// One button wired to the pin at BUTTON_PIN. Automatically uses the default
// ButtonConfig. The alternative is to call the AceButton::init() method in
// setup() below.
const int BUTTON_PIN = A0;
AceButton button(BUTTON_PIN);

const int ALARM_TOGGLE_1 = 4; // P4 (pin 9) of PCF8574 chip
const int ALARM_TOGGLE_2 = 5; // P5 (pin 10) of PCF8574 chip

struct button_t {
  bool PRESS        = 0;
  bool RELEASE      = 0;
  bool CLICK        = 0;
  bool DOUBLECLICK  = 0;
  bool LONGPRESS    = 0;
  bool REPEATING    = 0;
} button_status;

// Digital output for audio trigger
const int AUDIO_TRIGGER_OUT = 6;  // P6 (pin 11) of PCF8574 chip
const int audioOn = LOW;

// Analog input for lamp brightness
#define BRIGHTNESSPIN A3

RTC_DS3231 rtc;
DateTime alarm1 = DateTime(2021, 5, 22, 8, 19, 0);
DateTime alarm2 = DateTime(2021, 2, 21, 20, 45, 0);
DateTime alarmstart;
DateTime snoozestart;

// TimeSpan(days, hours, minutes, seconds)
TimeSpan TS_one_day = TimeSpan(1,0,0,0);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

uint8_t alarm1enable = 0;
uint8_t alarm2enable = 0;

uint16_t sunrise_duration_minutes = 1;
uint8_t sunrise_led_colour, sunrise_led_brightness = 0;

// Alarm constants
const uint8_t MONTOFRI  = 0;
const uint8_t SEVENDAYS = 1;
const uint8_t NEXTDAY   = 2;

const char *alarmdays_string[] = {
  "Monday to Friday", "Every day", "One day"
  };

uint8_t alarm1days = MONTOFRI;
uint8_t alarm2days = MONTOFRI;

uint8_t snoozemin = 10;
uint8_t snoozecounter = 0;
uint8_t snoozemaxtimes = 3;

// For selecting whether to modify the clock, alarms, or snooze time
const uint8_t CLOCK = 0;
const uint8_t ALARM1 = 1;
const uint8_t ALARM2 = 2;
const uint8_t SNOOZE = 3;

// Alarm States
const uint8_t ALARM_IDLE        = 0;
const uint8_t ALARM_SET         = 1;
const uint8_t ALARM_VISUAL_RING = 2;
const uint8_t ALARM_AUDIO_RING  = 3;
const uint8_t ALARM_SNOOZING    = 4;

uint8_t alarm1_fsm_state = 0;

// Use these for status/debugging output
const char *alarmstates[] = {
  "IDLE", "SET", "VISUAL_RING",
  "AUDIO_RING", "SNOOZING" };

// Built-in LED
const uint8_t led = 13;

// Trinket M0 SPI pins (for DotStar LED)
const uint8_t DATAPIN  = 7;
const uint8_t CLOCKPIN = 8;


// DotStar RGB LED setup
const uint8_t NUMPIXELS = 1; // Number of LEDs in strip

Adafruit_DotStar strip = Adafruit_DotStar(
  NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
// The last parameter is optional -- this is the color data order of the
// DotStar strip, which has changed over time in different production runs.
// Your code just uses R,G,B colors, the library then reassigns as needed.
// Default is DOTSTAR_BRG, so change this if you have an earlier strip.

// Counter for tracking when to provide output
unsigned long previousMillis = 0;
const long interval = 1000;
const long ledrefreshinterval = 100;

void ss_setup();
void led_setup();
void alarm_snooze();
void led_visual_ring(DateTime);
void button_setup();
void rtc_setup();
void rtc_display_current_time();
void lamp_update(void);

void setup() {

  // Use serial port for debug messages
  Serial.begin(115200);
  
  ss_setup();
  led_setup();
  button_setup();

  rtc_setup();

  pinMode(LED_BUILTIN, OUTPUT);
//  pinMode(AUDIO_TRIGGER_OUT, OUTPUT);
  //digitalWrite(AUDIO_TRIGGER_OUT, HIGH);

    
  // if using the i2c IO expander we must make sure Wire is initialised.
  // This would not normally be done in library code, but by the callee.
  Wire.begin();
  
  // here we set the direction of pins on the IO expander
  // Alarm Toggle 1
  ioDevicePinMode(ioExpander, ALARM_TOGGLE_1, INPUT);
  // Alarm Toggle 2
  ioDevicePinMode(ioExpander, ALARM_TOGGLE_2, INPUT);
  // Audio Trigger
  ioDevicePinMode(ioExpander, AUDIO_TRIGGER_OUT, OUTPUT);
  // Turn audio off immediately to prevent unwanted triggers
  ioDeviceDigitalWriteS(ioExpander, AUDIO_TRIGGER_OUT, !audioOn);

}

void loop() {

  unsigned long currentMillis = millis();

  char timestring[8], alarmstring[5];


  // Update the output if it has been <interval> seconds since the last update
  if (currentMillis - previousMillis >= interval) {

    // save the time when this if statement was last evaluated
    previousMillis = currentMillis;

    // Update the clock display
    rtc_display_current_time(); 
    
  #ifdef DEBUG
      DateTime now = rtc.now();
      
      sprintf(timestring,"%2d:%02d:%02d\t\t",now.hour(),now.minute(),now.second());
      Serial.print("Current time: ");
      Serial.println(timestring);
      
      Serial.print("Alarm1 state: ");
      Serial.print(alarmstates[alarm1_fsm_state]);
      Serial.print("\t");
      Serial.print("alarm1enable: ");
      Serial.println(alarm1enable);
      
      sprintf(alarmstring,"%2d:%02d:%02d  %04d-%02d-%02d\t\t",alarm1.hour(),alarm1.minute(),alarm1.second(),alarm1.year(),alarm1.month(),alarm1.day());
      Serial.print("Alarm1 time: ");
      Serial.print(alarmstring);
      Serial.print("Alarm1 days: ");
      Serial.println(alarmdays_string[alarm1days]);
      Serial.println("");
  #endif

    if(alarm1_fsm_state == ALARM_VISUAL_RING){
      led_visual_ring(alarmstart);
      }

    if(alarm1_fsm_state == ALARM_AUDIO_RING){
      // Trigger audio
      //digitalWrite(AUDIO_TRIGGER_OUT,LOW);
      ioDeviceDigitalWriteS(ioExpander, AUDIO_TRIGGER_OUT, audioOn);
      #ifdef DEBUG
        Serial.println("AUDIO RING. TIME TO WAKE UP, SUCKER!");
      #endif
    }

    if(alarm1_fsm_state == ALARM_SNOOZING){
      alarm_snooze();
    }

  }

  /*****************************************************************/
  /* FAST/FREQUENT FUNCTIONS BELOW                                 */
  /* Everything below this point should be actions that need to be */
  /* run or updated as quickly or as frequently as possible        */
  /*****************************************************************/
  
  // Check if a change in lamp brightness has been requested. Only if the alarm isn't ringing or snoozing.
  if(alarm1_fsm_state == ALARM_SET | alarm1_fsm_state == ALARM_IDLE) lamp_update();

  // Check if any AceButtons have been pressed
  // (for now, just the main pushbutton)
  // Should be called every 4-5ms or faster, for the default debouncing time of ~20ms.
  button.check();

  
  // Check the i2c IO expander
  ioDeviceSync(ioExpander);

  // here we read from the IO expander and write to serial.
  alarm1enable = ioDeviceDigitalRead(ioExpander, ALARM_TOGGLE_1);
  alarm2enable = ioDeviceDigitalRead(ioExpander, ALARM_TOGGLE_2);

    // Alarm state machine
  switch(alarm1_fsm_state){
    
    case ALARM_IDLE:
      if (alarm1enable) {
        rtc_set_alarm(1,alarm1,alarm1days);
        alarm1_fsm_state = ALARM_SET;
      }
      clear_button_flags();
      break;
      
    case ALARM_SET:
      // Check if the toggle switch has been turned off
      if (alarm1enable != 1){
          rtc.clearAlarm(1);
          rtc.disableAlarm(1);
          alarm1_fsm_state = ALARM_IDLE;
      }
      
      if(rtc.alarmFired(1)) {
        rtc.clearAlarm(1);
        alarmstart = rtc.now();
      
        // Check if today is a day that the alarm should ring
        if(rtc_check_alarm_days(1)){
          alarm1_fsm_state = ALARM_VISUAL_RING;
          }
        else{
            Serial.println("Alarm rang but it's a weekend. Doing nothing.");
        }
      }
        
      if (button_status.LONGPRESS){
        clear_button_flags();
        menu_loop();
      }
        
      clear_button_flags();
      break;
      
    case ALARM_VISUAL_RING:
      if(rtc_get_seconds_since_alarm(alarmstart) > (sunrise_duration_minutes * 60) ){
        alarm1_fsm_state = ALARM_AUDIO_RING;
      }
        
      // don't do anything if there is a button press during a visual ring
      if(button_status.CLICK){
        button_status.CLICK = 0;
      }
      
      // Allow long-press during visual ring to skip to audio ring.
      if(button_status.LONGPRESS){
        button_status.LONGPRESS = 0;
        alarm1_fsm_state = ALARM_AUDIO_RING;
      }
      clear_button_flags();
    break;

    case ALARM_AUDIO_RING:
      
      if(button_status.CLICK){
        button_status.CLICK = 0;

        //Set a reference time for when the snooze button was pressed.
        snoozestart = rtc.now();
        snoozecounter += 1;
        if(snoozecounter <= snoozemaxtimes) alarm1_fsm_state = ALARM_SNOOZING;
        }
      
      if(button_status.LONGPRESS){
        button_status.LONGPRESS = 0;
        
        strip.setBrightness(0);
        strip.show();

        
        // Set the alarm for the same time tomorrow
        // if it wasn't set only for a single day
        if(alarm1days != NEXTDAY){        
          // Set the alarm for the same time NEXTDAY after it's acknowledged
          alarm1 = alarm1 + TS_one_day;
          rtc_set_alarm(1,alarm1,alarm1days);
          alarm1_fsm_state = ALARM_SET;
        }
        else{
          alarm1_fsm_state = ALARM_IDLE;
        }

        // Stop triggering audio
        //digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
        ioDeviceDigitalWriteS(ioExpander, AUDIO_TRIGGER_OUT, !audioOn);
      }
      clear_button_flags();
      break;
      
    case ALARM_SNOOZING:
      if(button_status.CLICK){
        button_status.CLICK = 0;
      }
      
      if(snoozecounter > snoozemaxtimes){
        #ifdef DEBUG
          Serial.println("No more snoozing!");
        #endif
        alarm1_fsm_state = ALARM_AUDIO_RING;
      }

      if(button_status.LONGPRESS){
        button_status.LONGPRESS = 0;
        
        strip.setBrightness(0);
        strip.show();
        
       // Set the alarm for the same time tomorrow
       // if it wasn't set only for a single day
       if(alarm1days != NEXTDAY){
          alarm1 = alarm1 + TS_one_day;
          rtc_set_alarm(1,alarm1,alarm1days);
          alarm1_fsm_state = ALARM_SET;
          }
       else{
          alarm1_fsm_state = ALARM_IDLE;
          }
          
        // Stop triggering audio
        //digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
        ioDeviceDigitalWriteS(ioExpander, AUDIO_TRIGGER_OUT, !audioOn);
      }
      clear_button_flags();
      break;
  }
}
