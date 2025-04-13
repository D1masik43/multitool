#include <TJpg_Decoder.h>
#include <Audio.h>  
#include <WiFi.h>  
#include "SD_func.h"
#include "pins.h"
#include "constants.h"
#include "RF24.h"
#include "printf.h"
#include <AHT20.h>
#include <SPI.h>
#include <Wire.h>
#include <QMC5883LCompass.h>
#include <Adafruit_BMP280.h>
#include <TFT_eSPI.h> 
#include <SPI.h>  
#include "images.h"
#include <DS3231.h>
#include <Adafruit_INA219.h>
#include "BluetoothA2DPSink.h"  
#include <SPI.h>
#include <TFT_eSPI.h>
#include <math.h>
#include <QMC5883LCompass.h>
#include "battery.h"
#include "esp_heap_caps.h"

Battery battery;
QMC5883LCompass compass;
BluetoothA2DPSink a2dp_sink;
Audio* audio = nullptr;  // Pointer to Audio object
Adafruit_INA219 ina219;
RTClib RTClib;
DS3231 myRTC;
TFT_eSPI tft = TFT_eSPI(); 

#define SCREEN_WIDTH  128  // Width of the TFT display
#define SCREEN_HEIGHT 160  // Height of the TFT display


void draw_miniUI_One_off();   
void draw_miniUI();


void init_display()
{
  pinMode(TFT_RST, OUTPUT);
  tft.init();  
  digitalWrite(TFT_RST, LOW);
  digitalWrite(TFT_RST, HIGH);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
}
void init_sd_card()
{

  if(!SD.begin(5)) {
      Serial.println("Card Mount Failed with CS pin 5");
  } else if(!SD.begin()) {
      Serial.println("Card Mount Failed with default CS pin");
  } else {
      Serial.println("Card successfully mounted");
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}
void disable_all_spi_devices() {
  digitalWrite(12, HIGH);
 digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
}
void init_new_spi_device()
{
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(23, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  digitalWrite(14, HIGH);
}

int ButtonVal = 0;
int button_value = 0;
int last_button_value = 0;

int ButtonCheck()
{
    ButtonVal= 0;
    for(int s =0; s < button_samples; s++)
    {
     ButtonVal += analogRead(BUT_PIN); 
     delay(1);
    }
    ButtonVal = ButtonVal / button_samples;
  for(int i = 0; i <= 6; i++)
  {
    
    if(ButtonVal >= Button[i][1] && ButtonVal <= Button[i][2])
    {
      //Serial.println(ButtonVal);
     return Button[i][0] ;          
    }
    
  }
  return 0;
}

unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; 

int handle_buttons() {
  int current_button_value = ButtonCheck();
 
  if (current_button_value != last_button_value) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (current_button_value != button_value) {
      button_value = current_button_value;
      if (button_value != 0) {
        last_button_value = button_value;
        return button_value;
      }
    }
  }

  last_button_value = current_button_value;
  return 0;
}

void draw_boot_line(int perc)
{
  disable_all_spi_devices();
  tft.fillRect(21, 66, perc, 10 , TFT_WHITE);   
}

void set_pins()
{
   pinMode(BUT_PIN, INPUT);
   pinMode(BACKL_PIN, OUTPUT);
   pinMode(BUZ_PIN, OUTPUT);
   pinMode(LED_PIN, OUTPUT);
}

int entered = 0;
volatile bool shouldExit = false;


AppState currentState =  STATE_MAIN_MENU;
AppState  previousState=  STATE_MAIN_MENU;




void drawImage(const uint16_t image[32][32], int startX, int startY) {
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            tft.drawPixel(startX + x, startY + y, image[y][x]);
        }
    }
}


AHT20 aht20;
Adafruit_BMP280 bmp;

float temperature = 0;
float humidity = 0;
float pressure = 0;
float altitude = 0;

unsigned long envStartTime;

bool isBeeping = false; 
unsigned long beepStartTime;

void check_beep_stop()
{
  if (isBeeping && (millis() - beepStartTime >= beepDuration)) {
     noTone(BUZ_PIN);              
    isBeeping = false;           
   }
}
unsigned long compStartTime ;
void draw_compass(){
 
  int azimuth;
  byte bearing;
  char direction[4];

  compass.read();
  azimuth = compass.getAzimuth();
  bearing = compass.getBearing(azimuth);
  compass.getDirection(direction, azimuth);
  direction[3] = '\0';

 
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(50,120);
  tft.println(direction);
  tft.setCursor(50,100);
  tft.setTextSize(1);
  tft.println(F("Direction: "));

  int radius = 23;
  int centerX = 25;
  int centerY = 120;
  if (millis() - compStartTime >= 100) {
  tft.fillRect(50,120,78,21,TFT_BLUE); 
  compStartTime = millis();
  }
  tft.fillCircle(centerX, centerY, radius, TFT_BLACK);
  tft.drawCircle(centerX, centerY, radius, TFT_WHITE);

  float angle = azimuth;
  float angleRad = angle * PI / 180 + PI;
  int endX = centerX + radius * cos(angleRad);
  int endY = centerY - radius * sin(angleRad);
  tft.drawLine(centerX, centerY, endX, endY, TFT_RED);
}
void draw_environment(){ 

  disable_all_spi_devices();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLUE);
  draw_miniUI_One_off();   
  pressure = bmp.readPressure() / 100.0F * 0.750062;  
  temperature = aht20.getTemperature();
  humidity = aht20.getHumidity();
  altitude = bmp.readAltitude(1013.25);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 26);
  tft.println("Temperature: " + String(temperature) + " C");  
  tft.setCursor(0, 46);
  tft.println("Humidity: " + String(humidity) + " %");
  tft.setCursor(0, 66);       
  tft.println("Pressure: " + String(pressure) + " mmHg");
  tft.setCursor(0, 86);       
  tft.println("Altitude: " + String(altitude) + " m");                                         
}

void draw_environment_wrapper()
{  
  draw_miniUI();
  if(entered==0) 
  {
     draw_environment();
     draw_compass();
     entered = 1;
  }
  draw_compass();
  unsigned long currentMillis = millis();
   if (currentMillis  - envStartTime > env_upd_Duration) {
   envStartTime = currentMillis ; // Reset the timer
   draw_environment();
   draw_compass();  
  }
}
void beep_start()
{
        tone(BUZ_PIN,but_buzz_freq);   
        isBeeping = true;           
        beepStartTime = millis(); 
}

bool bat_updated = 0;
void battery_handler()
{
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float busVoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();
  float loadVoltage = busVoltage + (shuntVoltage / 1000);
  float temperature = aht20.getTemperature();
  battery.updateBattery(loadVoltage,current_mA,power_mW,temperature);
  bat_updated = 1;
}

void draw_battery(){
  draw_miniUI();
  disable_all_spi_devices();
  tft.fillRect(0,16,128,144,TFT_BLACK);
  // Set text color and size
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);


  tft.setCursor(1, 16);
  tft.print("Current: ");
  tft.print(battery.getCurrent());
  tft.println(" mA");

  tft.setCursor(1, 32);
  tft.print("Power: ");
  tft.print(battery.getPower());
  tft.println(" mW");

  tft.setCursor(1, 48);
  tft.print("Load Voltage: ");
  tft.print(battery.getVoltage());
  tft.println(" V");

 
  tft.setCursor(1, 64);
  tft.print("Percentage: ");
  tft.print(battery.getPercentage());
  tft.println(" %");

  float hours = battery.getEstimatedHours();
  int h = (int)hours;  // Get the whole hours part
  int m = (int)((hours - h) * 60);  // Convert fractional part to minutes

  tft.setCursor(1, 80);
  tft.println("Battery will last for: ");
  tft.print(h);
  tft.print("h ");
  tft.print(m);
  tft.println("m");

}

void draw_battery_wrapper()
{
  if(entered == 0)
  {
    draw_battery();
    entered=1;
  }
  if(bat_updated)
  {
     draw_battery();
    bat_updated = 0;
  }
}


byte year;
byte month;
byte date;
byte dOW;
byte hour;
byte minute;
byte second;

