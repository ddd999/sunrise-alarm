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

bool blinkflag = 0;

void menu_loop(void){

  Serial.println(__func__);

  char templedbuffer[4];
  
  ss_write("SET ",0);
  
  // Counter for tracking when to provide output
  unsigned long menuPreviousMillis = 0;
  const long refreshinterval = 1000;
  
  // Timeout timer for exiting the menu after no user input
  const uint8_t timeout = 180;
  const uint8_t exitcountdown = 30;
  uint8_t timeout_counter = timeout;

  uint8_t menu_selection = -1;
  
  while(1){
    unsigned long menuCurrentMillis = millis();
        
    // Check if any buttons have been pressed
    // Should be called every 4-5ms or faster, for the default debouncing time of ~20ms.
    button.check();
    taskManager.runLoop();


    // Slow stuff here (update timeout counter)
    // Update the output if it has been 1 second since the last update
    if (menuCurrentMillis - menuPreviousMillis >= refreshinterval) {
      
      // save the time when this if statement was last evaluated
      menuPreviousMillis = menuCurrentMillis;

      blinkflag = !blinkflag;
          
      // Decrement the timeout timer, exit if no input after the designated time
      if (timeout_counter >= 0){

        // Reset the timeout counter if a button was pressed
        if(button_status.PRESS){
          button_status.PRESS = 0;
          timeout_counter = timeout;
          Serial.println("reset timer");
        }

        timeout_counter--;
        
        // Display a counter before exiting menu
        if (timeout_counter <= exitcountdown){
          sprintf(ledbuffer, "%4d", timeout_counter);
          Serial.println(timeout_counter);
          UPDATE7SEG = 1;
        }
        
      }
      
    }

    // Fast stuff here
    
    // Go to the next menu selection if the encoder rotates clockwise
    if(button_status.ENC_UP){
      button_status.ENC_UP = 0;
    
      // Wrap around to the first item if we were already on the last item
      if(menu_selection == ( (sizeof(menu_options)/sizeof(menu_options[0])) - 1) ){
        menu_selection = 0;
      }
      else{
        menu_selection++;
      }
    
      ss_buff(menu_options[menu_selection]);
    
    }

    // Go to the previous menu selection if the encoder rotates counter-clockwise
    if(button_status.ENC_DOWN){
      button_status.ENC_DOWN = 0;
    
      // Wrap around to the last item if we were already on the first item
      if(menu_selection == 0){
        menu_selection = 4;
      }
      else{
        menu_selection--;
      }
    
      ss_buff(menu_options[menu_selection]);
    
    }

    
     if(timeout_counter == 0){
      break;
     }
     
    // Click to select menu option
    if(button_status.CLICK){
      timeout_counter = timeout;
      button_status.CLICK = 0;
      
      if(menu_selection == MENU_EXIT) break;
      

      // Save the display state before going to any other functions
      sprintf(templedbuffer, ledbuffer);
      
      switch(menu_selection){
        case MENU_SET_TIME:
          menu_set_time(CLOCK);
        break;
        case MENU_SET_ALARM1:
          menu_set_time(ALARM1);
        break;
        case MENU_SET_ALARM2:
          menu_set_time(ALARM2);
        break;
        case MENU_SET_SNOOZE:
          menu_set_time(SNOOZE);
        break;
        case MENU_EXIT:
        break;
      }

      // Restore the previous display state
      sprintf(ledbuffer, templedbuffer);
      UPDATE7SEG = 1;
    }

    if(UPDATE7SEG){
      ss_write(ledbuffer,0);
      UPDATE7SEG = 0;
    }
    
    }
}

