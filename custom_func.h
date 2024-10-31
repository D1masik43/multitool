
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
#include <FS.h>;
#include <SD.h>; 
#include <DS3231.h>
#include <Adafruit_INA219.h>; 
#include "BluetoothA2DPSink.h"  
#include <SPI.h>
#include <TFT_eSPI.h>
 #include <math.h>
 
BluetoothA2DPSink a2dp_sink;


Adafruit_INA219 ina219;
RTClib RTClib;
DS3231 myRTC;
TFT_eSPI tft = TFT_eSPI(); 


#define SCREEN_WIDTH  128  // Width of the TFT display
#define SCREEN_HEIGHT 160  // Height of the TFT display

uint16_t screenData[SCREEN_WIDTH * SCREEN_HEIGHT];

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
  for(int i = 0; i <= 21; i++)
  {
    
    if(ButtonVal >= Button[i][1] && ButtonVal <= Button[i][2])
    {
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
   pinMode(BAT_PIN, INPUT);
   pinMode(BUZ_PIN, OUTPUT);
   pinMode(LED_PIN, OUTPUT);
}

int entered = 0;
volatile bool shouldExit = false;


AppState currentState =  STATE_MAIN_MENU;
AppState  previousState=  STATE_MAIN_MENU;




void drawImage(uint16_t image[32][32], int startX, int startY) {
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

void draw_environment(){ 

  disable_all_spi_devices();
  tft.setRotation(0);
  entered = 0;
  tft.fillScreen(TFT_BLUE);    
  pressure = bmp.readPressure() / 100.0F * 0.750062;  
  check_beep_stop();   
  temperature = aht20.getTemperature();
  check_beep_stop(); 
  humidity = aht20.getHumidity();
  check_beep_stop(); 
  altitude = bmp.readAltitude(1013.25);
  check_beep_stop(); 
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

void beep_start()
{
        tone(BUZ_PIN,but_buzz_freq);   
        isBeeping = true;           
        beepStartTime = millis(); 
}

float getBatteryPercentage(float voltage) {
    // Define the discharge curve table (you may need to adjust these values)
    const float voltageLevels[] = {4.20, 4.00, 3.85, 3.70, 3.50, 3.30, 3.20, 3.00};
    const int percentages[] = {100, 85, 70, 50, 25, 10, 5, 0};

    // Find where the voltage falls in the table
    int i;
    for (i = 0; i < 7; i++) {
        if (voltage >= voltageLevels[i]) {
            break;
        }
    }

    // Linear interpolation between the two closest points
    if (i == 0) {
        return 100.0; // Over 100% or full charge
    } else if (i == 7) {
        return 0.0; // Below 0% or empty
    } else {
        float v1 = voltageLevels[i];
        float v2 = voltageLevels[i - 1];
        int p1 = percentages[i];
        int p2 = percentages[i - 1];

        // Interpolate percentage
        return p1 + (voltage - v1) * (p2 - p1) / (v2 - v1);
    }
}

void draw_battery(){
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float busVoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float power_mW = ina219.getPower_mW();
  float loadVoltage = busVoltage + (shuntVoltage / 1000);
  float batteryPercentage = getBatteryPercentage(busVoltage);

  disable_all_spi_devices();
  tft.fillScreen(TFT_BLACK);
  // Set text color and size
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);


  tft.setCursor(1, 5);
  tft.print("Current: ");
  tft.print(current_mA);
  tft.println(" mA");

  tft.setCursor(1, 20);
  tft.print("Power: ");
  tft.print(power_mW);
  tft.println(" mW");

  tft.setCursor(1, 35);
  tft.print("Load Voltage: ");
  tft.print(loadVoltage);
  tft.println(" V");
  
  int sum = 0;       
  for (int i = 0; i < battery_samples; i++) {
    sum += analogRead(BAT_PIN);
    delay(1);
  }         
  float v  = sum / battery_samples;
  v = (v * 3.3) / 4095.0;
  v = v * dividerAffect;      
 
  tft.setCursor(1, 50);
  tft.print("Analog Voltage: ");
  tft.print(v);
  tft.println(" V");
 
  tft.setCursor(1, 65);
  tft.print("Percentage: ");
  tft.print(batteryPercentage);
  tft.println(" %");

  tft.setCursor(1, 80);
  tft.println("Battery will last for: ");
  tft.println((batteryPercentage*550/100)/current_mA);
  tft.println(" H");

  delay(10);
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
    float current_mA = ina219.getCurrent_mA();
    if(current_mA<0) color = TFT_BLUE;
    // Draw the battery outline
    tft.drawRect(x, y, width, height, TFT_WHITE);
    tft.drawRect(x+width, y + 1,1, height - 2 , TFT_WHITE);
    
    // Fill the battery based on percentage
    int fillWidth = (width - 2) * percentage / 100;
    tft.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
}

void draw_UI_bat()
{
  float shuntVoltage = ina219.getShuntVoltage_mV();
  float busVoltage = ina219.getBusVoltage_V();
  float loadVoltage = busVoltage + (shuntVoltage / 1000);
  float batteryPercentage = getBatteryPercentage(busVoltage);

  disable_all_spi_devices();
  draw_bat_ico(100, 2, 20, 12, batteryPercentage);
}

unsigned long time_previousMillis = 0; // Store the last update time
const long time_interval = 1000; 

void draw_time()
{
  disable_all_spi_devices();
  tft.fillRect(0,0,128,16,tft.color565(102, 0, 255));
  tft.setTextColor(TFT_WHITE); 
  DateTime now = RTClib.now(); 
  tft.setCursor(5, 3);
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
    draw_time(); // Call the function to draw the time
    draw_UI_bat();
  }
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
       tft.startWrite(); // Start SPI transaction
        tft.setAddrWindow(0, 0, 128, 160); // Set full-screen window (128x160)

        for (int y = 0; y < 160; y++)
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
      draw_time(); 
      }
}

