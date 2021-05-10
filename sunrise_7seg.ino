// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)

const uint8_t APOST = 0b00100000;
const uint8_t COLON = 0b00010000;
const uint8_t RESET = 0x76;

uint8_t decimal_state = 0;

// Create a buffer for the 7-seg display's four characters.
// This will be accessed from multiple places.
char ledbuffer[4];

// A flag for whether the display needs to be updated or not
bool UPDATE7SEG = 0;


void ss_setup(void) {
  // 7-segment display setup
  Wire.begin(); //Join the bus as master
   
  //Send the reset command to the display - this forces the cursor to return to the beginning of the display
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(RESET);
  Wire.endTransmission();
  
  // Turn off the colon and decimals
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x77);
  Wire.write(decimal_state);
  Wire.endTransmission();

  // Set brightness to max 
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x7A);  // Brightness control command
  Wire.write(100);  // brightest value
  Wire.endTransmission();
  
  delay(500);

  ss_write("helo",0);
  delay(500);
}

void ss_write(const char text[4],uint8_t decimals){

  decimal_state = decimals;
  
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.write(text);
  Wire.endTransmission();
  
}

void ss_buff(const char text[4]){
  
  sprintf(ledbuffer, "%s", text);
  UPDATE7SEG = 1;

}

void ss_colon(bool colon_toggle){

    if (colon_toggle) {
      decimal_state |= COLON;
    }
    else {
      decimal_state &= ~COLON;
    }
    
    Wire.beginTransmission(DISPLAY_ADDRESS1);
    Wire.write(0x77);
    Wire.write(decimal_state);
    Wire.endTransmission();
}

void ss_clear(){
  decimal_state = 0;
  sprintf(ledbuffer, "    ");
  
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  //Clear the display
  Wire.write(0x76);
  Wire.endTransmission();

  //Clear colon/decimals
  Wire.beginTransmission(DISPLAY_ADDRESS1);
  Wire.write(0x77);
  Wire.write(0);
  Wire.endTransmission();
}