void getDateStuff(byte& year, byte& month, byte& date, byte& dOW,
                  byte& hour, byte& minute, byte& second) {
    // Call this if you notice something coming in on
    // the serial port. The stuff coming in should be in
    // the order YYMMDDwHHMMSS, with an 'x' at the end.
    boolean gotString = false;
    char inChar;
    byte temp1, temp2;
    char inString[20];
    
    byte j=0;
    while (!gotString) {
        if (Serial.available()) {
            inChar = Serial.read();
            inString[j] = inChar;
            j += 1;
            if (inChar == 'x') {
                gotString = true;
            }
        }
    }
    Serial.println(inString);
    // Read year first
    temp1 = (byte)inString[0] -48;
    temp2 = (byte)inString[1] -48;
    year = temp1*10 + temp2;
    // now month
    temp1 = (byte)inString[2] -48;
    temp2 = (byte)inString[3] -48;
    month = temp1*10 + temp2;
    // now date
    temp1 = (byte)inString[4] -48;
    temp2 = (byte)inString[5] -48;
    date = temp1*10 + temp2;
    // now Day of Week
    dOW = (byte)inString[6] - 48;
    // now hour
    temp1 = (byte)inString[7] -48;
    temp2 = (byte)inString[8] -48;
    hour = temp1*10 + temp2;
    // now minute
    temp1 = (byte)inString[9] -48;
    temp2 = (byte)inString[10] -48;
    minute = temp1*10 + temp2;
    // now second
    temp1 = (byte)inString[11] -48;
    temp2 = (byte)inString[12] -48;
    second = temp1*10 + temp2;
}

void set_time()
{
  disable_all_spi_devices();
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(1, 55);
  tft.println("listening for updates in serial");

  if (Serial.available()) {
        getDateStuff(year, month, date, dOW, hour, minute, second);
        
        myRTC.setClockMode(false);  // set to 24h
        //setClockMode(true); // set to 12h
        
        myRTC.setYear(year);
        myRTC.setMonth(month);
        myRTC.setDate(date);
        myRTC.setDoW(dOW);
        myRTC.setHour(hour);
        myRTC.setMinute(minute);
        myRTC.setSecond(second);
        
        // Test of alarm functions
        // set A1 to one minute past the time we just set the clock
        // on current day of week.
        myRTC.setA1Time(dOW, hour, minute+1, second, 0x0, true,
                        false, false);
        // set A2 to two minutes past, on current day of month.
        myRTC.setA2Time(date, hour, minute+2, 0x0, false, false,
                        false);
        // Turn on both alarms, with external interrupt
        myRTC.turnOnAlarm(1);
        myRTC.turnOnAlarm(2);
        
    }
}

uint16_t getBatteryColor(int percentage) {
    if (percentage <= 20) {
        return TFT_RED; // Replace with the correct color code for your display
    } else if (percentage <= 40) {
        return TFT_ORANGE; // Replace with the correct color code
    } else if (percentage <= 70) {
        return TFT_YELLOW; // Replace with the correct color code
    } else {
        return TFT_GREEN; // Replace with the correct color code
    }
}
void draw_bat_ico(int x, int y, int width, int height, int percentage) {
    uint16_t color = getBatteryColor(percentage);
    if(battery.isCharging()) color = TFT_BLUE;
    // Draw the battery outline
    tft.drawRect(x, y, width, height, TFT_WHITE);
    tft.drawRect(x+width, y + 1,1, height - 2 , TFT_WHITE);
    
    // Fill the battery based on percentage
    int fillWidth = (width - 2) * percentage / 100;
    tft.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(0);
    tft.setCursor(70,4);
    tft.print(battery.getPercentage());
    tft.print("%");
}

void draw_UI_bat()
{
  draw_bat_ico(100, 2, 20, 12, battery.getPercentage());
}

unsigned long time_previousMillis = 0; // Store the last update time
const long time_interval = 1000; 

void draw_time()
{
  tft.setTextColor(TFT_WHITE); 
  DateTime now = RTClib.now(); 
  tft.setCursor(5, 4);
  tft.print(now.hour(), DEC);
  tft.print(':');
  tft.print(now.minute(), DEC);
  tft.print(':');
  tft.print(now.second(), DEC);
}



void draw_miniUI()
{
    
    unsigned long time_currentMillis = millis();
  
  if (time_currentMillis - time_previousMillis >= time_interval) {
    time_previousMillis = time_currentMillis; // Save the last update time
    disable_all_spi_devices();
    tft.fillRect(0,0,128,16,tft.color565(102, 0, 255));
    draw_time(); // Call the function to draw the time
    draw_UI_bat();
   // draw_free_heap();
  }
  if(entered == 0){
    disable_all_spi_devices();
    tft.fillRect(0,0,128,16,tft.color565(102, 0, 255));
    draw_time(); // Call the function to draw the time
    draw_UI_bat();
  }
}

void draw_miniUI_One_off()
{
    disable_all_spi_devices();
    tft.fillRect(0,0,128,16,tft.color565(102, 0, 255));
    draw_time(); // Call the function to draw the time
    draw_UI_bat();
}


const int num_reps = 128;
bool constCarrierMode = 0;
int h = 0;
RF24 radio(NRF_CS, NRF_CSN);
const uint8_t num_channels = 126;
uint8_t values[num_channels];

void get_scann()
{
    entered = 0;
    disable_all_spi_devices();
   
    tft.setRotation(1);
    tft.fillScreen(TFT_BLUE);
   
    tft.drawRect(0, 16, 129, 93, TFT_RED);
     draw_time(); // Call the function to draw the time
    draw_UI_bat();
    // Variable to track the time
    unsigned long previousMillis = 0;
    const long interval = 1000; // 1 second interval

    // Send 'g' over Serial to begin CCW output
    if (Serial.available()) {
        char c = Serial.read();
        if (shouldExit) return;  
        if (c == 'g') {
            constCarrierMode = 1;
            radio.stopListening();
            if (shouldExit) return;  
            Serial.println("Starting Carrier Out");
            radio.startConstCarrier(RF24_PA_LOW, 40);
            if (shouldExit) return;  
        } else if (c == 'e') {
            constCarrierMode = 0;
            radio.stopConstCarrier();
            Serial.println("Stopping Carrier Out");
        }
    }

    int rep_counter = num_reps;
    while (rep_counter-- && !shouldExit) {
        int i = num_channels;
        while (i-- && !shouldExit) {
            radio.setChannel(i);
            if (shouldExit) return;  
            radio.startListening();
            if (shouldExit) return;  
            delayMicroseconds(128);
            if (shouldExit) return;  
            radio.stopListening();
            if (shouldExit) return;  
            if (radio.testCarrier()) {
                ++values[i];    
            }
            disable_all_spi_devices();
            h = min((uint8_t)0xf, values[i]); 
            tft.fillRect(i + 1, 108 - h * 6, 1, h * 6, TFT_WHITE);         
        }
         tft.drawPixel(num_reps-rep_counter, 110, TFT_GREEN);
         tft.drawPixel(num_reps-rep_counter, 111, TFT_GREEN);
        // Check the elapsed time and call draw_miniUI every second
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            draw_miniUI();
        }

        tft.fillRect(0, 112, 130, 1, TFT_BLUE);    
    }
       
    for (int i = 0; i < num_channels; i++) {
        values[i] = 0;
    }
}



void draw_menu()
{      
    draw_miniUI();
      if(!entered){
        entered = 1;
        
        tft.setRotation(0);
        disable_all_spi_devices();
        draw_miniUI_One_off();
        tft.startWrite(); // Start SPI transaction
        tft.setAddrWindow(0, 16, 128, 144); // Set full-screen window (128x160)

          for (int y = 16; y < 160; y++)
          {
              for (int x = 0; x < 128; x++)
              {
                  tft.pushColor(wallpaper[y][x]); // Send each pixel in RGB565
              }
          }

          tft.endWrite(); // End SPI transaction
      // tft.setTextSize(1);
      //  tft.setRotation(0);
      // tft.setTextColor(TFT_WHITE); 
      
        //tft.drawString("Main Menu", 40, 80);  
    }
}

int app_X = 0;
int app_Y = 0;

int app_Xold = 1;
int app_Yold = 1;

int bluapp_X = 0;
int bluapp_Y = 0;

int braapp_X = 0;
int braapp_Y = 0;

bool mode = true;
bool redraw = 0;
bool inMenu = false;

