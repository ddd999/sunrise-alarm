void lamp_update(void) {
    
  int brightnessValue = 0;
  uint8_t brightness = 0;
  
  brightnessValue = analogRead(BRIGHTNESSPIN);

  // Convert the 10-bit analog result to an 8-bit integer
  brightness = (uint8_t) (brightnessValue / 4);
    
 
  if (brightness >= 10){
    strip.setBrightness(brightness);
    strip.show();
    }
  else {
    strip.setBrightness(0);
    strip.show();
    }
}
