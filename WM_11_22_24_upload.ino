/*
____________________

Well Monitor
11/22/2024
for Blynk

Notes
 - search for uuu to update scale factors and measures, ttt for testing code, 
   and *** or ??? for open issues 
 - Turn off serial monitor check before using in deployment
 
  Arduino MKR WiFi 1010
  This sketch asks NTP for the time and sets the internal Arduino MKR1010's RTC accordingly.
  This sketch then prints out the date and time and up to six sensor readings on a 132 by 32 oled
  This sketch also saves readings and generated reports to the sd card
  This sketch interfaces with a Blynk mobile app using wifi which can read sensors and generated reports
  
  Display example:
  
  20/03/25  13:01:50
  0000   1111   2222
  3333   4444   5555

  Hardware is quite simple:
  
  Split core current sensors SCT-013-000 input current 0-100 amps, output current 0-50ma

  sensor output   ------------------- 47k ---------------------------------- A input 
                    |
                    R    240 ohms used on A0-A2 and A4 for three well and one 
                    |         pressure tank pumps, 
                    |    3.9k ohms used on A3 for city water valve, and
                    |    1k ohms used on A5 for pressure tank psi gauge
                    |
                    |
                    G

  -  No diodes as signal voltage too low for forward voltage drops.
     Rely on internal diode protection with current reducing 47k ohm resistor.
     These should handle at least 1ma and would allow +/- 47 volts on input, 
     which is more than enough as signal would be +/- 12 volts at 100 amps 
     through split core current sensor.
  - 240 ohm resistor for 120VAC 15 amp well pumps on inputs A0, A1, A2 and A4.
  - 3.9k ohm resistor on input A3 for the valve used for adding city water into the holding tank.  
      Voltage is 240 and current is very low since it is just a sprinkler valve.  
      Current was measured at [[0.05]] amps with 240 volts.  To increase signal, split core has 5 wraps.
  - 1k ohm resistor on input A5 for use with the pressure transducer that outputs a voltage of 0 to 3.3.
    
  1 horsepower is equivalent to using electricity at 745 Watts.  
  At 120 volts that will equal 6.2 amps. Therefore 1.5 horsepower 
  pump can easily draw 9.3 amps, average, and could draw more instantaneously
  240 would be 1/2 of this.
  For pressure tank, reading seems high at over 11 amps at 240 volts.  
  
  Pump currents measured 5-9-20 
    Note - currents on start up are up to 2x higher.  
    A0 is a shallow well - reads 5.3 to 5.4 or an unscaled 240 to 275.   Divide by 5 to get current in 0.1 amp steps.
    A1 is a shallow well - reads 4.5 or an unscaled 225 t0 238.  Divide by 5 to get current in 0.1 amp steps.
    A2 is a deep well reads an unscaled 440.  Divide by 5 to get current in 0.1 amp steps. 
    A3 is the city water valve.  Current was [[0.050]] at 240 volts.  Have 5 wraps on split core to boost gain.
        Being amplified by 5 sensor wraps and a 3.9k resistor anything over 40 would be considered on.
    A4 is the pressure tank pump.  Current is 11.2 or an unscaled 620.  Divide by 5.5 to get current in 0.1 amp steps.
    
 Pressure sensor at A5 measures psi at the pressure tank using a Pressure Transducer Sender Sensor for Oil Fuel Air Water
    1/8"NPT Thread Stainless Steel (150PSI).  
    Sensor is rated at 5V
    Test sensor with air compressor to see if functional.  Non-operrative sensors just read vcc on the output
      On the sensor pigtail
        red is +5v dc
        black is ground
        green is output
      on MKR board jack
        red is +5v  tip
        green is ground  outside ring
        white is output  middle ring
      Sensor output runs to 510 resistor to input which is connected to 1k ohm resistor to ground and 47k resistor to 
      mkr analog input
        62 lbs  836
         0 lbs  151
           or around 11.0 per lb + 151
      This sketch scales the input at -100 then multiplies by 7.2 to get pressure in psi.    
      Low water pressure alarm is set at less than 38 psi.    
      Purchasing Autex 100 psi switches (unbranded) - First one purchased May 2020 from Amazon and lasted until November 2024.  
      Another purchased from Amazon in 2022 was returned.  Of two purchased in November 2024, one did not work and had to be 
      returned.
      with 0-150 psi sensor
      450 = 65
      350 = 55
      250 = 45
      est 150 = 35
      use read - 150 * 0.1 + 35

        
  Pressure tank pump runs for 49 miuntes for 75 minutes of watering (as of 3-24-22)

  SD Card - Samsung 32 GB - format samsung SD card as fat32 with default allocation
    SD socket (R to L from end)
    1  NC         DAT2/X  Connector Data line 2 No use
    2  4 CS       DAT3/CS Connector Data line 3 Chip Select
    3  8 MOSI     CMD/DI  Command / Response Line Data Input
    4  VCC 3.3    VDD/VDD Power supply (+3.3V)  Power supply (+3.3V)
    5  9 SCK      CLK/SCLK  Clock Serial Clock
    6  G          VSS/VSS Ground  Ground
    7  10 MISO    DAT0/D0 Connector Data line 0 Data Out
    8  NC         DAT1/X  Connector Data line 1 No use

  Select pushbutton switch betewwn D0 and gnd.  
  
  Scroll up pushbutton switch between D1 and gnd.
  
  Closing pushbutton switch between D2 and vcc will cause oled to remain illuminated for 2 minutes to help save the 
    oled from rapid decay.  Can also use a motion sensor on D2 which is high on motion.  Use pulldown on D2.
  
  Another reset push button added between reset and gnd.
*/


// comment out line for different deployments uuu
#define test 
//#define production

#ifdef test
// test 
#define BLYNK_TEMPLATE_ID "XXXXXXXXX"
#define BLYNK_DEVICE_NAME "XXXXXXXXX"
#define BLYNK_AUTH_TOKEN "XXXXXXXXX"
char auth[] = BLYNK_AUTH_TOKEN;
#define BLYNK_FIRMWARE_VERSION "1.0.1"
const char ssid[]= "XXXXXXXX"; // network SSID (name) - Test WM
const char pass[] = "XXXXXXXX"; // linksys1 password (use for WPA, or use as key for WEP)
int notification_delay_ms_int = 25000;  // daily notifcation delay in ms
#endif

#ifdef production
// production 
// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "XXXXXXXXXXX"
#define BLYNK_TEMPLATE_NAME "XXXXXXXXXXX"
#define BLYNK_AUTH_TOKEN "XXXXXXXXXXX"
#define BLYNK_FIRMWARE_VERSION "1.0.1"
char auth[] = BLYNK_AUTH_TOKEN;
const char ssid[] = "XXXXXXXX"; // network SSID (name) - Production WM in Garage
const char pass[] = "XXXXXXXX"; // linksys6 password (use for WPA, or use as key for WEP)
int notification_delay_ms_int = 15000;  // daily notifcation delay in ms
#endif

const char about_0_str[] = "Well Monnitor 11-22-24";
const char about_1_str[] = "Version 2.0.2";

// for wifi, Blynk and rtc
#include <SPI.h>
#include <WiFiNINA.h> // for mkr 1010
#include <BlynkSimpleWiFiNINA.h>  // for Blynk
//#include <WiFi101.h>  // for mkr 1000 and uno shield
//#include <WiFiUdp.h>  // not needed for mkr 1010  

#include <RTCZero.h>
RTCZero rtc;
//#include "arduino_secrets.h" // for putting the ssid and pwd on a separate tab
//  can enter sensitive data in the Secret tab/arduino_secrets.h
// cont char ssid = ; // network SSID (name)
// const char pass = ; // network password (use for WPA, or use as key for WEP)
//int key_index = 0; // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
const int k_GMT = -7; //-8 for California ST and -7 for California DST
unsigned long epoch;
long rssi;
char rssi_str[16];

// for watch dog timer
// Note - for below, must delete lines 58 to 66 of WDTZero.cpp 
// to avoid clock configuation conflict with RTCZero that 
// uses external crystal for accuracy, not internal oscillator
#include <WDTZero.h>  
WDTZero WDTimer; // Define watch dog timer

// for oled 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// even though the display has no reset pin, it must be initialized 
//   as follows for the library to work - 4 is the reset pin
Adafruit_SSD1306 display(4);
char oled_line1_str[32];  // oled can display 18 characters per line inside of rectangle box
char oled_line2_str[32];
char oled_line3_str[32];
char oled_text_str[32];
int oled_on_int = 1;  // for turnign on oled display using motion sensor to preserve oled
int oled_motion_cnt_int  = 0;  // for maintaining oled on for period of time after motion

// for date and time functions
char seconds_str[8];
int seconds_int;
char minutes_str[8];
int minutes_int;
char hours_str[8];
int hours_int;
char days_str[8];
int days_int;
char months_str[8];
int months_int;
char years_str[8];
int years_int;
char date_str[16]; // date string
char time_str[16]; // time string

// for values read, the OLED display and the SD card logging
int sum_log_time_array_int[6] = {0, 0, 0, 0, 0, 0};
char sum_log_time_array_str[6][8];
char values_read_0to2_str[32]; // values read string 0 to 2 first line
char values_read_3to5_str[32]; // values read string 3 to 5 second line
char values_display_0to2_str[32]; // values string 0 to 2 first line for oled display
char values_display_3to5_str[32]; // values string 3 to 5 second line for oled display
char values_single_line_0to5_str[64]; // values for SD file, serial and terminal outputs

// for error logging and resets
char error_type_str[32];
char error_type_display_str[32];
// set the reset flag on error and clear the reset flag when restarting
int mkr_reset_flag_int = 0;

// for low pressure alarm
// use multiple low pressure reads to trigger alarm
int water_pressure_low_read_count_int = 0;
int water_pressure_alarm_flag_int = 0;
int water_pressure_alarm_count_int = 0;
int water_pressure_report_flag = 0; // on if > 0 as also used as counter
int value_pressure_int = 0; // for reporting unscaled raw pressure read

// for city water use notifcation
int city_water_notice_flag_int = 0;
int city_water_notice_count_int = 0;

// for sd card
#include <SD.h>
const int k_SD_chip_select = 4;
File myFile;
File base;  // for testing
char file_name_prefix_new_str[16];
char file_name_prefix_old_str[16];
char file_name_log_old_str[16];
char file_name_log_new_str[16];
char prior_days_str[8] = "0";
char prior_months_str[8] = "0";
char prior_years_str[8] = "0";
//char file_rpt_array_str[8][32]; // for holding report for each channel - not used anymore
char menu_file_name_array_str[8][16]; // for holding Menu files  
char menu_file_name_desired_array_str[8][16]; // for holding desired Menu files
char SD_print_str[128]; // string for each line printed to SD file
int SD_write_skip_cnt = 0;  // used to log data every 5th read (5 seconds) to reduce data collected
// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// For Serial commands
char cmd_str[128];
int cmd_length_int = 0;
int cmd_flag_int = 0;

// for Blynk
// #define BLYNK_MAX_SENDBYTES 256 //to increase from 128 bytes default - update reserve below as required
#define BLYNK_PRINT Serial // optional - can disable prints
// Connection error handling
int blynk_connect_count = 0;
// BlynkTimer timer; // Do not use BlynkTimer (based on SimpleTimer) as it may have been causing resets
// led V1 is the blue LED on the phone
WidgetLED led1(V1);
int led1_state_int = 0;
// led V3 is the yellow LED on the phone
WidgetLED led3(V3);
int led3_state_int = 1;  // set to one to ensure turn-off on reset
// led V4 is controlled by switch at digital pin 0
WidgetLED led4(V4);
int value_sw_4_int;
int led4_state_int = 0;
// controlled v9 is latching for pin 27 blue LED on the mkr
WidgetLED sw9(V9);
int value_sw9_int;
// for pressure value display on V10
int blynk_pressure_read_int = 38;
// Attach virtual serial terminal to Virtual Pin V16
WidgetTerminal terminal(V16);
// for reading a second input line in the Blynk terminal
int terminal_second_line_flag_int = 0;
// for reading a second input line in the Serial terminal
int serial_second_line_flag_int = 0;
// for com error reporting
int comTst_cnt_int = 0;
// for notifications
char notification_str[256];
int daily_notification_flag_int = 0; // Set this at 0 to start.  However, a reset may affect sending the notice
char daily_notification_str[256]; 
int onBoardLEDValue = 0;

// for manual timers
unsigned long previousMillis_Blynk = 0;
unsigned long previousMillis_blinkOnBoardLED = 0; 
unsigned long previousMillis_swWidget = 0; 
unsigned long previousMillis_mainLoop = 0; 
unsigned long previousMillis_mkrReset = 0; 
// unsigned long previousMillis_Blynk_term_fix = 0; 
unsigned long previousMillis_comTst = 0;
unsigned long previousMillis_daily_notification = 0; 
unsigned long previousMillis_pressureReport = 0;
unsigned long previousMillis_pressureReportFast = 0;
unsigned long currentMillis = 0;

// for push buttons, led and menu
#include <Bounce2.h>
#include <utility/wifi_drv.h>
int value_d0_int;  // variables for reading the pin status (not used right now)
int value_d1_int;
int value_d2_int;  // for motion sensor read
// Instantiate Bounce objects
Bounce debouncer_d0 = Bounce();  // lower button as "Select" active low 
Bounce debouncer_d1 = Bounce();  // upper button as "Scroll" active low 

// for file reads
// read up to 16 128 char lines from a file 
char file_read_str[2200];  // for holding last 2200 characters of file
char file_read_line_read_str[128];
int file_read_number_of_lines_int;
char file_read_line_array_str[16][128];
char file_read_file_name_str[16];
unsigned long file_read_page_pointer_ulong = 0;

// for anlaog inputs
int value_interim_read_array_int[6] = {0, 0, 0 ,0 ,0 ,0};  // interim reads
int value_current_read_array_int[6] = {0, 0, 0, 0, 0, 0};  // current read
char value_current_read_array_str[6][8] = {"0", "0", "0", "0", "0", "0"}; // current reads
// used to select lowest wp before in 10 seconds before pump activation as pressure tank 
//   pump causes pressure to somtimes rise right before read with 5 second read intervals
int water_pressure_low_array_int[5] = {50, 50, 50, 50, 50}; 
int water_pressure_low_int;  

// for on-off reports
char value_prior_state_array_str[6][8] = {"off", "off", "off", "off", "off", "off"}; // prior state
char value_current_state_array_str[6][8] = {"off", "off", "off", "off", "off", "off"}; // current state
char value_current_state_array_10_str[6][8] = {"0", "O", "0", "0", "0", "0"}; // current state
// array for file YYYY.RPX or 2022.RP2, etc. and 2022.RPT for all channels
char annual_on_off_file_name_str[8][16];
char SD_rpt_file_str[16];
char SD_rpt_str[128];
char SD_rpt2_str[128];
// for file "DYY.RPT" that includes time on for channel for each day of the year
char day_report_file_name_str[16];
char day_report_line_str[64];
// Right now, only using file "M.RPT" that includes time on for channel for each month of all of the years
// could break this down by years and also add a single report for the year totals with the following
// char month_report_file_name_str[16];
// char month_report_line_str[64];
// for file "Y.RPT" that includes time on for channel for each year
// char year_report_file_name_str[16]; 
// char year_report_line_str[64];
char error_report_file_name_str[16];
char error_report_line_1_str[64];
char error_report_line_2_str[64];

