char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(6));
  }

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

  if(alarm1set) rtc_set_alarm(1,alarm1);
  if(alarm2set) rtc_set_alarm(2,alarm2);
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

void rtc_set_alarm(uint8_t alarm_number,DateTime alarmtime){
    
    snoozecounter = 0;
    
    if(alarm_number == 1) {
      rtc.clearAlarm(1);
      // Alarm when hours, minutes and seconds match, regardless of day
      rtc.setAlarm1(alarmtime, DS3231_A1_Hour); 
      alarm1set = 1;
    }
    else if(alarm_number = 2) {
      rtc.clearAlarm(2);
      // Alarm when day (day of week), hours, and minutes match, regardless of day
      rtc.setAlarm2(alarm2, DS3231_A2_Hour);
      alarm2set = 1;
    }
    else{
      Serial.println("Invalid alarm number specified. No alarm set.");
    }
}

int32_t rtc_get_seconds_since_alarm(DateTime alarmtime){
    char timestring[8],alarmstring[8];
    DateTime now = rtc.now();
    TimeSpan timeSinceAlarm = now - alarmtime;

    int32_t seconds_since_alarm = timeSinceAlarm.totalseconds();

    return seconds_since_alarm;
}

void alarm_snooze(){
  
  // Use built-in LED as a stand-in for the audio alarm
  digitalWrite(LED_BUILTIN,LOW);
  digitalWrite(AUDIO_TRIGGER_OUT,HIGH);
  
  Serial.println(__func__);
  Serial.print("Snooze interval (s): ");

  Serial.print(snoozemin * 60);
  Serial.print("\tSnooze counter: ");
  Serial.print(snoozecounter);
  Serial.print("\tMax snoozes: ");
  Serial.println(snoozemaxtimes);
  
  int32_t timesnoozing = rtc_get_seconds_since_alarm(snoozestart);

  Serial.print("Elapsed snooze time (s): ");
  Serial.println(timesnoozing);

  if(timesnoozing >= (snoozemin * 60)) alarm1_fsm_state = ALARM_AUDIO_RING;
 
  return;
}
