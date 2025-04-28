#include "custom_func.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>
#include <vector>
unsigned long light_tap_Start = 0;
unsigned long light_tap_Duration = 1000;
int light_tap_Count = 0;
bool light = false;
std::vector<AppState> stateStack;

void pushState(AppState state) {
    // Prevent adding duplicate consecutive states
    if (!stateStack.empty() && stateStack.back() == state) {
        return;
    }
    stateStack.push_back(state);
}

AppState popState() {
    if (stateStack.empty()) {
        return STATE_MAIN_MENU;  // Default fallback
    }

    AppState lastState = stateStack.back();
    stateStack.pop_back();

    // Check if popped state is the same as the current one, skip it
    if (!stateStack.empty() && lastState == currentState) {
        lastState = stateStack.back();
        stateStack.pop_back();
    }

    return lastState;
}

void buttonTask(void *pvParameters) {
  for (;;) {
    int but = handle_buttons();
    if (but > 0 & but <8) {
      
      if (currentState == SUBSTATE_TETRIS) {
        tetbut = but;
        exec = 0;
      }

      if (but == 1) {  // Exit button pressed
        deinitAudio();
        tetrisentered = 0;
        currentState = popState();  // Restore previous state
        shouldExit = 1;
        beep_start();
       inMenu = false;
     
      }

      if (but == 2) {
        if (currentState == STATE_MAIN_MENU) {
          if (millis() - light_tap_Start <= light_tap_Duration) {
            light = !light; // Toggle LED state
            digitalWrite(LED_PIN, light);
            light_tap_Count = 0; // Reset count
          } else {
            // First tap
            light_tap_Count++;
            light_tap_Start = millis(); // Reset start time

            if (millis() - light_tap_Start > light_tap_Duration) {
              light_tap_Count = 0;
            }
          }
        }
        if (currentState == STATE_BRAIN_ROOT) {
         if(braapp_X>0) braapp_X--;  // Changed to braapp_X
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleY += angle;
        }
        if (currentState == STATE_SUB_MENU && app_X > 0) {
          app_Xold = app_X;
           app_Yold = app_Y;
          app_X--;  // Changed to braapp_X
        }
         if (currentState == STATE_SETTINGS) {
          if(settings_x>0) settings_x--;  
         
        }
        beep_start();
        if (currentState == STATE_BLUETOOTH && bluapp_X > 0) {  // Changed to bluapp_X
          bluapp_X--;  // Changed to bluapp_X
        }
        if (currentState == STATE_FILES) {
          if (selectedFileIndex > 0) {
            selectedFileIndex--;  // Move selection up
            if (selectedFileIndex < currentFileIndex) {
              currentFileIndex--;  // Scroll up if selection goes above current display
            }
           
          }
        }
      }

      if (but == 3) {
        deinitAudio();
        shouldExit = 0;
        tetrisentered = 0;
        currentState = STATE_MAIN_MENU;
        shouldExit = 1;
        beep_start();
        inMenu = false;
        stateStack.clear();
      }

      if (but == 4) {
        if (currentState == STATE_SUB_MENU && app_Y > 0) {  // Changed to braapp_Y
        app_Yold = app_Y;
        app_Xold = app_X;
          app_Y--;  // Changed to braapp_Y
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleX += angle;
        }
        if (currentState == STATE_SETTINGS) {
          if(settings_x == 0 && but_buzz_freq >=100 ) but_buzz_freq-=100;  
          if(settings_x ==1 && Screen_brightness >1 && Screen_brightness <= 10 ){
          Screen_brightness-=1;  
          setBrightness(Screen_brightness);
          }
          if(settings_x == 1 && Screen_brightness >10 ){
          Screen_brightness-=10;  
          setBrightness(Screen_brightness);
          }

          if(settings_x == 2 && speaker_volume > 2 ) speaker_volume-=1;  
        }
        beep_start();
      }

      if (but == 5) {
        pushState(currentState);
        inMenu = false;
        beep_start();
       entered=0;
        shouldExit = 0;
        switch (currentState) {
          case STATE_MAIN_MENU:
            currentState = STATE_SUB_MENU;
           
            break;
          case STATE_SETTINGS:
            if (but_buzz_freq != 0) {
              last_but_buzz_freq= but_buzz_freq;
              but_buzz_freq = 0;  
            } else if (but_buzz_freq == 0) {
              but_buzz_freq = last_but_buzz_freq;  
            }
            break;
          case STATE_SUB_MENU:
            currentState = appStateArray[app_X][app_Y];  // Changed to braapp_X and braapp_Y      
            break;

          case STATE_BLUETOOTH:
            currentState = blueToothaArray[bluapp_X];  // Changed to bluapp_X          
            break;

             case STATE_WIFI:
            rescanWIFI = true;     
            break;

          case STATE_FILES:
            currentState = SUBSTATE_VIEW_FILE;
            previousState = STATE_FILES;
           
            break;
          case SUBSTATE_3DENGINE:
            antiAliasedMode = !antiAliasedMode;
            break;
          case STATE_BRAIN_ROOT:
            previousState = STATE_BRAIN_ROOT;
            currentState = BrainrootArray[braapp_X];  // Changed to braapp_X
          
            break;
        }
          
      }

      if (but == 6) {
        if (currentState == STATE_SUB_MENU && app_Y < 2) {  // Changed to braapp_Y
        app_Yold = app_Y;
        app_Xold = app_X;
          app_Y++;  // Changed to braapp_Y
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleX -= angle;
        }
        if (currentState == STATE_SETTINGS) {
          if(settings_x ==0 && but_buzz_freq <=900 ) but_buzz_freq+=100;  
          if(settings_x ==1 && Screen_brightness <10 && Screen_brightness ){
          Screen_brightness+=1;  
          setBrightness(Screen_brightness);
          }
          if(settings_x ==1 && Screen_brightness <=90 && Screen_brightness >= 10 ){
             Screen_brightness+=10;  
          setBrightness(Screen_brightness);
          }
          if(settings_x == 2 & speaker_volume < 21 ) speaker_volume+=1;  
        }
        beep_start();
      }

      if (but == 7) {
        if (currentState == STATE_SUB_MENU && app_X < 3) {  // Changed to braapp_X
          app_Xold = app_X;
          app_Yold = app_Y;
          app_X++;  // Changed to braapp_X
        }
        if (currentState == STATE_BLUETOOTH && bluapp_X < 1) {  // Changed to bluapp_X
          bluapp_X++;  // Changed to bluapp_X
        }
        if (currentState == STATE_FILES) {
          if (selectedFileIndex <   fileCount - 1) {
            selectedFileIndex++;  // Move selection down
            if (selectedFileIndex >= currentFileIndex + filesOnScreen) {
              currentFileIndex++;  // Scroll down if selection goes below current display
            }
           
          }
        }
        if (currentState == STATE_BRAIN_ROOT) {
          braapp_X++;  // Changed to braapp_X
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleY -= angle;
        }
        if (currentState == STATE_SETTINGS) {
          if(settings_x<2) settings_x++;  
         
        }
        beep_start();
        
      }
      delay(10);
       drawWIFI = true;   
      entered = 0;
      Serial.println(but);
      vTaskDelay(debounceDelay / portTICK_PERIOD_MS); // Short delay to handle debouncing
    } else {
      vTaskDelay(10 / portTICK_PERIOD_MS); // Check buttons frequently
    }
    check_beep_stop();
  }
}