void drawIcos()
{
  if(mode)
      {
      for(int y = 0;y <3; y++)
        {
          for(int x = 0;x <3; x++)
          { 
            drawImage(imageArray[y][x], x * 32 + 8 * x + 8, y * 32 + 8 * y + 32);
          }
        }
        tft.drawRect(2, 32,2 ,52, 0x632c);  
        tft.drawRect(2, 92,2 ,52, TFT_BLUE);  
      }
      else
      {

        for(int y = 0;y <3; y++)
        {
          for(int x = 0;x <3; x++)
          {  
            drawImage(imageArray[y+1][x], x * 32 + 8 * x + 8, y * 32 + 8 * y + 32);
          }
        }
        tft.drawRect(2, 32,2 ,52, TFT_BLUE); 
        tft.drawRect(2, 92,2 ,52, 0x632c);  
      }
}
void drawRed()
{
  if(mode)
  {
    tft.drawRect(app_Y*32+8*app_Y+7, app_X*32+15+8*app_X+16,34 ,34, TFT_RED);   
    tft.drawRect(app_Y*32+8*app_Y+6, app_X*32+15+8*app_X+15,36 ,36, TFT_RED); 
  }
  else
  {
    tft.drawRect(app_Y*32+8*app_Y+7, (app_X-1)*32+15+8*(app_X-1)+16,34 ,34, TFT_RED);  
    tft.drawRect(app_Y*32+8*app_Y+6, (app_X-1)*32+15+8*(app_X-1)+15,36 ,36, TFT_RED);   
  }
}
void drawBlue()
{
  if(mode)
  {
    tft.drawRect(app_Yold*32+8*app_Yold+7, app_Xold*32+15+8*app_Xold+16,34 ,34, TFT_BLUE);   
    tft.drawRect(app_Yold*32+8*app_Yold+6, app_Xold*32+15+8*app_Xold+15,36 ,36, TFT_BLUE); 
  }
  else
  {
    tft.drawRect(app_Yold*32+8*app_Yold+7, (app_Xold-1)*32+15+8*(app_Xold-1)+16,34 ,34, TFT_BLUE);  
    tft.drawRect(app_Yold*32+8*app_Yold+6, (app_Xold-1)*32+15+8*(app_Xold-1)+15,36 ,36, TFT_BLUE);   
  }
}

void draw_sub_menu()
{
  draw_miniUI();
  if(entered== 0){
    entered = 1;
    
    if(app_X < 1)
    {
     redraw = 1;
     mode = true;
    }
    if(app_X > 2) 
    {
      redraw = 1;
      mode = false;
    }

    disable_all_spi_devices();
    tft.setRotation(0);
    tft.setTextColor(TFT_WHITE); 
    if(inMenu)
    {
      if(redraw){
        drawIcos();
        redraw = 0;
      }
      drawRed();
      drawBlue();
    }
    else
    {
     tft.fillScreen(TFT_BLUE);
     draw_miniUI_One_off();
     drawIcos();
     drawRed();
     drawBlue();
    }
    inMenu = true;
  }
}

void draw_amplifier_controls()
{
  if (shouldExit)  return;  
   if(entered == 0){
     entered = 1;
    i2s_pin_config_t my_pin_config = {
        .bck_io_num = 25, ////BCLK 
        .ws_io_num = 26,  ////LRC pin 
        .data_out_num = 27, ///Din pin
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    
    a2dp_sink.set_pin_config(my_pin_config);
    a2dp_sink.start("MyMusic");
         a2dp_sink.set_volume(225);
         disable_all_spi_devices();
        tft.setRotation(0);
        tft.setTextColor(TFT_WHITE);
        tft.fillScreen(TFT_BLUE);
        draw_time();
        tft.setCursor(30, 50 + 16);
        tft.println("Bluetooth initialised :)");
   } 
}

const char* appStateToString(AppState state) {
    switch (state) {
        case SUBSTATE_NRF_SCANNER: return "NRF Scanner";
        case SUBSTATE_BLUETOOTH_AMPLIFIER: return "Bluetooth Amplifier";
         case SUBSTATE_3DENGINE: return "Simple 3D Engine®r";
        case SUBSTATE_TETRIS: return "Tetris";
        default: return "Unknown";
    }
}

void draw_bluetooth_submenu() {
    if (entered == 0) {     // Only execute once when entering the submenu
        entered = 1;
        
        disable_all_spi_devices();
        tft.setRotation(0);
        tft.setTextColor(TFT_WHITE);
        tft.fillScreen(TFT_BLUE);
        draw_miniUI_One_off();
        // Start drawing Bluetooth submenu options
        for (int i = 0; i < sizeof(blueToothaArray) / sizeof(blueToothaArray[0]); i++) {
            tft.setCursor(0, i * 10 + 16);
            if (i == bluapp_X) {
                tft.print("> ");  // Highlight selected item
            } else {
                tft.print("");  // Remove highlight for unselected items
            }
            tft.println(appStateToString(blueToothaArray[i])); 
        }
        disable_all_spi_devices();
    }
  draw_miniUI();
}


int currentFileIndex = 0;   // Track the current file index
int filesOnScreen = 8;      // Number of files to display on the screen at once
String fileList[MAX_FILES]; // Array to store file names
int fileCount = 0;    
int selectedFileIndex = 0;  // Track the selected file index for highlighting
String currentFileName;  // To hold the currently selected file's name
int currentLineIndex = 0;  // To track where you are in the file

uint16_t read16(File &file) {
  uint16_t result = file.read();
  result |= file.read() << 8;
  return result;
}

// Function to read 4 bytes (unsigned) from file
uint32_t read32(File &file) {
  uint32_t result = file.read();
  result |= file.read() << 8;
  result |= file.read() << 16;
  result |= file.read() << 24;
  return result;
}

void displayImage(String fileName) {
  File file = SD.open(fileName);
  if (!file) {
    Serial.println("Error opening image file");
    return;
  }

  // Prepare for BMP image display
  Serial.println("Displaying BMP image...");

  // Check if the file is a valid BMP file
  if (fileName.endsWith(".bmp")) {
    // Read BMP header
    uint16_t bfType = read16(file);
    if (bfType != 0x4D42) {  // 'BM' in little-endian
      Serial.println("Not a valid BMP file");
      file.close();
      return;
    }

    // Skip BMP file header
    file.seek(18);  // Skip to width/height
    uint32_t width = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t bitCount = read16(file);
    uint32_t compression = read32(file);
    
    // We assume 24-bit uncompressed BMP
    if (bitCount != 24 || compression != 0) {
      Serial.println("Only supports uncompressed 24-bit BMP images");
      file.close();
      return;
    }

    // Skip additional header info
    file.seek(54);

    // Prepare to draw image
    tft.setRotation(0);  // Set orientation
    tft.fillScreen(TFT_BLACK);  // Clear screen

    // Get screen dimensions
    uint32_t screenWidth = tft.width();
    uint32_t screenHeight = tft.height();

    // Check if the image is smaller or larger than the screen
    if (width <= screenWidth && height <= screenHeight) {
      // Image is smaller than the screen, no resizing
      // Just center it without scaling
      int offsetX = (screenWidth - width) / 2;
      int offsetY = (screenHeight - height) / 2;

      // Start drawing without scaling
      int padding = (4 - (width * 3) % 4) % 4; // BMP row padding
      for (int y = height - 1; y >= 0; y--) {  // BMP images are stored upside down
        for (int x = 0; x < width; x++) {
          // Read RGB values
          uint8_t b = file.read();  // Blue component
          uint8_t g = file.read();  // Green component
          uint8_t r = file.read();  // Red component

          // Calculate position to center image
          int displayX = x + offsetX;
          int displayY = (height - y - 1) + offsetY;  // Flip Y axis

          // Avoid drawing outside the screen bounds
          if (displayX < screenWidth && displayY < screenHeight) {
            uint32_t color = tft.color565(r, g, b);  // Convert to TFT color
            tft.drawPixel(displayX, displayY, color);  // Draw pixel
          }
        }
        file.seek(file.position() + padding);  // Skip padding at the end of each row
      }
    } else {
      // Image is larger than the screen, just display it without resizing
      int padding = (4 - (width * 3) % 4) % 4; // BMP row padding
      for (int y = height - 1; y >= 0; y--) {  // BMP images are stored upside down
        for (int x = 0; x < width; x++) {
          // Read RGB values
          uint8_t b = file.read();  // Blue component
          uint8_t g = file.read();  // Green component
          uint8_t r = file.read();  // Red component

          // Display the pixel in its original position
          if (x < screenWidth && (height - y - 1) < screenHeight) {
            uint32_t color = tft.color565(r, g, b);  // Convert to TFT color
            tft.drawPixel(x, (height - y - 1), color);  // Draw pixel
          }
        }
        file.seek(file.position() + padding);  // Skip padding at the end of each row
      }
    }

    file.close();
  } else {
    Serial.println("The file is not a BMP image");
  }
}



void displayFileContent(String fileName) {
  disable_all_spi_devices();
  tft.fillScreen(TFT_BLACK);  // Clear the screen
  
  Serial.print("Attempting to open file: ");
  Serial.println(fileName);  // Print the file name for debugging

  File file = SD.open(fileName);  // Try to open the file
  if (!file) {
    tft.setCursor(0, 0);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Error: Could not open file!");

    // Print error for debugging
    Serial.println("Error: Could not open file!");
    return;
  }

  // File opened successfully, proceed to display content
  Serial.println("File opened successfully!");

  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int y = 0;  // Y position for text
  while (file.available() && shouldExit!=1) {
    String line = file.readStringUntil('\n');
    tft.setCursor(0, y);  // Set cursor for each new line
    tft.println(line);    // Display the line
    y += 10;  // Move to next line (adjust according to your font size)
    
    if (y >= tft.height()) {
      break;  // Stop if the screen is full
    }
  }

  file.close();
  Serial.println("File read and displayed.");
}

void dispalyError()
{
 disable_all_spi_devices() ;
   tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);  // Highlight color (adjust as needed)
      tft.print("Cant open file");
}
int speaker_volume = 15;
void initAudio() {
    if (audio == nullptr) {  // Create only if it doesn’t exist
        audio = new Audio();
        audio->setPinout(AMP_BCLK, AMP_LRCLK, AMP_DIN);
        audio->setVolume(speaker_volume);
    }
}
void deinitAudio() {
    if (audio != nullptr) {
        audio->stopSong();
        delay(30);    
        delete audio;  // Free memory 
        audio = nullptr;   
    }
}
void handleAudio() {
    if (audio != nullptr) {
       disable_all_spi_devices() ;
        audio->loop();
         disable_all_spi_devices() ;
    }
}