void setup() {
  //  for real time clock 
  //  Note - This selects the external crystal and confiures the clock.
  //  This must be in front of the wdt setup as the modified version
  //  of the wdt code has the clock configuration deleted as it 
  //used inaccurate low power osclilator.
  rtc.begin();
  // for watch dog timer
  // Note - Modified WDTZero header file.  16 seconds avail too.
  WDTimer.setup(WDT_HARDCYCLE8S);  // initialize WDT counter refesh cycle on 8 sec. 
  // when time runs out, processor does a hardware reset
  
  // for serial monitor
  // this works well.  lowered from 74880.  could also try 14400, 19200, 28800, 38400, 57600, and 115200
  Serial.begin(9600); // for serial monitor 
  
  // while (!Serial) {WDTimer.clear();}  // refresh watch dog timer  uuu
  // wait for serial port to connect. Needed for native USB port only
  
  // for oled
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay(); // This before display.display gets rid of the oled Adafruit logo
  // display.display();
  oledDisplay_display();  // includes display.display() 
  display.setRotation(2);  // rotate display 180 choices 0, 1 (90), 2 (180) and 3 (270) 
  delay(100);
   
  // for push buttoms
  pinMode(0, INPUT_PULLUP);  // declare pushbutton as input (active low - when button pressed, you get 0)
  debouncer_d0.attach(0); // set up debouncer 0
  debouncer_d0.interval(25);  // in ms
  pinMode(1, INPUT_PULLUP);  // declare pushbutton as input (active low - when button pressed, you get 0)
  debouncer_d1.attach(1); // set up debouncer 1
  debouncer_d1.interval(25);  // in ms
  // declare motion sensor or switch as input 
  // debouncer not needed for this type of input
  pinMode(2, INPUT_PULLDOWN);  // for push button swtich, use pull down with switch to vcc
  // also works with motin sensor that goes high when motion detected
  // does not work if motion sensor not connected.  In that case use dis command
  
  // create visual indicator for push buttons
  WiFiDrv::pinMode(25, OUTPUT); //GREEN - flashing for logging  
  WiFiDrv::pinMode(26, OUTPUT); //RED - constant for menus
  WiFiDrv::pinMode(27, OUTPUT); //BLUE - constant for blynk control
  
 // attempt to connect to WiFi networks :
int number_of_tries_ssid = 0;
char number_of_tries_ssid_str[4]; 
while ( status != WL_CONNECTED) {
  // update display for connection attempts
  WDTimer.clear();  // refresh watch dog timer
  number_of_tries_ssid++;
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print("  ");
  Serial.println(number_of_tries_ssid);
  sprintf(number_of_tries_ssid_str, "%i", number_of_tries_ssid);
  strcpy(oled_line1_str, "Connecting ...     ");
  strcpy(oled_line2_str, "   ");
  strcat(oled_line2_str, ssid);
  strcpy(oled_line3_str, "                 ");
  strcat(oled_line3_str, number_of_tries_ssid_str);
  display.clearDisplay(); // This before display.display gets rid of the current text
  oledText(oled_line1_str, 8, 3, 1, false);
  oledText(oled_line2_str, 8, 11, 1, false);
  oledText(oled_line3_str, 8, 21, 1, false);
  // draw rectangle - includes display.display (so can use false above)
  oledRect();   
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
  status = WiFi.begin(ssid, pass);
  // wait 10 seconds for connection:
  WDTimer.clear();  // refresh watch dog timer
  delay(5000);
  WDTimer.clear();  // refresh watch dog timer
  delay(5000);
  }

  // you're connected now, so print out the status on the serial port
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print and display the received signal strength
  rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  strcpy(oled_line1_str, "Connected: ");  //  19 characters per line
  strcpy(oled_line2_str, "   ");
  strcat(oled_line2_str, ssid);
  sprintf(rssi_str, "%4d", rssi);
  strcpy(oled_line3_str, "          ");  //  19 characters per line
  strcat(oled_line3_str, rssi_str);
  strcat(oled_line3_str, " dBm");
  //Serial.println(oled_line1_str);
  //Serial.println(oled_line2_str);
  //Serial.println(oled_line3_str);
  display.clearDisplay(); // This before display.display gets rid of the current text
  oledText(oled_line1_str, 8, 3, 1, false);  // update oled
  oledText(oled_line2_str, 8, 11, 1, false); //
  oledText(oled_line3_str, 8, 21, 1, false); //
  // draw rectangle - includes display.display (so can use false above)
  oledRect();
  WDTimer.clear();  // refresh watch dog timer
  delay(5000);
    
  // for starting rtc and obtaining epoch
  int number_of_tries_NTP = 0;
  do {
    WDTimer.clear();  // refresh watch dog timer
    epoch = WiFi.getTime();
    number_of_tries_NTP++;
    Serial.print("NTP tries:  ");
    Serial.println(number_of_tries_NTP);
    delay(1000);
  }
  while ((epoch == 0) && (number_of_tries_NTP < 10));
  if (number_of_tries_NTP >= 10) {
    strcpy(error_type_str, "NTP-0");
    mkrError();
  }
  else {
    WDTimer.clear();  // refresh watch dog timer
    Serial.print("Epoch received: ");
    // print out epoch on serial port
    Serial.println(epoch);
    epoch = epoch + (k_GMT * 60 * 60);  // adjsut for GMT standard time
    rtc.setEpoch(epoch);
    Serial.println();
    // cdt hard coded    
    // overrides are below for testing and troubleshooting  ttt
    // format rtc.setDate(byte day, byte month, byte year)
    // format rtc.setTime(byte hours, byte minutes, byte seconds)
    // rtc.setDate(31, 12, 22);  //*****
    // rtc.setTime(23, 59, 45);
  }
    
  // for SD
  // print card info
  WDTimer.clear();  // refresh watch dog timer
  SDcardinfo();
  SD.begin(k_SD_chip_select); // first SD.begin.  Note - docs says subsequent calls of SD.begin will return false.
  // print SD files
  //Serial.println("Directories and files on SD card:");
  //base = SD.open("/", FILE_READ);
  //printDirectory(base, 0);
  //Serial.println();
  //base.close();
    
  WDTimer.clear();  // refresh watch dog timer
  Blynk.begin(auth, ssid, pass);
  // Blynk.begin is blocking
  // Blynk.config(auth);  // work better? not in my experience
  updateDate(); 
  updateTime();
  Serial.println();
  Serial.print(date_str);
  Serial.print("  ");
  Serial.println(time_str);
  Serial.println("Well Monitor is online!");
  Serial.print(ssid);
  Serial.print("  ");
  rssi = WiFi.RSSI();
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.println(); // add line feed
  Serial.println("Type cmd for list of commands.");
  Serial.println(); // add line feed
  // Blynk termnial
  // Clear the terminal content  
  terminal.clear();
  terminal.print(date_str);
  terminal.print("  ");
  terminal.println(time_str);
  terminal.println("Well Monitor is online.");
  terminal.print(ssid);
  terminal.print("  ");
  rssi = WiFi.RSSI();
  terminal.print(rssi);
  terminal.println(" dBm");
  terminal.println(); // add line feed
  terminal.println("Type cmd for list of commands.");
  terminal.println(); // add line feed
  terminal.flush();
  // note - not using the Blynk timer function
  
  // record the last log and the reset time
  // read the last line of the log file
  WDTimer.clear();  // refresh watch dog timer
  strcpy(file_read_file_name_str, date_str);
  strcat(file_read_file_name_str, ".LOG"); 
  fileRead();  // reads file_read_file_name_str
  if (file_read_number_of_lines_int == 0){
    strcpy(error_report_line_1_str, "no log file on SD card");
    }
  else {
    char date_discard_str[16] = "";
    char time_discard_str[16] = "";
    char line_read_array_str[16][8];
    sscanf(file_read_line_array_str[0], "%s %s %s %s %s %s %s %s", date_discard_str, time_discard_str, line_read_array_str[0], 
    line_read_array_str[1], line_read_array_str[2], line_read_array_str[3],
    line_read_array_str[4], line_read_array_str[5]);
    strcpy(error_report_line_1_str, date_discard_str);
    strcat(error_report_line_1_str, "  ");
    strcat(error_report_line_1_str, time_discard_str);
    strcat(error_report_line_1_str, "  last log");
    }
  WDTimer.clear();  // refresh watch dog timer
  // output log to SD card
  strcpy(error_report_line_2_str, date_str);
  strcat(error_report_line_2_str, "  ");
  strcat(error_report_line_2_str, time_str);
  strcat(error_report_line_2_str, "  restarted"); 
  strcpy(error_report_file_name_str, "E");
  strcat(error_report_file_name_str, years_str);
  strcat(error_report_file_name_str, ".RPT");
  myFile = SD.open(error_report_file_name_str, FILE_WRITE);
    if (myFile) {
      myFile.println(error_report_line_1_str);
      myFile.println(error_report_line_2_str);
      myFile.close(); // always close files after writing
    }
    // if the file does not open, report error
    else {
      strcpy(error_type_str, "SD-0");
      mkrError();
      }
  WDTimer.clear();  // refresh watch dog timer
  Blynk.logEvent("wm_restarted"); // log restart in timeline nnnnn
  
  // Set initial blynk switch and Blue LED indicator
  Blynk.virtualWrite(V9, LOW);
  Blynk.setProperty(V9, "color", "#000000"); // dark
  led1.off();  
  WiFiDrv::digitalWrite(27, LOW); // blue LED off 
  
  //report pressure at onset, then every 10 minutes
  mainLoop();
  Blynk.virtualWrite(V10, blynk_pressure_read_int);
  // terminal.println(); // add line feed
  terminal.print("Pressure is ");
  terminal.println(blynk_pressure_read_int);
  terminal.flush();
  Blynk.run();
}

void loop() 
{
  currentMillis = millis();
  //Serial.println("In Blynk");
  if(currentMillis - previousMillis_Blynk >= 500) {
    previousMillis_Blynk = currentMillis;  // Remember the time
    /*if (!Blynk.connected()){ // for testing  ttt ***
      Serial.println("Blynk was disconnected.... ");  
      Serial.println("Before blynk run .. Blynk was disconnected.... ");
      Blynk.run();
      Serial.println("After blynk run .. Blynk was disconnected.... ");
      }
      */
    Blynk.run();  // repeated above to check if error is here
    checkSerialcommand(); // after checking for Blynk command, check for serial command
    } 
  //Serial.println("Out Blynk, In blinkOnBoardLED");
  if(currentMillis - previousMillis_blinkOnBoardLED >= 1000) {
    previousMillis_blinkOnBoardLED = currentMillis;  // Remember the time
    blinkOnBoardLED();
    }
  //Serial.println("Out blinkOnBoardLED, In swWidget");
  if(currentMillis - previousMillis_swWidget >= 1000) {
    previousMillis_swWidget = currentMillis;  // Remember the time
    swWidget();
    }
  
  //Serial.println("Out swWidget, In mainLoop");
  if(currentMillis - previousMillis_mainLoop >= 1000) {
    previousMillis_mainLoop = currentMillis;  // Remember the time
    mainLoop();
    }
 
  // report pressure every second (fast)
  if((water_pressure_report_flag > 0) && (currentMillis - previousMillis_pressureReportFast >= 5000)) {
    previousMillis_pressureReportFast = currentMillis;  // Remember the time
    //report pressure
    Blynk.virtualWrite(V10, blynk_pressure_read_int);
    // terminal.println(); // add line feed
    terminal.print("Calc pressure is ");
    terminal.print(blynk_pressure_read_int);
    terminal.print(" - raw data was ");
    terminal.println(value_pressure_int);
    terminal.flush();
    Blynk.run();    
    // report pressure every second for 2 minutes for calibration
    water_pressure_report_flag++;
    if (water_pressure_report_flag > 120000){
      water_pressure_report_flag = 0;
      terminal.println("Fast pressure reporting ended");
      terminal.flush();
      Blynk.run();
      }
    }
  // if not fast reporting, report pressure every 10 minutes
  if((water_pressure_report_flag == 0) && (currentMillis - previousMillis_pressureReport >= 600000)) {
    previousMillis_pressureReport = currentMillis;  // Remember the time
    //report pressure
    Blynk.virtualWrite(V10, blynk_pressure_read_int);
    // terminal.println(); // add line feed
    terminal.print("Pressure is ");
    terminal.println(blynk_pressure_read_int);
    terminal.flush();
    Blynk.run();
    }

  // daily minutes-on and pressure notification
  // wait 25 seconds to send notification to avoid blynk messaging congestion
  currentMillis = millis();  //update here as previous millis was greater than current??? ***
  if((daily_notification_flag_int == 1) && (currentMillis - previousMillis_daily_notification >= notification_delay_ms_int)) {  
    // send notifcation
    //Serial.print("on sending currentMillis is: "); 
    //Serial.println(currentMillis);
    //Serial.print("on sending previousMillis_daily_notification is: "); 
    //Serial.println(previousMillis_daily_notification);
    //Serial.println("is following a difference of 25000?");
    //Serial.println(currentMillis - previousMillis_daily_notification);
    
    if (Blynk.connected()){
      Blynk.logEvent("wm_daily_report", String(daily_notification_str));
      Serial.println("Daily notification sent.");
      Serial.println(daily_notification_str);
      Serial.println();
      // reset flag so notification sent just once
      daily_notification_flag_int = 0;
    }
    else {
      Serial.println("No Blynk.  Daily notification not sent.");
      Serial.println();
    }
  }
    
  //Serial.println("Out mainloop, In mkrReset");
  // error routine sets previousMillis_mkrReset
  if((mkr_reset_flag_int == 1) && (currentMillis - previousMillis_mkrReset >= 5000)) {  
    mkrReset();
  }
   
  //Serial.println("Out mkrReset, In WDTimer");
  WDTimer.clear();  // refresh watch dog timer
  //Serial.println("Out WDTimer");
}

void mainLoop() // this program runs through the Blink timer every second
{
  readValues(); // read analog values
  //Serial.println("Out readValues");
  fileUpdate(); // update file name - each day has own file
  //Serial.println("Out fileUpdate");
  updateDate(); 
  //Serial.println("Out updateDate");
  updateTime();
  //Serial.println("Out updateTime");
  updateDisplayValues(); // adds spaces to displayed values
  //Serial.println("Out updateDisplayValues");
  outputData(); // output to oled display, serial port and SD file
  //Serial.println("Out outputData");
  oledMotion();  // check if motion or pushbutton switch to turn on oled display
  // Serial.println("Out oledMotion");
  localMenu(); // check if menu has been entered through device push buttons
  //Serial.println("Out Menu");
}

