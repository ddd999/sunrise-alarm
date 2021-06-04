void button_setup() {
  // Button uses the built-in pull up register.
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);

  // Configure the ButtonConfig with the event handler, and enable all higher
  // level events.
  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);
//  buttonConfig->setClickDelay(150);
  buttonConfig->setDoubleClickDelay(1);
}


// The event handler for the button.
//void handleEvent(AceButton* /* button */, uint8_t eventType, uint8_t buttonState) {
void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {

  // Print out a message for all events.
  #ifdef DEBUG
    Serial.print(F("handleEvent(): eventType: "));
  #endif

  switch (eventType){
    case AceButton::kEventPressed:
      #ifdef DEBUG
        Serial.println("Pressed");
      #endif
      button_status.PRESS = 1;
    break;

    case AceButton::kEventReleased:
      #ifdef DEBUG
       Serial.println("Released");
      #endif
      button_status.RELEASE = 1;
    break;
    
    case AceButton::kEventClicked:
      #ifdef DEBUG
       Serial.println("Clicked");
      #endif
      button_status.CLICK = 1;
    break;
    
    case AceButton::kEventDoubleClicked:
      #ifdef DEBUG
       Serial.println("Double clicked");
      #endif
      button_status.DOUBLECLICK = 1;
    break;
    
    case AceButton::kEventLongPressed:
      #ifdef DEBUG
        Serial.println("Long pressed");
      #endif
      button_status.LONGPRESS = 1;
    break;
    
    case AceButton::kEventRepeatPressed:
      #ifdef DEBUG
       Serial.println("Repeat Pressed");
      #endif
      button_status.REPEATING = 1;
    break;
  }
}

// When the snooze button is clicked, this function will be run as we registered it as a callback
//
void onSnoozeClicked(pinid_t pin, bool heldDown) {
  snoozeflag = 1;
  if (heldDown) snoozeheld = 1;
}

//
// When the repeat button is pressed, this function will be repeatedly called. It's also a callback
//
void onRepeatButtonClicked(pinid_t pin, bool heldDown) {
  Serial.println("Repeat button pressed");
}


// Each time the encoder value changes, this function runs, as we registered it as a callback
void onEncoderChange(int newValue) {
    // Encoder rotated clockwise
    if(newValue > 0){
      button_status.ENC_UP = 1;
      button_status.ENC_DOWN = 0;
    }
    // Encoder rotated counter-clockwise
    else if (newValue < 0){
      button_status.ENC_UP = 0;
      button_status.ENC_DOWN = 1;
    }
}

void clear_button_flags(){
  button_status.PRESS        = 0;
  button_status.RELEASE      = 0;
  button_status.CLICK        = 0;
  button_status.DOUBLECLICK  = 0;
  button_status.LONGPRESS    = 0;
  button_status.REPEATING    = 0;
}