void setup() {


  Serial.begin(115200);
 set_pins();
  // Start with brightness at 0 (off)

 if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

 init_new_spi_device();

  init_display();
   setBrightness(Screen_brightness);
  tft.drawString("Loading", 20, 80);
  tft.drawRect(20, 65, 102, 12, TFT_WHITE);
  draw_boot_line(50);
  delay(10); 

 init_new_spi_device();

#ifdef REASSIGN_PINS
  
  if (!SD.begin(SD_CS)) {
#else
  if (!SD.begin()) {
#endif
    Serial.println("Card Mount Failed");
   
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
  }

  draw_boot_line(100);
 delay(10); 

  xTaskCreate(
    buttonTask,    // Function to be called
    "Button Task", // Name of the task
    2048,          // Stack size (bytes)
    NULL,// rameter to pass
    7,             // Task priority
    NULL         // Task handle
  );
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X2,   
                  Adafruit_BMP280::SAMPLING_X16, 
                 Adafruit_BMP280::FILTER_X16,     
                  Adafruit_BMP280::STANDBY_MS_500); 
                 
                 
 analogReadResolution(12);

  compass.init();
  compass.setCalibration(-61, 2930, -2588, 408, -1237, 2321);

  ///

  /*
compass.setCalibrationOffsets(-4435.00, -469.00, -5443.00);
compass.setCalibrationScales(0.84, 2.92, 0.68);

compass.setCalibrationOffsets(-4428.00, -134.00, -5408.00);
compass.setCalibrationScales(0.81, 4.04, 0.66);

compass.setCalibrationOffsets(-4158.00, -276.00, -4239.00);
compass.setCalibrationScales(0.71, 7.11, 0.69);
*/
compass.setCalibrationOffsets(-4158.00, -276.00, -4239.00);
compass.setCalibrationScales(0.71, 7.11, 0.69);

 Serial.print("Free RAM (Heap): ");
    Serial.print(ESP.getFreeHeap()/1024);
    Serial.println(" KB");

    if (psramFound()) {
        psramInit();
        Serial.print("Free PSRAM: ");
        Serial.print(ESP.getFreePsram()/1024);
        Serial.println(" KB");
    } else {
        Serial.println("PSRAM not found.");
    }
 battery_handler(); 
}

  unsigned long previousbatMillis = 0;
 void loop() {
 
  switch (currentState) {
    case STATE_MAIN_MENU:
    draw_menu();
    break;
    case STATE_SUB_MENU:
    draw_sub_menu();
    break;

    case STATE_ENVIRONMENT:
    draw_environment_wrapper();
    break;
    case STATE_BATTERY:
    draw_battery_wrapper();
    break;

    case  STATE_CLOCK:
    set_time();
    break;

    case STATE_BLUETOOTH:
    draw_bluetooth_submenu();  
    break;

    case SUBSTATE_NRF_SCANNER:
    get_scann();
   
    break;

    case SUBSTATE_BLUETOOTH_AMPLIFIER:
    draw_amplifier_controls();  
    break;

    case STATE_FILES:
    draw_files();
    break;
    
     case SUBSTATE_VIEW_FILE:
     handleFileSelection(); 
     handleAudio();
    break;

     case STATE_BRAIN_ROOT: 
    draw_brainroot();
    draw_miniUI();
    break;
    
     case SUBSTATE_TETRIS: 
    draw_tetris();
   tetrobuttonhandler();
    break;
    
     case SUBSTATE_3DENGINE: 
    _3DEngine();
    break;
     case STATE_ADD:
    draw_add();
    break;

     case STATE_SETTINGS: 
           draw_miniUI();
   settings();
    break;

   case STATE_WIFI: 

    draw_miniUI();
     if (millis() - envStartTime >= 200) {
       wifi();
        envStartTime = millis();
     }
    break;
  }  
    unsigned long currentMillis = millis();
    if (currentMillis - previousbatMillis >= interval) {
        previousbatMillis = currentMillis;
        battery_handler();  // Call function every 1 sec
    }
    tft.pushSprite(0, 0);  // Push the sprite to the display at (0,0)
}