void readValues()
{
   // Read analog pins
   for (int r=0; r<=5; r++){
     value_current_read_array_int[r] = 0;  // reset current read values
     }
   // with i = 100, it takes 0.3 seconds to cycle one time 
   // the input is an AC signal, so multiple reads required to determine the highest level reliably
        for (int i=1; i<=100; i++){ 
          for (int s=0; s<=5; s++){
            value_interim_read_array_int[s] = analogRead(s);  //get read
            if (value_interim_read_array_int[s] > value_current_read_array_int[s]){ // use highest value
              value_current_read_array_int[s] = value_interim_read_array_int[s]; // 
            }
          }  
        }
     //range values
     for (int r=0; r<=5; r++){
     if (value_current_read_array_int[r] < 0) {value_current_read_array_int[r] = 0;}
     if (value_current_read_array_int[r] >9999) {value_current_read_array_int[r] = 9999;}
     }
     // scale values - see data above for scale values
     // a0 to a2 are the three well pumps
     // scale as amps/10     
     value_current_read_array_int[0] = value_current_read_array_int[0] / 5; 
     // filter for offset and noise
     if (value_current_read_array_int[0] <= 10) {value_current_read_array_int[0] = 0;}
     value_current_read_array_int[1] = value_current_read_array_int[1] / 5; 
     if (value_current_read_array_int[1] <= 10) {value_current_read_array_int[1] = 0;}
     value_current_read_array_int[2] = value_current_read_array_int[2] / 5; 
     if (value_current_read_array_int[2] <= 10) {value_current_read_array_int[2] = 0;}
     // a3 is a 240 volt house water fill valve solenoind with very low current.  
     // It is amplified by 5 sensor windings and a 3.9k resistor as the load
     // over 40 would be considered on
     if (value_current_read_array_int[3] <= 10) {value_current_read_array_int[3] = 0;} 
     // a4 is the pressure tank pump
     // scale as amps/10 
     value_current_read_array_int[4] = value_current_read_array_int[4] / 5.5; 
     if (value_current_read_array_int[4] <= 10) {value_current_read_array_int[4] = 0;}
     value_pressure_int = value_current_read_array_int[5];  // save raw pressure reading 
     // a5 is the pressure tank pressure measured with pressure sensor
     // scale water pressure sensor a5 by subtracting -100 and then dividing by 7.2
     //value_current_read_array_int[5] = min(((value_current_read_array_int[5] - 100) / 7.2), 99); // cap at 99 if error   //uuu 0-100
     value_current_read_array_int[5] = constrain((((value_current_read_array_int[5] - 150) / 10) + 35), 0, 99); // cap at 99 if error   //uuu 0-150
     // range scaled value
     if (value_current_read_array_int[5] <0) {value_current_read_array_int[5] = 0;}
     // only report low value if this occurs at least 5 consequtive reads
     // while waiting, use 38 as the entry to avoid having a 0 on the day report for no reason
     // given this, for testing purposes, need to wait five seconds for low pressure to display
     if (value_current_read_array_int[5] <=38) {
       if (water_pressure_low_read_count_int <= 5) {
         value_current_read_array_int[5] = 38;
       }
       water_pressure_low_read_count_int++;
     }
     else {
       water_pressure_low_read_count_int = 0;
     }
     // for testing
     // for (int r=0; r<=5; r++){Serial.println(value_current_read_array_int[r]);} 
     
     // output pressure to blynk  
     // set variable for blynk pressue output
     blynk_pressure_read_int = value_current_read_array_int[5];
     // Blynk.virtualWrite(V10, blynk_pressure_read_int);
          
     // keep record of lowest read in prior 5 seconds
     //   as a4 pump activation causes pressure to jump
     water_pressure_low_int = 50;  // reset low value
     for (int w=4; w>=1; w--) {
       // make room for new read and find lowest value in old reads
        water_pressure_low_array_int[w] = water_pressure_low_array_int[w-1];
        water_pressure_low_int = min(water_pressure_low_int, water_pressure_low_array_int[w]);
        }
     water_pressure_low_array_int[0] = value_current_read_array_int[5];
     water_pressure_low_int = min(water_pressure_low_int, water_pressure_low_array_int[0]);
     // for testing
     //for (int x=0; x<=4; x++) {
     //  Serial.print(water_pressure_low_array_int[x]);
     //  Serial.print("  ");
     //  }
     //Serial.print("    ");
     //Serial.println(water_pressure_low_int);
     //Serial.println();
               
     // update prior and current on-off states
     // save last states as prior states
     for (int s=0; s<=5; s++){
     strcpy(value_prior_state_array_str[s], value_current_state_array_str[s]);
       }
     // pump currents on a0 to a2, city water valve current on a3 and pressure pump on a4
     // update current states as "on" or "off"
     for (int j=0; j<=4; j++ ) {
       if (value_current_read_array_int[j] > 20){strcpy(value_current_state_array_str[j], " on");}
       else{strcpy(value_current_state_array_str[j], "off");}
       }
     // water pressure on a5
     if (value_current_read_array_int[5] >= 38){strcpy(value_current_state_array_str[5], " on");}
     else{strcpy(value_current_state_array_str[5], "off");}  // used in a report ??? ****
                 
     // send notification if city water being used
     if (value_current_read_array_int[3] > 20) {city_water_notice_count_int++;}
     else {city_water_notice_count_int = 0;}
     // test value read 10 times
     if (city_water_notice_count_int > 10 && city_water_notice_flag_int == 0) {
       cityWaterNotification();  // using flag, only send notification one time
       } 
          
     // send notification if well water pressure is below 38 lbs
     // value_current_read_array_int[5] = 37; // for low pressure alarm testing
     if (value_current_read_array_int[5] < 38) {water_pressure_alarm_count_int++;}
     else {water_pressure_alarm_count_int = 0;}
     // test value read 10 times as pressure tank pump turn-on can cause low pressure
     if (water_pressure_alarm_count_int > 10 && water_pressure_alarm_flag_int == 0) {
       waterPressureAlarm();  // using flag, only send alarm one time
       } 
}

void fileUpdate()
{
  /* Process steps for later reference:
  get the current date and time from rtc
  create desired files
      day log file - values logged every 5 seconds
      ON-OFF report files - changes logged every second
  check if mkr restart as opposed to new day - if so not a new day
  check if new day by comparing new file name to last used file name
      if new day, 
      scan log file to determine minutes on and lowest pressure
      send daily notificaiton with results
      Update dayYY report with results
      Add current month day figures from daily file and divide by 60
  If new month append new line to month file
  */
  char line_month_read_array_str[16][8];
  char date_discard_str[16] = "";
  char time_discard_str[16] = "";
  char month_discard_str[8];
  int month_value_read_array_int[6] = {0, 0, 0, 0, 0, 0};
  char month_value_read_array_str[6][8];
  char month_value_write_str[64];
  int daily_value_read_array_int[6] = {0, 0, 0, 0, 0, 0};
  char line_read_str[64];
  char line_read_array_str[16][8];
  int line_value_read_array_int[8];
  char read_str[8];
  int c = 0;
  unsigned long file_size_ulong;
  unsigned long new_position_ulong;
    
  // create files and open for writing date, time and values data
  // get date data
  years_int = rtc.getYear(); // Note - year is just two digits - 00 to 99
  months_int = rtc.getMonth();
  days_int = rtc.getDay();
  hours_int = rtc.getHours();
   
  //  converts to 2 character decimal base - pads leading 0s by adding the 0
  sprintf(years_str, "%02d", years_int); 
  sprintf(months_str, "%02d", months_int);
  sprintf(days_str, "%02d", days_int);
 
  // create file prefix
  strcpy(file_name_prefix_new_str, years_str);
  strcat(file_name_prefix_new_str, "-");
  strcat(file_name_prefix_new_str, months_str);
  strcat(file_name_prefix_new_str, "-");
  strcat(file_name_prefix_new_str, days_str);
    
  // create desired files  
  // create log file
  // values logged every 5 seconds
  strcpy(file_name_log_new_str, file_name_prefix_new_str);
  strcat(file_name_log_new_str, ".LOG"); 
  
  // create file names for ON-OFF file name array
  // these will be used when logging to the SD card
  // changes logged every second
  //for (int z =0; z <= 7; z++){  // not used anymore
  //strcpy(file_rpt_array_str[z], "ON-OFF");
  //strcat(file_rpt_array_str[z], years_str);
  //strcat(file_rpt_array_str[z], ".RP");
  //char z_str[8];
  //sprintf(z_str, "%1d", z);
  //strcat(file_rpt_array_str[z], z_str);
  // Serial.println(file_rpt_array_str[z]);
  //}
      
  // check if a new day for logging purposes by comparing prior and current dates
  if (strcmp(prior_days_str, days_str) == 0) { // note - returns 0 if equal
    // Serial.println ("Same day");
    goto fileUpdateend;
    }
  // a mkr restart would set the prior_days_str to "0" 
  if (strcmp(prior_days_str, "0") == 0) { // note - returns 0 if equal
    Serial.println ("MKR1010 has restarted.");
    goto fileUpdateend;
    }    
  // new day - update reports and send daily notifcation
    Serial.println ("New Day");
    strcpy(file_name_prefix_old_str, prior_years_str);
    strcat(file_name_prefix_old_str, "-");
    strcat(file_name_prefix_old_str, prior_months_str);
    strcat(file_name_prefix_old_str, "-");
    strcat(file_name_prefix_old_str, prior_days_str);
    strcpy(file_name_log_old_str, file_name_prefix_old_str);
    strcat(file_name_log_old_str, ".LOG");
    myFile = SD.open(file_name_log_old_str, FILE_READ);
    sumLogFile(); // create report
    myFile.close();
    // output report to SD file "DYY.RPT"
    strcpy(day_report_file_name_str, "D");
    strcat(day_report_file_name_str, prior_years_str); // since report is for prior day, use prior year
    strcat(day_report_file_name_str, ".RPT");
    strcpy(day_report_line_str, file_name_prefix_old_str);
    for (int d = 0; d <= 5; d++) {
      strcat(day_report_line_str, "  ");
      strcat(day_report_line_str, sum_log_time_array_str[d]);
      }
    myFile = SD.open(day_report_file_name_str, FILE_WRITE);
    myFile.println(day_report_line_str);
    myFile.close();
    // prepare notification for later sending
    strcpy(daily_notification_str, file_name_prefix_old_str);
    strcat(daily_notification_str, "  ");
    strcat(daily_notification_str, values_display_0to2_str);
    strcat(daily_notification_str, "  ");
    strcat(daily_notification_str, values_display_3to5_str); 
    // set flag and timer
    daily_notification_flag_int = 1;
    previousMillis_daily_notification = millis();
      
  // update M.RPT
    /*
    format for DYY.RPT file is
    21-07-02   560   100    22    23    35    41
    YY-MM-DD  0000  1111  2222  3333  4444  5555  
        5  8     14    20    26    32    38    44 -- line count
    cr is 45
    nl is 46
    /0 is 47 (if building a string to write without println)
           
    format for M.RPT file is
    21-07   560   100    22    23    24
    YY-MM  0000  1111  2222  3333  4444  
        5     11    17    23    29    35 -- line count
    cr is 36
    nl is 37
    /0 is 38 (if building a string to write without println)
    Note - days rpt is in minues and months rpt is in hours, so when adding days divide by 60 and round up/down
    pressure tank pump 4 is good reference for how much water being used - watering time about 2x pump time
    */
  myFile = SD.open(day_report_file_name_str, FILE_READ);
  /*
    SD is a simplified wrapper for SdFat.  It defines new parameters as follows
    #define FILE_READ O_READ
    #define FILE_WRITE (O_READ | O_WRITE | O_CREAT | O_APPEND)
    where in SdFat they mean:
    O_READ - Open for reading.
    O_RDONLY - Same as O_READ.
    O_WRITE - Open for writing.
    O_WRONLY - Same as O_WRITE.
    O_RDWR - Open for reading and writing.
    O_APPEND - If set, the file offset shall be set to the end of the file prior to each write.
    O_AT_END - Set the initial position at the end of the file.
    O_CREAT - If the file exists, this flag has no effect except as noted under O_EXCL below. Otherwise, the
    file shall be created
    O_EXCL - If O_CREAT and O_EXCL are set, open() shall fail if the file exists.
    O_SYNC - Call sync() after each write. This flag should not be used with write(uint8_t) or any functions 
    do character at a time writes since sync() will be called after each byte.
    O_TRUNC - If the file exists and is a regular file, and the file is successfully opened and is not read 
    only, its length shall be truncated to 0.
    To overwrite (as opposed to append), file must be opened with O_WRITE.    
    Note that O_WRITE does not create a file if one does not exist.
    */
  // rewind 32 lines 32*46 is 
  if (myFile) {
    // determine length
    Serial.println("current position is");
    Serial.println(myFile.position());   // for testing
    file_size_ulong = myFile.size();
    Serial.println("file size is");
    Serial.println(myFile.size());
    // seek to new position
    if (file_size_ulong >= 32*46){ 
      new_position_ulong = 0, file_size_ulong - (32*46);  // rewind 32 lines - inlcudes CR and LF
      }
    else {
    // start at beginning of file
    new_position_ulong = 0;
      }
    Serial.println("seek to position ");
    Serial.println(new_position_ulong);
    myFile.seek(new_position_ulong);          
    Serial.println("actual position is:");
    Serial.println(myFile.position());  // for testing
    while (myFile.available()) { 
      Serial.println ("file read:"); // for testing
      // As background, Serial.println() prints data to the serial port as human-readable ASCII text followed by a carriage 
      //   return character (ASCII 13, or '\r') and then a newline character (ASCII 10, or '\n'). 
      myFile.read(line_read_str,46); // needs to read 46 characters to work with file output used.
      Serial.print(line_read_str); // file read has LF as last character  // for testing
      // for (c = 0; c <= 46; c++) {Serial.println(line_read_str[c]);}  // for testing
      sscanf(line_read_str, "%s %s %s %s %s %s %s", date_discard_str, line_read_array_str[0], 
      line_read_array_str[1], line_read_array_str[2], line_read_array_str[3],
      line_read_array_str[4], line_read_array_str[5]);
      Serial.println(date_discard_str);  // for testing
      // isolate month
      month_discard_str[0] = date_discard_str[3];
      month_discard_str[1] = date_discard_str[4];
      month_discard_str[2] = '\0';
      // if line is for current month get data
      Serial.println("reading day file to calculate monthly result to date");
      if (strcmp(month_discard_str, prior_months_str) == 0){  // note - returns 0 if equal  - changed 
        //  this to prior_months_str to include last day of month in month total??
        Serial.print("prior_months_str is:  ");
        Serial.println(prior_months_str); // for testing
        Serial.print("month_discard_str is:  ");
        Serial.println(month_discard_str);
        Serial.println("same month in day file, so day data in line added");
        // curent month, so add day numbers to month numbers (do not include well pressure [5])
        // convert the values read to integers
        for (int d = 0; d <= 4; d++) {
          sscanf(line_read_array_str[d], "%d", &line_value_read_array_int[d]);
          //Serial.println(line_value_read_array_str[d]);  // for testing
          }
        // add day numbers
        for (int d = 0; d <= 4; d++) {
          month_value_read_array_int[d] = month_value_read_array_int[d] + line_value_read_array_int[d];
          Serial.println(month_value_read_array_int[d]);  // for testing
          }
        }  // Note - if new month, no lines counted and month_value_read_array_int[] should all be 0
           // but this is a mistake, as last day of month not logged in prior month  ****
      }
      myFile.close();
    }
  else {
    strcpy(error_type_str, "SD-1");
    mkrError();
    }
  // done reading relevant data from day file
  // convert monthly minutes number to hours string - divide by 60 with rounding up
  for (int d = 0; d <= 4; d++) {
    sprintf(month_value_read_array_str[d], "%4d", (month_value_read_array_int[d] + 30) / 60);  // pads with spaces
    Serial.println(month_value_read_array_str[d]);  // for testing
    }
  // create last line of month file
  strcpy(month_value_write_str, prior_years_str);
  strcat(month_value_write_str, "-");
  strcat(month_value_write_str, prior_months_str);
  for (int d = 0; d <= 4; d++) {
    strcat(month_value_write_str, "  ");
    strcat(month_value_write_str, month_value_read_array_str[d]);
    }
  Serial.println("Last line of month file:");  // for testing
  Serial.println(month_value_write_str);
  // open month file
  // if same month, update, or if new month, append
  myFile = SD.open("M.RPT", O_READ | O_WRITE | O_CREAT); 
  // Note - O_WRITE allows overwriting but would need O-CREAT too if file does not exist, 
  // unlike FILE_WRITE - see details above - which does not allow overwriting!
    if (myFile) {
      // determine length
      Serial.println("current position is");
      Serial.println(myFile.position());   // for testing
      file_size_ulong = myFile.size();
      Serial.println("file size is");
      Serial.println(myFile.size());
      // seek to new position
      if (file_size_ulong >= 37){ 
        new_position_ulong = file_size_ulong - 37;  // rewind 1 lines - inlcudes CR and LF
        }
      else {
        // start at beginning of file
        new_position_ulong = 0;
        }
      Serial.println("seek to position ");
      Serial.println(new_position_ulong);
      myFile.seek(new_position_ulong);          
      Serial.println("actual position is:");
      Serial.println(myFile.position());  // for testing
      }
    else {
      strcpy(error_type_str, "SD-2");
      mkrError();
      }
   if (file_size_ulong == 0){ // if new file, just write line
     myFile.println(month_value_write_str);  // println adds CR and LF
     }
   else {
     while (myFile.available()) { 
        // Serial.println ("file read:"); // for testing
        // Serial.println() prints text followed by a carriage return character (ASCII 13, or '\r') 
        //   and then a newline character (ASCII 10, or '\n'). 
        myFile.read(line_read_str,37); // needs to read 37 characters to work with file output used.
        //  Note prior error above prrevented the file from reading, since O_Write was used.
        
        Serial.print(line_read_str); // file read has LF as last character  // for testing
        // for (c = 0; c <= 37; c++) {Serial.println(line_read_str[c]);}  // for testing
        sscanf(line_read_str, "%s %s %s %s %s %s", date_discard_str, line_read_array_str[0], 
        line_read_array_str[1], line_read_array_str[2], line_read_array_str[3],
        line_read_array_str[4]);
        Serial.println(date_discard_str);  // for testing
        // isolate month
        month_discard_str[0] = date_discard_str[3];
        month_discard_str[1] = date_discard_str[4];
        month_discard_str[2] = '\0';
        // if line is for current month overwrite with new data
        if (strcmp(month_discard_str, prior_months_str) == 0){  // note - returns 0 if equal  - made same change to 1090
          Serial.print("prior_months_str is:  ");
          Serial.println(prior_months_str); // for testing
          Serial.print("month_discard_str is:  ");
          Serial.println(month_discard_str);
          Serial.println("same month for overwrite test");
          // over write last line of file
          myFile.seek(new_position_ulong);  // new_position_ulong = file_size_ulong - 37    
          Serial.println(new_position_ulong); // for testing
          Serial.println("actual position is before write is:");
          Serial.println(myFile.position());  // for testing
          myFile.println(month_value_write_str);  // println adds CR and LF
          Serial.println("actual position after write is:");
          Serial.println(myFile.position());  // for testing
          }
        // if month line is not the same month, append new month data
        else {
          // move to end of file and append
          Serial.print("prior_months_str is:  ");
          Serial.println(prior_months_str); // for testing
          Serial.print("month_discard_str is:  ");
          Serial.println(month_discard_str);
          Serial.println("different month for append test");
          
          myFile.seek(file_size_ulong);   
          Serial.println("actual position before append write is:");
          Serial.println(myFile.position());  // for testing
          myFile.println(month_value_write_str);  // println adds CR and LF
          Serial.println("actual position after append write is:");
          Serial.println(myFile.position());  // for testing
          }
       }
    } 
  myFile.close(); // always close files after writing
  fileRead();  // for testing - reads file_read_file_name_str
  goto fileUpdateend;
    
  fileUpdateend:
  // old file name set to new file name
  strcpy(prior_days_str, days_str);
  //Serial.println(prior_days_str);
  //Serial.println(days_str);
  strcpy(prior_months_str, months_str);
  //Serial.println(prior_months_str);
  //Serial.println(months_str);
  strcpy(prior_years_str, years_str);
  //Serial.println(prior_years_str);
  //Serial.println(years_str);
  strcpy(file_name_prefix_old_str, file_name_prefix_new_str); 
  //Serial.println(file_name_prefix_old_str);
  //Serial.println(file_name_prefix_new_str);
  strcpy(file_name_log_old_str, file_name_log_new_str);
  //Serial.println(file_name_log_old_str);
  //Serial.println(file_name_log_new_str);
}