void playmp3(String &fileName) {
  if (audio == nullptr) {
    initAudio();
     disable_all_spi_devices() ;
    audio->connecttoFS(SD, fileName.c_str());  // Convert String to const char*
     disable_all_spi_devices() ;
  }
  if(audio != nullptr)
  {
     disable_all_spi_devices() ;
    audio->connecttoFS(SD, fileName.c_str());  // Convert String to const char*
     disable_all_spi_devices() ;
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return 0; // Stop drawing if out of bounds
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void drawJpg(const char *filename, int x, int y) {
  File jpgFile = SD.open(filename);
  if (!jpgFile) {
    Serial.println("Failed to open JPG file!");
    return;
  }
  
  Serial.println("Displaying: " + String(filename));
  TJpgDec.drawSdJpg(x, y, filename);  // Draw JPG from SD
  
  jpgFile.close();
}





void handleFileSelection() {
   if (entered == 0) {
    entered = 1;
    disable_all_spi_devices();
  String fileName = "/" + fileList[selectedFileIndex];  // Ensure the file path starts with "/"
  
  Serial.print("Opening file with full path: ");
  Serial.println(fileName);  // Print the full path for debugging

  if (fileName.endsWith(".txt")) {
    currentFileName = fileName;  // Store the selected file's name
    displayFileContent(currentFileName);  // Display the file content
  }
  else if (fileName.endsWith(".bmp"))
  {
     currentFileName = fileName; 
     displayImage(currentFileName);
  }
  else if (fileName.endsWith(".mp3"))
  {
     currentFileName = fileName; 
     playmp3(currentFileName);
  }
   else if (fileName.endsWith(".m4a"))
  {
     currentFileName = fileName; 
     playmp3(currentFileName);
  }
  else if (fileName.endsWith(".jpg")) {  // Add PNG handling
         currentFileName = fileName;
          TJpgDec.setJpgScale(1); // 1 = full resolution, 2 = half, 4 = quarter
          TJpgDec.setCallback(tft_output);
        drawJpg(currentFileName.c_str(), 0, 0);
  }
  else  dispalyError();
  }
}

void displayFiles() {
  // Ensure currentFileIndex doesn't go out of bounds
  disable_all_spi_devices();
  currentFileIndex = max(0, min(currentFileIndex, fileCount - filesOnScreen));

  // Clear the screen before drawing
  tft.fillScreen(TFT_BLACK);

  // Determine how many files to show
  int startIdx = currentFileIndex;
  int endIdx = min(currentFileIndex + filesOnScreen, fileCount);  // Don't exceed fileCount

  // Draw file names on the screen
  for (int i = startIdx; i < endIdx; i++) {
    int y = (i - startIdx) * 20;  // Calculate Y position for each file name (adjust for your font size)
    tft.setCursor(0, y);

    // Highlight the selected file with a ">"
    if (i == selectedFileIndex) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);  // Highlight color (adjust as needed)
      tft.print("> ");
    } else {
      tft.setTextColor(TFT_WHITE, TFT_BLACK);   // Normal color
    }

    tft.println(fileList[i]);
  }

  // Clear any remaining space if there are fewer files than filesOnScreen
  for (int i = endIdx; i < currentFileIndex + filesOnScreen; i++) {
    int y = (i - startIdx) * 20;
    tft.setCursor(0, y);
    tft.fillRect(0, y, tft.width(), 20, TFT_BLACK);  // Clear empty lines
  }

}
void dispalySD_OPEN_Error(String err)
{
 disable_all_spi_devices() ;
   tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);  // Highlight color (adjust as needed)
      tft.print(err);
}

void draw_files() {
  if (entered == 0) {
    entered = 1;
    disable_all_spi_devices();

    #ifdef REASSIGN_PINS
  
  if (!SD.begin(SD_CS)) {
#else
  if (!SD.begin()) {
#endif
    Serial.println("Card Mount Failed");
     dispalySD_OPEN_Error("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
     dispalySD_OPEN_Error("No SD card attached");
    return;
  }

    File root = SD.open("/");
    if (!root) {
      Serial.println("Failed to open sd card");
      dispalySD_OPEN_Error("Failed to open sd card");
      return;
    }

    // Reset file list and count
    fileCount = 0;

    // Get the list of files
    while (true) {
      File entry = root.openNextFile();
      if (!entry) {
        break;  // No more files
      }
      if (!entry.isDirectory()) {
        fileList[fileCount] = entry.name();
        fileCount++;
      }
      entry.close();
    }
    root.close();

    // Ensure selectedFileIndex is valid
    selectedFileIndex = min(selectedFileIndex, fileCount - 1);

    displayFiles();
  }
}

void draw_add()
{
  if(!entered)
  {
    entered = 1;
    disable_all_spi_devices();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLUE);
     tft.setCursor(30, 6);       
     tft.println("link to git page");
      tft.startWrite(); 
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 128; x++) {
            tft.drawPixel(0 + x, 16 + y, git[y][x]);
        }
    }
     tft.endWrite();
  }
}
  
void draw_brainroot() {
    draw_miniUI();
    if (entered == 0) {     // Only execute once when entering the submenu
        entered = 1;
        disable_all_spi_devices();
        tft.setRotation(0);
        tft.setTextColor(TFT_WHITE);
        tft.fillScreen(TFT_BLUE);
        draw_miniUI_One_off();
        // Start drawing Bluetooth submenu options
        for (int i = 0; i < sizeof(BrainrootArray) / sizeof(BrainrootArray[0]); i++) {
            tft.setCursor(0, i * 10 + 16);
            if (i == braapp_X) {
                tft.print("> ");  // Highlight selected item
            } else {
                tft.print("");  // Remove highlight for unselected items
            }
            tft.println(appStateToString(BrainrootArray[i])); 
        }
        disable_all_spi_devices();
    }
}
void draw_free_heap()
{
  float freeRAM = ESP.getFreeHeap() / 1024.0; // Convert free heap to KB
  float freePSRAM = ESP.getFreePsram() / 1024.0; // Convert free heap to KB
  // Use String class to format with 2 decimal places
  String formattedText = String(freeRAM, 2);
   String formattedTexta = String(freePSRAM, 2);
  tft.setTextColor(TFT_WHITE); 
  tft.setCursor(0,30);
  tft.print("RAM: ");
  tft.print(formattedText);
  tft.print("KB");
  tft.setTextColor(TFT_WHITE); 
  tft.setCursor(0,46);
  tft.print("PSRAM: ");
  tft.print(formattedTexta);
  tft.print("KB");
}


int last_but_buzz_freq = 400;
int Screen_brightness = 50;
int settings_x = 0;

void setBrightness(int brightness) {
  brightness = constrain(brightness, 0, 100);
  int dutyCycle = map(brightness, 0, 100, 0, 225);
  analogWrite(BACKL_PIN, dutyCycle);
}

