#include <Kelvin2RGB.h>

//--------------------------------------------------
// Gamma correction table so that colours make sense
//--------------------------------------------------
//const uint8_t gamma8[] = {
//    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
//    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
//    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
//    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
//    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
//   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
//   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
//   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
//   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
//   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
//   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
//   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
//  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
//  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
//  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
//  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
//  };

void led_setup(){
  strip.begin(); // Initialize pins for output
  //strip.show();  // Turn all LEDs off ASAP
  strip.setBrightness(0);
  strip.setPixelColor(0, 0xFFB87B); // 3200K
  strip.show();
}

void led_visual_ring(DateTime alarmtime){
  Serial.println(__func__);
  Kelvin2RGB KRGB;

  int32_t seconds_since_alarm = rtc_get_seconds_since_alarm(alarmtime);
  
  // Start with a very warm temperature
  float start_temp = 1000;
  
  // Finish with warm white
  float finish_temp = 3500;
  
  // Calculate the amount to change each second
  float temp_step = (finish_temp - start_temp) / (sunrise_duration_minutes * 10);
  
  float current_temp = start_temp + (temp_step * seconds_since_alarm);
  if (current_temp >= finish_temp) current_temp = finish_temp;
  
  Serial.print("Seconds since alarm: ");
  Serial.print(seconds_since_alarm);
  Serial.println("s");

  Serial.print("Kelvin: ");
  Serial.println(current_temp);
  
  KRGB.convert_NB(current_temp,100);
  strip.setBrightness(50);
  strip.setPixelColor(0, KRGB.RGB());
  strip.show();
  return;
}