void updateDate()
{
  // get date data
  years_int = rtc.getYear(); // Note - year is just two digits - 00 to 99
  sprintf(years_str, "%02d", years_int); //  converts to 2 character decimal base - pads leading 0s by adding the 0
  months_int = rtc.getMonth();
  sprintf(months_str, "%02d", months_int);
  days_int = rtc.getDay();
  sprintf(days_str, "%02d", days_int);
  strcpy(date_str, years_str);
  strcat(date_str, "-");
  strcat(date_str, months_str);
  strcat(date_str, "-");
  strcat(date_str, days_str);
}  

void updateTime()
{
  //get time data
  hours_int = rtc.getHours();
  sprintf(hours_str, "%02d", hours_int); // converts to 2 character decimal base - pads leading 0s by adding the 0
  minutes_int = rtc.getMinutes();
  sprintf(minutes_str, "%02d", minutes_int); 
  seconds_int = rtc.getSeconds();
  sprintf(seconds_str, "%02d", seconds_int); 
  strcpy(time_str, hours_str);
  strcat(time_str, ":");
  strcat(time_str, minutes_str);
  strcat(time_str, ":");
  strcat(time_str, seconds_str);
}  

void updateDisplayValues()
{
  int i;
  for(i = 0; i <= 5; i++){ // note - values ranged above between 0 and 9999
    sprintf(value_current_read_array_str[i], "%4d", value_current_read_array_int[i]);  // pads with spaces
    // Serial.println(value_current_read_array_str[i]);
  }
  // for various outputs/displays
  strcpy(values_read_0to2_str, value_current_read_array_str[0]);
  strcat(values_read_0to2_str, "  ");
  strcat(values_read_0to2_str, value_current_read_array_str[1]);
  strcat(values_read_0to2_str, "  ");
  strcat(values_read_0to2_str, value_current_read_array_str[2]);
    
  strcpy(values_read_3to5_str, value_current_read_array_str[3]);
  strcat(values_read_3to5_str, "  ");
  strcat(values_read_3to5_str, value_current_read_array_str[4]);
  strcat(values_read_3to5_str, "  ");
  strcat(values_read_3to5_str, value_current_read_array_str[5]);
  
  strcpy(values_single_line_0to5_str, values_read_0to2_str);
  strcat(values_single_line_0to5_str, "  ");
  strcat(values_single_line_0to5_str, values_read_3to5_str);
}

void outputData(){
  // blink green led on mkr for each write to SD card
  WiFiDrv::digitalWrite(25, HIGH); // green on at full brightness  ttt *** check if analogWrite works better
  // One of the MKR1010s has the grren and red led chip installed reversed - test is red, production is green
  // output to serial port for testing
  //Serial.print(date_str);
  //Serial.print("  ");
  //Serial.print(time_str);
  //Serial.print("  ");
  //Serial.println(values_single_line_0to5_str);
  
  // output log to SD card
  // output every 4 counts
  if (SD_write_skip_cnt <=0){
    SD_write_skip_cnt = 4;
    strcpy(SD_print_str, date_str);
    strcat(SD_print_str, "  ");
    strcat(SD_print_str, time_str);
    strcat(SD_print_str, "  ");
    strcat(SD_print_str, values_single_line_0to5_str);
    myFile = SD.open(file_name_log_new_str, FILE_WRITE);
      if (myFile) {
      myFile.println(SD_print_str);
      myFile.close(); // always close files after writing
      }
      // if the file does not open, report error
      else {
      strcpy(error_type_str, "SD-3");
      mkrError();
      }
    }
  else{
      SD_write_skip_cnt = SD_write_skip_cnt - 1;  
    }
  
  // output on-off report to SD card as needed up to every second
  // water pressure is now reported for each pressure pump on-off
  // create files for on-off logs
  for (int r = 0; r <= 6; r++){  // 0 to 5 are channels - 6 is all channels
    char r_str[8];
    sprintf(r_str, "%1d", r); // this is where individual channel is identified 
    strcpy (SD_rpt_file_str, r_str); 
    strcat (SD_rpt_file_str, "-");
    strcat (SD_rpt_file_str, years_str);
    strcat (SD_rpt_file_str, ".RPT");
    strcpy(annual_on_off_file_name_str[r], SD_rpt_file_str);
    }
  for (int r=0; r<=4; r++){  // check if value changed, and write to SD if it did
    if(strcmp(value_current_state_array_str[r], value_prior_state_array_str[r]) != 0)
      {
      if (strcmp(value_current_state_array_str[r], " on") == 0) {
        strcpy(value_current_state_array_10_str[r], "1");
        }
        else{
          strcpy(value_current_state_array_10_str[r], "0");
        }
      // convert water pressure to 2 digit string with no leading 0s
      sprintf(value_current_state_array_10_str[5], "%2d", value_current_read_array_int[5]);

      // if water pressure is being reported for a4 "on", use water_pressure_low_int // *****
      //    calculated above as loweset pressure during prior 5 reads to avoid pump turn-on high pressure
      if ((r == 4) && (strcmp(value_current_state_array_10_str[4], "1") == 0 )) { // note - returns 0 if equal
        sprintf(value_current_state_array_10_str[5], "%2d", water_pressure_low_int);
        // for testing  ****
        //Serial.print(r);
        //Serial.print("   ");
        //Serial.println(value_current_state_array_10_str[4]);
        //Serial.print("   ");
        //Serial.println(value_current_state_array_10_str[5]);
        }
       strcpy(SD_rpt_str, date_str);
      strcat(SD_rpt_str, "  ");
      strcat(SD_rpt_str, time_str);
      if (r == 0){
        strcat(SD_rpt_str, "  |");
        strcat(SD_rpt_str, value_current_state_array_10_str[r]);
        strcat(SD_rpt_str, "| | | | |  |");} 
      if (r == 1){
        strcat(SD_rpt_str, "  | |");
        strcat(SD_rpt_str, value_current_state_array_10_str[r]);
        strcat(SD_rpt_str, "| | | |  |");} 
      if (r == 2){
        strcat(SD_rpt_str, "  | | |");
        strcat(SD_rpt_str, value_current_state_array_10_str[r]);
        strcat(SD_rpt_str, "| | |  |");} 
      if (r == 3){
        strcat(SD_rpt_str, "  | | | |");
        strcat(SD_rpt_str, value_current_state_array_10_str[r]);
        strcat(SD_rpt_str, "| |  |");} 
      if (r == 4){
        strcat(SD_rpt_str, "  | | | | |");
        strcat(SD_rpt_str, value_current_state_array_10_str[r]);
        strcat(SD_rpt_str, "|"); 
        // note - this is a 2 digit number with two leading spaces, not an on-off state
        strcat(SD_rpt_str, value_current_state_array_10_str[5]);
        strcat(SD_rpt_str, "|");}
      // always ouput to 6-yy.rpt file.  Individual channel x-yy.rpt outputs below
      myFile = SD.open(annual_on_off_file_name_str[6], FILE_WRITE);
      if (myFile) {
        myFile.println(SD_rpt_str);
        myFile.close(); // always close files after writing
        }
      // if the file does not open, report error
      else {
        strcpy(error_type_str, "SD-4");
        mkrError();
        }
      //Serial.println();
      //Serial.println(SD_rpt_str);
      //Serial.println();
     
      // output to individual report file
      strcpy(SD_rpt2_str, date_str);
      strcat(SD_rpt2_str, "  ");
      strcat(SD_rpt2_str, time_str);
      strcat(SD_rpt2_str, "  ");
      char r_str[8];
      sprintf(r_str, "%1d", r); // this is where individual channel is identified 
      strcat(SD_rpt2_str, r_str);
      strcat(SD_rpt2_str, "  ");
      strcat(SD_rpt2_str, value_current_state_array_str[r]); 
      myFile = SD.open(annual_on_off_file_name_str[r], FILE_WRITE);
      if (myFile) {
        myFile.println(SD_rpt2_str);
        myFile.close(); // always close files after writing
      }
      // if the file does not open, report error
      else {
        strcpy(error_type_str, "SD-5");
        mkrError();
       }
      
      // special case for psi channel 5, as it gets written at same time as channel 4 pressure pump
      if (r == 4) {
        strcpy(SD_rpt2_str, date_str);
        strcat(SD_rpt2_str, "  ");
        strcat(SD_rpt2_str, time_str);
        strcat(SD_rpt2_str, "  ");
        sprintf(r_str, "%1d", 5); // this is where individual channel is identified 
        strcat(SD_rpt2_str, r_str);
        strcat(SD_rpt2_str, "  ");
        // note below has been corrected to use array_10 instead of value_current_read_array_str[5] 
        strcat(SD_rpt2_str, value_current_state_array_10_str[5]); 
        myFile = SD.open(annual_on_off_file_name_str[5], FILE_WRITE);
        if (myFile) {
          myFile.println(SD_rpt2_str);
          myFile.close(); // always close files after writing
        }
        // if the file does not open, report error
        else {
          strcpy(error_type_str, "SD-6");
          mkrError();
        }
      }      
    }
  }
    
  // output to oled
  display.clearDisplay(); // clear oled display
  oledText(date_str, 8, 3, 1, false);
  oledText(time_str, 72, 3, 1, false);
  oledText(values_read_0to2_str, 8, 11, 1, false);
  oledText(values_read_3to5_str, 8, 21, 1, false);
  oledRect(); // also displays display
  // output pressue to blynk  needed?  does blynk slow down logging?  ***
  blynk_pressure_read_int = value_current_read_array_int[5];
  // Blynk.virtualWrite(V10, blynk_pressure_read_int);
  WiFiDrv::digitalWrite(25, LOW); // green off
}  

void printDirectory(File dir, int numTabs) {
   while(true) {

     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
        break;
     }
     for (int i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       // files have sizes, directories do not
       Serial.print("\t\t");
       Serial.println(entry.size(), DEC);
     }
     entry.close();
     }
 }