int app_X = 0;
int app_Y = 0;

bool mode = true;

void draw_sub_menu()
{
  draw_miniUI();
  if(!entered){
    entered = 1;
    draw_time(); 
    disable_all_spi_devices();
    tft.setRotation(0);
    tft.setTextColor(TFT_WHITE); 
    tft.fillScreen(TFT_BLUE);
    if(app_X < 1) mode = true;
     if(app_X > 2) mode = false;
    if(mode)
    {
     for(int y = 0;y <3; y++)
      {
        for(int x = 0;x <3; x++)
        { 
          drawImage(imageArray[y][x], x * 32 + 8 * x + 8, y * 32 + 8 * y + 32);
        }
      }
      tft.drawRect(app_Y*32+8*app_Y+7, app_X*32+15+8*app_X+16,34 ,34, TFT_RED);   
      tft.drawRect(app_Y*32+8*app_Y+6, app_X*32+15+8*app_X+15,36 ,36, TFT_RED);     
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
      tft.drawRect(app_Y*32+8*app_Y+7, (app_X-1)*32+15+8*(app_X-1)+16,34 ,34, TFT_RED);  
      tft.drawRect(app_Y*32+8*app_Y+6, (app_X-1)*32+15+8*(app_X-1)+15,36 ,36, TFT_RED);   
    }
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
        default: return "Unknown";
    }
}