void menu_set_time(const uint8_t clockalarmsnooze){

  // Save what was on the 7-segment display
  char templedbuffer[4];
  char ledbufferbackup[4];
  uint8_t decimal_state_backup = decimal_state;
  
  sprintf(ledbufferbackup,ledbuffer);
  Serial.print("ledbufferbackup: ");
  Serial.println(ledbufferbackup);
  
  DateTime now = rtc.now();

  uint16_t year;
  uint8_t month,day,hour,minute,second;
  
  year = now.year();
  month = now.month();
  bool tomorrow = 0;
  second = 0;
  
  uint8_t alarmdays = MONTOFRI;

// Wait for the button to be released
if(!button_status.RELEASE){
  while(1){
    if(button_status.RELEASE) break;
  }
}
    button_status.RELEASE = 0;
    button_status.PRESS = 0;
    button_status.CLICK = 0;
    button_status.LONGPRESS = 0;
    button_status.REPEATING = 0; 

    //Get hour only if we are setting the clock or an alarm
    if(clockalarmsnooze != SNOOZE) hour = get_hour(clockalarmsnooze);

// Wait for the button to be released
if(button_status.PRESS){
  while(1){
    if(button_status.RELEASE) break;
  }
  button_status.RELEASE = 0;
  button_status.PRESS = 0;
  button_status.CLICK = 0;
  button_status.LONGPRESS = 0;
  button_status.REPEATING = 0;
  delay(300);
}

  minute = get_minute(clockalarmsnooze);

  // Wait for the button to be released
if(!button_status.RELEASE){
  while(1){
    if(button_status.RELEASE) break;
  }
    button_status.RELEASE = 0;
    button_status.PRESS = 0;
    button_status.CLICK = 0;
    button_status.LONGPRESS = 0;
    button_status.REPEATING = 0; 
}
    // Get days only if we are setting an alarm
    if(clockalarmsnooze == ALARM1){
      alarm1days = get_alarm_days();
      if(alarm1days == NEXTDAY) tomorrow = 1;
    }
    else if(clockalarmsnooze == ALARM2){
      alarm2days = get_alarm_days();
      if(alarm2days == NEXTDAY) tomorrow = 1;
    }

  switch(clockalarmsnooze){
    case CLOCK:
        // Write the new time to the RTC
        rtc.adjust(DateTime(year,month,day,hour,minute,second));
      
        ss_write("N CL",0);
        delay(1000);
        sprintf(templedbuffer, "%02d%02d", hour,minute);
        ss_write(templedbuffer,COLON);
        delay(1200);
    break;
    
    case ALARM1:
        alarm1 = DateTime(year,month,day,hour,minute,second);
        
        if(tomorrow){
          alarm1 = alarm1 + TimeSpan(1,0,0,0);
        }
        
        rtc_set_alarm(1,alarm1,alarm1days);
        
        ss_write("AL 1",0);
        delay(700);
        sprintf(templedbuffer, "%02d%02d", alarm1.hour(),alarm1.minute());
        ss_write(templedbuffer,COLON);
        delay(700);    
    break;
    
    case ALARM2:
        alarm2 = DateTime(year,month,day,hour,minute,second);
        
        if(tomorrow){
          alarm2 = alarm2 + TimeSpan(1,0,0,0);
        }

        rtc_set_alarm(2,alarm2,alarm2days);
        
        ss_write("AL 2",0);
        delay(700);
        sprintf(templedbuffer, "%02d%02d", alarm2.hour(),alarm2.minute());
        ss_write(templedbuffer,COLON);
        delay(700);
    break;
    
    case SNOOZE:
        snoozemin = minute;
        ss_write("SNoo",0);
        delay(700);
        sprintf(templedbuffer, "  %02d", minute);
        ss_write(templedbuffer,COLON);
        delay(700);
    break;
  }

  UPDATE7SEG = 1;

}

uint8_t get_hour(uint8_t clockalarmsnooze){

  Serial.println(__func__);
  
  char templedbuffer[4];
  uint8_t hour = 0;

  // Set the minute to the snooze time if we're adjusting the snooze
  if (clockalarmsnooze == ALARM1){
    hour = alarm1.hour();
  }
  else if (clockalarmsnooze == ALARM2){
    hour = alarm2.hour();
  }
  else if (clockalarmsnooze == CLOCK){
    DateTime now = rtc.now();
    hour = now.hour();
  }

  sprintf(templedbuffer,"%02dh ",hour);
  ss_write(templedbuffer,0);
  
  while(1){
    button.check();
    taskManager.runLoop();
    
    //Store the setting on click
    if(button_status.CLICK){
      button_status.CLICK = 0;
      break;
    }
    
    // Increment and display the hour on click
    if(button_status.ENC_UP){
      button_status.ENC_UP = 0;
      
      if(hour == 23){
        hour = 0;
      }
      else{
        hour++; 
      }
      sprintf(templedbuffer,"%02dh ", hour);
      ss_write(templedbuffer,0);
    }
    
    if(button_status.ENC_DOWN){
      button_status.ENC_DOWN = 0;
      
      if(hour == 0){
        hour = 23;
      }
      else{
        hour--; 
      }
      sprintf(templedbuffer,"%02dh ", hour);
      ss_write(templedbuffer,0);
    }
     
  }


    // Blink the selection before exiting
    ss_write(templedbuffer, 0);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, 0);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, 0);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, 0);
  return hour;
}