// oledText(char *text, int x, int y,int size, boolean d)
//   text is the text string to be printed
//   x is the integer x position of text
//   y is the integer y position of text
//   z is the text size - supports sizes from 1 to 8
//   d is either "true" or "false". True draws the display
void oledText(char *text, int x, int y, int size, boolean d) {
  display.setTextSize(size);
  display.setTextColor(WHITE);
  display.setCursor(x,y);
  display.println(text);
  if(d){
    // display.display();  // flushes all changes to the display hardware
    oledDisplay_display();  // includes display.display
  }
}

// draws oled rectangle around three text lines
void oledRect() {
  display.drawRect(0, 0, 128, 32, WHITE);  //128 by 32 is maximum - one pixel line
  // is this 129 by 33 of 128 by 32 - appears the earlier
  //display.display(); // draws display by flushing all changes to the display hardware
  oledDisplay_display();  // includes display.display
  
  //other OLED display commands
  //display.setTextColor(BLACK, WHITE); //reverse
  //display.drawPixel(pixelX, pixelY, color) //single pixel
  //display.width() //returns value
  //display.height()
  //display.drawRect(upperLeftX, upperLeftY, width, height, color)
  //display.drawLine(startX, startY, endX, endY, color)
  //display.fillRect(upperLeftX, upperLeftY, width, height, WHITE)
  //display.drawRoundRect(upperLeftX, upperLeftY, width, height, cornerRadius, color)
  //display.drawFillRoundRect(upperLeftX, upperLeftY, width, height, cornerRadius, color)
  //display.drawCircle(centerX, centerY, radius, color)
  //display.fillCircle(centerX, centerY, radius, color)
  //display.drawTriangle(poin1X, point1Y, point2X, point2Y, point3X, point3Y, color)
  //display.fillTriangle(poin1X, point1Y, point2X, point2Y, point3X, point3Y, color)
  //display.write() function to display a single character
  //display.drawBitmap(topLeftX, topLeftY, imageData, width, height, color) // for custom graphic
  //display.setRotation(2);  // rotate display 180 choices 0, 1 (90), 2 (180) and 3 (270)
}

// draws oled display
// replacement for display.diplay to allow for turning display off to preserve oled
void oledDisplay_display() {  
   if (oled_on_int != 1){
     display.clearDisplay(); // clear oled display 
     }
   display.display(); // draws display by flushing all changes to the display hardware
   }

// check for oled motion or switch activation - used for display timeout 
// adjust input pull down up to pull up own
void oledMotion() {
  value_d2_int = digitalRead(2);
  if (value_d2_int == 1) { // motion detected
    oled_on_int = 1;
    oled_motion_cnt_int = 0;
  }
  else {
    oled_motion_cnt_int ++; 
    if (oled_motion_cnt_int >= 120) {  // 2 minute display timeout
      oled_on_int = 0;
      oled_motion_cnt_int = 120;  //  keep at 120 until motion detected
    }
  }
  // Serial.println (oled_motion_cnt_int);  // for testing
}