void settings()
{
  if(entered== 0)
  {
    entered = 1;
    draw_miniUI_One_off();
    disable_all_spi_devices();
    tft.setRotation(0);
    tft.fillRect(0,16,128,144,TFT_BLACK);
    tft.setCursor(0, 16);
    tft.setTextColor(TFT_GREEN);
    if(settings_x == 0)tft.print("> ");
    tft.setTextColor(TFT_WHITE);
    tft.print("buzzer:  ");
    tft.print(but_buzz_freq);
    draw_free_heap();

    tft.setCursor(0, 62);
    tft.setTextColor(TFT_GREEN);
    if(settings_x == 1)tft.print("> ");
    tft.setTextColor(TFT_WHITE);
    tft.print("brightness:  ");
    tft.print(Screen_brightness); 

    tft.setCursor(0, 78);
    tft.setTextColor(TFT_GREEN);
    if(settings_x == 2)tft.print("> ");
    tft.setTextColor(TFT_WHITE);
    tft.print("volume:  ");
    tft.print(speaker_volume); 
  }
}


void checkLines();
void breakLine(short line);
bool gameover();
void gameoverscreen();
void refresh();
void drawGrid();
boolean nextHorizontalCollision(short piece[2][4], int amount);
boolean nextCollision();
void generate();
void drawPiece(short type, short rotation, short x, short y);
void drawNextPiece();
void copyPiece(short piece[2][4], short type, short rotation);
short getMaxRotation(short type);
boolean canRotate(short rotation);
void drawLayout();
short getNumberLength(int n);
void drawText(const char* text, int x, int y);


#define WIDTH 64 // OLED display width, in pixels

#define HEIGHT 128 // OLED display height, in pixels

const char pieces_S_l[2][2][4] = { { {0, 0, 1, 1}, {0, 1, 1, 2} },
                                   { {0, 1, 1, 2}, {1, 1, 0, 0} } };

const char pieces_S_r[2][2][4] = { { {1, 1, 0, 0}, {0, 1, 1, 2}},
                                   { {0, 1, 1, 2}, {0, 0, 1, 1} } };

const char pieces_L_l[4][2][4] = { { {0, 0, 0, 1}, {0, 1, 2, 2} },
                                   { {0, 1, 2, 2}, {1, 1, 1, 0} }, 
                                   { {0, 1, 1, 1}, {0, 0, 1, 2} },
                                   { {0, 0, 1, 2}, {1, 0, 0, 0} } };

const char pieces_Sq[1][2][4] = { { {0, 1, 0, 1}, {0, 0, 1, 1} } };

const char pieces_T[4][2][4] =  { { {0, 0, 1, 0}, {0, 1, 1, 2} },
                                  { {0, 1, 1, 2}, {1, 0, 1, 1} },
                                  { {1, 0, 1, 1}, {0, 1, 1, 2} },
                                  { {0, 1, 1, 2}, {0, 0, 1, 0} } };

const char pieces_l[2][2][4] =  { { {0, 1, 2, 3}, {0, 0, 0, 0} },
                                  { {0, 0, 0, 0}, {0, 1, 2, 3} } };

const short MARGIN_TOP = 19;
const short MARGIN_LEFT = 3;
const short SIZE = 5;
const short TYPES = 6;

word currentType, nextType, rotation;
short pieceX, pieceY;
short piece[2][4];
int interval = 20, score;
long timer;
boolean grid[10][18];

int restart = 0;
int highscore = 0;

void checkLines() {
  boolean full;
  for (short y = 17; y >= 0; y--) {
    full = true;
    for (short x = 0; x < 10; x++) {
      full = full && grid[x][y];
    }
    if (full) {
      breakLine(y);
      y++;
    }
  }
}


void breakLine(short line) {
  for (short y = line; y >= 0; y--) {
    for (short x = 0; x < 10; x++) {
      grid[x][y] = grid[x][y - 1];
    }
  }
  for (short x = 0; x < 10; x++) {
    grid[x][0] = 0;
  }
  score += 10;
}

bool gameover() {
  boolean go;
  int t = 0;
  for (short y = 0; y < 19; y++) {
    go = false;
    for (short x = 0; x < 10; x++) {
      go = go || grid[x][y];
    }
    if (go) {
      t++;
    }
  }
  if (t > 18) return true;
  return false;
}


void drawGrid() {
  for (short x = 0; x < 10; x++)
    for (short y = 0; y < 18; y++)
      if (grid[x][y])
        tft.fillRect(MARGIN_LEFT + (SIZE + 1)*x, MARGIN_TOP + (SIZE + 1)*y, SIZE, SIZE, TFT_WHITE);
}

boolean nextHorizontalCollision(short piece[2][4], int amount) {
  for (short i = 0; i < 4; i++) {
    short newX = pieceX + piece[0][i] + amount;
    if (newX > 9 || newX < 0 || grid[newX][pieceY + piece[1][i]])
      return true;
  }
  return false;
}
boolean nextCollision() {
  for (short i = 0; i < 4; i++) {
    short y = pieceY + piece[1][i] + 1;
    short x = pieceX + piece[0][i];
    if (y > 17 || grid[x][y])
      return true;
  }
  return false;
}

void generate() {
  randomSeed(analogRead(0));
  currentType = nextType;
  nextType = random (TYPES);
  if (currentType != 5)
    pieceX = 4;
  else
    pieceX = 3;
  pieceY = 0;
  rotation = 0;
  copyPiece(piece, currentType, rotation);
}

void drawPiece(short type, short rotation, short x, short y) {
  for (short i = 0; i < 4; i++)
    tft.fillRect(MARGIN_LEFT + (SIZE + 1) * (x + piece[0][i]), MARGIN_TOP + (SIZE + 1) * (y + piece[1][i]), SIZE, SIZE, TFT_WHITE);
}

void drawNextPiece() {
  short nPiece[2][4];
  copyPiece(nPiece, nextType, 0);
  for (short i = 0; i < 4; i++)
    tft.fillRect(50 + 3 * nPiece[0][i], 4 + 3 * nPiece[1][i], 2, 2, TFT_WHITE);
}

void copyPiece(short piece[2][4], short type, short rotation) {
  switch (type) {
    case 0: //L_l
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_L_l[rotation][0][i];
        piece[1][i] = pieces_L_l[rotation][1][i];
      }
      break;
    case 1: //S_l
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_S_l[rotation][0][i];
        piece[1][i] = pieces_S_l[rotation][1][i];
      }
      break;
    case 2: //S_r
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_S_r[rotation][0][i];
        piece[1][i] = pieces_S_r[rotation][1][i];
      }
      break;
    case 3: //Sq
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_Sq[0][0][i];
        piece[1][i] = pieces_Sq[0][1][i];
      }
      break;
    case 4: //T
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_T[rotation][0][i];
        piece[1][i] = pieces_T[rotation][1][i];
      }
      break;
    case 5: //l
      for (short i = 0; i < 4; i++) {
        piece[0][i] = pieces_l[rotation][0][i];
        piece[1][i] = pieces_l[rotation][1][i];
      }
      break;
  }
}

short getMaxRotation(short type) {
  if (type == 1 || type == 2 || type == 5)
    return 2;
  else if (type == 0 || type == 4)
    return 4;
  else if (type == 3)
    return 1;
  else
    return 0;
}

boolean canRotate(short rotation) {
  short piece[2][4];
  copyPiece(piece, currentType, rotation);
  return !nextHorizontalCollision(piece, 0);
}
void drawText(const char* text, int x, int y) {
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(x, y);
  tft.print(text);
}
void resetGame() {
  for (short y = 0; y < 18; y++) {
    for (short x = 0; x < 10; x++) {
      grid[x][y] = false;
    }
  }
  highscore = score;
  score = 0;
  drawLayout();
  generate();
  timer = millis();
}


void drawLayout() {
  tft.drawLine(0, 15, WIDTH, 15, TFT_WHITE);
  tft.drawRect(0, 0, WIDTH, HEIGHT, TFT_WHITE);
  drawNextPiece();
  char text[6];
  itoa(score, text, 10);
  drawText(text,  7, 4);
}

void gameoverscreen() {
  tft.fillScreen(TFT_BLACK);
  drawLayout();
  drawText("GAMEOVER",  9, 40);
  drawText("Highscore",  5, 50);
  char text1[6];
  itoa(highscore, text1, 10);
  drawText(text1, 15, 60);
  drawText("Score",  18, 70);
  char text2[6];
  itoa(score, text2, 10);
  drawText(text2,  15, 80);
}

