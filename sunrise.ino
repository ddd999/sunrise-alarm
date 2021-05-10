// Sunrise Alarm Clock v0.001
// February 21, 2021

const uint8_t FWVERSION = 1;

#define DEBUG

// For onboard DotStar RGB LED
#include <Adafruit_DotStar.h>
#include <SPI.h>


// Needed for 7-segment display
#include <Wire.h>
const uint8_t DISPLAY_ADDRESS1 = 0x71; //This is the default address of the OpenSegment with both solder jumpers open

// For button event handling
#include <AceButton.h>
using namespace ace_button;
// One button wired to the pin at BUTTON_PIN. Automatically uses the default
// ButtonConfig. The alternative is to call the AceButton::init() method in
// setup() below.
const int BUTTON_PIN = A0;
AceButton button(BUTTON_PIN);

struct button_t {
  bool PRESS        = 0;
  bool RELEASE      = 0;
  bool CLICK        = 0;
  bool DOUBLECLICK  = 0;
  bool LONGPRESS    = 0;
  bool REPEATING    = 0;
} button_status;

// Forward reference to prevent Arduino compiler becoming confused.
void handleEvent(AceButton*, uint8_t, uint8_t);

// Digital output for audio trigger
#define AUDIO_TRIGGER_OUT A4
// Analog input for lamp brightness
#define BRIGHTNESSPIN A3

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <RTClib.h>
RTC_DS3231 rtc;
DateTime alarm1 = DateTime(2021, 3, 7, 22, 19, 45);
DateTime alarm2 = DateTime(2021, 2, 21, 20, 45, 0);
DateTime snoozestart;

bool alarm1set = 1;
bool alarm2set = 1;

uint16_t sunrise_duration_minutes = 1;
uint8_t sunrise_led_colour, sunrise_led_brightness = 0;

// Alarm constants
const uint8_t MONTOFRI  = 0;
const uint8_t SEVENDAYS = 1;
const uint8_t ONEDAY    = 2;

const char *alarmdays[] = {
  "Monday to Friday", "Every day", "Single Day"
  };

uint8_t alarm1days = MONTOFRI;
uint8_t alarm2days = MONTOFRI;

uint8_t snoozemin = 1;
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


// Just use these for status/debugging output
const char *alarmstates[] = {
  "IDLE", "SET", "VISUAL_RING",
  "AUDIO_RING", "SNOOZING" };

// Menu Options
const uint8_t MENU_SET_TIME     = 0;
const uint8_t MENU_SET_ALARM1   = 1;
const uint8_t MENU_SET_ALARM2   = 2;
const uint8_t MENU_SET_SNOOZE   = 3;
const uint8_t MENU_EXIT         = 4;


const char *menu_options[] = {
  "CLOC",
  "AL 1",
  "AL 2",
  "Snoo",
  "RETN"
};

void menu_loop(void);

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

void setup() {

  // Use serial port for debug messages
  Serial.begin(115200);
  
  ss_setup();
  led_setup();
  button_setup();

  rtc_setup();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(AUDIO_TRIGGER_OUT, OUTPUT);
  digitalWrite(AUDIO_TRIGGER_OUT, HIGH);

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

    Serial.print("\n\n\nSunrise Alarm Clock v");
    Serial.println(FWVERSION);
       
    DateTime now = rtc.now();
    sprintf(timestring,"%2d:%02d:%02d\t\t",now.hour(),now.minute(),now.second());
    Serial.print("Current time: ");
    Serial.println(timestring);

    Serial.print("Alarm1 state: ");
    Serial.println(alarmstates[alarm1_fsm_state]);
    
    sprintf(alarmstring,"%2d:%02d\t\t",alarm1.hour(),alarm1.minute());
    Serial.print("Alarm1 time: ");
    Serial.print(alarmstring);
    Serial.print("Alarm1 days: ");
    Serial.println(alarmdays[alarm1days]);

//    Serial.print("Alarm2 state: ");
//    Serial.println(alarmstates[alarm2_fsm_state]);

    sprintf(alarmstring,"%2d:%02d\t\t",alarm2.hour(),alarm2.minute());
    Serial.print("Alarm2 time: ");
    Serial.print(alarmstring);
    Serial.print("Alarm2 days: ");
    Serial.println(alarmdays[alarm2days]);

    if(alarm1_fsm_state == ALARM_VISUAL_RING){
      led_visual_ring(alarm1);
      }

    if(alarm1_fsm_state == ALARM_AUDIO_RING){
      digitalWrite(LED_BUILTIN,HIGH);
      digitalWrite(AUDIO_TRIGGER_OUT,LOW);
      Serial.println("AUDIO RING. TIME TO WAKE UP, SUCKER!");
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

  // Check if any buttons have been pressed
  // Should be called every 4-5ms or faster, for the default debouncing time of ~20ms.
  button.check();
  
  // Alarm state machine
  switch(alarm1_fsm_state){
    case ALARM_IDLE:
      if (alarm1set) {
        alarm1_fsm_state = ALARM_SET;
      }
      break;
    case ALARM_SET:
      if(rtc.alarmFired(1)) {
        rtc.clearAlarm(1);
        alarm1_fsm_state = ALARM_VISUAL_RING;
      }
        if (button_status.LONGPRESS){
          clear_button_flags();
        menu_loop();
      }
      break;
    case ALARM_VISUAL_RING:
        if(rtc_get_seconds_since_alarm(alarm1) >= (sunrise_duration_minutes * 60) ){
        alarm1_fsm_state = ALARM_AUDIO_RING;
        }

      if(button_status.CLICK){
        button_status.CLICK = 0;
      }
      if(button_status.LONGPRESS){
        button_status.LONGPRESS = 0;
        
        strip.setBrightness(0);
        strip.show();
        
        rtc_set_alarm(1,alarm1);
        
        alarm1_fsm_state = ALARM_SET;
        digitalWrite(LED_BUILTIN,LOW);
        digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
      }
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
        
        rtc_set_alarm(1,alarm1);
        
        alarm1_fsm_state = ALARM_SET;
        digitalWrite(LED_BUILTIN,LOW);
        digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
      }
      break;
      
    case ALARM_SNOOZING:
      if(button_status.CLICK){
        button_status.CLICK = 0;
        //snoozecounter += 1;
      }
      
      if(snoozecounter > snoozemaxtimes){
        Serial.println("No more snoozing!");
        alarm1_fsm_state = ALARM_VISUAL_RING;
      }

      if(button_status.LONGPRESS){
        button_status.LONGPRESS = 0;
        
        strip.setBrightness(0);
        strip.show();
        
        rtc_set_alarm(1,alarm1);
        
        alarm1_fsm_state = ALARM_SET;
        digitalWrite(LED_BUILTIN,LOW);
        digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
      }
      break;
    default:
      break;
  }
}