void localMenu() {
  // Read digital pins that are push buttons
  // Update the debouncer instances
  debouncer_d0.update();
  // Call code if Select button transitions from high to low
  if (debouncer_d0.fell()) {         
    WiFiDrv::digitalWrite(26, HIGH); // red on at full brightness
    Serial.println("Select to continue."); 
    Serial.println("Scroll for reports.");} 
    else {
    WiFiDrv::digitalWrite(26, LOW); // red off
    }
  if (debouncer_d0.fell()) {
    WiFiDrv::digitalWrite(26, HIGH); // red on at full brightness
    // Demount sd card - 
    // to fix a card that is not working, press d0, remove good card and insert bad card and restart.
    //SD.end(); // does this even work?  ***
    root.close();  // this is suggested by doc instead of SD.end().  You can safely remove if all files closed
    
    // print to oled
    display.clearDisplay(); // This before display.display gets rid of the oled Adafruit logo
    //display.display(); 
    oledDisplay_display();  // includes display.display
    strcpy(oled_text_str, "Select to continue.");
    oledText(oled_text_str, 8, 11, 1, false);
    strcpy(oled_text_str, "Scroll for reports.");
    oledText(oled_text_str, 8, 21, 1, false);
    // draw rectangle - also displays display (so can use false above)
    oledRect();
    WDTimer.clear(); // reset WDT     
    // wait for next button press 
    Menu1:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {
      SD.begin(k_SD_chip_select); // if either button is pressed, will need SD to begin
      goto MenuExit;}
    if (debouncer_d1.fell()) {
      SD.begin(k_SD_chip_select); // if either button is pressed, will need SD to begin
      goto Menu2;}  
    goto Menu1;

    Menu2: // time menu for first file
    // load 8 most recent ".LOG" files in root directory as 0-7
    loadLogfiles();
  
    // read available files
    if (strcmp(menu_file_name_array_str[0], "no file") == 0) {sumLogFileNoFile();}   // note - returns 0 if equal
    else {
      myFile = SD.open(menu_file_name_array_str[0], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu2a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu3;}
    goto Menu2a;

    Menu3: // time menu for second file
    if (strcmp(menu_file_name_array_str[1], "no file") == 0) {sumLogFileNoFile();}   // note - returns 0 if equal
    else {
      myFile = SD.open(menu_file_name_array_str[1], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu3a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu4;}
    goto Menu3a;

    Menu4: // time menu for third file
    if (strcmp(menu_file_name_array_str[2], "no file") == 0) {sumLogFileNoFile();}  // note - returns 0 if equal
    else {
      myFile = SD.open(menu_file_name_array_str[2], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu4a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu5;}
    goto Menu4a;

    Menu5: // time menu for fourth file
    if (strcmp(menu_file_name_array_str[3], "no file") == 0) {sumLogFileNoFile();}   // note - returns 0 if equal
    else {
      myFile = SD.open(menu_file_name_array_str[3], FILE_READ);
      sumLogFile();
      myFile.close();
      }
   WDTimer.clear();
   // wait for next button press 
    Menu5a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()){goto MenuExit;}
    if (debouncer_d1.fell()){goto Menu6;}
    goto Menu5a;

    Menu6: // time menu for fifth file
    if (strcmp(menu_file_name_array_str[4], "no file") == 0) {sumLogFileNoFile();}   // note - returns 0 if equal
    else {
      myFile = SD.open(menu_file_name_array_str[4], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu6a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}    
    if (debouncer_d1.fell()) {goto Menu7;}
    goto Menu6a;

    Menu7: // time menu for sixth file
    if (strcmp(menu_file_name_array_str[5], "no file") == 0) {sumLogFileNoFile();}
    else {
      myFile = SD.open(menu_file_name_array_str[5], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu7a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu8;}
    goto Menu7a;
    
    Menu8: // time menu for seventh file
    if (strcmp(menu_file_name_array_str[6], "no file") == 0) {sumLogFileNoFile();}
    else {
      myFile = SD.open(menu_file_name_array_str[6], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu8a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu9;}
    goto Menu8a;

    Menu9: // time menu for eighth file
    if (strcmp(menu_file_name_array_str[7], "no file") == 0) {sumLogFileNoFile();}
    else {
      myFile = SD.open(menu_file_name_array_str[7], FILE_READ);
      sumLogFile();
      myFile.close();
      }
    WDTimer.clear();
    // wait for next button press 
    Menu9a:
    debouncer_d0.update();
    debouncer_d1.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    if (debouncer_d1.fell()) {goto Menu10;}
    goto Menu9a;

    Menu10: // back menu
    display.clearDisplay(); // This before display.display gets rid of the oled Adafruit logo
    // display.display(); 
    oledDisplay_display();  // includes display.display
    strcpy(oled_text_str, "Select to exit.");
    oledText(oled_text_str, 8, 11, 1, false);
    oledRect(); // also displays display
    Serial.println("Select to exit."); 
    WDTimer.clear();
    // wait for next button press 
    Menu10a:
    debouncer_d0.update();
    WDTimer.clear();
    if (debouncer_d0.fell()) {goto MenuExit;}
    goto Menu10a;
      
    MenuExit:
    // turn off red led
    WiFiDrv::digitalWrite(26, LOW); // red is off
    WDTimer.clear();
    }
}

void loadLogfiles()
{
// determine the 8 files we want based on current date
// create file array for wanted files
// check if each wanted file is on the SD card
years_int = rtc.getYear(); // Note - year is just two digits - 00 to 99
months_int = rtc.getMonth();
days_int = rtc.getDay();
for (int fn = 0; fn <=7; fn++){
  sprintf(years_str, "%02d", years_int);
  sprintf(months_str, "%02d", months_int);
  sprintf(days_str, "%02d", days_int);
  Serial.println(years_str);
  Serial.println(months_str);
  Serial.println(days_str); 
  strcpy(menu_file_name_desired_array_str[fn], years_str);
  strcat(menu_file_name_desired_array_str[fn], "-");
  strcat(menu_file_name_desired_array_str[fn], months_str);
  strcat(menu_file_name_desired_array_str[fn], "-");
  strcat(menu_file_name_desired_array_str[fn], days_str);
  strcat(menu_file_name_desired_array_str[fn], ".LOG"); 
  Serial.println(menu_file_name_desired_array_str[fn]);
  days_int --;
  if (days_int == 0){
    if (months_int == 1){
      days_int = 31; months_int = 12; years_int--; goto loadLogfilesbreak;
      }
    if (months_int == 2){days_int = 31; months_int = 1;}
    if (months_int == 3){
      days_int = 28; months_int = 2;
      // adjust for leap year
      if ((years_int % 4) == 0) {days_int = 29;} // Leap year
      if ((years_int % 100) == 0) {days_int = 28;} // But not if divisable by 100
      if ((years_int % 400) == 0) {days_int = 29;} // Unless divisible by 400
      }
    if (months_int == 4){days_int = 31; months_int = 3;}
    if (months_int == 5){days_int = 30; months_int = 4;}
    if (months_int == 6){days_int = 31; months_int = 5;}  
    if (months_int == 7){days_int = 30; months_int = 6;}
    if (months_int == 8){days_int = 31; months_int = 7;}  
    if (months_int == 9){days_int = 31; months_int = 8;}
    if (months_int == 10){days_int = 30; months_int = 9;}
    if (months_int == 11){days_int = 31; months_int = 10;}
    if (months_int == 12){days_int = 30; months_int = 11;}
    
    loadLogfilesbreak:
    ;  // why is this needed?
    }
  }
updateDate(); // refresh rtc if these are used elsewhere without a refresh

Serial.println("File names desired:");
for (int fn = 0; fn <=7; fn++){
  Serial.println(menu_file_name_desired_array_str[fn]);
  }
//load desired files into menufile array
Serial.println("File names available to read:");
for (int fn = 0; fn <=7; fn++){
  if (SD.exists(menu_file_name_desired_array_str[fn])){
    strcpy(menu_file_name_array_str[fn], menu_file_name_desired_array_str[fn]);
    }
  else {
    strcpy(menu_file_name_array_str[fn], "no file");
    }
  Serial.println(menu_file_name_array_str[fn]);
  }
}

void sumLogFile()
{ 
int log_value_read_array_int[6] = {0, 0, 0, 0, 0, 0};
char line_read_str[64];
char line_read_array_str[16][8];
char read_str[8];
char date_discard_str[16];
char time_discard_str[16];
int c = 0;
// read each line of the file into a string array
// parse string to channel values and convert to integers
// determin if channel is on, and increment channel count until done

// output message to oled
display.clearDisplay(); // clear oled display
strcpy(oled_text_str, "calculating ...");
oledText(oled_text_str, 8, 11, 1, false);
oledRect(); // also includes display.display
// Serial.println("calculating ...");

// get lowest well pressure reading durng the day on a5 - initialize at 75 lbs
int well_pressure_old_int = 75; 
int WDT_count_int = 0;
// clear sum_log_time_array_int
  for (c = 0; c <= 5; c++) {
    sum_log_time_array_int[c] = 0;
    }
while (myFile.available()) { 
  // Serial.println ("file read:"); // for testing
  // As background, Serial.println() prints data to the serial port as human-readable ASCII text followed by a carriage 
  //   return character (ASCII 13, or '\r') and then a newline character (ASCII 10, or '\n'). 
  myFile.read(line_read_str,56); // needs to read 56 characters to work with file output used.  0-53 plus 54 and 55 are CR LF.
  // 8 2 8 2 4 2 4 2 4 2 4 2 4 2 4CRLF  or 10 plus 10 plus 5*6 plus 4 for 54 or 0-53 plus CR LF 
  // 20-12-08  08:02:22  9999  9999  9999  9999  9999  9999CRLF
  //Serial.print(line_read_str); // file read has LF as last character  // for testing
  // for (c = 0; c <= 56; c++) {Serial.println(line_read_str[c]);}  // for testing
  sscanf(line_read_str, "%s %s %s %s %s %s %s %s", date_discard_str, time_discard_str, line_read_array_str[0], 
    line_read_array_str[1], line_read_array_str[2], line_read_array_str[3],
    line_read_array_str[4], line_read_array_str[5]);
  // Serial.println(date_discard_str);
  // Serial.println(time_discard_str);
  for (c = 0; c <= 5; c++) {
    sscanf(line_read_array_str[c], "%d", &log_value_read_array_int[c]);
    // for testng reads
    // Serial.println(line_read_array_str[c]);
    // Serial.println(log_value_read_array_int[c]);
    }
    // increment values if channel A0 to A5 are on
  for (c = 0; c <= 5; c++) {
    if (log_value_read_array_int[c] > 15) {sum_log_time_array_int[c] ++;}
  }
  // get lowest pressure reading on A6
  if (log_value_read_array_int[5] < well_pressure_old_int) {well_pressure_old_int = log_value_read_array_int[5];}
  //note - this is blocking and can take awhile - need to clear WDT periodically
    WDT_count_int++;
  if (WDT_count_int >= 2256){ //clears every two seconds - approx 2256 line reads
    WDTimer.clear();
    Blynk.run(); // keep Blynk from timing out too?  ***
    //Serial.println(WDT_count_int);  // for testing
    //Serial.println("WDTimer cleared");
    WDT_count_int = 0;
    }
  }

// scale value to minutes (note samples are written every 5 seconds)
// for A6, record lowest value read
for (c = 0; c <= 5; c++) {
  // round by adding 1/2 of 12
  sum_log_time_array_int[c] = (sum_log_time_array_int[c] + 6) / 12;
  // cap value at 9999
  if(sum_log_time_array_int[c] > 9999) {sum_log_time_array_int[c] = 9999;}
  }
sum_log_time_array_int[5] = well_pressure_old_int; // instead of time on, return the lowest value read

// convert integers to strings
for (c = 0; c <= 5; c++) {
  sprintf(sum_log_time_array_str[c], "%4d", sum_log_time_array_int[c]); // pads with spaces
  // example   sprintf(minutes_str, "%02d", minutes_int); 
  
  // add leading spaces the old way without using sprintf
  //if (sum_log_time_array_int[c] < 100) {strcpy(sum_log_time_array_str[c], " ");}
  //if (sum_log_time_array_int[c] < 10) {strcat(sum_log_time_array_str[c], " ");}
  //strcat(sum_log_time_array_str[c], read_str);
  //Serial.println(sum_log_time_array_int[c]);
  //Serial.println(sum_log_time_array_str[c]);
  if (sum_log_time_array_int[c] > 9999) {strcpy(sum_log_time_array_str[c], "9999");} // cap display at 9999
  }
    
// print to oled
display.clearDisplay(); // clear oled display
// get file date
char fString[16];
strcpy(fString, myFile.name());
oledText(fString, 8, 3, 1, false);

strcpy(values_display_0to2_str, sum_log_time_array_str[0]);
strcat(values_display_0to2_str,"  ");
strcat(values_display_0to2_str, sum_log_time_array_str[1]);
strcat(values_display_0to2_str, "  ");
strcat(values_display_0to2_str, sum_log_time_array_str[2]);

strcpy(values_display_3to5_str, sum_log_time_array_str[3]);
strcat(values_display_3to5_str, "  ");
strcat(values_display_3to5_str,sum_log_time_array_str[4]);
strcat(values_display_3to5_str, "  ");
strcat(values_display_3to5_str, sum_log_time_array_str[5]); 

oledText(values_display_0to2_str, 8, 11, 1, false);
oledText(values_display_3to5_str, 8, 21, 1, false);
oledRect(); // also displays display
// for testing
//Serial.print(fString);
//Serial.print("  ");
//Serial.print(values_display_0to2_str);
//Serial.print("  ");
//Serial.println(values_display_3to5_str);
}

void sumLogFileNoFile()
{
// output
// print to oled
display.clearDisplay(); // clear oled display
strcpy(oled_text_str, "calculating ...");
oledText(oled_text_str, 8, 11, 1, false);
oledRect(); // also displays display
display.clearDisplay(); // clear oled display
// print to serial port
Serial.println("calculating ...");
delay(750);
strcpy(oled_text_str, "no file ...");
oledText(oled_text_str, 8, 11, 1, false);
oledRect(); // also displays display
// print to serial port
Serial.println("no file");
}

void SDcardinfo(){
  Serial.println("Initializing SD card...");
  // we'll use the initialization code from the utility libraries
  // since we're just testing if the card is working!
  //if (!card.init(SPI_HALF_SPEED, k_SD_chip_select)) {
  if (!card.init(k_SD_chip_select)){
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the SD chip select pin to match your shield or module?");
    // if error
    strcpy(error_type_str, "SD-7");
    mkrError();
  }
  else {
    Serial.println("Wiring is correct and a card is present.");
  }
  
  // print the type of card
  Serial.print("Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    // if error
   strcpy(error_type_str, "SD-8");
   mkrError();
  }

  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  
  // print the type and size of the first FAT-type volume
  int volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1KB)
  Serial.print("Volume size (Kb):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (Mb):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print(("Volume size (Gb):  "));
  Serial.println((float)volumesize / 1024.0);
// for testing
// Serial.println("\nFiles found on the card (name, date and size in bytes): ");
// root.openRoot(volume);
// list all files in the card with date and size
// root.ls(LS_R | LS_DATE | LS_SIZE);
}

void waterPressureAlarm() // to clear alarm flag use command or reset device
{
Blynk.logEvent("wm_low_pressure"); // log and send in app notice (with email - no sms under selected Blynk plan) 
Serial.println("WM has low pressure");
terminal.println("WM has lowpressure");
water_pressure_alarm_flag_int = 1; // using flag, only send alarm one time
}

void cityWaterNotification() // to clear notice flag use command or reset device
{
Blynk.logEvent("wm_city_water_notification"); // log and send in app notice (with email - no sms under selected Blynk plan) 
Serial.println("City water is being used");
terminal.println("City water is being used");
city_water_notice_flag_int = 1; // using flag, only send notice one time
}

void mkrError()
{
  // do not log new errors if one has been reported 
  //   and now in prcess of logging and resetting
  if (mkr_reset_flag_int == 1) {return;}
  // general case errors
  strcpy(error_type_display_str, error_type_str);
  strcat(error_type_display_str, " Error");
  // special case error
  if (strcmp(error_type_str, "BT") == 0) {
    strcpy(error_type_display_str, "BT restart");
    }  
  // set flag for error reporting and shutdown
  mkr_reset_flag_int = 1;
  }

// processor software reset 
void mkrReset()  // runs at end of 5 second mkrReset timer
{
  // display message
  strcpy(oled_line1_str, "Restarting ...");  //  19 characters per line
  strcpy(oled_line2_str, "  ");
  strcat(oled_line2_str, error_type_display_str);
  strcpy(oled_line3_str, "");
  display.clearDisplay(); // This before display.display gets rid of the current text
  oledText(oled_line1_str, 8, 3, 1, false);  // update oled
  oledText(oled_line2_str, 8, 11, 1, false); //
  oledText(oled_line3_str, 8, 21, 1, false); //
  // draw rectangle - includes display.display (so can use false above)
  oledRect();
  //Serial.println(oled_line1_str);
  //Serial.println(oled_line2_str);
  // send message
  strcpy(notification_str, "Restarting - ");
  strcat(notification_str, error_type_display_str); 
  Serial.println(notification_str);
  terminal.println(notification_str);   
  Blynk.logEvent("wm_restarted", String(error_type_display_str)); // log restart in timeline nnnnn
  // log message to SD card - if possible
  updateDate(); 
  updateTime();
  strcpy(error_report_line_2_str, date_str);
  strcat(error_report_line_2_str, "  ");
  strcat(error_report_line_2_str, time_str);
  strcat(error_report_line_2_str, "  ");
  strcat(error_report_line_2_str, error_type_display_str);
  strcpy(error_report_file_name_str, "E");
  strcat(error_report_file_name_str, years_str);
  strcat(error_report_file_name_str, ".RPT");
  myFile = SD.open(error_report_file_name_str, FILE_WRITE);
    if (myFile) {
      myFile.println(error_report_line_2_str);
      myFile.close(); // always close files after writing
    }
  // Note - no error reporting on SD functon above right now
  delay(5000);  // display for at least 5 seconds
  NVIC_SystemReset();
}

// reads file_read_file_name_str into up to 16 strings
// if file does not exists, one is created
void fileRead(){
  //clear all strings in the event there are less than 16 in file
  // or file is not able to be read
  int k;
  for (k = 0; k<=15; k++){
  strcpy(file_read_line_array_str[k], "");
    }
  // if file does not exist return 0 lines read
  if (!SD.exists(file_read_file_name_str)) {
    file_read_number_of_lines_int = 0;
    return;
    }
    // if file does not exists, create one by opening
    //if (!SD.exists(file_read_file_name_str)) {
  //  myFile = SD.open(file_read_file_name_str, FILE_WRITE);  // opening for file write creates a new file
  //  // if the file does not open, report error
  //  if (!myFile){
  //    strcpy(error_type_str, "SD-9");
  //    mkrError();
  //    }
  //  myFile.close();
  // }
  
  // open the file. note that only one file can be open at a time 
  myFile = SD.open(file_read_file_name_str, FILE_READ);
  // if the file does not open, report error
    if (!myFile){
      strcpy(error_type_str, "SD-10");
      mkrError();
    }
  unsigned long file_size_ulong = myFile.size();
  // for testing
  //Serial.print("File size is:  ");
  //Serial.println(file_size_ulong);
  unsigned long file_position_ulong;
  unsigned long new_position_ulong;
  unsigned long line_length_ulong = 55;
  int newline_position_int = 0;
  //select proper page
  // initial reads have not adjustment
  file_size_ulong = file_size_ulong - file_read_page_pointer_ulong;
  // check if remaining file size less than 2100 char
  if (file_size_ulong >= 2100){
    new_position_ulong = file_size_ulong - 2100;
  }
  else {
    // start at beginning of file
    new_position_ulong = 0;
  }
  myFile.seek(new_position_ulong);
  int read_length_int = min(file_size_ulong, 2100);
  myFile.read(file_read_str, read_length_int);
  myFile.close();
  file_read_str[read_length_int + 1] = '\0';
  // for testing
  //Serial.println("file_read_str was as follows:");
  //Serial.print(file_read_str);
  int pos = 0;  // large string position
  int str_num = 0;  // number of strings read
  int str_pos = 0;  // each string position
  myFile.seek(new_position_ulong);
  while (pos <= 2100){  
    file_read_line_read_str[str_pos] = file_read_str[pos];
    
    // for testing
    // characters read at line end are CR, NL.  
    // strings terminated with '\0'
    // '\r' = 13 = 0x0D = CR = carriage return - can use 13
    // '\n' = 10 = 0x0A = LF = line feed or new line -  can use 10
    /*
    if(file_read_line_read_str[str_pos] == '\r') {
      Serial.print (str_pos);
      Serial.println(" carraige return");
      }
    if(file_read_line_read_str[str_pos] == '\n') {
      Serial.print (str_pos); 
      Serial.println(" new line");
      }
    if(file_read_line_read_str[str_pos] == '\0') {
      Serial.print (str_pos); 
      Serial.println(" string null termination");
    }
    */
    
    if (file_read_line_read_str[str_pos] == '\n'){
      file_read_line_read_str[str_pos + 1] = '\0';  // terminate string
      str_pos = -1; // reset string position - will be incremented to 0 below
      str_num++;  //count strings with '/n'
      // advance string positions
      for (k = 15; k >= 1; k--){
        strcpy(file_read_line_array_str[k], file_read_line_array_str[k - 1]);
        }
      strcpy(file_read_line_array_str[0], file_read_line_read_str);  // fill new string
      }   
      if (file_read_str[pos+1] == '\0'){
        goto fileReadreturn;
      } 
    str_pos++;
    pos++;
   }   
  fileReadreturn:
  // for testing
  // print lines read and number of lines
  //Serial.println();
  //for (k = 0; k<=15; k++){
  //Serial.print(file_read_line_array_str[k]);
  //  }
  file_read_number_of_lines_int = min(str_num, 16);
  //Serial.println();
  //Serial.print("Number of lines = ");
  //Serial.println(file_read_number_of_lines_int);
}

// Serial Command functions
// non-blocking serial input routine
void checkSerialcommand(){
  commandInput();
  if (cmd_flag_int == 1){
    Serial.println(cmd_str);
    Serial.println();
    cmd_flag_int = 0;
  // *** jw note - removed bracket so this is now a condition to all of the code below
  //Serial.println(serial_second_line_flag_int);
  //Serial.println(cmd_str);
  if ((serial_second_line_flag_int == 1) && (strcmp(cmd_str, "") != 0)) { // returns 0 if equal
      strcpy(file_read_file_name_str, cmd_str);  
      //Serial.print("File is: ");
      //Serial.println(file_read_file_name_str);  
      serial_second_line_flag_int = 0; // reset file read flag 
      // read first page
      file_read_page_pointer_ulong = 0;
      fileRead();
      // print out lines 
      // Serial.println(); // add line feed
      if (file_read_number_of_lines_int == 0){
        Serial.println("no file");
        }
      else{
        // terminal.println("File contents");
        for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
          Serial.print(file_read_line_array_str[i]); 
          }
        }
      // Serial.println(); 
      if ((file_read_number_of_lines_int < 16) && (file_read_number_of_lines_int > 0)) {
        Serial.println("end of file"); 
        Serial.println();
        }
      return;
    }
   if ((serial_second_line_flag_int == 2) && (strcmp(cmd_str, "") != 0)) { // returns 0 if equal
     // overrides are below for testing and troubleshooting  ttt ***
     // format rtc.setDate(byte day, byte month, byte year)
     // format rtc.setTime(byte hours, byte minutes, byte seconds)
     //rtc.setDate(01, 06, 22);
     //rtc.setTime(23, 59, 45);
     int c_day_int; 
     int c_month_int; 
     int c_year_int; 
     int c_hours_int; 
     int c_minutes_int; 
     int c_seconds_int;
     char c_day_str[4]; 
     char c_month_str[4]; 
     char c_year_str[4]; 
     char c_hours_str[4]; 
     char c_minutes_str[4]; 
     char c_seconds_str[4];
     if (strlen(cmd_str) != 17) { // note- null character ‘\0’ not counted
       Serial.println ("Invalid entry");
       Serial.println();  // add line feed
       serial_second_line_flag_int = 0; // reset file read flag 
       return;
       }
     sscanf(cmd_str, "%d%*c%d%*c%d%*c%d%*c%d%*c%d", &c_year_int, &c_month_int, 
       &c_day_int, &c_hours_int, &c_minutes_int, &c_seconds_int);
     sscanf(cmd_str, "%2s%*c%2s%*c%2s%*c%2s%*c%2s%*c%2s", c_year_str, c_month_str, 
       c_day_str, c_hours_str, c_minutes_str, c_seconds_str);
     Serial.print("Date and time changed to: ");
     Serial.print(c_year_str);
     Serial.print("/");
     Serial.print(c_month_str);
     Serial.print("/");
     Serial.print(c_day_str);
     Serial.print(" ");
     Serial.print(c_hours_str);
     Serial.print(":");
     Serial.print(c_minutes_str);
     Serial.print(":");
     Serial.println(c_seconds_str);
     serial_second_line_flag_int = 0; // reset file read flag 
     // format rtc.setDate(byte day, byte month, byte year)
     // format rtc.setTime(byte hours, byte minutes, byte seconds)
     rtc.setDate(c_day_int, c_month_int, c_year_int);
     rtc.setTime(c_hours_int, c_minutes_int, c_seconds_int);
     // also update prior date used for determining a date change
     sscanf(cmd_str, "%2s%*c%2s%*c%2s", prior_years_str, prior_months_str, prior_days_str);    
     //Serial.print("prior years: ");  // for testing
     //Serial.println(prior_years_str);
     //Serial.print("prior months: ");
     //Serial.println(prior_months_str);
     //Serial.print("prior days: ");
     //Serial.println(prior_days_str);    
     Serial.println();  // add line feed
    
   /*strcpy(prior_days_str, days_str);  ***  all deleted code not needed as log file created when logging on first write
  //Serial.println(prior_days_str);
  //Serial.println(days_str);
  strcpy(prior_months_str, months_str);
  //Serial.println(prior_months_str);
  //Serial.println(months_str);
  strcpy(prior_years_str, years_str);
  //Serial.println(prior_years_str);
  //Serial.println(years_str);
  strcpy(file_name_prefix_old_str, file_name_prefix_new_str);  not needed as not used later
  //Serial.println(file_name_prefix_old_str);
  //Serial.println(file_name_prefix_new_str);
  strcpy(file_name_log_old_str, file_name_log_new_str);  not need as not used later

  
     // update date with results of adjsutment   zzzzz - needed?  or done automatically when logging?
     //  converts to 2 character decimal base - pads leading 0s by adding the 0
     sprintf(years_str, "%02d", years_int); 
     sprintf(months_str, "%02d", months_int);
     sprintf(days_str, "%02d", days_int);
 
     // create file prefix
     strcpy(file_name_prefix_new_str, years_str);
     strcat(file_name_prefix_new_str, "-");
     strcat(file_name_prefix_new_str, months_str);
     strcat(file_name_prefix_new_str, "-");
     strcat(file_name_prefix_new_str, days_str);
    
     // create desired files  
     // create log file
     // values logged every 5 seconds
     strcpy(file_name_log_new_str, file_name_prefix_new_str);
     strcat(file_name_log_new_str, ".LOG"); 
     */
     return;
  }
  
  if (strcmp(cmd_str, "p") == 0) { // read another page of currently selected file
    // check for end of file
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      Serial.println();  // add line feed
      return;
      }
    // calculate new page pointer by summing prior string lengths
    unsigned long sum_string_lengths_ulong = 0;
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      sum_string_lengths_ulong = sum_string_lengths_ulong + strlen(file_read_line_array_str[i]);
      }
    file_read_page_pointer_ulong = file_read_page_pointer_ulong + sum_string_lengths_ulong;
    fileRead();
     // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println();  // add line feed
    }        
  if (strcmp(cmd_str, "cmd") == 0) { // print out list of commands
    Serial.println("c   - clear terminal screen");
    Serial.println("v   - current values");
    Serial.println("r   - current day minutes-on and lowest psi");
    Serial.println("d   - minutes-on for prior days");
    Serial.println("m   - hours-on for each month");
    Serial.println("e   - prior errors and resets"); 
    Serial.println("s   - WiFi signal strength"); 
    Serial.println("t   - rtc and current WiFi times");
    Serial.println("f   - named file read"); 
    Serial.println("0-2 - on-off times for well pump 0, 1 or 2");
    Serial.println("3   - on-off times for city water valve");
    Serial.println("4   - on-off times for pressure tank pump");
    Serial.println("5   - low and high psi");
    Serial.println("6   - combines all of the foregoing");
    Serial.println("dis - toggle oled display on off");
    Serial.println("a   - about file");
    Serial.println("clp - cause low pressure alarm");
    Serial.println("rlp - reset low pressure flag");
    Serial.println("ccw - cause city water notice");
    Serial.println("rcw - reset city water flag");
    Serial.println("rst - reset well monitor");
    Serial.println("t   - old rtc and current wifi times");
    Serial.println("centp - cause ntp error");
    Serial.println("cesd - cause sd errorr");
    Serial.println("wdt - test wdt");
    Serial.println("cdt - change date and time");
    Serial.println("clr - local terminal clear");
    Serial.println("cmd - list available commands");
    Serial.println(); // add line feed
    }
 if (strcmp(cmd_str, "v") == 0) { // return current read values
    strcpy(notification_str, time_str); 
    strcat(notification_str, "  ");
    strcat(notification_str, values_single_line_0to5_str); 
    Serial.println("WM values as of ");
    Serial.println(notification_str);
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "r") == 0) { // return current day's report or part thereof
    Serial.println("calculating ...");
    Serial.println(); // add line feed
    myFile = SD.open(file_name_log_new_str, FILE_READ);
    sumLogFile(); // create report
    myFile.close();
    strcpy(notification_str, date_str);
    strcat(notification_str, "  ");
    strcat(notification_str, values_display_0to2_str);
    strcat(notification_str, "  ");
    strcat(notification_str, values_display_3to5_str); 
    Serial.print("WM minutes-on and lowest psi as of ");
    Serial.println(time_str);
    Serial.println(notification_str);
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "d") == 0) { // return current read values
    // read current D__.RPT file
    strcpy(file_read_file_name_str, "D");
    strcat(file_read_file_name_str, years_str);
    strcat(file_read_file_name_str, ".RPT");
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    if (file_read_number_of_lines_int == 0){
      Serial.println("no file");
      }
    else{
      Serial.println("WM minutes-on and lowest psi");
      for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
        Serial.print(file_read_line_array_str[i]); 
        }
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "m") == 0) { // return current read values
    // read current M.RPT file
    strcpy(file_read_file_name_str, "M.RPT");
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    if (file_read_number_of_lines_int == 0){
      Serial.println("no file");
      }
    else{
      Serial.println("WM hours-on per month");
      for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
        Serial.print(file_read_line_array_str[i]); 
        }
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }  
  if (strcmp(cmd_str, "e") == 0) { 
    // return last resets from SD file
    // log message to SD card - if possible
    updateDate(); 
    strcpy(error_report_file_name_str, "E");
    strcat(error_report_file_name_str, years_str);
    strcat(error_report_file_name_str, ".RPT");
    strcpy(file_read_file_name_str, error_report_file_name_str);
    //Serial.println(file_read_file_name_str);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "f") == 0) { // print file
     Serial.println("Enter file name");
     serial_second_line_flag_int = 1;  // set flag for next line read
     Serial.println(); // add line feed
     }
  if (strcmp(cmd_str, "0") == 0) { 
    // return latest 0- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[0]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }
 if (strcmp(cmd_str, "1") == 0) {
    // return latest 1- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[1]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "2") == 0) {
    // return latest 2- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[2]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }   
  if (strcmp(cmd_str, "3") == 0) { 
    // return latest 3- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[3]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "4") == 0) { 
    // return latest 4- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[4]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }   
  if (strcmp(cmd_str, "5") == 0) { 
    // return latest 5- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[5]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
    Serial.println(); // add line feed
    }    
    if (strcmp(cmd_str, "6") == 0) {
    // return latest 6- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[6]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      Serial.print(file_read_line_array_str[i]); 
      }
     // check for end of file
    if (file_read_number_of_lines_int < 16) {
      Serial.println("end of file"); 
      }
      Serial.println(); // add line feed
    }   
    if (strcmp(cmd_str, "clp") == 0) {  // cause low pressure alarm
      Serial.println("Low pressure alarm triggered");
      Serial.println(); // add line feed
      waterPressureAlarm();
      }    
    if (strcmp(cmd_str, "rlp") == 0) {  // reset low pressure flag
      water_pressure_alarm_flag_int = 0;
      water_pressure_alarm_count_int = 0;
      Serial.println("Low pressure flag reset");
      Serial.println(); // add line feed
      }
    if (strcmp(cmd_str, "ccw") == 0) {  // cause city water notice
      Serial.println("City water notice triggered");
      Serial.println(); // add line feed
      cityWaterNotification(); 
      }
    if (strcmp(cmd_str, "rcw") == 0) {  // reset city water flag
      city_water_notice_flag_int = 0;
      city_water_notice_count_int = 0;
      Serial.println("City water flag reset");
      Serial.println(); // add line feed
      }
    if (strcmp(cmd_str, "dis") == 0) {  // toggle oled on and off
      if (oled_on_int == 1) {
        oled_on_int = 0;
        oled_motion_cnt_int = 30; // eliminate post motion time
        Serial.println("Oled display off");
        Serial.println(); // add line feed
      }
    else {
      oled_on_int = 1;
      oled_motion_cnt_int = 0;  // start post motion time
      Serial.println("Oled display on");
      Serial.println(); // add line feed
      }
    }
  if (strcmp(cmd_str, "a") == 0) {  // about file
    Serial.println(about_0_str);
    Serial.println(about_1_str);
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "rst") == 0) {  // reset mkr
    Serial.println("Device reset through serial terminal");
    strcpy(error_type_str, "ST");  // report type of error
    mkrError();
    }
  if (strcmp(cmd_str, "s") == 0) {  // report wifi signal strength
    rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println(); // add line feed
    }
  if (strcmp(cmd_str, "t") == 0) {  // report old rtc and current wifi times
    //get time data
    //Serial.println(); // add line feed
    updateDate();
    updateTime();
    Serial.print("RTC time:  ");
    Serial.print(date_str);
    Serial.print("  ");
    Serial.println(time_str);
    //get NTP wifi time
    int number_of_tries_NTP = 0;
    do {
      epoch = WiFi.getTime();
      number_of_tries_NTP++;
      }
    while ((epoch == 0) && (number_of_tries_NTP < 6));
    if (number_of_tries_NTP >= 6) {
      strcpy(error_type_str, "NTP-1");
      mkrError();
    }
    else {
      // print out epoch on serial port
      // Serial.print("Epoch received: ");
      // Serial.println(epoch);
      epoch = epoch + (k_GMT * 60 * 60);  // adjust for GMT standard time
      rtc.setEpoch(epoch);
      // read new time
      updateDate();
      updateTime();
      Serial.print("WiFi time: ");
      Serial.print(date_str);
      Serial.print("  ");
      Serial.println(time_str);
      Serial.println(); // add line feed
      } 
   }
  // for testing error reporting
  if (strcmp(cmd_str, "cesd") == 0) {  // cause sd error for testing
    Serial.println("SD Error reset through Blynk terminal");
    strcpy(error_type_str, "SD-11"); // report type of error
    mkrError();
    }
  if (strcmp(cmd_str, "centp") == 0) {  // casue ntp error for testing
    Serial.println("NTP Error reset through Blynk terminal");
    strcpy(error_type_str, "NTP-3"); // report type of error
    mkrError();
    }
  if (strcmp(cmd_str, "wdt") == 0) {  // check wdt function
    unsigned int t;
    Serial.println("\nWatchdog Test - run 18 seconds with a WDTimer.clear()\n");
    for (t = 1; t <= 18; ++t) {
      WDTimer.clear();  // refresh wdt - before it loops
      delay(950);
      Serial.print(t);
      Serial.print(".");
      Blynk.run(); // keep Blynk from timing out
      }
    Serial.println("\n\nWatchdog Test - free run wait for reset at 8 seconds\n");
    for (t = 1; t >= 1; ++t) {
      delay(950);
      Serial.print(t);
      Serial.print(".");
      Blynk.run(); // keep Blynk from timing out
      }
  }   
  if (strcmp(cmd_str, "cdt") == 0) { // change date and time
     Serial.println("Enter date and time (yy/mm/dd hh:mm:ss)");
     serial_second_line_flag_int = 2;  // set flag for next line read
     Serial.println(); // add line feed
     }
  }
}