void refresh() {
    tft.fillScreen(TFT_BLACK);
  drawLayout();
  drawGrid();
  drawPiece(currentType, 0, pieceX, pieceY);
}
int tetbut = 0;
int exec = 0;
void tetrobuttonhandler()
{
  if(!exec){
          switch (tetbut ) {
          case 4: // Left move
            if (!nextHorizontalCollision(piece, -1)) {
              pieceX--;
              refresh();
            }
            break;
          case 6: // Right move
            if (!nextHorizontalCollision(piece, 1)) {
              pieceX++;
              refresh();
            }
            break;
          case 5: // Rotate
            if (rotation == getMaxRotation(currentType) - 1 && canRotate(0)) {
              rotation = 0;
            } else if (canRotate(rotation + 1)) {
              rotation++;
            }
            copyPiece(piece, currentType, rotation);
            refresh();
            break;
          case 7: // Speed up
            interval = 20;
            break;
          case 2: // Normal speed
            interval = 400;
            break;
          default:
            break;
        } 
        exec = 1;
  }
}
int  tetrisentered = 0;
void draw_tetris()
{
  if (!tetrisentered) {
    tetrisentered = 1;
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_WHITE);
    drawText("TETRIS", 40, 20);
    drawText("GAME", 50, 30);
    delay(2000);
    tft.fillScreen(TFT_BLACK);

    randomSeed(analogRead(36));
    nextType = random(TYPES);
    drawLayout();
    generate();
    timer = millis();
  }

  if (millis() - timer > interval) {
    checkLines();
    refresh();
    if (nextCollision()) {
      for (short i = 0; i < 4; i++)
        grid[pieceX + piece[0][i]][pieceY + piece[1][i]] = 1;
      generate();
    } else {
      pieceY++;
    }
    timer = millis();
  }
  if (gameover()) {
    gameoverscreen();
    delay(2000);
    resetGame();
  }

}
int numNetworks = -1;
bool rescanWIFI = false;
bool drawWIFI = true;
void wifi()
{
  draw_miniUI();
  if (!entered)
  {
    entered = 1;
    WiFi.mode(WIFI_STA);          // Set Wi-Fi to station mode
    WiFi.disconnect();           // Disconnect from any previous connection
     tft.setRotation(3);          // Adjust the display rotation to suit your screen
    tft.fillScreen(TFT_BLACK);   // Clear the screen with black
    draw_miniUI_One_off();
    tft.setTextColor(TFT_WHITE); // Set text color to white
    tft.setTextSize(1);          // Set text size
    tft.setCursor(0, 16);         // Start writing from the top left corner
     tft.println("Wi-Fi Network Analyzer");
      WiFi.scanNetworks(true); // Start an async scan
      drawWIFI == true;     
  }
  numNetworks = WiFi.scanComplete();
  if(rescanWIFI) 
  {
     WiFi.scanNetworks(true);
     rescanWIFI = false;
  }
  if (numNetworks == 0) {
    tft.println("No networks found");
  }
  else if (numNetworks > 0 && drawWIFI == true){
     tft.fillScreen(TFT_BLACK);  
     draw_miniUI_One_off();
       tft.setCursor(0, 16); 
    for (int i = 0; i < numNetworks; i++) {
      tft.print("SSID: ");
      tft.println(WiFi.SSID(i));
      tft.print("Signal Strength (RSSI): ");
      tft.println(WiFi.RSSI(i));
      tft.println();   
    }
    drawWIFI = false;
    
  }
}




                                       ////// 3D ENGINE //////


uint16_t* frameBuffer = nullptr;  // Pointer for dynamic allocation


  #define PIXEL_DENSITY 91
// Cube properties
float angleX = 0.0, angleY = 0.0;
float cubeSize = 64.0;
float centerX = 80, centerY = 60;  // Center of display
bool antiAliasedMode = true; 
bool anisotropicFilteringMode = true; 
bool FPScounterMode = true; 
bool flashLightMode = true; 
unsigned long previousfpsMillis = 0;  // To store the last time the FPS was updated
unsigned long currentfpsMillis = 0;   // Current time
float frameCount = 0;                // Frame counter
float fps = 0;                       // Store the FPS value

int lightVec[3] = {0,0,2};
float maxDist = cubeSize*sqrtf(2)/2.1;
float lightPower = 2;

// 3D cube vertices
float vertices[8][3] = {
  {-1, -1, -1},
  { 1, -1, -1},
  { 1,  1, -1},
  {-1,  1, -1},
  {-1, -1,  1},
  { 1, -1,  1},
  { 1,  1,  1},
  {-1,  1,  1}
};

// Lines between vertices
int edges[12][2] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0},
  {4, 5}, {5, 6}, {6, 7}, {7, 4},
  {0, 4}, {1, 5}, {2, 6}, {3, 7}
};


 void drawPixelToBuffer(int x, int y, uint16_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        frameBuffer[y * SCREEN_WIDTH + x] = color; // Write color to buffer
    }
}
// Function to blend two colors with intensity
uint16_t blendColors(uint16_t color1, uint16_t color2, float intensity) {
  uint8_t r1 = (color1 >> 11) & 0x1F;
  uint8_t g1 = (color1 >> 5) & 0x3F;
  uint8_t b1 = color1 & 0x1F;

  uint8_t r2 = (color2 >> 11) & 0x1F;
  uint8_t g2 = (color2 >> 5) & 0x3F;
  uint8_t b2 = color2 & 0x1F;

  uint8_t r = r1 * (1 - intensity) + r2 * intensity;
  uint8_t g = g1 * (1 - intensity) + g2 * intensity;
  uint8_t b = b1 * (1 - intensity) + b2 * intensity;

  return (r << 11) | (g << 5) | b;
}
#define FIXED_POINT_MULT 1024  // Scaling factor for fixed-point arithmetic

void drawTexturedFace(float projected[4][2], const uint16_t* texture) {
    // Define the four corners of the face on the screen
    float x0 = projected[0][0], y0 = projected[0][1];
    float x1 = projected[1][0], y1 = projected[1][1];
    float x2 = projected[2][0], y2 = projected[2][1];
    float x3 = projected[3][0], y3 = projected[3][1];

    // Precompute edge interpolation increments (fixed-point)
    int x0_fp = x0 * FIXED_POINT_MULT;
    int y0_fp = y0 * FIXED_POINT_MULT;
    int x1_fp = x1 * FIXED_POINT_MULT;
    int y1_fp = y1 * FIXED_POINT_MULT;
    int x2_fp = x2 * FIXED_POINT_MULT;
    int y2_fp = y2 * FIXED_POINT_MULT;
    int x3_fp = x3 * FIXED_POINT_MULT;
    int y3_fp = y3 * FIXED_POINT_MULT;

    int dx0_fp = (x3_fp - x0_fp) / PIXEL_DENSITY;
    int dy0_fp = (y3_fp - y0_fp) / PIXEL_DENSITY;
    int dx1_fp = (x2_fp - x1_fp) / PIXEL_DENSITY;
    int dy1_fp = (y2_fp - y1_fp) / PIXEL_DENSITY;

    // Precompute texture indices for each row and column
    int texXRow[PIXEL_DENSITY];
    for (int tx = 0; tx < PIXEL_DENSITY; tx++) {
        texXRow[tx] = (tx * 64) / PIXEL_DENSITY;
    }

    for (int ty = 0; ty < PIXEL_DENSITY; ty++) {
        // Interpolate along the edges (fixed-point)
        int xLeft_fp = x0_fp + ty * dx0_fp;
        int yLeft_fp = y0_fp + ty * dy0_fp;
        int xRight_fp = x1_fp + ty * dx1_fp;
        int yRight_fp = y1_fp + ty * dy1_fp;

        int dx_fp = (xRight_fp - xLeft_fp) / PIXEL_DENSITY;
        int dy_fp = (yRight_fp - yLeft_fp) / PIXEL_DENSITY;

        // Precompute texture row index
        int texY = (ty * 64) / PIXEL_DENSITY;

        for (int tx = 0; tx < PIXEL_DENSITY; tx++) {
            // Interpolate screen position (fixed-point)
            int x_fp = xLeft_fp + tx * dx_fp;
            int y_fp = yLeft_fp + tx * dy_fp;

            // Convert fixed-point back to integer
            int x = x_fp / FIXED_POINT_MULT;
            int y = y_fp / FIXED_POINT_MULT;

            // Access the precomputed texture column index
            int texX = texXRow[tx];

            // Access the color from the texture
            uint16_t color = texture[texY * 64 + texX];
                   if(flashLightMode)
            {
              
            uint8_t r = (color >> 11) & 0x1F;  // Extract 5-bit red
            uint8_t g = (color >> 5) & 0x3F;   // Extract 6-bit green
            uint8_t b = color & 0x1F;          // Extract 5-bit blue

            float distance = sqrtf((x - centerX)*(x - centerX)+(y - centerY)*(y - centerY));
            float dot = distance/(maxDist);
            dot = 1-dot;
            dot *= lightPower;
            if(dot<0.25f) dot = 0.25f;
            r = (uint8_t)fminf(r * dot, 31);  // Ensure max value is not exceeded
            g = (uint8_t)fminf(g * dot, 63);  // Clamp to 255 if the result exceeds
            b = (uint8_t)fminf(b * dot, 31);



            uint16_t shadedColor = (r << 11) | (g << 5) | b;
            color = shadedColor;
            }
            // Draw the pixel at the interpolated screen position
            drawPixelToBuffer(x, y, color);
        }
    }
}