void draw_bluetooth_submenu() {
    if (shouldExit) return; // Prevent drawing if exit flag is set
    if (entered == 0) {     // Only execute once when entering the submenu
        entered = 1;
        
        disable_all_spi_devices();
        tft.setRotation(0);
        tft.setTextColor(TFT_WHITE);
        tft.fillScreen(TFT_BLUE);
        draw_time();

        // Start drawing Bluetooth submenu options
        for (int i = 0; i < sizeof(blueToothaArray) / sizeof(blueToothaArray[0]); i++) {
            tft.setCursor(0, i * 10 + 16);
            if (i == app_X) {
                tft.print("> ");  // Highlight selected item
            } else {
                tft.print("");  // Remove highlight for unselected items
            }
            tft.println(appStateToString(blueToothaArray[i])); 
        }
        disable_all_spi_devices();
    }
}
int currentFileIndex = 0;   // Track the current file index
int filesOnScreen = 5;      // Number of files to display on the screen at once
String fileList[MAX_FILES]; // Array to store file names
int fileCount = 0;    
int selectedFileIndex = 0;  // Track the selected file index for highlighting
String currentFileName;  // To hold the currently selected file's name
int currentLineIndex = 0;  // To track where you are in the file


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

void draw_files() {
  if (entered == 0) {
    entered = 1;
    disable_all_spi_devices();
    File root = SD.open("/");
    if (!root) {
      Serial.println("Failed to open directory");
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




// Cube properties
float angleX = 0.0, angleY = 0.0;
float cubeSize = 64.0;
float centerX = 80, centerY = 60;  // Center of display
bool antiAliasedMode = true; 

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
// Assuming you already have a 64x64 texture (uint16_t or int16_t)
void drawTexturedFace(float projected[4][2], const uint16_t* texture) {
    // Define the four corners of the face on the screen
    float x0 = projected[0][0], y0 = projected[0][1];
    float x1 = projected[1][0], y1 = projected[1][1];
    float x2 = projected[2][0], y2 = projected[2][1];
    float x3 = projected[3][0], y3 = projected[3][1];

    // Start SPI transaction
    tft.startWrite();

    // Loop through each pixel in the 64x64 texture
    for (int ty = 0; ty < 64; ty++) {
        for (int tx = 0; tx < 64; tx++) {
            // Calculate the interpolation factor for the texture
            float u = tx / 63.0f;  // Normalize tx to [0, 1]
            float v = ty / 63.0f;  // Normalize ty to [0, 1]

            // Interpolate the position on the screen using texture coordinates (u, v)
            float x = (1 - u) * ((1 - v) * x0 + v * x3) + u * ((1 - v) * x1 + v * x2);
            float y = (1 - u) * ((1 - v) * y0 + v * y3) + u * ((1 - v) * y1 + v * y2);

            // Access the color from the texture in PROGMEM using pointer arithmetic
            uint16_t color = pgm_read_word(&texture[ty * 64 + tx]);

            // Draw the pixel at the interpolated screen position
            tft.drawPixel((int)x, (int)y, color);
        }
    }

    // End SPI transaction
    tft.endWrite();
}

// Helper function to draw a pixel with intensity (0.0 to 1.0)
void drawPixelWithIntensity(int x, int y, float intensity, uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * intensity;
  uint8_t g = ((color >> 5) & 0x3F) * intensity;
  uint8_t b = (color & 0x1F) * intensity;
  uint16_t fadedColor = (r << 11) | (g << 5) | b;
  tft.drawPixel(x, y, fadedColor);
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

        // Draw the textured face
        drawTexturedFace(face, textureData[0]); // Pass the texture data here

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
            tft.drawLine((int)projected[v0][0], (int)projected[v0][1],
                         (int)projected[v1][0], (int)projected[v1][1], TFT_WHITE);
            tft.drawLine((int)projected[v1][0], (int)projected[v1][1],
                         (int)projected[v2][0], (int)projected[v2][1], TFT_WHITE);
            tft.drawLine((int)projected[v2][0], (int)projected[v2][1],
                         (int)projected[v3][0], (int)projected[v3][1], TFT_WHITE);
            tft.drawLine((int)projected[v3][0], (int)projected[v3][1],
                         (int)projected[v0][0], (int)projected[v0][1], TFT_WHITE);
        }
    }
}



// Main engine function
void _3DEngine() {
  if(entered == 0)
  {
  entered = 1;
    disable_all_spi_devices();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    drawCube();
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
  
