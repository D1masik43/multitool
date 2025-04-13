
const int Button[7][3] = {{1, 1600, 1700}, // button 1
                     {2, 1400, 1500}, // button 2
                     {3, 200, 400}, // button 3
                     {4, 1700, 1850}, // button 4
                     {5, 1150, 1300}, // button 5
                     {6, 500, 700}, // button 6
                     {7, 900, 1100}}; // button 7
  

int button_samples = 4;
int but_buzz_freq = 400;
const unsigned long beepDuration = 20; 
const unsigned long env_upd_Duration = 1000; 

 enum AppState {
  STATE_MAIN_MENU, //**
  STATE_SUB_MENU, //**
  STATE_ENVIRONMENT,
  STATE_SETTINGS,
  STATE_BATTERY,
  STATE_FILES,
  STATE_WIFI,
  STATE_BLUETOOTH,
  STATE_ADD,   
  STATE_SERIAL,
  STATE_MUSIC,
  STATE_CALENDAR,
  STATE_CLOCK,
  STATE_BRAIN_ROOT,
  SUBSTATE_TETRIS,
  SUBSTATE_3DENGINE,
  STATE_NULL,// -->
  SUBSTATE_NRF_SCANNER,
  SUBSTATE_BLUETOOTH_AMPLIFIER,
  SUBSTATE_COMPASS,
  SUBSTATE_VIEW_FILE
};

 AppState appStateArray[4][3] = {
    {STATE_ENVIRONMENT, STATE_SETTINGS, STATE_BATTERY},
    {STATE_FILES, STATE_WIFI, STATE_BLUETOOTH},
    {STATE_ADD, STATE_SERIAL, STATE_MUSIC},
    {STATE_CALENDAR, STATE_CLOCK, STATE_BRAIN_ROOT}
};
 AppState blueToothaArray[2] = {
    SUBSTATE_NRF_SCANNER,
    SUBSTATE_BLUETOOTH_AMPLIFIER,
};
 AppState BrainrootArray[2] = {
    SUBSTATE_3DENGINE,
    SUBSTATE_TETRIS,
};
float dividerAffect = 1.319865;
int battery_samples = 8;

String amplifier_name = "bluetoothAmplifier";
const int MAX_FILES = 100; 
float angle = 0.15;