// Helper function to perform bilinear interpolation between four colors
uint16_t interpolateColor(uint16_t texTL, uint16_t texTR, uint16_t texBL, uint16_t texBR, float fracX, float fracY) {
    // Extract RGB565 components
    int rTL = (texTL >> 11) & 0x1F, gTL = (texTL >> 5) & 0x3F, bTL = texTL & 0x1F;
    int rTR = (texTR >> 11) & 0x1F, gTR = (texTR >> 5) & 0x3F, bTR = texTR & 0x1F;
    int rBL = (texBL >> 11) & 0x1F, gBL = (texBL >> 5) & 0x3F, bBL = texBL & 0x1F;
    int rBR = (texBR >> 11) & 0x1F, gBR = (texBR >> 5) & 0x3F, bBR = texBR & 0x1F;

    // Interpolate red, green, and blue channels
    int rTop = rTL + fracX * (rTR - rTL);
    int gTop = gTL + fracX * (gTR - gTL);
    int bTop = bTL + fracX * (bTR - bTL);
    int rBottom = rBL + fracX * (rBR - rBL);
    int gBottom = gBL + fracX * (gBR - gBL);
    int bBottom = bBL + fracX * (bBR - bBL);

    int r = rTop + fracY * (rBottom - rTop);
    int g = gTop + fracY * (gBottom - gTop);
    int b = bTop + fracY * (bBottom - bTop);

    // Combine interpolated components into a single RGB565 value
    return (r << 11) | (g << 5) | b;
}


void drawTexturedFaceWithFiltering(float projected[4][2], const uint16_t* texture) {
    // Define the four corners of the face on the screen
    float x0 = projected[0][0], y0 = projected[0][1];
    float x1 = projected[1][0], y1 = projected[1][1];
    float x2 = projected[2][0], y2 = projected[2][1];
    float x3 = projected[3][0], y3 = projected[3][1];

    // Precompute edge interpolation increments (fixed-point)
    int x0_fp = x0 * FIXED_POINT_MULT;
    int y0_fp = y0 * FIXED_POINT_MULT;
    int x1_fp = x1 * FIXED_POINT_MULT;
    int y1_fp = y1 * FIXED_POINT_MULT;
    int x2_fp = x2 * FIXED_POINT_MULT;
    int y2_fp = y2 * FIXED_POINT_MULT;
    int x3_fp = x3 * FIXED_POINT_MULT;
    int y3_fp = y3 * FIXED_POINT_MULT;

    int dx0_fp = (x3_fp - x0_fp) / PIXEL_DENSITY;
    int dy0_fp = (y3_fp - y0_fp) / PIXEL_DENSITY;
    int dx1_fp = (x2_fp - x1_fp) / PIXEL_DENSITY;
    int dy1_fp = (y2_fp - y1_fp) / PIXEL_DENSITY;

    for (int ty = 0; ty < PIXEL_DENSITY; ty++) {
        // Interpolate along the edges (fixed-point)
        int xLeft_fp = x0_fp + ty * dx0_fp;
        int yLeft_fp = y0_fp + ty * dy0_fp;
        int xRight_fp = x1_fp + ty * dx1_fp;
        int yRight_fp = y1_fp + ty * dy1_fp;

        int dx_fp = (xRight_fp - xLeft_fp) / PIXEL_DENSITY;
        int dy_fp = (yRight_fp - yLeft_fp) / PIXEL_DENSITY;

        // Precompute texture row index
        float v = (float)ty / PIXEL_DENSITY; // Normalize ty
        int texY = v * 64;
        float fracY = (v * 64) - texY;

        for (int tx = 0; tx < PIXEL_DENSITY; tx++) {
            // Interpolate screen position (fixed-point)
            int x_fp = xLeft_fp + tx * dx_fp;
            int y_fp = yLeft_fp + tx * dy_fp;

            // Convert fixed-point back to integer
            int x = x_fp / FIXED_POINT_MULT;
            int y = y_fp / FIXED_POINT_MULT;

            // Calculate texture coordinates
            float u = (float)tx / PIXEL_DENSITY; // Normalize tx
            int texX = u * 64;
            float fracX = (u * 64) - texX;

            // Sample neighboring texels
            uint16_t texTL = texture[texY * 64 + texX];            // Top-Left
            uint16_t texTR = texture[texY * 64 + (texX + 1) % 64]; // Top-Right
            uint16_t texBL = texture[(texY + 1) % 64 * 64 + texX]; // Bottom-Left
            uint16_t texBR = texture[(texY + 1) % 64 * 64 + (texX + 1) % 64]; // Bottom-Right

            // Perform bilinear interpolation
            uint16_t color = interpolateColor(texTL, texTR, texBL, texBR, fracX, fracY);
            if(flashLightMode)
            {
              
            uint8_t r = (color >> 11) & 0x1F;  // Extract 5-bit red
            uint8_t g = (color >> 5) & 0x3F;   // Extract 6-bit green
            uint8_t b = color & 0x1F;          // Extract 5-bit blue

            float distance = sqrtf((x - centerX)*(x - centerX)+(y - centerY)*(y - centerY));
            float dot = distance/(maxDist);
            dot = 1-dot;
            dot *= lightPower;
            if(dot<0.25f) dot = 0.25f;
            r = (uint8_t)fminf(r * dot, 31);  // Ensure max value is not exceeded
            g = (uint8_t)fminf(g * dot, 63);  // Clamp to 255 if the result exceeds
            b = (uint8_t)fminf(b * dot, 31);



            uint16_t shadedColor = (r << 11) | (g << 5) | b;
            color = shadedColor;
            }
            // Draw the pixel at the interpolated screen position
            drawPixelToBuffer(x, y, color);
        }
    }
}


// Helper function to draw a pixel with intensity (0.0 to 1.0)
void drawPixelWithIntensity(int x, int y, float intensity, uint16_t color) {
  if (x < 0 || y < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) return;

  uint16_t existingColor =  frameBuffer[y * SCREEN_WIDTH + x]; // Function to read pixel color from the buffer
  color = blendColors(existingColor, color, intensity);
  if(flashLightMode)
  {
    
            uint8_t r = (color >> 11) & 0x1F;  // Extract 5-bit red
            uint8_t g = (color >> 5) & 0x3F;   // Extract 6-bit green
            uint8_t b = color & 0x1F;          // Extract 5-bit blue

    float distance = sqrtf((x - centerX)*(x - centerX)+(y - centerY)*(y - centerY));
            float dot = distance/(maxDist);
            dot = 1-dot;
            dot *= lightPower;
            if(dot<0.25f) dot = 0.25f;
            r = (color >> 11) & 0x1F;  // Extract 5-bit red
             g = (color >> 5) & 0x3F;   // Extract 6-bit green
             b = color & 0x1F;          // Extract 5-bit blue

            r = (uint8_t)fminf(r * dot, 31);  // Ensure max value is not exceeded
            g = (uint8_t)fminf(g * dot, 63);  // Clamp to 255 if the result exceeds
            b = (uint8_t)fminf(b * dot, 31);



            uint16_t shadedColor = (r << 11) | (g << 5) | b;
            // Draw the pixel at the interpolated screen position
            color = shadedColor;
  }

   frameBuffer[y * SCREEN_WIDTH + x]= color; // Function to set pixel color in the buffer
}

// Draw an anti-aliased line using Wu's line algorithm