uint8_t get_minute(uint8_t clockalarmsnooze){
  Serial.println(__func__);

  // Loop for setting minute

  // Save what was on the 7-segment display
  char templedbuffer[4];
  int8_t minute = 0;

  // Set the initial minute to display according to what is being adjusted
  if (clockalarmsnooze == SNOOZE){
    minute = snoozemin;
  }
  else if (clockalarmsnooze == ALARM1){
    minute = alarm1.minute();
  }
  else if (clockalarmsnooze == ALARM2){
    minute = alarm2.minute();
  }
  else if (clockalarmsnooze == CLOCK){
    DateTime now = rtc.now();
    minute = now.minute();
  }
  
  sprintf(templedbuffer,"  %02d",minute);
  ss_write(templedbuffer,COLON);
  
  while(1){
    button.check();
    taskManager.runLoop();

    //Store the setting on click
    if(button_status.CLICK){
      button_status.CLICK = 0;
      break;                       
    }

    // Increment and display the hour on clockwise encoder rotation
     if(button_status.ENC_UP){
      button_status.ENC_UP = 0;
      minute += 1;
      
      if (minute > 59) minute = 0;
      
      sprintf(templedbuffer,"  %02d", minute);
      ss_write(templedbuffer,COLON);
    }
    
    if(button_status.ENC_DOWN){
      button_status.ENC_DOWN = 0;
      minute -= 1;
      
      if (minute < 0) minute = 59;
      
      sprintf(templedbuffer,"  %02d", minute);
      ss_write(templedbuffer,COLON);
    }

    
    
  }

  
    // Blink the selection before exiting
    ss_write(templedbuffer, COLON);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, COLON);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, COLON);
    delay(150);
    ss_write("    ",0);
    delay(150);
    ss_write(templedbuffer, COLON);

    Serial.print("Exiting ");
    Serial.println(__func__);
  return minute;
}

uint8_t get_alarm_days(){

  Serial.println(__func__);

  const char *day_options[] = {
  "5DAY",
  "7DAY",
  "1DAY",
  };

  uint8_t day_options_counter = 0; 

  ss_write(day_options[day_options_counter],0);  
 
  while(1){
    button.check();
    taskManager.runLoop();

    // Save on click
    if(button_status.CLICK){
      button_status.CLICK = 0;
      break;
      }

    // Scroll up through the day options
    if(button_status.ENC_UP){
      button_status.ENC_UP = 0;

      if(day_options_counter <2){
        day_options_counter++;
      }
      else{
        day_options_counter = 0;
      }
      ss_write(day_options[day_options_counter],0);
      }


    // Scroll down through the day options
    if(button_status.ENC_DOWN){
      button_status.ENC_DOWN = 0;

      if(day_options_counter > 0){
        day_options_counter--;
      }
      else{
        day_options_counter = 2;
      }
      ss_write(day_options[day_options_counter],0);
      }

  }
  
  // Blink the selection before exiting
  ss_write(day_options[day_options_counter],0);  
  delay(150);
  ss_write("    ",0);
  delay(150);
  ss_write(day_options[day_options_counter],0);  
  delay(150);
  ss_write("    ",0);
  delay(150);
  ss_write(day_options[day_options_counter],0);  
  delay(150);
  ss_write("    ",0);
  delay(150);
  ss_write(day_options[day_options_counter],0);  
  return day_options_counter;
}
