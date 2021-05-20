#define DEBUG

void rtc_setup() {
  if (!rtc.begin()) {
    #ifdef DEBUG
      Serial.println("Couldn't find RTC");
      Serial.flush();
    #endif
    abort();
  }

  // Set date and time to the computer time at compilation
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(7));

//  if (rtc.lostPower()) {
//    Serial.println("RTC lost power, let's set the time!");
//    // When time needs to be set on a new device, or after a power loss, the
//    // following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(6));
//  }

  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  rtc.adjust(DateTime(2021, 5, 20, 23, 59, 58));

  //we don't need the 32K Pin, so disable it
  rtc.disable32K();


  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  
  // turn off alarms (in case they aren't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  if(alarm1set) rtc_set_alarm(1,alarm1,alarm1days);
  if(alarm2set) rtc_set_alarm(2,alarm2,alarm2days);
}

void rtc_display_current_time() {
    uint8_t day,hour,minute;
    DateTime now = rtc.now();
    day = now.day();

    hour = now.hour();
    minute = now.minute();

    // Combine the hour & minute into a 4-character string to write to the 7-segment LED panel
    sprintf(ledbuffer, "%2d%02d", hour,minute);
            
    //Display the time on the 7-seg display
    Wire.beginTransmission(DISPLAY_ADDRESS1);
    //Wire.write(0x76);
    Wire.write(ledbuffer);
    Wire.write(0x77);
    Wire.write(COLON);
    Wire.endTransmission();
}

void rtc_set_alarm(uint8_t alarm_number,DateTime alarmtime, uint8_t alarmdays){
    
    snoozecounter = 0;
      
    switch(alarm_number){
		case 1:
		  rtc.clearAlarm(1);
		  // Set alarm for the next day only
		  if (alarm1days == NEXTDAY){
        rtc.setAlarm1(alarmtime, DS3231_A1_Date);
		  }
      // Alarm when hours, minutes and seconds match, regardless of day
		  else{
        rtc.setAlarm1(alarmtime, DS3231_A1_Hour); 
		  }
		  alarm1days = alarmdays;
		  alarm1set = 1;
		  break;
		case 2:
		  rtc.clearAlarm(2);
		  // Alarm when day (day of week), hours, and minutes match, regardless of day
		  rtc.setAlarm2(alarm2, DS3231_A2_Hour);
		  alarm2days = alarmdays;
		  alarm2set = 1;		  
		  break;
		default:
		  #ifdef DEBUG
			Serial.println("Invalid alarm number specified. No alarm set.");
		  #endif
    }
}

int32_t rtc_get_seconds_since_alarm(DateTime alarmtime){
    char timestring[8],alarmstring[8];
    DateTime now = rtc.now();
    TimeSpan timeSinceAlarm = now - alarmtime;

    int32_t seconds_since_alarm = timeSinceAlarm.totalseconds();

    return seconds_since_alarm;
}

uint8_t rtc_check_alarm_days(uint8_t alarm_number){

  DateTime now = rtc.now();
  uint8_t dayofweek = now.dayOfTheWeek();
  
  uint8_t alarm_days;

  uint8_t ring_alarm;

  if (alarm_number == 1) {
    alarm_days = alarm1days;
  }
  else {
    alarm_days = alarm2days;
  }

//  if dayofweek is 0 (Sunday) or 6 (Saturday), it's a weekend
//  alarm should NOT go off if:
// alarm_days is 0 (Mon to Fri) AND dayofweek is a weekend day

  if ((alarm_days == 0) && ((dayofweek == 0)  || (dayofweek == 6))){
    ring_alarm = 0;
  }
  else{
    ring_alarm = 1;
  }

  return ring_alarm;
}

void alarm_snooze(){
  
  // Stop audio triggers
  //digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
  ioDeviceDigitalWriteS(ioExpander, AUDIO_TRIGGER_OUT, !audioOn);

  #ifdef DEBUG
    Serial.println(__func__);
    Serial.print("Snooze interval (s): ");
  
    Serial.print(snoozemin * 60);
    Serial.print("\tSnooze counter: ");
    Serial.print(snoozecounter);
    Serial.print("\tMax snoozes: ");
    Serial.println(snoozemaxtimes);
  #endif
  
  int32_t timesnoozing = rtc_get_seconds_since_alarm(snoozestart);

  #ifdef DEBUG
    Serial.print("Elapsed snooze time (s): ");
    Serial.println(timesnoozing);
  #endif

  #ifdef DEBUG
    // Use 10 second "minutes" in debug mode
    if(timesnoozing >= (snoozemin * 10)) alarm1_fsm_state = ALARM_AUDIO_RING;
  #endif
  
  #ifndef DEBUG
  if(timesnoozing >= (snoozemin * 60)) alarm1_fsm_state = ALARM_AUDIO_RING;
  #endif
 
  return;
}