void drawAntialiasedLine(int x0, int y0, int x1, int y1, uint16_t color) {

  bool steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    int temp = x0; x0 = y0; y0 = temp;
    temp = x1; x1 = y1; y1 = temp;
  }
  if (x0 > x1) {
    int temp = x0; x0 = x1; x1 = temp;
    temp = y0; y0 = y1; y1 = temp;
  }

  int dx = x1 - x0;
  int dy = y1 - y0;
  float gradient = (dx == 0) ? 1 : (float)dy / dx;

  // Start point
  float xEnd = round(x0);
  float yEnd = y0 + gradient * (xEnd - x0);
  float xGap = 1 - (x0 + 0.5 - floor(x0 + 0.5));
  int xPixel1 = xEnd;
  int yPixel1 = floor(yEnd);

  if (steep) {
    drawPixelWithIntensity(yPixel1, xPixel1, (1 - (yEnd - yPixel1)) * xGap, color);
    drawPixelWithIntensity(yPixel1 + 1, xPixel1, (yEnd - yPixel1) * xGap, color);
  } else {
    drawPixelWithIntensity(xPixel1, yPixel1, (1 - (yEnd - yPixel1)) * xGap, color);
    drawPixelWithIntensity(xPixel1, yPixel1 + 1, (yEnd - yPixel1) * xGap, color);
  }

  // Main loop
  float intery = yEnd + gradient;
  for (int x = xPixel1 + 1; x < x1; x++) {
    if (steep) {
      drawPixelWithIntensity(floor(intery), x, 1 - (intery - floor(intery)), color);
      drawPixelWithIntensity(floor(intery) + 1, x, intery - floor(intery), color);
    } else {
      drawPixelWithIntensity(x, floor(intery), 1 - (intery - floor(intery)), color);
      drawPixelWithIntensity(x, floor(intery) + 1, intery - floor(intery), color);
    }
    intery += gradient;
  }

  // End point
  xEnd = round(x1);
  yEnd = y1 + gradient * (xEnd - x1);
  xGap = x1 + 0.5 - floor(x1 + 0.5);
  int xPixel2 = xEnd;
  int yPixel2 = floor(yEnd);

  if (steep) {
    drawPixelWithIntensity(yPixel2, xPixel2, (1 - (yEnd - yPixel2)) * xGap, color);
    drawPixelWithIntensity(yPixel2 + 1, xPixel2, (yEnd - yPixel2) * xGap, color);
  } else {
    drawPixelWithIntensity(xPixel2, yPixel2, (1 - (yEnd - yPixel2)) * xGap, color);
    drawPixelWithIntensity(xPixel2, yPixel2 + 1, (yEnd - yPixel2) * xGap, color);
  }
}

// Rotate and project vertex
void rotateAndProject(float *vertex, float *projectedVertex) {
  float xRotated = vertex[0];
  float yRotated = cos(angleX) * vertex[1] - sin(angleX) * vertex[2];
  float zRotated = sin(angleX) * vertex[1] + cos(angleX) * vertex[2];

  float xFinal = cos(angleY) * xRotated + sin(angleY) * zRotated;
  float yFinal = yRotated;
  float zFinal = -sin(angleY) * xRotated + cos(angleY) * zRotated;

  float distance = 3;
  float scale = cubeSize / (zFinal + distance);
  projectedVertex[0] = centerX + scale * xFinal;
  projectedVertex[1] = centerY + scale * yFinal;
  projectedVertex[2] = zFinal;  // Store z for sorting
}
void drawCube() {
    float projected[8][3];  // Store x, y, and z for depth sorting

    // Project vertices to 2D space
    for (int i = 0; i < 8; i++) {
        rotateAndProject(vertices[i], projected[i]);
    }

    // Define the faces of the cube (each face is made up of 4 vertices)
    int faces[6][4] = {
        {0, 1, 2, 3}, // Front face
        {4, 5, 6, 7}, // Back face
        {0, 1, 5, 4}, // Left face
        {2, 3, 7, 6}, // Right face
        {0, 3, 7, 4}, // Top face
        {1, 2, 6, 5}  // Bottom face
    };

    // Create an array to store the depth of each face
    float depths[6];

    // Calculate the average depth of each face to determine draw order
    for (int i = 0; i < 6; i++) {
        float zSum = 0.0;
        for (int j = 0; j < 4; j++) {
            int vertexIdx = faces[i][j];
            zSum += projected[vertexIdx][2];  // Use transformed z
        }
        depths[i] = zSum / 4.0;  // Average depth of the face
    }

    // Sort faces based on depth (back faces first)
    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            if (depths[i] < depths[j]) {  // Swap if i is nearer than j
                std::swap(depths[i], depths[j]);
                std::swap(faces[i], faces[j]);
            }
        }
    }

    // Draw each face of the cube in back-to-front order
     for (int i = 3; i < 6; i++) {  // Only the farthest 3 faces
        int v0 = faces[i][0];
        int v1 = faces[i][1];
        int v2 = faces[i][2];
        int v3 = faces[i][3];

        // Create an array of projected coordinates for the face
        float face[4][2] = {
            {projected[v0][0], projected[v0][1]},
            {projected[v1][0], projected[v1][1]},
            {projected[v2][0], projected[v2][1]},
            {projected[v3][0], projected[v3][1]}
        };

        if(anisotropicFilteringMode)
        {
          drawTexturedFaceWithFiltering(face, textureData[0]);
        }
        else{
          // Draw the textured face
          drawTexturedFace(face, textureData[0]); // Pass the texture data here
        }

        // Optional: Draw the edges of the cube for better visualization
        if (antiAliasedMode) {
            drawAntialiasedLine((int)projected[v0][0], (int)projected[v0][1],
                                (int)projected[v1][0], (int)projected[v1][1], TFT_WHITE);
            drawAntialiasedLine((int)projected[v1][0], (int)projected[v1][1],
                                (int)projected[v2][0], (int)projected[v2][1], TFT_WHITE);
            drawAntialiasedLine((int)projected[v2][0], (int)projected[v2][1],
                                (int)projected[v3][0], (int)projected[v3][1], TFT_WHITE);
            drawAntialiasedLine((int)projected[v3][0], (int)projected[v3][1],
                                (int)projected[v0][0], (int)projected[v0][1], TFT_WHITE);
        } else {
            // Draw normal lines for edges
        // Draw normal lines for edges with x and y inverted
        tft.drawLine((int)projected[v0][1], (int)projected[v0][0], 
                    (int)projected[v1][1], (int)projected[v1][0], TFT_WHITE);
        tft.drawLine((int)projected[v1][1], (int)projected[v1][0], 
                    (int)projected[v2][1], (int)projected[v2][0], TFT_WHITE);
        tft.drawLine((int)projected[v2][1], (int)projected[v2][0], 
                    (int)projected[v3][1], (int)projected[v3][0], TFT_WHITE);
        tft.drawLine((int)projected[v3][1], (int)projected[v3][0], 
                    (int)projected[v0][1], (int)projected[v0][0], TFT_WHITE);
        }
    }
}

void allocateBuffer() {
  if (frameBuffer == nullptr) {
        frameBuffer = (uint16_t*) ps_malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));
        if (frameBuffer == nullptr) {
          //  Serial.println("Failed to allocate framebuffer in PSRAM!");
        } else {
           // Serial.println("Framebuffer allocated in PSRAM.");
        }
    }
}

void freeBuffer() {
    if (frameBuffer != nullptr) {
        free(frameBuffer);
        frameBuffer = nullptr;
       //Serial.println("Framebuffer freed.");
    }
}


// Main engine function
void _3DEngine() {

  if(!FPScounterMode)
  {
    if(entered == 0)
    {
      entered = 1;
      allocateBuffer();  // Allocate when needed
      disable_all_spi_devices();
      tft.setRotation(1);
      memset(frameBuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)); // Fill with black

      drawCube();
      tft.startWrite();

      // Loop through each pixel in the 120x160 buffer
      for (int ty = 0; ty < 128; ty++) {
          for (int tx = 0; tx < 160; tx++) {
              tft.drawPixel(ty, tx,  frameBuffer[ty * SCREEN_WIDTH + tx]);
          }
      }
      tft.endWrite();
      freeBuffer();
    }
  }
  else{

    currentfpsMillis = millis();  // Get the current time

    // Increment frame counter
    frameCount++;

    // Calculate FPS every second (or whenever it's time to update FPS)
    if (currentfpsMillis - previousfpsMillis >= 1000) {
        // One second has passed, update FPS
        float timeElapsed = (currentfpsMillis - previousfpsMillis) / 1000.0;  // Convert to seconds
        fps = frameCount / timeElapsed;  // Calculate FPS

        // Reset frame count and update time
        frameCount = 0;
        previousfpsMillis = currentfpsMillis;
    }
      allocateBuffer();  // Allocate when needed
      disable_all_spi_devices();
      tft.setRotation(0);
      memset(frameBuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t)); // Fill with black

      drawCube();
      tft.startWrite();

    // Loop through each pixel in the 120x160 buffer
      for (int ty = 0; ty < 128; ty++) {
          for (int tx = 0; tx < 160; tx++) {
              tft.drawPixel(ty, tx,  frameBuffer[ty * SCREEN_WIDTH + tx]);
          }
      }


    // Draw the FPS value on the top-left corner
    tft.setTextColor(TFT_WHITE, TFT_BLACK); // White text on black background
    tft.setTextSize(1);  // Set text size
    tft.setCursor(0, 0); // Position to top-left
    tft.print("FPS: ");
    tft.print(fps);

    tft.endWrite();
   freeBuffer();
  }
}