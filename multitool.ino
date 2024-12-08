#include "custom_func.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Arduino.h>

unsigned long light_tap_Start = 0;
unsigned long light_tap_Duration = 1000;
int light_tap_Count = 0;
bool light = false;

void buttonTask(void *pvParameters) {
  for (;;) {
    int but = handle_buttons();
    if (but != 0 && but != 1000) {
      entered = 0;

      if (currentState == SUBSTATE_TETRIS) {
        tetbut = but;
        exec = 0;
      }

      if (but == 1) {  // Exit button pressed
        tetrisentered = 0;
        currentState = previousState;
        shouldExit = 1;
        beep_start();
        entered = 0;  // Reset 'entered' so it can be redrawn when re-entering
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
          braapp_X--;  // Changed to braapp_X
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleY += angle;
        }
        if (currentState == STATE_SUB_MENU && app_X > 0) {
          app_X--;  // Changed to braapp_X
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
            entered = 0;
          }
        }
      }

      if (but == 3) {
        shouldExit = 0;
        tetrisentered = 0;
        currentState = STATE_MAIN_MENU;
        shouldExit = 1;
        beep_start();
        entered = 0;
      }

      if (but == 4) {
        if (currentState == STATE_SUB_MENU && app_Y > 0) {  // Changed to braapp_Y
          app_Y--;  // Changed to braapp_Y
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleX += angle;
        }
        beep_start();
      }

      if (but == 5) {
        previousState = currentState;
        beep_start();
        entered = 0;
        shouldExit = 0;
        switch (currentState) {
          case STATE_MAIN_MENU:
            currentState = STATE_SUB_MENU;
            entered = 0;
            break;
          case STATE_SETTINGS:
            if (but_buzz_freq == 400) {
              but_buzz_freq = 0;  // If it's 400, set it to 0
            } else if (but_buzz_freq == 0) {
              but_buzz_freq = 400;  // If it's 0, set it to 400
            }
            break;
          case STATE_SUB_MENU:
            currentState = appStateArray[app_X][app_Y];  // Changed to braapp_X and braapp_Y
            entered = 0;
            break;
          case STATE_BLUETOOTH:
            currentState = blueToothaArray[bluapp_X];  // Changed to bluapp_X
            entered = 0;
            break;
          case STATE_FILES:
            currentState = SUBSTATE_VIEW_FILE;
            previousState = STATE_FILES;
            entered = 0;
            break;
          case SUBSTATE_3DENGINE:
            antiAliasedMode = !antiAliasedMode;
            break;
          case STATE_BRAIN_ROOT:
            previousState = STATE_BRAIN_ROOT;
            currentState = BrainrootArray[braapp_X];  // Changed to braapp_X
            entered = 0;
            break;
        }
      }

      if (but == 6) {
        if (currentState == STATE_SUB_MENU && app_Y < 2) {  // Changed to braapp_Y
          app_Y++;  // Changed to braapp_Y
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleX -= angle;
        }
        beep_start();
      }

      if (but == 7) {
        if (currentState == STATE_SUB_MENU && app_X < 3) {  // Changed to braapp_X
          app_X++;  // Changed to braapp_X
        }
        if (currentState == STATE_BLUETOOTH && bluapp_X < 1) {  // Changed to bluapp_X
          bluapp_X++;  // Changed to bluapp_X
        }
        if (currentState == STATE_FILES) {
          if (selectedFileIndex < fileCount - 1) {
            selectedFileIndex++;  // Move selection down
            if (selectedFileIndex >= currentFileIndex + filesOnScreen) {
              currentFileIndex++;  // Scroll down if selection goes below current display
            }
            entered = 0;
          }
        }
        if (currentState == STATE_BRAIN_ROOT) {
          braapp_X++;  // Changed to braapp_X
        }
        if (currentState == SUBSTATE_3DENGINE) {
          angleY -= angle;
        }
        beep_start();
      }

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
 
 if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

 init_new_spi_device();

  init_display();
  tft.drawString("Loading", 20, 80);
  tft.drawRect(20, 65, 102, 12, TFT_WHITE);
  draw_boot_line(50);
  delay(10); 

  draw_boot_line(100);
 delay(10); 

  xTaskCreate(
    buttonTask,    // Function to be called
    "Button Task", // Name of the task
    4096,          // Stack size (bytes)
    NULL,          // Parameter to pass
    1,             // Task priority
    NULL           // Task handle
  );

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
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

}


 void loop() {
 
  switch (currentState) {
    case STATE_MAIN_MENU:
    draw_menu();
    break;
    case STATE_SUB_MENU:
    draw_sub_menu();
    break;

    case STATE_ENVIRONMENT:
      draw_compass();  
      draw_miniUI();
    if (millis() - envStartTime >= env_upd_Duration) {
                temperature = aht20.getTemperature();
                check_beep_stop(); 
                 humidity = aht20.getHumidity();
                draw_environment();
                envStartTime = millis(); // Reset the timer

            }
    break;
    case STATE_BATTERY:
    draw_battery();
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
  }  
   
}