void commandInput(){ // non-blocking serial input routine
  while (Serial.available() && (cmd_flag_int == 0)) {
    int data = Serial.read();
    if (data == '\b'){// back-space
      if (cmd_length_int) {
        cmd_length_int--;
        }
     }
     else {
       if (data == '\r' || data == '\n') {
         cmd_str[cmd_length_int] = '\0';
         if (cmd_length_int) cmd_flag_int = 1;
         // command received so reset command length
         cmd_length_int = 0;
         }
      else {
       cmd_str[cmd_length_int] = data;
       cmd_length_int ++;
       }
     }
   }
 }


// Blynk functions
// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
// Can do a local terminal clear by typing clr
//
// Blynk set up like this
// Yellow city water led on phone is V3
// Blue LED on phone is V1 - triggered by V8 or V9
// V8 is latching switch - value written to a vaiable that is serial printed 
// V9 is momentary switch - value written to a variable that is serial printed
// V10 is well pressure in lbs 0 to 1023 updated each second

BLYNK_WRITE(V16)
{
  //Serial.println(terminal_second_line_flag_int);
  //Serial.println(param.asStr());
  if ((terminal_second_line_flag_int == 1) && (param.asStr() != "")){
      //&& SD.exists(param.asStr())) {
      //  (strlen(param.asStr()) <= 8)){ // check if in second line read
      strcpy(file_read_file_name_str, param.asStr());  
      // Serial.println(file_read_file_name_str);  
      terminal_second_line_flag_int = 0; // reset file read flag 
      // read first page
      file_read_page_pointer_ulong = 0;
      fileRead();
      // print out lines 
      terminal.println(); // add line feed
      Serial.println(); // add line feed
      if (file_read_number_of_lines_int == 0){
        terminal.println("no file");
        Serial.println("no file");
        }
      else{
        // terminal.println("File contents");
        for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
          terminal.print(file_read_line_array_str[i]);
          Serial.print(file_read_line_array_str[i]); 
          }
        }
      terminal.println();
      terminal.flush();
      Serial.println(); 
      if ((file_read_number_of_lines_int < 16) && (file_read_number_of_lines_int > 0)) {
        terminal.println("end of file");
        terminal.println();
        terminal.flush();
        Serial.println("end of file"); 
        Serial.println();
        }
      return;
    }
  
   if ((terminal_second_line_flag_int == 2) && (strcmp(param.asStr(), "") != 0)) { // returns 0 if equal
     int c_day_int; 
     int c_month_int; 
     int c_year_int; 
     int c_hours_int; 
     int c_minutes_int; 
     int c_seconds_int;
     char c_day_str[4]; 
     char c_month_str[4]; 
     char c_year_str[4]; 
     char c_hours_str[4]; 
     char c_minutes_str[4]; 
     char c_seconds_str[4];
     if (strlen(param.asStr()) != 17) { // note- null character ‘\0’ not counted
       terminal.println();  // add line feed
       terminal.println ("Invalid entry");
       terminal.println();  // add line feed
       terminal_second_line_flag_int = 0; // reset file read flag 
       terminal.flush();
       return;
       }
     sscanf(param.asStr(), "%d%*c%d%*c%d%*c%d%*c%d%*c%d", &c_year_int, &c_month_int, 
       &c_day_int, &c_hours_int, &c_minutes_int, &c_seconds_int);
     sscanf(param.asStr(), "%2s%*c%2s%*c%2s%*c%2s%*c%2s%*c%2s", c_year_str, c_month_str, 
       c_day_str, c_hours_str, c_minutes_str, c_seconds_str);
     terminal.println();  // add line feed
     terminal.print("Date and time changed to: ");
     terminal.print(c_year_str);
     terminal.print("/");
     terminal.print(c_month_str);
     terminal.print("/");
     terminal.print(c_day_str);
     terminal.print(" ");
     terminal.print(c_hours_str);
     terminal.print(":");
     terminal.print(c_minutes_str);
     terminal.print(":");
     terminal.println(c_seconds_str);
     terminal_second_line_flag_int = 0; // reset file read flag 
     // format rtc.setDate(byte day, byte month, byte year)
     // format rtc.setTime(byte hours, byte minutes, byte seconds)
     rtc.setDate(c_day_int, c_month_int, c_year_int);
     rtc.setTime(c_hours_int, c_minutes_int, c_seconds_int);
     sscanf(param.asStr(), "%2s%*c%2s%*c%2s", prior_years_str, prior_months_str, prior_days_str);    
     //terminal.print("prior years: ");  // for testing
     //terminal.println(prior_years_str);
     //terminal.print("prior months: ");
     //terminal.println(prior_months_str);
     //terminal.print("prior days: ");
     //terminal.println(prior_days_str);    
     terminal.println();  // add line feed
     // Ensure everything is sent
     terminal.flush();
     return;
  }
  
  if (strcmp(param.asStr(), "p") == 0) { // read another page of currently selected file
    // check for end of file
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      //Serial.println("end of file"); 
      terminal.println(); // add line feed
      //Serial.println();  // add line feed
      // Ensure everything is sent
      terminal.flush();
      return;
      }
    // calculate new page pointer by summing prior string lengths
    unsigned long sum_string_lengths_ulong = 0;
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      sum_string_lengths_ulong = sum_string_lengths_ulong + strlen(file_read_line_array_str[i]);
      }
    file_read_page_pointer_ulong = file_read_page_pointer_ulong + sum_string_lengths_ulong;
    fileRead();
     // print out lines 
    terminal.println(); // add line feed
    // Serial.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    // Ensure everything is sent
    terminal.flush();
    Serial.println();  // add line feed
    }        
  if (strcmp(param.asStr(), "c") == 0) { // Clear the terminal content    // note - returns 0 if equal
    terminal.clear();  // this is the remote clear.  type clr for a local clear.
    }
  if (strcmp(param.asStr(), "cmd") == 0) { // print out list of commands
    terminal.println("c   - clear terminal screen");
    terminal.println("v   - current values");
    terminal.println("r   - current day minutes-on and lowest psi");
    terminal.println("d   - minutes-on for prior days");
    terminal.println("m   - hours-on for each month");
    terminal.println("pf  - report pressure every second toggle");
    terminal.println("e   - prior errors and resets"); 
    terminal.println("s   - WiFi signal strength"); 
    terminal.println("t   - rtc and current WiFi times");
    terminal.println("f   - named file read"); 
    terminal.println("0-2 - on-off times for well pump 0, 1 or 2");
    terminal.println("3   - on-off times for city water valve");
    terminal.println("4   - on-off times for pressure tank pump");
    terminal.println("5   - low and high psi");
    terminal.println("6   - combines all of the foregoing");
    terminal.println("dis - toggle oled display on off");
    terminal.println("a   - about file");
    terminal.println("clp - cause low pressure alarm");
    terminal.println("rlp - reset low pressure flag");
    terminal.println("ccw - cause city water notice");
    terminal.println("rcw - reset city water flag");
    terminal.println("rst - reset well monitor");
    terminal.println("t   - report old rtc and current wifi times");
    terminal.println("centp - cause ntp error");
    terminal.println("cesd - cause sd errorr");
    terminal.println("wdt - test wdt");
    terminal.println("cdt - change date and time");
    terminal.println("clr - local terminal clear");
    terminal.println("cmd - list available commands");
    terminal.println(); // add line feed
    }
  if (strcmp(param.asStr(), "v") == 0) { // return current read values
    terminal.println(); // add line feed
    strcpy(notification_str, time_str); 
    strcat(notification_str, "  ");
    strcat(notification_str, values_single_line_0to5_str); 
    terminal.println("WM values as of ");
    terminal.println(notification_str);
    terminal.println(); // add line feed
    }
  if (strcmp(param.asStr(), "r") == 0) { // return current day's report or part thereof
    terminal.println(); // add line feed
    terminal.println("calculating ...");
    terminal.println(); // add line feed
    terminal.flush();
    //Blynk.run(); // for Blynk mobile operation  needed?  ***
    myFile = SD.open(file_name_log_new_str, FILE_READ);
    sumLogFile(); // create report
    myFile.close();
    strcpy(notification_str, date_str);
    strcat(notification_str, "  ");
    strcat(notification_str, values_display_0to2_str);
    strcat(notification_str, "  ");
    strcat(notification_str, values_display_3to5_str); 
    terminal.print("WM minutes-on and lowest psi as of ");
    terminal.println(time_str);
    terminal.println(notification_str);
    terminal.println(); // add line feed
    }
  if (strcmp(param.asStr(), "pf") == 0) { // report pressure every 1 sec toggle
    terminal.println(); // add line feed
    if (water_pressure_report_flag != 0) {
      water_pressure_report_flag = 0;
      terminal.println("Fast pressure reporting ended");
      }
    else {
      water_pressure_report_flag = 1;
      terminal.println("Reporting pressure every second for 2 minutes");
      }
    }  
  if (strcmp(param.asStr(), "d") == 0) {
    // return current read values
    // read current D__.RPT file
    strcpy(file_read_file_name_str, "D");
    strcat(file_read_file_name_str, years_str);
    strcat(file_read_file_name_str, ".RPT");
    //Serial.println(file_read_file_name_str);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    if (file_read_number_of_lines_int == 0){
      terminal.println("no file");
      Serial.println("no file");
      }
    else{
      terminal.println("WM minutes-on and lowest psi");
      for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
        terminal.print(file_read_line_array_str[i]);
        Serial.print(file_read_line_array_str[i]); 
        }
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println();  // add line feed
    }
  if (strcmp(param.asStr(), "m") == 0) { 
    // read M.RPT file
    strcpy(file_read_file_name_str, "M.RPT");
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    if (file_read_number_of_lines_int == 0){
      terminal.println("no file");
      Serial.println("no file");
      }
    else{
      terminal.println("WM hours-on per month");
      for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
        terminal.print(file_read_line_array_str[i]);
        Serial.print(file_read_line_array_str[i]); 
        }
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println();  // add line feed
    }
  if (strcmp(param.asStr(), "e") == 0) { 
    // return last resets from SD file
    updateDate(); 
    strcpy(error_report_file_name_str, "E");
    strcat(error_report_file_name_str, years_str);
    strcat(error_report_file_name_str, ".RPT");
    strcpy(file_read_file_name_str, error_report_file_name_str);
    //Serial.println(file_read_file_name_str);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }
  if (strcmp(param.asStr(), "0") == 0) { 
    // return latest 0- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[0]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }
 if (strcmp(param.asStr(), "1") == 0) {
    // return latest 1- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[1]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }
  if (strcmp(param.asStr(), "2") == 0) {
    // return latest 2- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[2]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }   
  if (strcmp(param.asStr(), "3") == 0) { 
    // return latest 3- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[3]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }
  if (strcmp(param.asStr(), "4") == 0) { 
    // return latest 4- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[4]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }   
  if (strcmp(param.asStr(), "5") == 0) { 
    // return latest 5- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[5]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
    terminal.println(); // add line feed
    Serial.println(); // add line feed
    }    
  if (strcmp(param.asStr(), "6") == 0) {
    // return latest 6- file from SD
    strcpy(file_read_file_name_str, annual_on_off_file_name_str[6]);
    // read first page
    file_read_page_pointer_ulong = 0;
    fileRead();
    // print out lines 
    terminal.println(); // add line feed
    for (int i = 0; i <= file_read_number_of_lines_int - 1; i++) {
      terminal.print(file_read_line_array_str[i]);
      Serial.print(file_read_line_array_str[i]); 
      }
     // check for end of file
    if (file_read_number_of_lines_int < 16) {
      terminal.println("end of file");
      Serial.println("end of file"); 
      }
   terminal.println(); // add line feed
   Serial.println(); // add line feed
     }   
   if (strcmp(param.asStr(), "f") == 0) { // print file
     terminal.println(); // add line feed
     terminal.println("Enter file name");
     //Serial.println("Enter file name");
     terminal_second_line_flag_int = 1;  // set flag for next line read
     terminal.println(); // add line feed
     //Serial.println(); // add line feed
    }
  if (strcmp(param.asStr(), "dis") == 0) {  // toggle oled on and off
    terminal.println(); // add line feed
    if (oled_on_int == 1) {
      oled_on_int = 0;
      oled_motion_cnt_int = 30; // eliminate post motion time
      terminal.println("Oled display off");
      // Serial.println("Oled display off");
      terminal.println(); // add line feed
      // Serial.println(); // add line feed
    }
    else {
      oled_on_int = 1;
      oled_motion_cnt_int = 0; // start post motion time
      terminal.println("Oled display on");
      // Serial.println("Oled display on");
      terminal.println(); // add line feed
      // Serial.println(); // add line feed
    }
   }
   if (strcmp(param.asStr(), "a") == 0) {  // about file
     terminal.println(); // add line feed
     terminal.println(about_0_str);
     terminal.println(about_1_str);
     terminal.println(); // add line feed
     }
   if (strcmp(param.asStr(), "clp") == 0) {  // cause low pressure alarm
      terminal.println("Low pressure alarm triggered");
      terminal.println(); // add line feed
      waterPressureAlarm();
      }    
    if (strcmp(param.asStr(), "rlp") == 0) {  // reset low pressure flag
      water_pressure_alarm_flag_int = 0;
      water_pressure_alarm_count_int = 0;
      terminal.println(); // add line feed
      terminal.println("Low pressure flag reset");
      terminal.println(); // add line feed
      }
    if (strcmp(param.asStr(), "ccw") == 0) {  // cause city water notice
      terminal.println("City water notice triggered");
      terminal.println(); // add line feed
      cityWaterNotification(); 
      }
    if (strcmp(param.asStr(), "rcw") == 0) {  // reset city water flag
      city_water_notice_flag_int = 0;
      city_water_notice_count_int = 0;
      terminal.println(); // add line feed
      terminal.println("City water flag reset");
      terminal.println(); // add line feed
      }
  if (strcmp(param.asStr(), "rst") == 0) {  // reset mkr
    terminal.println(); // add line feed
    terminal.println("Device reset through Blynk terminal");
    // report type of error
    strcpy(error_type_str, "BT");
    mkrError();
    }
  if (strcmp(param.asStr(), "s") == 0) {  // report wifi signal strength
    rssi = WiFi.RSSI();
    terminal.println(); // add line feed
    terminal.print("Signal strength (RSSI): ");
    terminal.print(rssi);
    terminal.println(" dBm");
    terminal.println(); // add line feed
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");
    Serial.println(); // add line feed
   }
  if (strcmp(param.asStr(), "t") == 0) {  // report old rtc and current wifi times
    //get time data
    terminal.println(); // add line feed
    updateDate();
    updateTime();
    terminal.print("RTC time:  ");
    terminal.print(date_str);
    terminal.print("  ");
    terminal.println(time_str);
    //get NTP wifi time
    int number_of_tries_NTP = 0;
    do {
      epoch = WiFi.getTime();
      number_of_tries_NTP++;
      }
    while ((epoch == 0) && (number_of_tries_NTP < 6));
    if (number_of_tries_NTP >= 6) {
      strcpy(error_type_str, "NTP-2");
      mkrError();
    }
    else {
      epoch = epoch + (k_GMT * 60 * 60);  // adjsut for GMT standard time
      rtc.setEpoch(epoch);
      // read new time
      updateDate();
      updateTime();
      terminal.print("WiFi time: ");
      terminal.print(date_str);
      terminal.print("  ");
      terminal.println(time_str);
      terminal.println(); // add line feed
      } 
   }
  // for testing error reporting
  if (strcmp(param.asStr(), "cesd") == 0) {  // cause sd error for testing
    terminal.println(); // add line feed
    terminal.println("SD Error reset through Blynk terminal");
    terminal.println(); // add line feed
    // report type of error
    strcpy(error_type_str, "SD-12");
    mkrError();
    }
  if (strcmp(param.asStr(), "centp") == 0) {  // casue ntp error for testing
    terminal.println(); // add line feed
    terminal.println("NTP Error reset through Blynk terminal");
    terminal.println(); // add line feed
    // report type of error
    strcpy(error_type_str, "NTP-3");
    mkrError();
    }
 if (strcmp(param.asStr(), "wdt") == 0) {  // check wdt function
    unsigned int t;
    terminal.println("\nWatchdog Test - run 18 seconds with a WDTimer.clear()\n");
    Serial.println("\nWatchdog Test - run 18 seconds with a WDT.clear()\n");
    for (t = 1; t <= 18; ++t) {
      WDTimer.clear();  // refresh wdt - before it loops
      delay(950);
      terminal.print(t);
      terminal.print(".");
      terminal.flush();
      Blynk.run(); 
      Serial.print(t);
      Serial.print(".");
      }
    terminal.println("\n\nWatchdog Test - free run wait for reset at 8 seconds\n");
    Serial.println("\n\nWatchdog Test - free run wait for reset at 8 seconds\n");
    for (t = 1; t >= 1; ++t) {
      delay(950);
      terminal.print(t);
      terminal.print(".");
      terminal.flush();
      Blynk.run();
      Serial.print(t);
      Serial.print(".");
      }
    }   
  if (strcmp(param.asStr(), "cdt") == 0) { // change date and time
     terminal.println(); // add line feed
     terminal.println("Enter date and time (yy/mm/dd hh:mm:ss)");
     terminal_second_line_flag_int = 2;  // set flag for next line read
     terminal.println(); // add line feed
     }
  
  
  
  
  /* old example code
  // if you type "Marco" into Terminal Widget - it will respond: "Polo:"
  if (String("Marco") == param.asStr()) {
    terminal.println("You said: 'Marco'") ;
    terminal.println("I said: 'Polo'") ;
  } else {
    // Send it back
    terminal.print("You said:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }
  */

// Ensure everything is sent
terminal.flush();
}

// for blinking MKR1010 built in LED indicating link
void blinkOnBoardLED() 
{
if (onBoardLEDValue == 1) {
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  onBoardLEDValue = 0;
  }
else {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  onBoardLEDValue = 1;
  }
}

void swWidget()
{
// use state flags to reduce continuous use of WiFiDrv that may have been resetting mkr
//Serial.println("In swWidget");  // for testing
if (value_sw9_int == 1) {
  if (led1_state_int != 1) {
  // Serial.println("iPhone sw: on");  // for testing
  led1_state_int = 1;
  led1.on();
  WiFiDrv::digitalWrite(27, HIGH); // blue LED on at full brightness
  //Serial.println("In swWidget pos 1");
  // Serial.println("Pin 27 blue LED controlled is: on");
  }
}
else {
  if (led1_state_int != 0) {
  // Serial.println("iPhone sw: off");
  led1_state_int = 0;
  led1.off();  
  WiFiDrv::digitalWrite(27, LOW); // blue LED off 
  //Serial.println("In swWidget pos 2");
  // Serial.println("Pin 27 blue LED controlled is: off");}
  } 
}

if (city_water_notice_flag_int == 0){
  if (led3_state_int == 1) {
    //Serial.println("yellow led off");
    led3_state_int = 0;
    led3.off();
    //Serial.println("In swWidget pos 2-1");
    } 
  }
else {
  if (led3_state_int == 0) {
  //Serial.println("yellow led on");
  led3_state_int = 1;
  led3.on();
  //Serial.println("In swWidget pos 2-2");
    }  
  }

value_sw_4_int = digitalRead(1);  // consider adding debouncer
if (value_sw_4_int == 0){
  if (led4_state_int != 1) {
    // Serial.println("bd sw: on");
    led4_state_int = 1;
    led4.on();
    WiFiDrv::digitalWrite(26, HIGH); // red LED on at full brighness
    //Serial.println("In swWidget pos 3");
    } 
  }
  else {
    if (led4_state_int != 0) {
    // Serial.println("bd sw: off");
    led4_state_int = 0;
    led4.off();
    WiFiDrv::digitalWrite(26, LOW); // red LED off
    //Serial.println("In swWidget pos 4");
    }  
  }
}

// This function will be called every time Switch Widget
// in Blynk app writes values to the Virtual Pin V9
BLYNK_WRITE(V9)
{
value_sw9_int = param.asInt(); // assigning incoming value from pin V9 to a variable
// You can also use:
// String i = param.asStr();
// double d = param.asDouble();
Serial.print("V9 switch value is: ");
Serial.println(value_sw9_int);
if (value_sw9_int == 0) {Blynk.setProperty(V9, "color", "#000000");}  // dark
if (value_sw9_int == 1) {Blynk.setProperty(V9, "color", "#0000c8");}  // blue
}

BLYNK_WRITE(V10)
{
// writes pressure value to display
Blynk.virtualWrite(V10, blynk_pressure_read_int);
}
