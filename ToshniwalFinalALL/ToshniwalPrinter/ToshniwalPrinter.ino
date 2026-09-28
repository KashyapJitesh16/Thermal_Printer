// 29/01/2024 stable tested version
//   arduino ide : 1.8.19
//   esp32 board version : 2.0.10

#include <esp32-hal-psram.h>
#include <PS2Keyboard.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <ADS1220_WE.h>
#include <SPI.h>
#include <SimpleKalmanFilter.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define relay_pin 25

#define TXp2 17
#define RXp2 16

bool auto_mode_on = false;
bool force_on = false;

bool send_temp = false;
unsigned long start_time_1 = 0;

String unique_file_name;

float capsule_gcv = 0.0;
float final_gcv = 0.0;
float calculated_weq = 0.0;

// ==================== GLOBAL RTC MENU VARIABLES ====================
bool in_datetime_menu = false;
int dt_cursor = 0; // 0=Year, 1=Month, 2=Day, 3=Hour, 4=Minute, 5=Save

int edit_year   = 2026;
int edit_month  = 7;
int edit_day    = 24;
int edit_hour   = 12;
int edit_minute = 0;

char datetime_input_buf[5] = "";
int datetime_buf_pos = 0;

#define NUM_DATA_LINES 15          // The number of lines in your data
String dataLines[NUM_DATA_LINES];  // Array to store each line

#define NUM_DATA_LINES1 12           // The number of lines in yrray to store each
String dataLines1[NUM_DATA_LINES1];  // Array to store each l ine

bool error_opening_file = false;  // Flag to indicate if editing mode is active
bool editingValue = false;        // Flag to indicate if editing mode is active
bool editingValue1 = false;       // Flag to indicate if editing mode is active
char enteredValue[13];            // Buffer to store entered values
char enteredValue1[13];           // Buffer to store previously entered values
char enteredValue2[13];           // Buffer to store entered values
char enteredValue3[13];           // Buffer to store previously entered values
char enteredValue4[13];           // Buffer to store entered values
char enteredValue5[13];           // Buffer to store previously entered values
char enteredValue6[13];           // Buffer to store previously entered values
char enteredValue7[13];           // Buffer to store entered values
char enteredValue8[13];           // Buffer to store previously entered values
char enteredValue9[13];           // Buffer to store entered values
char enteredValue10[13];          // Buffer to store previously entered values
char enteredValue11[13];          // Buffer to store previously entered values
char enteredValue12[13];          // Buffer to store previously entered values
char enteredValue13[13];          // Buffer to store entered values
char enteredValue14[13];          // Buffer to store previously entered values
char enteredValue15[13];          // Buffer to store entered values
char enteredValue16[13];          // Buffer to store previously entered values
char enteredValue17[13];          // Buffer to store previously entered values
char enteredValue18[13];          // Buffer to store previously entered values
char enteredValue19[13];          // Buffer to store previously entered values
char enteredValue20[13];          // Buffer to store previously entered values



char setting_Value[13];    // Buffer to store entered values
char setting_Value1[13];   // Buffer to store previously entered values
char setting_Value2[13];   // Buffer to store entered values
char setting_Value3[13];   // Buffer to store previously entered values
char setting_Value4[13];   // Buffer to store entered values
char setting_Value5[13];   // Buffer to store previously entered values
char setting_Value6[13];   // Buffer to store previously entered values
char setting_Value7[13];   // Buffer to store entered values
char setting_Value8[13];   // Buffer to store previously entered values
char setting_Value9[13];   // Buffer to store entered values
char setting_Value10[13];  // Buffer to store previously entered values

char curr_temp1[13];   // Buffer to store previously entered values
char curr_temp2[13];   // Buffer to store entered values
char curr_temp3[13];   // Buffer to store previously entered values
char curr_temp4[13];   // Buffer to store entered values
char curr_temp5[13];   // Buffer to store previously entered values
char curr_temp6[13];   // Buffer to store previously entered values
char curr_temp7[13];   // Buffer to store entered values
char curr_temp8[13];   // Buffer to store previously entered values
char curr_temp9[13];   // Buffer to store entered values
char curr_temp10[13];  // Buffer to store previously entered values
char curr_temp11[13];  // Buffer to store previously entered values
char curr_temp12[13];  // Buffer to store entered values
char curr_temp13[13];  // Buffer to store previously entered values

bool printed = false;
static bool conditionMet = false;
static int targetMinute = -1;

////////-------SD CARD-SECTION-------//////////

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}


#include <FS.h>

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  // Attempt to remove the existing file
  while (fs.exists(path) && !fs.remove(path)) {
    Serial.println("Failed to delete the file, retrying...");
    delay(100);  // Add a short delay before retrying
  }

  // Check if the file was successfully deleted or doesn't exist
  if (!fs.exists(path)) {
    Serial.println("File deleted successfully");

    // Create a new file
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    // Write the new data to the file
    file.print(message);

    // Flush the data to ensure it is written immediately
    file.flush();

    // Close the file
    file.close();

    Serial.println("File written");
  } else {
    Serial.println("Failed to delete the file");
  }
}



void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  } else {
    Serial.println("Failed to open file for reading");
  }


  file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}


bool readFileIntoBuffer(const char *fullPath, char *buffer, int bufferSize) {
  // Open the file
  File myFile = SD.open(fullPath, FILE_READ);  // Open the file for reading
  if (!myFile) {
    Serial.println("Error opening file");
    return false;
  }

  // Check if the file is empty
  if (myFile.size() == 0) {
    Serial.println("File is empty");
    myFile.close();
    return false;
  }

  // Clear the buffer before filling it with file content
  memset(buffer, 0, bufferSize);

  // Read file content into the buffer
  int bytesRead = myFile.readBytes(buffer, bufferSize - 1);  // Read up to bufferSize - 1 characters
  buffer[bytesRead] = '\0';                                  // Null-terminate the buffer

  // Close the file
  myFile.close();

  Serial.printf("Read %d bytes from file\n", bytesRead);
  return true;
}


bool checkAndCreateFile(File dir, const char *path, const char *content) {
  if (SD.exists(path)) {
    Serial.print("File already exists: ");
    Serial.println(path);
    return false;
  } else {
    File file = SD.open(path, FILE_WRITE);
    if (file) {
      file.println(content);
      file.close();
      Serial.print("File created: ");
      Serial.println(path);
      return true;
    } else {
      Serial.print("Error creating file: ");
      Serial.println(path);
      return false;
    }
  }
}

bool checkAndCreateDir(File root, const char *path) {
  if (SD.exists(path)) {
    Serial.print("Directory already exists: ");
    Serial.println(path);
    return false;
  } else {
    if (SD.mkdir(path)) {
      Serial.print("Directory created: ");
      Serial.println(path);
      return true;
    } else {

      Serial.print("Error creating directory: ");
      Serial.println(path);
      return false;
    }
  }
}


String getUniqueFileName(const char *filePath, const char *prefix) {
  // Read the last used number from the file
  int fileNumber = 0;
  File file = SD.open(filePath, FILE_READ);
  if (file) {
    String line = file.readStringUntil('\n');
    fileNumber = line.toInt() + 1;
    file.close();
  }

  // Create and write the new number back to the file
  file = SD.open(filePath, FILE_WRITE);
  if (file) {
    file.println(fileNumber);
    file.close();
  }

  // Format the file number as a 3-digit string with leading zeros
  String fileNumberStr = String(fileNumber);
  fileNumberStr = String("000").substring(0, 3 - fileNumberStr.length()) + fileNumberStr;

  // Create the new file name
  String fullFilePath = prefix + fileNumberStr;

  return fullFilePath;
}




////////-------NTC-ADC-SECTION-------//////////

#define ADS1220_CS_PIN 5    // chip select pin ESP 8266
#define ADS1220_DRDY_PIN 4  // data ready pin   ESP8266
#define ADS1220_MISO_PIN 19
#define ADS1220_MOSI_PIN 23
#define ADS1220_SCK_PIN 18

ADS1220_WE ads2 = ADS1220_WE(ADS1220_CS_PIN, ADS1220_DRDY_PIN);



int test = 9;
float Voltage = 0.0;
int thermistor_25 = 100;
float refCurrent = 0.0001;

const double VCC = 3.32;              // NodeMCU on board 3.3v vcc
const double R2 = 10000;              // 10000 10k ohm series resistor
const double adc_resolution = 32768;  // 10-bit adc

const double A = 0.0007927873429;  // thermistor equation parameters
const double B = 0.0002673184548;
const double C = 0.0000001268821601;

// Kalman filter variables
SimpleKalmanFilter kalmanFilter(0.5, 0.5, 0.8);  // Q = 2, R = 2, initial estimate = 0.1

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

float read_temperature() {
  float result = 0.0;
  long longResult = 0;
  float Vout = 0, Rth = 0, temperature = 0, VCC = 0;
  float z = 0, c = 0, v = 0;

  //  for (int i = 0; i < 20; i++) {
  Vout = ads2.getVoltage_muV();
  z = z + Vout;
  //    delay(10);keyboard
  //  }

  Vout = z / 1000000;
  float tt = mapfloat(Vout, 0, 2.048, 0, 0.05);
  Vout = Vout + tt;

  Rth = Vout / 0.00005;
  temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)), 3))));
  temperature = temperature - 273.15;

  // Apply the Kalman filter to the temperature reading
  float filteredTemperature = kalmanFilter.updateEstimate(temperature);

  Serial.println(filteredTemperature, 4);
  return filteredTemperature;
}

float temperature_now = 25.0;
float previousTemperature = 0.0;
float peak_temperature = 0.0;

////////-------LCD-KEYBOARD-SECTION-------//////////

int a = 0;
int b = 0;
int d = 0;
int s = 0;
int i = 0;
int n = 0;
int valuecount = 0;
const int DataPin = 27;
const int IRQpin = 26;
PS2Keyboard keyboard;

char c;
////////-------rtc-sd_card-SECTION-------//////////

RTC_PCF8563 rtc;
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Set the I2C address and LCD size (20x4)
DateTime lastTime;

unsigned long startTime = 0;
int x = 0;
int y = 0;
int hours_sw = 0;
int minutes_sw = 0;
int seconds_sw = 0;

int durationMinutes = 0;

static unsigned long lastUpdateTime = 0;
static unsigned long lastUpdateTime1 = 0;

bool timer_expired = false;

// Function to format and print the date as "dd/mm/yyyy"
void printFormattedDate(DateTime dateTime) {
  lcd.print(dateTime.day(), DEC);
  lcd.print('/');
  lcd.print(dateTime.month(), DEC);
  lcd.print('/');
  lcd.print(dateTime.year(), DEC);
}


// Function to format and print the time as "hh:mm:ss"
void printFormattedTime(int hours, int minutes, int seconds) {
  if (hours < 10) lcd.print('0');  // Add leading zero for single-digit hours
  lcd.print(hours, DEC);
  lcd.print(':');
  if (minutes < 10) lcd.print('0');  // Add leading zero for single-digit minutes
  lcd.print(minutes, DEC);
  lcd.print(':');
  if (seconds < 10) lcd.print('0');  // Add leading zero for single-digit seconds
  lcd.print(seconds, DEC);
}


// Function to get and update real-time date and time on the LCD
void updateDateTime() {
  DateTime now = rtc.now();
  if (now != lastTime) {  // Check if the time has changed
    lcd.setCursor(0, 0);
    printFormattedDate(now);
    lcd.setCursor(12, 0);
    printFormattedTime(now.hour(), now.minute(), now.second());
    lastTime = now;
  }
}


void updateDateTime_for_auto() {
  DateTime now = rtc.now();
  if (now != lastTime) {  // Check if the time has changed
    lcd.setCursor(0, 0);
    printFormattedDate(now);
    lcd.setCursor(12, 0);
    printFormattedTime(now.hour(), now.minute(), now.second());
    lastTime = now;
    if (send_temp) {
      temperature_now = read_temperature();
      lcd.setCursor(12, 2);
      lcd.print(temperature_now, 4);
    }
  }
}


String getDateTime() {
  // Read the current date and time from the RTC module
  DateTime now = rtc.now();

  // Create a string to store the formatted date and time
  String dateTimeStr = String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

  return dateTimeStr;
}


// Function to create countdown timer using RTC
void rtcTimer(int timeInSeconds) {
  unsigned long startTime = millis();
  DateTime initialTime = rtc.now();

  // Calculate hours, minutes, and seconds from timeInSeconds
  int hours = timeInSeconds / 3600;
  int minutes = (timeInSeconds % 3600) / 60;
  int seconds = timeInSeconds % 60;

  initialTime = initialTime + TimeSpan(0, hours, minutes, seconds);  // Add countdown time to initial time

  bool force_fire = false;

  while (true) {
    DateTime now = rtc.now();
    if (now >= initialTime) {
      lcd.setCursor(1, 2);
      lcd.print("Countdown expired!");
      timer_expired = true;
      break;  // Timer has expired, exit the loop
    }
    char z = keyboard.read();
    if (!force_fire && z == PS2_ENTER) {
      c = PS2_ENTER;
      break;
    }
    if (z == PS2_RIGHTARROW) {
      force_fire = true;
      lcd.setCursor(1, 3);
      lcd.print(" BACK");
      lcd.setCursor(9, 3);
      lcd.print(">FIRE");
    }
    if (z == PS2_LEFTARROW) {
      force_fire = false;
      lcd.setCursor(1, 3);
      lcd.print(">BACK");
      lcd.setCursor(9, 3);
      lcd.print(" FIRE");
    }
    if (z == PS2_ENTER && force_fire) {
      timer_expired = true;
      break;  // Timer has expired, exit the loop
    }
    // Calculate the remaining time
    TimeSpan remainingTime = initialTime - now;
    int remainingHours = remainingTime.hours();
    int remainingMinutes = remainingTime.minutes();
    int remainingSeconds = remainingTime.seconds();

    if ((millis() - startTime) > 1000) {
      startTime = millis();
      lcd.setCursor(6, 2);
      printFormattedTime(remainingHours, remainingMinutes, remainingSeconds);  // Format the timer as "hh:mm:ss"
      temperature_now = read_temperature();
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
    }
  }
}

//void startStopwatchAndUpdateDisplay(int a, int b, int minutes) {
//  x = a;
//  y = b;
//  durationMinutes = minutes;
//  startTime = millis();
//  previousTemperature = read_temperature();
//  while (startTime != 0) {
//    lcd.setCursor(x, y); // Set the cursor to the specified position
//    unsigned long currentTime = millis();
//    unsigned long elapsedTime = currentTime - startTime;
//    hours_sw = (elapsedTime / 3600000) % 60;
//    minutes_sw = (elapsedTime / 60000) % 60;
//    seconds_sw = (elapsedTime / 1000) % 60;
//
//    if (hours_sw < 10) lcd.print("0");
//    lcd.print(hours_sw);
//    lcd.print(":");
//    if (minutes_sw < 10) lcd.print("0");
//    lcd.print(minutes_sw);
//    lcd.print(":");
//    if (seconds_sw < 10) lcd.print("0");
//    lcd.print(seconds_sw);
//
//    lastUpdateTime1 = millis();
//    previousTemperature = temperature_now;
//    temperature_now = read_temperature();
//    if (temperature_now > previousTemperature) {
//      if (peak_temperature < temperature_now) {
//        peak_temperature = temperature_now;
//      }
//    }
//    dtostrf(temperature_now, 6, 4, curr_temp3);
//    lcd.setCursor( 12, 1);
//    lcd.print(curr_temp3);
//
//    if ((millis() - startTime) > 180000) {
//      if ((temperature_now < previousTemperature) || (temperature_now == previousTemperature)) {
//        startTime = 0;
//        dtostrf(peak_temperature, 6, 4, curr_temp3);
//        lcd.setCursor( 12, 1);
//        lcd.print(curr_temp3);
//      }
//    }
//    if (minutes_sw >= durationMinutes) {
//      startTime = 0; // Stop the stopwatch when the duration is reached
//    }
//    delay(1000); // Update the display every second
//  }
//}


void startStopwatchAndUpdateDisplay(int a, int b, int minutes) {
  x = a;
  y = b;
  durationMinutes = minutes;
  startTime = millis();
  unsigned long ST_1 = millis();  // Properly initialize ST_1
  previousTemperature = read_temperature();
  peak_temperature = previousTemperature;
  char z = 0;
  static unsigned long stableStartTime = 0;
  static bool temperatureWasStable = false;

  // Reset for new experiment
  temperatureWasStable = false;
  stableStartTime = 0;

  bool key_pressed = false;
  while (startTime != 0) {

    if (keyboard.available()) {
      z = keyboard.read();
    }

    if (z == PS2_ENTER) {
      z = 0;
      key_pressed = true;
      int g = 1;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("FORCE END EXP. ?");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(0, 3);
      lcd.print(" END");
      delay(100);

      while (1) {
        if (keyboard.available()) {
          z = keyboard.read();
        }

        if (g == 1 && z == PS2_DOWNARROW) {
          g = 2;
          z = 0;  // Clear key buffer
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("FORCE END EXP. ?");
          lcd.setCursor(0, 2);
          lcd.print(" BACK");
          lcd.setCursor(0, 3);
          lcd.print(">END");
          delay(100);
        } else if (g == 2 && z == PS2_UPARROW) {
          g = 1;
          z = 0;  // Clear key buffer
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("FORCE END EXP. ?");
          lcd.setCursor(0, 2);
          lcd.print(">BACK");
          lcd.setCursor(0, 3);
          lcd.print(" END");
          delay(100);
        }

        if (g == 1 && z == PS2_ENTER) {
          g = 0;
          z = 0;  // Clear key buffer
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("INT TEMP = ");
          lcd.setCursor(12, 0);
          lcd.print(previousTemperature, 4);  // Fixed temperature display
          lcd.setCursor(0, 1);
          lcd.print("CUR TEMP = ");
          lcd.setCursor(0, 2);
          lcd.print("TIME LAPSED = ");
          lcd.setCursor(0, 3);
          lcd.print(">FORCE END");
          key_pressed = false;
          break;
        }

        if (g == 2 && z == PS2_ENTER) {
          g = 0;
          z = 0;  // Clear key buffer
          force_on = true;
          break;
        }
      }
    }

    if (force_on) {
      z = 0;  // Clear key buffer
      break;
    }

    if (!key_pressed && millis() - ST_1 > 1000) {
      lcd.setCursor(x, y);
      unsigned long currentTime = millis();
      unsigned long elapsedTime = currentTime - startTime;
      hours_sw = (elapsedTime / 3600000) % 24;  // Fixed hours cycle
      minutes_sw = (elapsedTime / 60000) % 60;
      seconds_sw = (elapsedTime / 1000) % 60;

      if (hours_sw < 10) lcd.print("0");
      lcd.print(hours_sw);
      lcd.print(":");
      if (minutes_sw < 10) lcd.print("0");
      lcd.print(minutes_sw);
      lcd.print(":");
      if (seconds_sw < 10) lcd.print("0");
      lcd.print(seconds_sw);

      lastUpdateTime1 = millis();
      previousTemperature = temperature_now;
      temperature_now = read_temperature();
      if (temperature_now > previousTemperature) {
        if (peak_temperature < temperature_now) {
          peak_temperature = temperature_now;
        }
      }
      dtostrf(temperature_now, 6, 4, curr_temp3);
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);

      if ((millis() - startTime) > 180000) {
        if ((temperature_now <= previousTemperature)) {
          // Temperature is stable
          if (!temperatureWasStable) {
            // First time detecting stable temperature
            stableStartTime = millis();
            temperatureWasStable = true;
          } else {
            // Check if stable for 10 seconds
            if ((millis() - stableStartTime) >= 10000) {
              startTime = 0;
              dtostrf(peak_temperature, 6, 4, curr_temp3);
              lcd.setCursor(12, 1);
              lcd.print(curr_temp3);
              temperatureWasStable = false;  // Reset for next experiment
            }
          }
        } else {
          // Temperature rising again, reset stability tracking
          temperatureWasStable = false;
          stableStartTime = 0;
        }
      }
      if (minutes_sw >= durationMinutes) {
        startTime = 0;  // Stop stopwatch
        break;          // Ensure loop exits
      }

      ST_1 = millis();  // Reset update time
    }
  }
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define LCD_COLUMNS 19
#define LCD_ROWS 4
#define READ_CHUNK_SIZE 80  // Read 80 characters at a time (4 rows × 20 columns)

File currentDir;
String currentPath = "/";
String fileList[50];
bool isDirectory[50];
int fileCount = 0;
int selectedFile = 0;
int displayStart = 0;
String commandBuffer = "";
String getFullPath(String fileName);
bool go_out = false;

String getFullPath(String fileName) {
  // Construct full path based on the current directory
  return currentPath + "/" + fileName;
}

void listDirectory(File dir) {
  fileCount = 0;
  lcd.clear();
  lcd.print("Loading...");
  delay(500);

  while (true) {
    File entry = dir.openNextFile();
    if (!entry)
      break;

    if (fileCount < 50) {
      fileList[fileCount] = String(entry.name());
      isDirectory[fileCount] = entry.isDirectory();
      fileCount++;
    }
    entry.close();
  }

  selectedFile = 0;
  displayStart = 0;

  Serial.println("Directory listed:");
  for (int i = 0; i < fileCount; i++) {
    Serial.printf("%d: %s (%s)\n", i, fileList[i].c_str(), isDirectory[i] ? "Directory" : "File");
  }
}

void displayFiles() {
  lcd.clear();
  for (int i = 0; i < LCD_ROWS; i++) {
    int index = displayStart + i;
    if (index < fileCount) {
      lcd.setCursor(0, i);
      lcd.print((index == selectedFile) ? ">" : " ");

      // Trim long filenames to fit the LCD display
      String displayName = fileList[index];
      if (displayName.length() > LCD_COLUMNS - 1) {
        displayName = displayName.substring(0, LCD_COLUMNS - 1);
      }
      lcd.print(displayName);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//void readAndDisplayFile(String filePath) {
//  Serial.println("opening file : "); Serial.println(filePath.c_str());
//  File file = SD.open(filePath.c_str());
//  if (!file) {
//    lcd.clear();
//    lcd.print("Error opening file");
//    Serial.println("Error opening file");
//    return;
//  }
//
//  Serial.println("\nFile opened. Commands: next, previous, exit");
//
//  int lineCount = 0;
//
//  if (file) {
//    while (file.available() && lineCount < NUM_DATA_LINES) {
//      String line = file.readStringUntil('\n');
//      // Check for an empty line
//      if (!line.isEmpty()) {
//        line.trim(); // Trim in place
//        dataLines[lineCount] = line; // Assign the trimmed line
//        lineCount++;
//      }
//    }
//    file.close();
//    Serial.println("Data read from the file.");
//    // Print each line
//    for (int i = 0; i < NUM_DATA_LINES; i++) {
//      Serial.println(dataLines[i]);
//    }
//  }
//
//  if (filePath.indexOf("/gcv") != -1) {
//    s = 0; c = 0; b = 0; a = 0; i = 7;
//
//    int f = 1;
//    char z;
//    lcd.clear();
//    lcd.setCursor( 0, 0);
//    lcd.print(dataLines[0]);
//    lcd.setCursor(0, 2);
//    lcd.print(dataLines[1]);
//    lcd.setCursor(0, 3);
//    lcd.print(dataLines[2]);
//    delay(100);
//
//    while (1) {
//
//      if (keyboard.available()) {
//        z = keyboard.read();
//      }
//
//      if (f == 1 && z == PS2_DOWNARROW) {
//        f = 2; c = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[3]);
//        lcd.setCursor( 0, 1);
//        lcd.print(dataLines[4]);
//        lcd.setCursor( 0, 2);
//        lcd.print(dataLines[5]);
//        lcd.setCursor(0, 3);
//        lcd.print(dataLines[6]);
//        delay(100);
//      }
//
//      else if (f == 2 && z == PS2_UPARROW) {
//        f = 1; z = 0;
//        lcd.clear();
//        lcd.setCursor( 0, 0);
//        lcd.print(dataLines[0]);
//        lcd.setCursor(0, 2);
//        lcd.print(dataLines[1]);
//        lcd.setCursor(0, 3);
//        lcd.print(dataLines[2]);
//        delay(100);
//      }
//
//      else if (f == 2 && z == PS2_DOWNARROW) {
//        f = 3; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[7]);
//        lcd.setCursor( 0, 1);
//        lcd.print(dataLines[8]);
//        lcd.setCursor( 0, 2);
//        lcd.print(dataLines[9]);
//        lcd.setCursor(0, 3);
//        lcd.print(dataLines[10]);
//        delay(100);
//      }
//
//      else if (f == 3 && z == PS2_UPARROW) {
//        f = 2; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[3]);
//        lcd.setCursor( 0, 1);
//        lcd.print(dataLines[4]);
//        lcd.setCursor( 0, 2);
//        lcd.print(dataLines[5]);
//        lcd.setCursor(0, 3);
//        lcd.print(dataLines[6]);
//        delay(100);
//      }
//
//      else if (f == 3 && z == PS2_DOWNARROW) {
//        f = 4; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[11]);
//        lcd.setCursor( 0, 1);
//        lcd.print(">SAVE & MENU");
//        lcd.setCursor(1, 2);
//        lcd.print("BACK");
//        lcd.setCursor(1, 3);
//        lcd.print("SAVE, PRINT & MENU");
//        delay(100);
//      }
//
//      else if (f == 4 && z == PS2_UPARROW) {
//        f = 3; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[7]);
//        lcd.setCursor( 0, 1);
//        lcd.print(dataLines[8]);
//        lcd.setCursor( 0, 2);
//        lcd.print(dataLines[9]);
//        lcd.setCursor(0, 3);
//        lcd.print(dataLines[10]);
//        delay(100);
//      }
//
//      else if (f == 4 && z == PS2_DOWNARROW) {
//        f = 5; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[11]);
//        lcd.setCursor( 1, 1);
//        lcd.print("SAVE & MENU");
//        lcd.setCursor(0, 2);
//        lcd.print(">BACK");
//        lcd.setCursor(1, 3);
//        lcd.print("SAVE, PRINT & MENU");
//        delay(100);
//      }
//
//      else if  (f == 5 && z == PS2_UPARROW) {
//        f = 4; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[11]);
//        lcd.setCursor( 0, 1);
//        lcd.print(">SAVE & MENU");
//        lcd.setCursor(1, 2);
//        lcd.print("BACK");
//        lcd.setCursor(1, 3);
//        lcd.print("SAVE, PRINT & MENU");
//        delay(100);
//      }
//
//      else if (f == 5 && z == PS2_DOWNARROW) {
//        f = 6; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[11]);
//        lcd.setCursor( 1, 1);
//        lcd.print("SAVE & MENU");
//        lcd.setCursor(1, 2);
//        lcd.print("BACK");
//        lcd.setCursor(0, 3);
//        lcd.print(">SAVE, PRINT & MENU");
//        delay(100);
//      }
//
//      else if (f == 6 && z == PS2_UPARROW) {
//        f = 5; z = 0;
//        lcd.clear();
//        lcd.setCursor(0, 0);
//        lcd.print(dataLines[11]);
//        lcd.setCursor( 1, 1);
//        lcd.print("SAVE & MENU");
//        lcd.setCursor(0, 2);
//        lcd.print(">BACK");
//        lcd.setCursor(1, 3);
//        lcd.print("SAVE, PRINT & MENU");
//        delay(100);
//      }
//
//      else if (f == 5 && z == PS2_ENTER) {
//        f = 0; z = 0;
//        lcd.clear();
//        break;
//      }
//
//      else if (f == 4 && z == PS2_ENTER) {
//        f = 0; z = 0;
//        lcd.clear();
//        break;
//      }
//
//      else if (f == 6 && z == PS2_ENTER) {
//        Serial2.println(" ");
//        Serial2.println("           TOSHNIWAL");
//        Serial2.println(" ");
//        Serial2.println(dataLines[0]);
//        Serial2.println(dataLines[1]);
//        Serial2.println(dataLines[2]);
//        Serial2.println(dataLines[3]);
//        Serial2.println(dataLines[4]);
//        Serial2.println(dataLines[5]);
//        Serial2.println(dataLines[6]);
//        Serial2.println(dataLines[7]);
//        Serial2.println(dataLines[8]);
//        Serial2.println(dataLines[10]);
//        Serial2.println(dataLines[11]);
//        Serial2.println(" ");
//        Serial2.println(" ");
//        Serial2.println(" ");
//        lcd.clear();
//        break;
//      }
//    }
//  } else if (filePath.indexOf("/gcvc") != -1) {
//    while (1) {
//      delay(1000);
//      break;
//    }
//  } else if (filePath.indexOf("/weq") != -1) {
//    while (1) {
//      delay(1000);
//      break;
//    }
//  }
//}

void readAndDisplayFile(String filePath) {
  Serial.println("opening file : ");
  Serial.println(filePath.c_str());
  File file = SD.open(filePath.c_str());
  if (!file) {
    lcd.clear();
    lcd.print("Error opening file");
    Serial.println("Error opening file");
    return;
  }

  Serial.println("\nFile opened. Commands: next, previous, exit");

 int lineCount = 0;

  if (file) {
    while (file.available() && lineCount < NUM_DATA_LINES) {
      // Fast line reading using readBytesUntil (bypasses string timeouts)
      char buffer[64];
      int bytesRead = file.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
      buffer[bytesRead] = '\0'; // Null-terminate string
      
      String line = String(buffer);
      line.trim();
      
      if (line.length() > 0) {
        dataLines[lineCount] = line;
        lineCount++;
      }
    }
    file.close();   
    Serial.println("Data read from the file.");
    // Print each line
    for (int i = 0; i < NUM_DATA_LINES; i++) {
      Serial.println(dataLines[i]);
    }
  }

  if (filePath.indexOf("/gcv/") != -1) {
    s = 0;
    c = 0;
    b = 0;
    a = 0;
    i = 7;

    int f = 1;
    char z;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dataLines[0]);
    lcd.setCursor(0, 1);
    lcd.print(dataLines[1]);
    lcd.setCursor(0, 2);
    lcd.print(dataLines[2]);
    lcd.setCursor(0, 3);
    lcd.print(dataLines[3]);
    delay(100);

    while (1) {

      if (keyboard.available()) {
        z = keyboard.read();
      }

      if (f == 1 && z == PS2_DOWNARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[5]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[6]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[7]);
        delay(100);
      }

      else if (f == 2 && z == PS2_UPARROW) {
        f = 1;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[0]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[1]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[2]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[3]);
        delay(100);
      }

      else if (f == 2 && z == PS2_DOWNARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[8]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[10]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[11]);
        delay(100);
      }

      else if (f == 3 && z == PS2_UPARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[5]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[6]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[7]);
        delay(100);
      }

      else if (f == 3 && z == PS2_DOWNARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 4 && z == PS2_UPARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[8]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[10]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[11]);
        delay(100);
      }

      else if (f == 4 && z == PS2_DOWNARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(0, 2);
        lcd.print(">BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_UPARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_DOWNARROW) {
        f = 6;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(0, 3);
        lcd.print(">SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 6 && z == PS2_UPARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(0, 2);
        lcd.print(">BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 4 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 6 && z == PS2_ENTER) {
        Serial2.println(" ");
        Serial2.println("           TOSHNIWAL");
        Serial2.println(" ");
        Serial2.println(dataLines[0]);
        Serial2.println(dataLines[1]);
        Serial2.println(dataLines[2]);
        Serial2.println(dataLines[3]);
        Serial2.println(dataLines[4]);
        Serial2.println(dataLines[5]);
        Serial2.println(dataLines[6]);
        Serial2.println(dataLines[7]);
        Serial2.println(dataLines[8]);
        Serial2.println(dataLines[9]);
        Serial2.println(dataLines[10]);
        Serial2.println(dataLines[11]);
        Serial2.println(dataLines[12]);
        Serial2.println(" ");
        Serial2.println(" ");
        Serial2.println(" ");
        lcd.clear();
        break;
      }
    }
  } else if (filePath.indexOf("/gcvc/") != -1) {
    s = 0;
    c = 0;
    b = 0;
    a = 0;
    i = 7;

    int f = 1;
    char z;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dataLines[0]);
    lcd.setCursor(0, 1);
    lcd.print(dataLines[1]);
    lcd.setCursor(0, 2);
    lcd.print(dataLines[2]);
    lcd.setCursor(0, 3);
    lcd.print(dataLines[3]);
    delay(100);

    while (1) {

      if (keyboard.available()) {
        z = keyboard.read();
      }

      if (f == 1 && z == PS2_DOWNARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[5]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[6]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[7]);
        delay(100);
      }

      else if (f == 2 && z == PS2_UPARROW) {
        f = 1;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[0]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[1]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[2]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[3]);
        delay(100);
      }

      else if (f == 2 && z == PS2_DOWNARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[8]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[10]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[11]);
        delay(100);
      }

      else if (f == 3 && z == PS2_UPARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[5]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[6]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[7]);
        delay(100);
      }

      else if (f == 3 && z == PS2_DOWNARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[13]);  // New line added
        lcd.setCursor(0, 2);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 3);
        lcd.print("BACK");
        delay(100);
      }

      else if (f == 4 && z == PS2_UPARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[8]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[10]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[11]);
        delay(100);
      }

      else if (f == 4 && z == PS2_DOWNARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[13]);  // New line added
        lcd.setCursor(0, 2);
        lcd.print(">BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_UPARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[13]);  // New line added
        lcd.setCursor(0, 2);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 3);
        lcd.print("BACK");
        delay(100);
      }

      else if (f == 5 && z == PS2_DOWNARROW) {
        f = 6;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[13]);  // New line added
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(0, 3);
        lcd.print(">SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 6 && z == PS2_UPARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[12]);
        lcd.setCursor(0, 1);
        lcd.print(dataLines[13]);  // New line added
        lcd.setCursor(1, 2);
        lcd.print("SAVE & MENU");
        lcd.setCursor(0, 3);
        lcd.print(">BACK");
        delay(100);
      }

      else if (f == 5 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 4 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 6 && z == PS2_ENTER) {
        Serial2.println(" ");
        Serial2.println("           TOSHNIWAL");
        Serial2.println(" ");
        Serial2.println(dataLines[0]);
        Serial2.println(dataLines[1]);
        Serial2.println(dataLines[2]);
        Serial2.println(dataLines[3]);
        Serial2.println(dataLines[4]);
        Serial2.println(dataLines[5]);
        Serial2.println(dataLines[6]);
        Serial2.println(dataLines[7]);
        Serial2.println(dataLines[8]);
        Serial2.println(dataLines[9]);
        Serial2.println(dataLines[10]);
        Serial2.println(dataLines[11]);
        Serial2.println(dataLines[12]);
        Serial2.println(dataLines[13]);
        Serial2.println(" ");
        Serial2.println(" ");
        Serial2.println(" ");
        lcd.clear();
        break;
      }
    }
  } else if (filePath.indexOf("/weq/") != -1) {
    s = 0;
    c = 0;
    b = 0;
    a = 0;
    i = 7;

    int f = 1;
    char z;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dataLines[0]);
    lcd.setCursor(0, 2);
    lcd.print(dataLines[1]);
    lcd.setCursor(0, 3);
    lcd.print(dataLines[2]);
    delay(100);

    while (1) {

      if (keyboard.available()) {
        z = keyboard.read();
      }

      if (f == 1 && z == PS2_DOWNARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[3]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[5]);
        delay(100);
      }

      else if (f == 2 && z == PS2_UPARROW) {
        f = 1;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[0]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[1]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[2]);
        delay(100);
      }

      else if (f == 2 && z == PS2_DOWNARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[6].substring(0, 20));
        lcd.setCursor(0, 1);
        lcd.print(dataLines[6].substring(20));
        lcd.setCursor(0, 2);
        lcd.print(dataLines[7]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[8]);
        delay(100);
      }

      else if (f == 3 && z == PS2_UPARROW) {
        f = 2;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[3]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines[4]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[5]);
        delay(100);
      }

      else if (f == 3 && z == PS2_DOWNARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 1);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 4 && z == PS2_UPARROW) {
        f = 3;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[6].substring(0, 20));
        lcd.setCursor(0, 1);
        lcd.print(dataLines[6].substring(20));
        lcd.setCursor(0, 2);
        lcd.print(dataLines[7]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines[8]);
        delay(100);
      }

      else if (f == 4 && z == PS2_DOWNARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[9]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(0, 2);
        lcd.print(">BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_UPARROW) {
        f = 4;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[9]);
        lcd.setCursor(0, 1);
        lcd.print(">SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_DOWNARROW) {
        f = 6;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[9]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(1, 2);
        lcd.print("BACK");
        lcd.setCursor(0, 3);
        lcd.print(">SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 6 && z == PS2_UPARROW) {
        f = 5;
        z = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines[9]);
        lcd.setCursor(1, 1);
        lcd.print("SAVE & MENU");
        lcd.setCursor(0, 2);
        lcd.print(">BACK");
        lcd.setCursor(1, 3);
        lcd.print("SAVE, PRINT & MENU");
        delay(100);
      }

      else if (f == 5 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 4 && z == PS2_ENTER) {
        f = 0;
        z = 0;
        lcd.clear();
        break;
      }

      else if (f == 6 && z == PS2_ENTER) {
        Serial2.println(" ");
        Serial2.println("           TOSHNIWAL");
        Serial2.println(" ");
        Serial2.println(dataLines[0]);
        Serial2.println(dataLines[1]);
        Serial2.println(dataLines[2]);
        Serial2.println(dataLines[3]);
        Serial2.println(dataLines[4]);
        Serial2.println(dataLines[5]);
        Serial2.println(dataLines[6]);
        Serial2.println(dataLines[7]);
        Serial2.println(dataLines[8]);
        Serial2.println(dataLines[9]);
        Serial2.println(" ");
        Serial2.println(" ");
        Serial2.println(" ");
        lcd.clear();
        break;
      }
    }
  }
}
///////////////////////////////////////////////////////////////////////////////
void handleCommand(char command) {
  switch (command) {
    case PS2_UPARROW:
      if (selectedFile > 0) {
        selectedFile--;
        if (selectedFile < displayStart) {
          displayStart = selectedFile;
        }
        displayFiles();
      }
      break;

    case PS2_DOWNARROW:
      if (selectedFile < fileCount - 1) {
        selectedFile++;
        if (selectedFile >= displayStart + LCD_ROWS) {
          displayStart = selectedFile - LCD_ROWS + 1;
        }
        displayFiles();
      }
      break;

    case PS2_BACKSPACE:
      // Prevent navigation to parent directory
      Serial.println("Navigation restricted to current directory");
      lcd.clear();
      lcd.print("please wait !");
      delay(1000);
      go_out = true;
      break;

    case PS2_ENTER:
      if (selectedFile >= 0 && selectedFile < fileCount) {
        String selectedPath = getFullPath(fileList[selectedFile]);
        if (!isDirectory[selectedFile]) {  // Only allow file access
          Serial.print("Opening file: ");
          Serial.println(selectedPath);
          readAndDisplayFile(selectedPath);
          displayFiles();
        } else {
          Serial.println("Navigation restricted to current directory");
          lcd.clear();
          lcd.print("Restricted Access");
          delay(1000);
          displayFiles();
        }
      }
      break;
  }

  // Debug output
  Serial.print("Selected: ");
  Serial.println(selectedFile);
}

void returnToSettingsDateTime() {
  in_datetime_menu = false;
  datetime_buf_pos = 0;
  datetime_input_buf[0] = '\0';
  
  // Return to Settings menu screen with cursor pointing at DATE/TIME (state s = 555)
  s = 555;
  a = 0; b = 0; c = 0;
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("ADD ON GCVS");
  lcd.setCursor(0, 1);
  lcd.print(">DATE/TIME");
  lcd.setCursor(1, 2);
  lcd.print("BACK");
}

void displayDateTimeMenu() {
  lcd.clear();
  char buf[21];

  switch (dt_cursor) {
    case 0: // YEAR
      lcd.setCursor(0, 0);
      lcd.print("ENTER YEAR (YYYY):");
      lcd.setCursor(6, 1);
      snprintf(buf, sizeof(buf), "[ %04d ]", edit_year);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:NEXT");
      break;

    case 1: // MONTH
      lcd.setCursor(0, 0);
      lcd.print("ENTER MONTH (1-12):");
      lcd.setCursor(6, 1);
      snprintf(buf, sizeof(buf), "[ %02d ]", edit_month);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:NEXT");
      break;

    case 2: // DAY
      lcd.setCursor(0, 0);
      lcd.print("ENTER DAY (1-31):");
      lcd.setCursor(6, 1);
      snprintf(buf, sizeof(buf), "[ %02d ]", edit_day);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:NEXT");
      break;

    case 3: // HOUR
      lcd.setCursor(0, 0);
      lcd.print("ENTER HOUR (0-23):");
      lcd.setCursor(6, 1);
      snprintf(buf, sizeof(buf), "[ %02d ]", edit_hour);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:NEXT");
      break;

    case 4: // MINUTE
      lcd.setCursor(0, 0);
      lcd.print("ENTER MINUTE (0-59):");
      lcd.setCursor(6, 1);
      snprintf(buf, sizeof(buf), "[ %02d ]", edit_minute);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:NEXT");
      break;

    case 5: // SAVE CONFIRMATION
      lcd.setCursor(2, 0);
      lcd.print("SAVE DATE & TIME?");
      lcd.setCursor(1, 1);
      snprintf(buf, sizeof(buf), "%04d/%02d/%02d  %02d:%02d", 
               edit_year, edit_month, edit_day, edit_hour, edit_minute);
      lcd.print(buf);
      lcd.setCursor(0, 3);
      lcd.print("ESC:EXIT|ENT:SAVE");
      break;
  }

  // Display the active buffer overlay on Line 2 if user is typing
  if (datetime_buf_pos > 0 && dt_cursor < 5) {
    lcd.setCursor(6, 2);
    lcd.print("IN: ");
    lcd.print(datetime_input_buf);
  }
}

void handleDateTimeInput(char key) {
  // 1. ESC key pressed -> Exit setup and return to Settings Menu
  if (key == PS2_ESC || key == 27) {
    returnToSettingsDateTime();
    return;
  }

  // 2. BACKSPACE / DELETE key pressed -> Clear input value for current step only
  if (key == PS2_BACKSPACE || key == PS2_DELETE || key == '\b' || key == 0x7F) {
    datetime_buf_pos = 0;
    datetime_input_buf[0] = '\0';
    displayDateTimeMenu();
    return;
  }

  // 3. ENTER key pressed -> Validate typed input and advance to next step
  if (key == PS2_ENTER || key == '\n' || key == '\r') {
    if (dt_cursor == 5) {
      // Final confirmation: Save to RTC module hardware
      rtc.adjust(DateTime(edit_year, edit_month, edit_day, edit_hour, edit_minute, 0));
      
      lcd.clear();
      lcd.setCursor(2, 1);
      lcd.print("RTC TIME SAVED!");
      delay(1200);

      returnToSettingsDateTime();
      return;
    } else {
      // Update value if digits were typed in buffer
      if (datetime_buf_pos > 0) {
        datetime_input_buf[datetime_buf_pos] = '\0';
        int val = atoi(datetime_input_buf);

        if (dt_cursor == 0 && val >= 2020 && val <= 2099) edit_year = val;
        else if (dt_cursor == 1 && val >= 1 && val <= 12) edit_month = val;
        else if (dt_cursor == 2 && val >= 1 && val <= 31) edit_day = val;
        else if (dt_cursor == 3 && val >= 0 && val <= 23) edit_hour = val;
        else if (dt_cursor == 4 && val >= 0 && val <= 59) edit_minute = val;

        datetime_buf_pos = 0;
        datetime_input_buf[0] = '\0';
      }

      dt_cursor++; // Next field
      displayDateTimeMenu();
    }
  } 
  // 4. DIGIT entry (0-9)
  else if (key >= '0' && key <= '9') {
    if (datetime_buf_pos < 4 && dt_cursor < 5) {
      datetime_input_buf[datetime_buf_pos++] = key;
      datetime_input_buf[datetime_buf_pos] = '\0';
      displayDateTimeMenu();
    }
  }
}

////////------- setup ---------////////

void setup() {
  Serial.begin(1200);
  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush(); 
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC is NOT initialized,set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();
  delay(500);

  Wire.begin();
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  lcd.clear();
  lcd.init();
  lcd.backlight();
  lcd.begin(20, 4);
  lcd.setCursor(4, 1);  // move cursor the first row
  lcd.print("Initializing");

  pinMode(relay_pin, OUTPUT);

  Serial.println("Shifting to ADS1220");
  if (!ads2.init()) {
    Serial.println("ADS1220 is not connected!");
    test = 9;
  } else {
    ads2.setCompareChannels(ADS1220_MUX_1_AVSS);
    ads2.setOperatingMode(ADS1220_NORMAL_MODE);
    ads2.setFIRFilter(ADS1220_50HZ_60HZ);
    ads2.setIdacCurrent(ADS1220_IDAC_50_MU_A);
    ads2.setIdac1Routing(ADS1220_IDAC_AIN2);
    ads2.bypassPGA(true);
  }
  delay(3000);
  SD.begin(32);
  delay(100);
  if (!SD.begin(32)) {
    Serial.println("Card Mount Failed");
    return;
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

  File root = SD.open("/");

  // Check and create files in the "settings" directory
  checkAndCreateDir(root, "/settings");

  checkAndCreateFile(root, "/settings/countdown_timer.txt", "100");
  checkAndCreateFile(root, "/settings/weq.txt", "200");
  checkAndCreateFile(root, "/settings/benzoic_acid_gcv.txt", "300");
  checkAndCreateFile(root, "/settings/cotton_gcv.txt", "1");
  checkAndCreateFile(root, "/settings/nichrome_wire_gcv.txt", "2");
  checkAndCreateFile(root, "/settings/capsule_gcv.txt", "3");

  checkAndCreateDir(root, "/settings/admin");

  checkAndCreateDir(root, "/settings/admin/gcv_index");
  checkAndCreateFile(root, "/settings/admin/gcv_index/gcv_aut_index.txt", "0");
  checkAndCreateFile(root, "/settings/admin/gcv_index/gcv_man_index.txt", "0");

  checkAndCreateDir(root, "/settings/admin/gcvc_index");
  checkAndCreateFile(root, "/settings/admin/gcvc_index/gcvc_aut_index.txt", "0");
  checkAndCreateFile(root, "/settings/admin/gcvc_index/gcvc_man_index.txt", "0");

  checkAndCreateDir(root, "/settings/admin/weq_index");
  checkAndCreateFile(root, "/settings/admin/weq_index/weq_aut_index.txt", "0");
  checkAndCreateFile(root, "/settings/admin/weq_index/weq_man_index.txt", "0");

  // Check and create directories
  checkAndCreateDir(root, "/gcv");
  checkAndCreateDir(root, "/gcv/gcv_auto");
  checkAndCreateDir(root, "/gcv/gcv_manual");
  checkAndCreateDir(root, "/gcvc");
  checkAndCreateDir(root, "/gcvc/gcvc_auto");
  checkAndCreateDir(root, "/gcvc/gcvc_manual");
  checkAndCreateDir(root, "/weq");
  checkAndCreateDir(root, "/weq/weq_auto");
  checkAndCreateDir(root, "/weq/weq_manual");

  readFileIntoBuffer("/settings/countdown_timer.txt", setting_Value, 13);
  readFileIntoBuffer("/settings/weq.txt", setting_Value1, 13);
  readFileIntoBuffer("/settings/benzoic_acid_gcv.txt", setting_Value2, 13);
  readFileIntoBuffer("/settings/cotton_gcv.txt", setting_Value5, 13);
  readFileIntoBuffer("/settings/nichrome_wire_gcv.txt", setting_Value6, 13);
  readFileIntoBuffer("/settings/capsule_gcv.txt", setting_Value7, 13);

  lcd.clear();
  
  pinMode(DataPin, INPUT_PULLUP);
  pinMode(IRQpin, INPUT_PULLUP);
  delay(1000);

  keyboard.begin(DataPin, IRQpin);

  lcd.setCursor(3, 1);  // move cursor the first row
  lcd.print("CALORIFIC METER");
  lcd.setCursor(6, 2);
  lcd.print("TOSHNIWAL");
  delay(3000);
  lcd.clear();
  a = 1;
  lcd.setCursor(4, 0);
  lcd.print("SELECT MODE");
  lcd.setCursor(0, 2);
  lcd.print(">AUTO");
  lcd.setCursor(1, 3);
  lcd.print("MANUAL");
  start_time_1 = millis();

  auto_mode_on = true;
  send_temp = true;
}


void loop() {
  if (Serial.available() > 0) {
    String receivedData = Serial.readStringUntil('\n');
    receivedData.trim();
    Serial.print("received data: ");
    Serial.println(receivedData);
    if (receivedData == "check_status") {
      delay(10);
      Serial.println("Connected4");
      auto_mode_on = true;
    }
  }

  if ((a == 19 || a == 20 || a == 21 || a == 22 || a == 23 || a == 24 || a == 33 || a == 34) && (millis() - start_time_1 > 1800)) {
    temperature_now = read_temperature();
    lcd.setCursor(12, 0);
    lcd.print(temperature_now, 4);
    start_time_1 = millis();
  }

  if ((b == 7 || b == 8 || b == 9 || b == 10 || b == 11 || b == 12 || b == 13 || b == 20 || b == 21) && (millis() - start_time_1 > 1800)) {
    temperature_now = read_temperature();
    lcd.setCursor(12, 0);
    lcd.print(temperature_now, 4);
    start_time_1 = millis();
  }

  if ((d == 19 || d == 20 || d == 21 || d == 22 || d == 23 || d == 24 || d == 33 || d == 34) && (millis() - start_time_1 > 1800)) {
    temperature_now = read_temperature();
    lcd.setCursor(12, 0);
    lcd.print(temperature_now, 4);
    start_time_1 = millis();
  }

  if (keyboard.available() || auto_mode_on) {
    // read the next key
    c = keyboard.read();

    if (in_datetime_menu) {
      handleDateTimeInput(c);
      return; // Skip normal menu handling
    }

    // IF USER SELECTS DATE/TIME FROM SETTINGS MENU:
    // (Replace '4' with whichever key/option corresponds to Date/Time in your settings)
    // if (in_settings_menu && c == '4') { 
    //   // Pre-fill current RTC values so user edits existing time
    //   DateTime now = rtc.now();
    //   edit_year   = now.year();
    //   edit_month  = now.month();
    //   edit_day    = now.day();
    //   edit_hour   = now.hour();
    //   edit_minute = now.minute();
      
    //   dt_cursor = 0;
    //   datetime_buf_pos = 0;
    //   in_datetime_menu = true;
    //   displayDateTimeMenu();
    //   return;
    // }

    if (s == 1 && c == '5') { 
      DateTime now = rtc.now();
      edit_year   = now.year();
      edit_month  = now.month();
      edit_day    = now.day();
      edit_hour   = now.hour();
      edit_minute = now.minute();
      
      dt_cursor = 0;
      datetime_buf_pos = 0;
      in_datetime_menu = true;
      displayDateTimeMenu();
      return;
    }

    if (a == 1 && c == PS2_DOWNARROW) {
      a = 2;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("SELECT MODE");
      lcd.setCursor(1, 2);
      lcd.print("AUTO");
      lcd.setCursor(0, 3);
      lcd.print(">MANUAL");
      delay(100);
    }

    else if (a == 2 && c == PS2_UPARROW) {
      a = 1;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("SELECT MODE");
      lcd.setCursor(0, 2);
      lcd.print(">AUTO");
      lcd.setCursor(1, 3);
      lcd.print("MANUAL");
      delay(100);
    }

    else if (a == 1 && c == PS2_ENTER) {
      a = 3;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("TEMPERATURE :");
      lcd.setCursor(13, 3);
      lcd.print(">BACK");
      send_temp = true;
      while (1) {
        updateDateTime_for_auto();
        char z = keyboard.read();
        if (z == PS2_ENTER) {
          break;
        }
      }
      send_temp = false;
      c = PS2_ENTER;
    }

    if (auto_mode_on) {
      a = 3;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("TEMPERATURE :");
      lcd.setCursor(13, 3);
      lcd.print(">BACK");
      while (1) {
        updateDateTime_for_auto();

        if (Serial.available() > 0) {
          // Read the incoming data as a string
          String receivedData = Serial.readStringUntil('\n');
          receivedData.trim();
          Serial.print("received data: ");
          Serial.println(receivedData);
          if (receivedData == "ts") {
            send_temp = true;
          } else if (receivedData == "st") {
            send_temp = false;
          } else if (receivedData == "fire") {
            temperature_now = read_temperature();
            digitalWrite(relay_pin, HIGH);
            delay(2000);
            digitalWrite(relay_pin, LOW);
          } else if (receivedData == "check_status") {
            delay(10);
            Serial.println("Connected4");
          }
        }
        char z = keyboard.read();
        if (z == PS2_ENTER) {
          break;
        }
      }
      auto_mode_on = false;
      c = PS2_ENTER;
    }

    else if (a == 3 && c == PS2_ENTER) {
      a = 1;
      c = 0;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("SELECT MODE");
      lcd.setCursor(0, 2);
      lcd.print(">AUTO");
      lcd.setCursor(1, 3);
      lcd.print("MANUAL");
      delay(100);
    }

    else if (a == 2 && c == PS2_ENTER) {

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 4 && c == PS2_DOWNARROW) {
      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 5 && c == PS2_DOWNARROW) {
      a = 201;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    else if (a == 201 && c == PS2_DOWNARROW) {
      a = 6;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">SETTINGS");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      delay(100);
    }

    else if (a == 6 && c == PS2_DOWNARROW) {
      a = 200;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("SETTINGS");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      delay(100);
    }

    // REVERSE MODE
    else if (a == 200 && c == PS2_UPARROW) {
      a = 6;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">SETTINGS");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      delay(100);
    }

    else if (a == 6 && c == PS2_UPARROW) {
      a = 201;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    else if (a == 201 && c == PS2_UPARROW) {
      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 5 && c == PS2_UPARROW) {
      a = 4;
      c = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 200 && c == PS2_ENTER) {
      a = 2;
      c = 0;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("SELECT MODE");
      lcd.setCursor(1, 2);
      lcd.print("AUTO");
      lcd.setCursor(0, 3);
      lcd.print(">MANUAL");
      delay(100);
    }

    //////____________________________GCV  SECTION____________________________//////

    //////______________forward direction______________//////

    if (a == 5 && c == PS2_ENTER) {
      a = 7;
      c = 0;
      b = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME ");
      if (strcmp(enteredValue, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 7 && c == PS2_LEFTARROW) {
      a = 8;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 8 && c == PS2_RIGHTARROW) {
      a = 7;
      c = 0;
      valuecount = 0;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((a == 7 || a == 8) && ('a' <= c && c <= 'z') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((a == 7 || a == 8) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue, "") != 0)) {
      memset(enteredValue, 0, sizeof(enteredValue));
      a = 7;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (a == 7 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue, "") != 0)) {
      a = 9;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      if (strcmp(enteredValue1, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue1);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 9 && c == PS2_LEFTARROW) {
      a = 10;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (a == 10 && c == PS2_RIGHTARROW) {
      a = 9;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((a == 9 || a == 10) && ('0' <= c && c <= '9') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue1[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((a == 9 || a == 10) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue1, "") != 0)) {
      memset(enteredValue1, 0, sizeof(enteredValue1));
      a = 9;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (a == 9 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue1, "") != 0)) {
      a = 11;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue2, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue2);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 11 && c == PS2_LEFTARROW) {
      a = 12;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 12 && c == PS2_RIGHTARROW) {
      a = 11;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((a == 11 || a == 12) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue2[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((a == 11 || a == 12) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue2, "") != 0)) {
      a = 11;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue2, 0, sizeof(enteredValue2));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (a == 11 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue2, "") != 0)) {
      a = 13;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue3, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue3);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 13 && c == PS2_LEFTARROW) {
      a = 14;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 14 && c == PS2_RIGHTARROW) {
      a = 13;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((a == 13 || a == 14) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue3[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((a == 13 || a == 14) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue3, "") != 0)) {
      a = 13;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue3, 0, sizeof(enteredValue3));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (a == 13 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue3, "") != 0)) {
      a = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      Serial.println(a);
      Serial.println(enteredValue4);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue4, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue4);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 15 && c == PS2_LEFTARROW) {
      a = 16;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 16 && c == PS2_RIGHTARROW) {
      a = 15;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((a == 15 || a == 16) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue4[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      Serial.println(a);
      Serial.println(enteredValue4);
      delay(100);
    }

    if ((a == 15 || a == 16) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue4, "") != 0)) {
      a = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      Serial.println(a);
      Serial.println(enteredValue4);
      memset(enteredValue4, 0, sizeof(enteredValue4));
      Serial.println(a);
      Serial.println(enteredValue4);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }


    if (a == 15 && (valuecount > 0 || strcmp(enteredValue4, "") != 0) && c == PS2_ENTER) {
      temperature_now = read_temperature();
      a = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (a == 19 && c == PS2_UPARROW) {
      a = 20;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(">");
      lcd.setCursor(5, 2);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 20 && c == PS2_DOWNARROW) {
      a = 19;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(" ");
      lcd.setCursor(5, 2);
      lcd.print(">");
      delay(100);
    }

    else if (a == 19 && c == PS2_DOWNARROW) {
      a = 21;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
      delay(100);
    }

    else if (a == 21 && c == PS2_UPARROW) {
      a = 19;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
      delay(100);
    }


    //////______________auto fire mode______________//////

    else if (a == 20 && c == PS2_ENTER) {  // auto fire
      a = 22;
      c = 0;
      i = 6;
      readFileIntoBuffer("/settings/countdown_timer.txt", enteredValue6, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      if (strcmp(enteredValue6, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue6);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 22 && c == PS2_LEFTARROW) {
      a = 23;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 23 && c == PS2_RIGHTARROW) {
      a = 22;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((a == 22 || a == 23) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue6[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }

    if ((a == 22 || a == 23) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue6, "") != 0)) {
      a = 22;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue6, 0, sizeof(enteredValue6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (a == 22 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue6, "") != 0)) {
      a = 24;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp1);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp1);
      lcd.setCursor(2, 1);
      lcd.print("COUNTDOWN TIMER");
      int t1 = atof(enteredValue6);
      lcd.setCursor(1, 3);
      lcd.print(">BACK");
      lcd.setCursor(10, 3);
      lcd.print("FIRE");
      rtcTimer(t1);
      delay(100);
    }

    if (a == 24 && timer_expired == true) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      a = 26;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP =");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (a == 26 && force_on == true) {
      force_on = false;

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (a == 26 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        a = 261;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        a = 262;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (a == 262 && c == PS2_ENTER) {
      a = 263;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (a == 263 && c == PS2_ENTER) {
      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (a == 261 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);
      dtostrf((y2 - y1), 6, 4, enteredValue7);

      // Convert the character arrays to numeric types (e.g., float or double)
      float settingValue1Float = atof(setting_Value1);
      float enteredValue7Float = atof(enteredValue7);
      float settingValue5Float = atof(setting_Value5);
      float settingValue6Float = atof(setting_Value6);
      float settingValue7Float = atof(setting_Value7);
      float enteredValue2Float = atof(enteredValue2);
      float enteredValue4Float = atof(enteredValue4);
      float enteredValue3Float = atof(enteredValue3);

      Serial.print("weq : ");
      Serial.println(settingValue1Float, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue7Float, 4);
      Serial.print("cotton gcv : ");
      Serial.println(settingValue5Float, 4);
      Serial.print("cotton weight : ");
      Serial.println(enteredValue3Float, 4);
      Serial.print("wire gcv : ");
      Serial.println(settingValue6Float, 4);
      Serial.print("wire length : ");
      Serial.println(enteredValue4Float, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue2Float, 4);

      final_gcv = (((settingValue1Float * enteredValue7Float) - ((settingValue5Float * enteredValue3Float) + (settingValue6Float * enteredValue4Float))) / enteredValue2Float);
      Serial.print("final_gcv : ");
      Serial.println(final_gcv, 4);

      a = 27;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcv_index/gcv_aut_index.txt", "aut");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue);
      delay(100);
    }


    else if (a == 27 && c == PS2_DOWNARROW) {
      a = 28;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue1);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue2);
      delay(100);
    }

    else if (a == 28 && c == PS2_UPARROW) {
      a = 27;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcv_index/gcv_aut_index.txt", "aut");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue);
      delay(100);
    }

    else if (a == 28 && c == PS2_DOWNARROW) {
      a = 29;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue3);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue4);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (a == 29 && c == PS2_UPARROW) {
      a = 28;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue1);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue2);
      delay(100);
    }

    else if (a == 29 && c == PS2_DOWNARROW) {
      a = 30;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue7);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }

    else if (a == 30 && c == PS2_UPARROW) {
      a = 29;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue3);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue4);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (a == 30 && c == PS2_DOWNARROW) {
      a = 31;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("GCV = ");
      lcd.setCursor(7, 0);
      lcd.print(final_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (a == 31 && c == PS2_DOWNARROW) {
      a = 32;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("GCV = ");
      lcd.setCursor(7, 0);
      lcd.print(final_gcv, 4);
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE & MENU");
      delay(100);
    }

    else if (a == 32 && c == PS2_UPARROW) {
      a = 31;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("GCV = ");
      lcd.setCursor(7, 0);
      lcd.print(final_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (a == 31 && c == PS2_UPARROW) {
      a = 30;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue7);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }


    else if (a == 31 && c == PS2_ENTER) {
      File root = SD.open("/");

      checkAndCreateFile(root, ("/gcv/gcv_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 13                // The number of lines in your data
      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue1);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue2);
      dataLines[5] = "THREAD WT: " + String(enteredValue3);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue4);
      dataLines[7] = "WEQ: " + String(setting_Value1);
      dataLines[8] = "INT TEMP: " + String(curr_temp2);
      dataLines[9] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[10] = "DELTA TEMP: " + String(enteredValue7);
      dataLines[11] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[12] = "FINAL GCV : " + String(final_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcv/gcv_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("     TOSHNIWAL     ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 32 && c == PS2_ENTER) {
      File root = SD.open("/");
      checkAndCreateFile(root, ("/gcv/gcv_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);

#define NUM_DATA_LINES 13  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue1);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue2);
      dataLines[5] = "THREAD WT: " + String(enteredValue3);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue4);
      dataLines[7] = "WEQ: " + String(setting_Value1);
      dataLines[8] = "INT TEMP: " + String(curr_temp2);
      dataLines[9] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[10] = "DELTA TEMP: " + String(enteredValue7);
      dataLines[11] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[12] = "FINAL GCV : " + String(final_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcv/gcv_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }


    //////______________manual fire mode______________//////

    else if (a == 19 && c == PS2_ENTER) {  // auto fire
      a = 33;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp3);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp3);
      lcd.setCursor(5, 2);
      lcd.print(">FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (a == 33 && c == PS2_DOWNARROW) {
      a = 34;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
      delay(100);
    }

    else if (a == 34 && c == PS2_UPARROW) {
      a = 33;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
      delay(100);
    }

    if (a == 33 && c == PS2_ENTER) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      a = 35;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP=");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (a == 35 && force_on == true) {
      force_on = false;

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (a == 35 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        a = 351;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        a = 352;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (a == 352 && c == PS2_ENTER) {
      a = 353;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (a == 353 && c == PS2_ENTER) {
      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (a == 351 && c == PS2_ENTER) {
      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);
      dtostrf((y2 - y1), 6, 4, enteredValue7);

      // Convert the character arrays to numeric types (e.g., float or double)
      float settingValue1Float = atof(setting_Value1);
      float enteredValue7Float = atof(enteredValue7);
      float settingValue5Float = atof(setting_Value5);
      float settingValue6Float = atof(setting_Value6);
      float settingValue7Float = atof(setting_Value7);
      float enteredValue2Float = atof(enteredValue2);
      float enteredValue5Float = atof(enteredValue5);
      float enteredValue4Float = atof(enteredValue4);
      float enteredValue3Float = atof(enteredValue3);

      Serial.print("weq : ");
      Serial.println(settingValue1Float, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue7Float, 4);
      Serial.print("cotton gcv : ");
      Serial.println(settingValue5Float, 4);
      Serial.print("cotton weight : ");
      Serial.println(enteredValue3Float, 4);
      Serial.print("wire gcv : ");
      Serial.println(settingValue6Float, 4);
      Serial.print("wire length : ");
      Serial.println(enteredValue4Float, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue2Float, 4);
      final_gcv = (((settingValue1Float * enteredValue7Float) - ((settingValue5Float * enteredValue3Float) + (settingValue6Float * enteredValue4Float))) / enteredValue2Float);
      Serial.print("final_gcv : ");
      Serial.println(final_gcv, 4);

      a = 37;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcv_index/gcv_man_index.txt", "man");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue);
      delay(100);
    }

    else if (a == 37 && c == PS2_DOWNARROW) {

      a = 38;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue1);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue2);
      delay(100);
    }

    else if (a == 38 && c == PS2_UPARROW) {

      a = 37;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue);
      delay(100);
    }

    else if (a == 38 && c == PS2_DOWNARROW) {
      a = 39;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue3);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue4);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (a == 39 && c == PS2_UPARROW) {
      a = 38;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue1);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue2);
      delay(100);
    }

    else if (a == 39 && c == PS2_DOWNARROW) {
      a = 40;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue7);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }

    else if (a == 40 && c == PS2_UPARROW) {
      a = 39;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue3);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue4);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (a == 40 && c == PS2_DOWNARROW) {
      a = 41;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("GCV:");
      lcd.setCursor(5, 0);
      lcd.print(final_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (a == 41 && c == PS2_UPARROW) {
      a = 40;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue7);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    } else if (a == 41 && c == PS2_DOWNARROW) {
      a = 42;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
      delay(100);
    }

    else if (a == 42 && c == PS2_UPARROW) {
      a = 41;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (a == 41 && c == PS2_ENTER) {

      File root = SD.open("/");

      checkAndCreateFile(root, ("/gcv/gcv_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 13                // The number of lines in your data
      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue1);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue2);
      dataLines[5] = "THREAD WT: " + String(enteredValue3);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue4);
      dataLines[7] = "WEQ: " + String(setting_Value1);
      dataLines[8] = "INT TEMP: " + String(curr_temp2);
      dataLines[9] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[10] = "DELTA TEMP: " + String(enteredValue7);
      dataLines[11] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[12] = "FINAL GCV : " + String(final_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcv/gcv_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("     TOSHNIWAL     ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }



    else if (a == 42 && c == PS2_ENTER) {

      File root = SD.open("/");
      checkAndCreateFile(root, ("/gcv/gcv_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 12  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue1);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue2);
      dataLines[5] = "CAPSULE WEIGHT: " + String(enteredValue5);
      dataLines[6] = "WEQ: " + String(setting_Value1);
      dataLines[7] = "INT TEMP: " + String(curr_temp2);
      dataLines[8] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[9] = "DELTA TEMP: " + String(enteredValue7);
      dataLines[10] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[11] = "CAPSULE GCV : " + String(final_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcv/gcv_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }


    //////______________reverse direction______________//////

    else if (a == 8 && c == PS2_ENTER) {

      memset(enteredValue, 0, sizeof(enteredValue));
      memset(enteredValue1, 0, sizeof(enteredValue));
      memset(enteredValue2, 0, sizeof(enteredValue));
      memset(enteredValue3, 0, sizeof(enteredValue));
      memset(enteredValue4, 0, sizeof(enteredValue));
      memset(enteredValue5, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));
      memset(enteredValue7, 0, sizeof(enteredValue));

      a = 5;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(0, 2);
      lcd.print(">GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (a == 10 && c == PS2_ENTER) {
      a = 7;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME");
      lcd.setCursor(i, 1);
      lcd.print(enteredValue);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (a == 12 && c == PS2_ENTER) {
      a = 9;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      lcd.setCursor(i, 1);
      lcd.print(enteredValue1);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (a == 14 && c == PS2_ENTER) {
      a = 11;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue2);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (a == 16 && c == PS2_ENTER) {
      a = 13;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue3);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (a == 21 && c == PS2_ENTER) {
      a = 15;
      c = 0;
      i = 6;
      Serial.println(a);
      Serial.println(enteredValue4);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue4);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (a == 23 && c == PS2_ENTER) {
      a = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
    }

    else if (a == 24 && c == PS2_ENTER) {
      a = 22;
      c = 0;
      readFileIntoBuffer("/settings/countdown_timer.txt", enteredValue6, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      if (strcmp(enteredValue6, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue6);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (a == 34 && c == PS2_ENTER) {
      a = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }


    //////____________________________GCVC  SECTION____________________________//////
    //////_____________________________________________________________________//////

    //////______________forward direction______________//////

    if (a == 201 && c == PS2_ENTER) {
      a = 0;
      d = 7;
      c = 0;
      b = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME ");
      if (strcmp(enteredValue13, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue13);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 7 && c == PS2_LEFTARROW) {
      d = 8;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 8 && c == PS2_RIGHTARROW) {
      d = 7;
      c = 0;
      valuecount = 0;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((d == 7 || d == 8) && ('a' <= c && c <= 'z') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue13[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((d == 7 || d == 8) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue13, "") != 0)) {
      memset(enteredValue13, 0, sizeof(enteredValue13));
      d = 7;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (d == 7 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue13, "") != 0)) {
      d = 9;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      if (strcmp(enteredValue14, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue14);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 9 && c == PS2_LEFTARROW) {
      d = 10;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (d == 10 && c == PS2_RIGHTARROW) {
      d = 9;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((d == 9 || d == 10) && ('0' <= c && c <= '9') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue14[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((d == 9 || d == 10) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue14, "") != 0)) {
      memset(enteredValue14, 0, sizeof(enteredValue14));
      d = 9;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (d == 9 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue14, "") != 0)) {
      d = 11;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue15, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue15);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 11 && c == PS2_LEFTARROW) {
      d = 12;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 12 && c == PS2_RIGHTARROW) {
      d = 11;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((d == 11 || d == 12) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue15[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((d == 11 || d == 12) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue15, "") != 0)) {
      d = 11;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue15, 0, sizeof(enteredValue15));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (d == 11 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue15, "") != 0)) {
      d = 13;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue16, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue16);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 13 && c == PS2_LEFTARROW) {
      d = 14;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 14 && c == PS2_RIGHTARROW) {
      d = 13;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((d == 13 || d == 14) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue16[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((d == 13 || d == 14) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue16, "") != 0)) {
      d = 13;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue16, 0, sizeof(enteredValue16));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (d == 13 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue16, "") != 0)) {
      d = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue17, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue17);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 15 && c == PS2_LEFTARROW) {
      d = 16;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 16 && c == PS2_RIGHTARROW) {
      d = 15;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((d == 15 || d == 16) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue17[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      Serial.println(a);
      Serial.println(enteredValue17);
      delay(100);
    }

    if ((d == 15 || d == 16) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue17, "") != 0)) {
      d = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      Serial.println(a);
      Serial.println(enteredValue17);
      memset(enteredValue17, 0, sizeof(enteredValue17));
      Serial.println(a);
      Serial.println(enteredValue17);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    //////////////////////////////////////////////////////////
    if (d == 15 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue17, "") != 0)) {
      d = 17;
      c = 0;
      i = 6;
      valuecount = 0;
      Serial.println(a);
      Serial.println(enteredValue18);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CAPSULE WEIGHT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      if (strcmp(enteredValue18, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(enteredValue18);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 17 && c == PS2_LEFTARROW) {
      d = 18;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 18 && c == PS2_RIGHTARROW) {
      d = 17;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((d == 17 || d == 18) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      enteredValue18[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((d == 17 || d == 18) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue18, "") != 0)) {
      d = 17;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue18, 0, sizeof(enteredValue18));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CAPSULE WEIGHT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }
    //////////////////////////////////////////////////////////

    if (d == 17 && (valuecount > 0 || strcmp(enteredValue18, "") != 0) && c == PS2_ENTER) {
      temperature_now = read_temperature();
      d = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (d == 19 && c == PS2_UPARROW) {
      d = 20;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(">");
      lcd.setCursor(5, 2);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 20 && c == PS2_DOWNARROW) {
      d = 19;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(" ");
      lcd.setCursor(5, 2);
      lcd.print(">");
      delay(100);
    }

    else if (d == 19 && c == PS2_DOWNARROW) {
      d = 21;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
      delay(100);
    }

    else if (d == 21 && c == PS2_UPARROW) {
      d = 19;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
      delay(100);
    }


    //////______________auto fire mode______________//////

    else if (d == 20 && c == PS2_ENTER) {  // auto fire
      d = 22;
      c = 0;
      i = 6;
      readFileIntoBuffer("/settings/countdown_timer.txt", enteredValue19, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      if (strcmp(enteredValue19, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue19);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 22 && c == PS2_LEFTARROW) {
      d = 23;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 23 && c == PS2_RIGHTARROW) {
      d = 22;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((d == 22 || d == 23) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue19[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }

    if ((d == 22 || d == 23) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue19, "") != 0)) {
      d = 22;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue19, 0, sizeof(enteredValue19));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (d == 22 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue19, "") != 0)) {
      d = 24;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp1);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp1);
      lcd.setCursor(2, 1);
      lcd.print("COUNTDOWN TIMER");
      int t1 = atof(enteredValue19);
      lcd.setCursor(1, 3);
      lcd.print(">BACK");
      lcd.setCursor(10, 3);
      lcd.print("FIRE");
      rtcTimer(t1);
      delay(100);
    }

    if (d == 24 && timer_expired == true) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      d = 26;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP =");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (d == 26 && force_on == true) {
      force_on = false;

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    if (d == 26 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        d = 261;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        d = 262;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (d == 262 && c == PS2_ENTER) {
      d = 263;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (d == 263 && c == PS2_ENTER) {
      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    if (d == 261 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);
      dtostrf((y2 - y1), 6, 4, enteredValue20);

      // Convert the character arrays to numeric types (e.g., float or double)
      float settingValue1Float = atof(setting_Value1);
      float enteredValue20Float = atof(enteredValue20);
      float settingValue5Float = atof(setting_Value5);
      float settingValue6Float = atof(setting_Value6);
      float settingValue7Float = atof(setting_Value7);
      float enteredValue15Float = atof(enteredValue15);
      float enteredValue16Float = atof(enteredValue16);
      float enteredValue17Float = atof(enteredValue17);
      float enteredValue18Float = atof(enteredValue18);

      Serial.print("weq : ");
      Serial.println(settingValue1Float, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue20Float, 4);
      Serial.print("cotton gcv : ");
      Serial.println(settingValue5Float, 4);
      Serial.print("cotton weight : ");
      Serial.println(enteredValue16Float, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue15Float, 4);
      Serial.print("wire gcv : ");
      Serial.println(settingValue6Float, 4);
      Serial.print("wire length : ");
      Serial.println(enteredValue17Float, 4);
      Serial.print("capsule weight : ");
      Serial.println(enteredValue18Float, 4);

      capsule_gcv = ((settingValue1Float * enteredValue20Float) - ((enteredValue16Float * settingValue5Float) + (enteredValue17Float * settingValue6Float) + (settingValue7Float * enteredValue18Float))) / (enteredValue15Float);
      Serial.print("capsule gcv");
      Serial.println(capsule_gcv, 4);

      d = 27;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcvc_index/gcvc_aut_index.txt", "aut");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue13);
      delay(100);
    }


    else if (d == 27 && c == PS2_DOWNARROW) {
      d = 28;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue14);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue15);
      delay(100);
    }

    else if (d == 28 && c == PS2_UPARROW) {
      d = 27;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcvc_index/gcvc_aut_index.txt", "aut");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue13);
      delay(100);
    }

    else if (d == 28 && c == PS2_DOWNARROW) {
      d = 29;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue16);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue17);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (d == 29 && c == PS2_UPARROW) {
      d = 28;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue14);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue15);
      delay(100);
    }

    else if (d == 29 && c == PS2_DOWNARROW) {
      d = 30;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue20);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }

    else if (d == 30 && c == PS2_UPARROW) {
      d = 29;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue16);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue17);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (d == 30 && c == PS2_DOWNARROW) {
      d = 31;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CAP. WEIGHT:");
      lcd.setCursor(13, 0);
      lcd.print(enteredValue18);
      lcd.setCursor(1, 1);
      lcd.print("GCV:");
      lcd.setCursor(5, 1);
      lcd.print(capsule_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (d == 31 && c == PS2_DOWNARROW) {
      d = 32;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CAP. WEIGHT:");
      lcd.setCursor(13, 0);
      lcd.print(enteredValue18);
      lcd.setCursor(1, 1);
      lcd.print("GCV:");
      lcd.setCursor(5, 1);
      lcd.print(capsule_gcv, 4);
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE & MENU");
      delay(100);
    }

    else if (d == 32 && c == PS2_UPARROW) {
      d = 31;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CAP. WEIGHT:");
      lcd.setCursor(13, 0);
      lcd.print(enteredValue18);
      lcd.setCursor(1, 1);
      lcd.print("GCV:");
      lcd.setCursor(5, 1);
      lcd.print(capsule_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (d == 31 && c == PS2_UPARROW) {
      d = 30;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue20);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }


    else if (d == 31 && c == PS2_ENTER) {
      File root = SD.open("/");

      checkAndCreateFile(root, ("/gcvc/gcvc_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);

#define NUM_DATA_LINES 14  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];   // Array to store each line
      String dataLines1[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue13);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue14);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue15);
      dataLines[5] = "THREAD WT: " + String(enteredValue16);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue17);
      dataLines[7] = "capsule WEIGHT: " + String(enteredValue18);
      dataLines[8] = "WEQ: " + String(setting_Value1);
      dataLines[9] = "INT TEMP: " + String(curr_temp2);
      dataLines[10] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[11] = "DELTA TEMP: " + String(enteredValue20);
      dataLines[12] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[13] = "CAPSULE GCV: " + String(capsule_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcv/gcv_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("     TOSHNIWAL     ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(dataLines[13]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    else if (d == 32 && c == PS2_ENTER) {
      File root = SD.open("/");
      checkAndCreateFile(root, ("/gcvc/gcvc_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);

#define NUM_DATA_LINES 14  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];   // Array to store each line
      String dataLines1[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue13);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue14);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue15);
      dataLines[5] = "THREAD WT: " + String(enteredValue16);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue17);
      dataLines[7] = "capsule WEIGHT: " + String(enteredValue18);
      dataLines[8] = "WEQ: " + String(setting_Value1);
      dataLines[9] = "INT TEMP: " + String(curr_temp2);
      dataLines[10] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[11] = "DELTA TEMP: " + String(enteredValue20);
      dataLines[12] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[13] = "CAPSULE GCV : " + String(capsule_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcvc/gcvc_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }


    //////______________manual fire mode______________//////

    else if (d == 19 && c == PS2_ENTER) {  // auto fire
      d = 33;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp3);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp3);
      lcd.setCursor(5, 2);
      lcd.print(">FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (d == 33 && c == PS2_DOWNARROW) {
      d = 34;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
      delay(100);
    }

    else if (d == 34 && c == PS2_UPARROW) {
      d = 33;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
      delay(100);
    }

    if (d == 33 && c == PS2_ENTER) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      d = 35;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP=");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (d == 35 && force_on == true) {
      force_on = false;

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    if (d == 35 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        d = 351;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        d = 352;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (d == 352 && c == PS2_ENTER) {
      d = 353;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (d == 353 && c == PS2_ENTER) {
      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    if (d == 351 && c == PS2_ENTER) {
      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);
      dtostrf((y2 - y1), 6, 4, enteredValue20);
      // Convert the character arrays to numeric types (e.g., float or double)
      float settingValue1Float = atof(setting_Value1);
      float enteredValue20Float = atof(enteredValue20);
      float settingValue5Float = atof(setting_Value5);
      float settingValue6Float = atof(setting_Value6);
      float settingValue7Float = atof(setting_Value7);
      float enteredValue15Float = atof(enteredValue15);
      float enteredValue18Float = atof(enteredValue5);
      float enteredValue17Float = atof(enteredValue17);
      float enteredValue16Float = atof(enteredValue16);

      Serial.print("weq : ");
      Serial.println(settingValue1Float, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue20Float, 4);
      Serial.print("cotton gcv : ");
      Serial.println(settingValue5Float, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue15Float, 4);
      Serial.print("cotton weight : ");
      Serial.println(enteredValue16Float, 4);
      Serial.print("wire gcv : ");
      Serial.println(settingValue6Float, 4);
      Serial.print("wire length : ");
      Serial.println(enteredValue17Float, 4);
      Serial.print("capsule weight : ");
      Serial.println(enteredValue18Float, 4);
      capsule_gcv = ((settingValue1Float * enteredValue20Float) - ((enteredValue16Float * settingValue5Float) + (enteredValue17Float * settingValue6Float))) / (enteredValue15Float);
      Serial.print("capsule gcv : ");
      Serial.println(capsule_gcv, 4);

      d = 37;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/gcvc_index/gcvc_man_index.txt", "man");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue);
      delay(100);
    }

    else if (d == 37 && c == PS2_DOWNARROW) {
      d = 38;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue14);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue15);
      delay(100);
    }

    else if (d == 38 && c == PS2_UPARROW) {
      d = 37;
      c = 0;
      lcd.clear();
      updateDateTime();
      lcd.setCursor(0, 1);
      lcd.print("EXP NO:");
      lcd.setCursor(9, 1);
      lcd.print(unique_file_name);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE NAME :");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue13);
      delay(100);
    }

    else if (d == 38 && c == PS2_DOWNARROW) {
      d = 39;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue16);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue17);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (d == 39 && c == PS2_UPARROW) {
      d = 38;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SAMPLE BATCH");
      lcd.setCursor(4, 1);
      lcd.print(enteredValue14);
      lcd.setCursor(0, 2);
      lcd.print("SAMPLE WEIGHT");
      lcd.setCursor(4, 3);
      lcd.print(enteredValue15);
      delay(100);
    }

    else if (d == 39 && c == PS2_DOWNARROW) {
      d = 40;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue20);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }

    else if (d == 40 && c == PS2_UPARROW) {
      d = 39;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("THREAD WT:");
      lcd.setCursor(15, 1);
      lcd.print(enteredValue16);
      lcd.setCursor(0, 2);
      lcd.print("FUSE WIRE WT:");
      lcd.setCursor(15, 2);
      lcd.print(enteredValue17);
      lcd.setCursor(0, 3);
      lcd.print("WEQ:");
      lcd.setCursor(5, 3);
      lcd.print(setting_Value1);
      delay(100);
    }

    else if (d == 40 && c == PS2_DOWNARROW) {
      d = 41;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("CAP. WEIGHT:");
      lcd.setCursor(13, 0);
      lcd.print(enteredValue18);
      lcd.setCursor(1, 1);
      lcd.print("GCV:");
      lcd.setCursor(5, 1);
      lcd.print(capsule_gcv, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (d == 41 && c == PS2_UPARROW) {
      d = 40;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 2);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue20);
      lcd.setCursor(0, 3);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 3);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      delay(100);
    }

    else if (d == 41 && c == PS2_DOWNARROW) {
      d = 42;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
      delay(100);
    }

    else if (d == 42 && c == PS2_UPARROW) {
      d = 41;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (d == 41 && c == PS2_ENTER) {
      File root = SD.open("/");

      checkAndCreateFile(root, ("/gcvc/gcvc_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 14                // The number of lines in your data
      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue13);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue14);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue15);
      dataLines[5] = "THREAD WT: " + String(enteredValue16);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue17);
      dataLines[7] = "capsule WEIGHT: " + String(enteredValue18);
      dataLines[8] = "WEQ: " + String(setting_Value1);
      dataLines[9] = "INT TEMP: " + String(curr_temp2);
      dataLines[10] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[11] = "DELTA TEMP: " + String(enteredValue20);
      dataLines[12] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[13] = "CAPSULE GCV : " + String(capsule_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcvc/gcvc_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("     TOSHNIWAL     ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(dataLines[13]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }



    else if (d == 42 && c == PS2_ENTER) {

      File root = SD.open("/");

      checkAndCreateFile(root, ("/gcvc/gcvc_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 14                // The number of lines in your data
      String dataLines[NUM_DATA_LINES];  // Array to store each line

      // Assign each line of data to a specelse ific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "SAMPLE NAME: " + String(enteredValue13);
      dataLines[3] = "SAMPLE BATCH: " + String(enteredValue14);
      dataLines[4] = "SAMPLE WEIGHT: " + String(enteredValue15);
      dataLines[5] = "THREAD WT: " + String(enteredValue16);
      dataLines[6] = "FUSE WIRE WT: " + String(enteredValue17);
      dataLines[7] = "CAPSULE WEIGHT: " + String(enteredValue18);
      dataLines[8] = "WEQ: " + String(setting_Value1);
      dataLines[9] = "INT TEMP: " + String(curr_temp2);
      dataLines[10] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[11] = "DELTA TEMP: " + String(enteredValue20);
      dataLines[12] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[13] = "CAPSULE GCV : " + String(capsule_gcv);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      // If you want to write the lines to a file
      File dataFile = SD.open("/gcvc/gcvc_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);
      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }


    //////______________reverse direction______________//////

    else if (d == 8 && c == PS2_ENTER) {

      memset(enteredValue13, 0, sizeof(enteredValue));
      memset(enteredValue14, 0, sizeof(enteredValue));
      memset(enteredValue15, 0, sizeof(enteredValue));
      memset(enteredValue16, 0, sizeof(enteredValue));
      memset(enteredValue17, 0, sizeof(enteredValue));
      memset(enteredValue18, 0, sizeof(enteredValue));
      memset(enteredValue19, 0, sizeof(enteredValue));
      memset(enteredValue20, 0, sizeof(enteredValue));

      a = 201;
      c = 0;
      b = 0;
      d = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(0, 3);
      lcd.print(">GCVC");
      delay(100);
    }

    else if (d == 10 && c == PS2_ENTER) {
      d = 7;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER SAMPLE NAME");
      lcd.setCursor(i, 1);
      lcd.print(enteredValue13);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 12 && c == PS2_ENTER) {
      d = 9;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" ENTER BATCH NUMBER");
      lcd.setCursor(i, 1);
      lcd.print(enteredValue14);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 14 && c == PS2_ENTER) {
      d = 11;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER SAMPLE WEIGHT");
      lcd.setCursor(16, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue15);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 16 && c == PS2_ENTER) {
      d = 13;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(i, 1);
      lcd.print(enteredValue16);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 18 && c == PS2_ENTER) {
      d = 15;
      c = 0;
      i = 6;
      Serial.println(a);
      Serial.println(enteredValue17);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue17);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 21 && c == PS2_ENTER) {
      d = 17;
      c = 0;
      i = 6;
      Serial.println(a);
      Serial.println(enteredValue18);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CAPSULE WEIGHT");
      lcd.setCursor(17, 1);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.setCursor(i, 1);
      lcd.print(enteredValue18);
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    else if (d == 23 && c == PS2_ENTER) {
      d = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
    }

    else if (d == 24 && c == PS2_ENTER) {
      d = 22;
      c = 0;
      readFileIntoBuffer("/settings/countdown_timer.txt", enteredValue19, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      if (strcmp(enteredValue19, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue19);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (d == 34 && c == PS2_ENTER) {
      d = 19;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }



    //////____________________________WEQ  SECTION____________________________//////

    //////______________forward direction______________//////


    if (a == 4 && c == PS2_ENTER) {
      b = 1;
      c = 0;
      i = 6;
      a = 0;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER BENZOIC ACID");
      lcd.setCursor(0, 1);
      lcd.print("WEIGHT :");
      if (strcmp(enteredValue8, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue8);
      }
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 1 && c == PS2_LEFTARROW) {
      b = 2;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (b == 2 && c == PS2_RIGHTARROW) {
      b = 1;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((b == 1 || b == 2) && ((c >= '0' && c <= '9') || c == '.') && i <= 13) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue8[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }

    if ((b == 1 || b == 2) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue8, "") != 0)) {
      memset(enteredValue8, 0, sizeof(enteredValue8));
      b = 1;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER BENZOIC ACID");
      lcd.setCursor(0, 1);
      lcd.print("WEIGHT :");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (b == 1 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue8, "") != 0)) {
      b = 3;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      if (strcmp(enteredValue9, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue9);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 3 && c == PS2_LEFTARROW) {
      b = 4;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (b == 4 && c == PS2_RIGHTARROW) {
      b = 3;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      valuecount++;
    }

    if ((b == 3 || b == 4) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue9[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }

    if ((b == 3 || b == 4) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue9, "") != 0)) {
      b = 3;
      c = 0;
      i = 6;
      memset(enteredValue9, 0, sizeof(enteredValue9));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (b == 3 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue9, "") != 0)) {
      b = 5;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      if (strcmp(enteredValue10, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue10);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 5 && c == PS2_LEFTARROW) {
      b = 6;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (b == 6 && c == PS2_RIGHTARROW) {
      b = 5;
      c = 0;
      valuecount = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((b == 5 || b == 6) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue10[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }


    if ((b == 5 || b == 6) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue10, "") != 0)) {
      b = 5;
      c = 0;
      i = 6;
      memset(enteredValue10, 0, sizeof(enteredValue10));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (b == 5 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue10, "") != 0)) {
      b = 7;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(5, 1);
      lcd.print(">AUTO FIRE");
      lcd.setCursor(6, 2);
      lcd.print("MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
    }

    else if (b == 7 && c == PS2_DOWNARROW) {
      b = 8;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(" ");
      lcd.setCursor(5, 2);
      lcd.print(">");
    }

    else if (b == 8 && c == PS2_DOWNARROW) {
      b = 9;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
    }

    else if (b == 9 && c == PS2_UPARROW) {
      b = 8;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
    }

    else if (b == 8 && c == PS2_UPARROW) {
      b = 7;
      c = 0;
      lcd.setCursor(5, 1);
      lcd.print(">");
      lcd.setCursor(5, 2);
      lcd.print(" ");
    }

    //////// AUTO FIRE MODE ////////////

    else if (b == 7 && c == PS2_ENTER) {
      b = 10;
      c = 0;
      i = 6;
      readFileIntoBuffer("/settings/countdown_timer.txt", enteredValue6, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      if (strcmp(enteredValue6, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue6);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 10 && c == PS2_LEFTARROW) {
      b = 11;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
      delay(100);
    }

    else if (b == 11 && c == PS2_RIGHTARROW) {
      b = 10;
      c = 0;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
      delay(100);
    }

    if ((b == 10 || b == 11) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 2);
      lcd.print(c);
      enteredValue6[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 2);
      valuecount++;
    }

    if ((b == 10 || b == 11) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue6, "") != 0)) {
      b = 10;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue6, 0, sizeof(enteredValue6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (b == 10 && c == PS2_ENTER && (valuecount > 0 || strcmp(enteredValue6, "") != 0)) {
      b = 12;
      c = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp1);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp1);
      lcd.setCursor(2, 1);
      lcd.print("COUNTDOWN TIMER");
      int t1 = atof(enteredValue6);
      lcd.setCursor(1, 3);
      lcd.print(">BACK");
      lcd.setCursor(10, 3);
      lcd.print("FIRE");
      rtcTimer(t1);
      Serial.println("timer_expired = true");
      delay(100);
    }

    if ((b == 12) && c == PS2_ENTER) {
      temperature_now = read_temperature();
      b = 10;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue6, 0, sizeof(enteredValue6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if ((b == 12) && timer_expired == true) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      b = 14;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP=");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (b == 14 && force_on == true) {
      force_on = false;

      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (b == 14 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        b = 141;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        b = 142;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (b == 142 && c == PS2_ENTER) {
      b = 143;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (b == 143 && c == PS2_ENTER) {
      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (b == 141 && c == PS2_ENTER) {

      float x1 = atof(curr_temp2);
      float x2 = atof(curr_temp3);
      dtostrf((x2 - x1), 6, 4, enteredValue12);

      float enteredValue5Int = atof(setting_Value5);
      float enteredValue6Int = atof(setting_Value6);
      float enteredValue8Int = atof(enteredValue8);
      float enteredValue12Int = atof(enteredValue12);
      float enteredValue9Int = atof(enteredValue9);
      float enteredValue10Int = atof(enteredValue10);
      float enteredValue11Int = atof(setting_Value2);

      Serial.print("cotton gcv : ");
      Serial.println(enteredValue5Int, 4);
      Serial.print("fuse wire gcv : ");
      Serial.println(enteredValue6Int, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue8Int, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue12Int, 4);
      Serial.print("cotton mass : ");
      Serial.println(enteredValue9Int, 4);
      Serial.print("wire mass : ");
      Serial.println(enteredValue10Int, 4);

      calculated_weq = (((enteredValue11Int * enteredValue8Int) + ((enteredValue5Int * enteredValue9Int) + (enteredValue6Int * enteredValue10Int))) / enteredValue12Int);
      Serial.print("weq : ");
      Serial.println(calculated_weq);

      b = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      updateDateTime();
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/weq_index/weq_aut_index.txt", "aut");
      lcd.setCursor(0, 1);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 2);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 2);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 3);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue12);
      delay(100);
    }

    else if (b == 15 && c == PS2_DOWNARROW) {
      b = 16;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 0);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      lcd.setCursor(0, 1);
      lcd.print("BENZOIC ACID WEIGHT:");
      lcd.setCursor(0, 2);
      lcd.print(enteredValue8);
      lcd.setCursor(0, 3);
      lcd.print("THREAD WT.:");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue9);
      delay(100);
    }

    else if (b == 16 && c == PS2_DOWNARROW) {
      b = 17;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (b == 17 && c == PS2_DOWNARROW) {
      b = 18;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE & MENU");
      delay(100);
    }

    else if (b == 18 && c == PS2_UPARROW) {
      b = 17;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (b == 17 && c == PS2_UPARROW) {
      b = 16;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 0);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      lcd.setCursor(0, 1);
      lcd.print("BENZOIC ACID WEIGHT:");
      lcd.setCursor(0, 2);
      lcd.print(enteredValue8);
      lcd.setCursor(0, 3);
      lcd.print("THREAD WT.:");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue9);
      delay(100);
    }

    else if (b == 16 && c == PS2_UPARROW) {
      b = 15;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      updateDateTime();
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/weq_index/weq_aut_index.txt", "aut");
      lcd.setCursor(0, 1);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 2);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 2);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 3);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue12);
      delay(100);
    }

    ////////  ENTER PRINT LOGIC BELOW
    else if (b == 17 && c == PS2_ENTER) {

      File root = SD.open("/");
      checkAndCreateFile(root, ("/weq/weq_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 10  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line
      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "INT TEMP: " + String(curr_temp2);
      dataLines[3] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[4] = "DELTA TEMP: " + String(enteredValue12);
      dataLines[5] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[6] = "BENZOIC ACID WEIGHT : " + String(enteredValue8);
      dataLines[7] = "COTTON MASS : " + String(enteredValue9);
      dataLines[8] = "WIRE MASS : " + String(enteredValue10);
      dataLines[9] = "WEQ : " + String(calculated_weq);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      File dataFile = SD.open("/weq/weq_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);

      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("          TOSHNIWAL          ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (b == 18 && c == PS2_ENTER) {
      File root = SD.open("/");
      checkAndCreateFile(root, ("/weq/weq_auto/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 10  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line
      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "INT TEMP: " + String(curr_temp2);
      dataLines[3] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[4] = "DELTA TEMP: " + String(enteredValue12);
      dataLines[5] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[6] = "BENZOIC ACID WEIGHT : " + String(enteredValue8);
      dataLines[7] = "COTTON MASS : " + String(enteredValue9);
      dataLines[8] = "WIRE MASS : " + String(enteredValue10);
      dataLines[9] = "WEQ : " + String(calculated_weq);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      File dataFile = SD.open("/weq/weq_auto/" + String(unique_file_name) + ".txt", FILE_WRITE);

      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue8, 0, sizeof(enteredValue8));
      memset(enteredValue9, 0, sizeof(enteredValue9));
      memset(enteredValue10, 0, sizeof(enteredValue10));
      memset(enteredValue11, 0, sizeof(enteredValue11));
      memset(enteredValue12, 0, sizeof(enteredValue12));
      memset(enteredValue6, 0, sizeof(enteredValue6));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    //////// MANUAL FIRE MODE ////////////
    else if (b == 8 && c == PS2_ENTER) {
      b = 20;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      dtostrf(temperature_now, 6, 4, curr_temp3);
      lcd.setCursor(12, 0);
      lcd.print(curr_temp3);
      lcd.setCursor(5, 2);
      lcd.print(">FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (b == 20 && c == PS2_DOWNARROW) {
      b = 21;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(" ");
      lcd.setCursor(5, 3);
      lcd.print(">");
      delay(100);
    }

    else if (b == 21 && c == PS2_UPARROW) {
      b = 20;
      c = 0;
      lcd.setCursor(5, 2);
      lcd.print(">");
      lcd.setCursor(5, 3);
      lcd.print(" ");
      delay(100);
    }

    if (b == 20 && c == PS2_ENTER) {
      temperature_now = read_temperature();
      digitalWrite(relay_pin, HIGH);
      delay(100);
      digitalWrite(relay_pin, LOW);
      b = 22;
      c = 0;
      lcd.clear();
      dtostrf(temperature_now, 6, 4, curr_temp2);
      lcd.setCursor(0, 0);
      lcd.print("INT TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 1);
      lcd.print("CUR TEMP=");
      lcd.setCursor(0, 2);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(0, 3);
      lcd.print(">FORCE END");
      peak_temperature = 0.0;
      startStopwatchAndUpdateDisplay(12, 2, 15);
      if (force_on == false) {
        c = PS2_ENTER;
        delay(3000);
      }
    }

    if (b == 22 && force_on == true) {
      force_on = false;

      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (b == 22 && c == PS2_ENTER) {

      float y1 = atof(curr_temp2);
      float y2 = atof(curr_temp3);

      if ((y2 - y1) > 0) {
        b = 221;
        c = 0;
        Serial.println("gcv auto fire experiment successful !");
        c = PS2_ENTER;
      } else {
        b = 222;
        c = 0;
        Serial.println("gcv auto fire experiment failed !");  // error in experiment
        c = PS2_ENTER;
      }
    }

    if (b == 222 && c == PS2_ENTER) {
      b = 223;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR IN EXPERIMENT");
      lcd.setCursor(0, 3);
      lcd.print(">GO BACK");
    }

    if (b == 223 && c == PS2_ENTER) {
      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    if (b == 221 && c == PS2_ENTER) {

      float x1 = atof(curr_temp2);
      float x2 = atof(curr_temp3);
      dtostrf((x2 - x1), 6, 4, enteredValue12);

      float enteredValue5Int = atof(setting_Value5);
      float enteredValue6Int = atof(setting_Value6);
      float enteredValue8Int = atof(enteredValue8);
      float enteredValue12Int = atof(enteredValue12);
      float enteredValue9Int = atof(enteredValue9);
      float enteredValue10Int = atof(enteredValue10);
      float enteredValue11Int = atof(setting_Value2);

      Serial.print("cotton gcv : ");
      Serial.println(enteredValue5Int, 4);
      Serial.print("fuse wire gcv : ");
      Serial.println(enteredValue6Int, 4);
      Serial.print("sample weight : ");
      Serial.println(enteredValue8Int, 4);
      Serial.print("delta temp : ");
      Serial.println(enteredValue12Int, 4);
      Serial.print("cotton mass : ");
      Serial.println(enteredValue9Int, 4);
      Serial.print("wire mass : ");
      Serial.println(enteredValue10Int, 4);

      calculated_weq = (((enteredValue11Int * enteredValue8Int) + ((enteredValue5Int * enteredValue9Int) + (enteredValue6Int * enteredValue10Int))) / enteredValue12Int);
      // calculated_weq = (((benzoic acid gcv * sample mass) + ((cotton gcv * cotton length) + (wire gcv * wire length))) / delta temp);
      Serial.println(calculated_weq);

      b = 23;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      updateDateTime();
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/weq_index/weq_man_index.txt", "man");
      lcd.setCursor(0, 1);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 2);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 2);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 3);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue12);
      delay(100);
    }

    else if (b == 23 && c == PS2_DOWNARROW) {
      b = 24;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 0);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      lcd.setCursor(0, 1);
      lcd.print("BENZOIC ACID WEIGHT:");
      lcd.setCursor(0, 2);
      lcd.print(enteredValue8);
      lcd.setCursor(0, 3);
      lcd.print("THREAD WT.:");
      lcd.setCursor(12, 2);
      lcd.print(enteredValue9);
      delay(100);
    }

    else if (b == 24 && c == PS2_DOWNARROW) {
      b = 25;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (b == 25 && c == PS2_DOWNARROW) {
      b = 26;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE & MENU");
      delay(100);
    }

    else if (b == 26 && c == PS2_UPARROW) {
      b = 25;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WIRE WT.: ");
      lcd.setCursor(12, 0);
      lcd.print(enteredValue10);
      lcd.setCursor(0, 1);
      lcd.print("WEQ : ");
      lcd.setCursor(7, 1);
      lcd.print(calculated_weq, 4);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("SAVE & MENU");
      delay(100);
    }

    else if (b == 25 && c == PS2_UPARROW) {
      b = 24;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TIME LAPSED=");
      lcd.setCursor(12, 0);
      printFormattedTime(hours_sw, minutes_sw, seconds_sw);
      lcd.setCursor(0, 1);
      lcd.print("BENZOIC ACID WEIGHT:");
      lcd.setCursor(0, 2);
      lcd.print(enteredValue8);
      lcd.setCursor(0, 3);
      lcd.print("THREAD WT.:");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue9);
      delay(100);
    }

    else if (b == 24 && c == PS2_UPARROW) {
      b = 23;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      updateDateTime();
      lcd.print("EXP NO:");
      unique_file_name = getUniqueFileName("/settings/admin/weq_index/weq_man_index.txt", "man");
      lcd.setCursor(0, 1);
      lcd.print("INT TEMP =");
      lcd.setCursor(12, 1);
      lcd.print(curr_temp2);
      lcd.setCursor(0, 2);
      lcd.print("PEAK TEMP =");
      lcd.setCursor(12, 2);
      lcd.print(curr_temp3);
      lcd.setCursor(0, 3);
      lcd.print("DELTA TEMP=");
      lcd.setCursor(12, 3);
      lcd.print(enteredValue12);
      delay(100);
    }

    ////////  ENTER PRINT LOGIC BELOW
    else if (b == 25 && c == PS2_ENTER) {

      File root = SD.open("/");
      checkAndCreateFile(root, ("/weq/weq_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 10  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line
      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "INT TEMP: " + String(curr_temp2);
      dataLines[3] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[4] = "DELTA TEMP: " + String(enteredValue12);
      dataLines[5] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[6] = "BENZOIC ACID WEIGHT : " + String(enteredValue8);
      dataLines[7] = "COTTON MASS : " + String(enteredValue9);
      dataLines[8] = "WIRE MASS : " + String(enteredValue10);
      dataLines[9] = "WEQ : " + String(calculated_weq);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      File dataFile = SD.open("/weq/weq_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);

      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      Serial2.println(" ");
      Serial2.println("          TOSHNIWAL          ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      memset(enteredValue8, 0, sizeof(enteredValue));
      memset(enteredValue9, 0, sizeof(enteredValue));
      memset(enteredValue10, 0, sizeof(enteredValue));
      memset(enteredValue11, 0, sizeof(enteredValue));
      memset(enteredValue12, 0, sizeof(enteredValue));
      memset(enteredValue6, 0, sizeof(enteredValue));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (b == 26 && c == PS2_ENTER) {

      File root = SD.open("/");
      checkAndCreateFile(root, ("/weq/weq_manual/" + String(unique_file_name) + ".txt").c_str(), FILE_WRITE);
#define NUM_DATA_LINES 10  // The number of lines in your data

      String dataLines[NUM_DATA_LINES];  // Array to store each line
      // Assign each line of data to a specific index
      dataLines[0] = getDateTime();
      dataLines[1] = "EXP NO: " + String(unique_file_name);
      dataLines[2] = "INT TEMP: " + String(curr_temp2);
      dataLines[3] = "PEAK TEMP: " + String(curr_temp3);
      dataLines[4] = "DELTA TEMP: " + String(enteredValue12);
      dataLines[5] = "TIME LAPSED: " + String(hours_sw) + ":" + String(minutes_sw) + ":" + String(seconds_sw);
      dataLines[6] = "BENZOIC ACID WEIGHT : " + String(enteredValue8);
      dataLines[7] = "COTTON MASS : " + String(enteredValue9);
      dataLines[8] = "WIRE MASS : " + String(enteredValue10);
      dataLines[9] = "WEQ : " + String(calculated_weq);

      // Print each line
      for (int i = 0; i < NUM_DATA_LINES; i++) {
        Serial.println(dataLines[i]);
      }

      File dataFile = SD.open("/weq/weq_manual/" + String(unique_file_name) + ".txt", FILE_WRITE);

      if (dataFile) {
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          dataFile.println(dataLines[i]);
        }
        dataFile.close();
        Serial.println("Data saved to the file.");
      } else {
        Serial.println("Error opening file for writing.");
      }

      memset(enteredValue8, 0, sizeof(enteredValue8));
      memset(enteredValue9, 0, sizeof(enteredValue9));
      memset(enteredValue10, 0, sizeof(enteredValue10));
      memset(enteredValue11, 0, sizeof(enteredValue11));
      memset(enteredValue12, 0, sizeof(enteredValue12));
      memset(enteredValue6, 0, sizeof(enteredValue6));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    //////______________REVERSE direction______________//////

    else if (b == 2 && c == PS2_ENTER) {

      memset(enteredValue8, 0, sizeof(enteredValue8));
      memset(enteredValue9, 0, sizeof(enteredValue9));
      memset(enteredValue10, 0, sizeof(enteredValue10));
      memset(enteredValue12, 0, sizeof(enteredValue12));

      a = 4;
      c = 0;
      b = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">WEQ");
      lcd.setCursor(1, 2);
      lcd.print("GCV");
      lcd.setCursor(1, 3);
      lcd.print("GCVC");
      delay(100);
    }

    else if (b == 4 && c == PS2_ENTER) {
      b = 1;
      c = 0;
      i = 8;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER BENZOIC ACID");
      lcd.setCursor(0, 1);
      lcd.print("WEIGHT :");
      if (strcmp(enteredValue8, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue8);
      }
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 6 && c == PS2_ENTER) {
      b = 3;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER THREAD WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      if (strcmp(enteredValue9, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue9);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 9 && c == PS2_ENTER) {
      b = 5;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE WIRE WT");
      lcd.setCursor(16, 2);
      lcd.print("Gm");
      if (strcmp(enteredValue10, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(enteredValue10);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 11 && c == PS2_ENTER) {
      b = 7;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(5, 1);
      lcd.print(">AUTO FIRE");
      lcd.setCursor(6, 2);
      lcd.print("MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
    }

    else if (b == 13 && c == PS2_ENTER) {
      b = 10;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue6, 0, sizeof(enteredValue6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 17 && c == PS2_ENTER) {
      b = 10;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(enteredValue6, 0, sizeof(enteredValue6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP=");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(1, 1);
      lcd.print("ENTER TIMER IN SEC:");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (b == 21 && c == PS2_ENTER) {
      b = 8;
      c = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("CUR TEMP =");
      lcd.setCursor(12, 0);
      lcd.print(temperature_now, 4);
      lcd.setCursor(6, 1);
      lcd.print("AUTO FIRE");
      lcd.setCursor(5, 2);
      lcd.print(">MANUAL FIRE");
      lcd.setCursor(6, 3);
      lcd.print("BACK");
    }



    //////____________________________settings  SECTION____________________________//////
    //////____________________________settings  SECTION____________________________//////

    if (a == 6 && c == PS2_ENTER) {
      s = 1;
      a = 0;
      b = 0;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 1 && c == PS2_DOWNARROW) {
      s = 2;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 2 && c == PS2_DOWNARROW) {
      s = 3;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 2);
      lcd.print(">");
      delay(100);
    }

    else if (s == 3 && c == PS2_DOWNARROW) {
      s = 4;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
      delay(100);
    }

    else if (s == 4 && c == PS2_DOWNARROW) {
      s = 5;
      a = 0;
      b = 0;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">ADD ON GCVS");
      lcd.setCursor(1, 1);
      lcd.print("Date/Time");
      lcd.setCursor(1,2);
      lcd.print("BACK");
      delay(100);
    } 
    else if (s == 5 && c == PS2_DOWNARROW) {
      s = 555; // State for DATE/TIME selection
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 555 && c == PS2_DOWNARROW) {
      s = 6;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 2);
      lcd.print(">");
      delay(100);
    }

    else if (s == 6 && c == PS2_ENTER) {
      a = 6;
      s = 0;
      b = 0;
      c = 0;
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("MANUAL MODE");
      lcd.setCursor(0, 1);
      lcd.print(">SETTINGS");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      delay(100);
    }
    else if (s == 555 && c == PS2_ENTER) {
      DateTime now = rtc.now();
      edit_year   = now.year();
      edit_month  = now.month();
      edit_day    = now.day();
      edit_hour   = now.hour();
      edit_minute = now.minute();
      
      dt_cursor = 0;
      datetime_buf_pos = 0;
      in_datetime_menu = true;
      displayDateTimeMenu();
      delay(100);
    }
    else if (s == 6 && c == PS2_UPARROW) {
      s = 555;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 555 && c == PS2_UPARROW) {
      s = 5;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 0);
      lcd.print(">");
      delay(100);
    }

    else if (s == 5 && c == PS2_UPARROW) {
      s = 4;
      a = 0;
      b = 0;
      c = 0;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(0, 3);
      lcd.print(">PREVIOUS EXP");
      delay(100);
    }

    else if (s == 4 && c == PS2_UPARROW) {
      s = 3;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 3);
      lcd.print(" ");
      lcd.setCursor(0, 2);
      lcd.print(">");
      delay(100);
    }

    else if (s == 3 && c == PS2_UPARROW) {
      s = 2;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 2);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 2 && c == PS2_UPARROW) {
      s = 1;
      a = 0;
      b = 0;
      c = 0;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 0);
      lcd.print(">");
      delay(100);
    }

    else if (s == 1 && c == PS2_ENTER) {
      s = 7;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      memset(setting_Value, 0, sizeof(setting_Value));
      readFileIntoBuffer("/settings/countdown_timer.txt", setting_Value, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("COUNTDOWN TIMER");
      if (strcmp(setting_Value, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">EDIT");
      delay(100);
    }

    else if (s == 7 && c == PS2_LEFTARROW) {
      s = 8;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 8 && c == PS2_RIGHTARROW) {
      s = 7;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    else if (s == 7 && c == PS2_ENTER) {
      memset(setting_Value, 0, sizeof(setting_Value));
      valuecount = 0;
      s = 777;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("COUNTDOWN TIMER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if ((s == 777 || s == 888) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 777 || s == 888) && c == PS2_DELETE && (valuecount > 0 || strcmp(enteredValue6, "") != 0)) {
      s = 7;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(setting_Value, 0, sizeof(setting_Value));
      lcd.clear();
      lcd.setCursor(3, 0);
      lcd.print("COUNTDOWN TIMER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 777 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      Serial.print("value of setting_Value : ");
      Serial.println(setting_Value);
      writeFile(SD, "/settings/countdown_timer.txt", setting_Value);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 8 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 2 && c == PS2_ENTER) {
      s = 10;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      memset(setting_Value1, 0, sizeof(setting_Value1));
      readFileIntoBuffer("/settings/weq.txt", setting_Value1, 13);
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("WEQ");
      lcd.setCursor(4, 1);
      if (strcmp(setting_Value1, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value1);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">EDIT");
      delay(100);
    }

    else if (s == 10 && c == PS2_LEFTARROW) {
      s = 11;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 11 && c == PS2_RIGHTARROW) {
      s = 10;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    else if (s == 11 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      valuecount = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 10 && c == PS2_ENTER) {
      memset(setting_Value1, 0, sizeof(setting_Value1));
      valuecount = 0;
      s = 12;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER WEQ-");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 12 && c == PS2_LEFTARROW) {
      s = 13;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 13 && c == PS2_RIGHTARROW) {
      s = 12;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 12 || s == 13) && ((c >= '0' && c <= '9') || c == '.') && i <= 15) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value1[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 12 || s == 13) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value1, "") != 0)) {
      memset(setting_Value1, 0, sizeof(setting_Value1));
      valuecount = 0;
      s = 12;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER WEQ-");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (s == 12 && c == PS2_ENTER && (valuecount > 0 || strcmp(setting_Value1, "") != 0)) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      Serial.print("value of setting_Value1 : ");
      Serial.println(setting_Value1);
      writeFile(SD, "/settings/weq.txt", setting_Value1);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 13 && c == PS2_ENTER) {
      s = 10;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      valuecount = 0;
      memset(setting_Value1, 0, sizeof(setting_Value1));
      readFileIntoBuffer("/settings/weq.txt", setting_Value1, 13);
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("WEQ FIGURE :");
      lcd.setCursor(4, 1);
      if (strcmp(setting_Value1, "") != 0) {
        lcd.setCursor(i, 2);
        lcd.print(setting_Value1);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">EDIT");
      delay(100);
    }

    else if (s == 3 && c == PS2_ENTER) {
      s = 14;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      memset(setting_Value2, 0, sizeof(setting_Value2));
      readFileIntoBuffer("/settings/benzoic_acid_gcv.txt", setting_Value2, 13);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(6, 1);
      if (strcmp(setting_Value2, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value2);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">EDIT");
      delay(100);
    }

    else if (s == 14 && c == PS2_LEFTARROW) {
      s = 15;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 15 && c == PS2_RIGHTARROW) {
      s = 14;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    else if (s == 15 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 14 && c == PS2_ENTER) {
      s = 16;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      memset(setting_Value2, 0, sizeof(setting_Value2));
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("ENTER BENZOIC GCV");
      lcd.setCursor(0, 1);
      lcd.print("GCV :");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 16 && c == PS2_LEFTARROW) {
      s = 17;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 17 && c == PS2_RIGHTARROW) {
      s = 16;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 16 || s == 17) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value2[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    else if (s == 17 && c == PS2_ENTER) {
      s = 14;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      memset(setting_Value2, 0, sizeof(setting_Value2));
      readFileIntoBuffer("/settings/benzoic_acid_gcv.txt", setting_Value2, 13);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(6, 1);
      if (strcmp(setting_Value2, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value2);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">EDIT");
      delay(100);
    }

    if ((s == 16 || s == 17) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value2, "") != 0)) {
      memset(setting_Value2, 0, sizeof(setting_Value2));
      valuecount = 0;
      s = 16;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("ENTER BENZOIC GCV");
      lcd.setCursor(0, 1);
      lcd.print("GCV :");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
    }

    if (s == 16 && c == PS2_ENTER && (valuecount > 0 || strcmp(setting_Value2, "") != 0)) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      writeFile(SD, "/settings/benzoic_acid_gcv.txt", setting_Value2);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    // Initial Page Setup
    else if (s == 4 && c == PS2_ENTER) {
      s = 18;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(7, 0);
      lcd.print(">GCV");
      lcd.setCursor(8, 1);
      lcd.print("GCVC");
      lcd.setCursor(8, 2);
      lcd.print("WEQ");
      lcd.setCursor(8, 3);
      lcd.print("BACK");
      delay(100);
    }

    // Navigation Downwards
    else if (s == 18 && c == PS2_DOWNARROW) {
      s = 19;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 0);
      lcd.print(" ");
      lcd.setCursor(7, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 19 && c == PS2_DOWNARROW) {
      s = 20;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 1);
      lcd.print(" ");
      lcd.setCursor(7, 2);
      lcd.print(">");
      delay(100);
    }

    else if (s == 20 && c == PS2_DOWNARROW) {
      s = 211;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 2);
      lcd.print(" ");
      lcd.setCursor(7, 3);
      lcd.print(">");
      delay(100);
    }

    // Navigation Upwards
    else if (s == 211 && c == PS2_UPARROW) {
      s = 20;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 3);
      lcd.print(" ");
      lcd.setCursor(7, 2);
      lcd.print(">");
      delay(100);
    }

    else if (s == 20 && c == PS2_UPARROW) {
      s = 19;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 2);
      lcd.print(" ");
      lcd.setCursor(7, 1);
      lcd.print(">");
      delay(100);
    }

    else if (s == 19 && c == PS2_UPARROW) {
      s = 18;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(7, 1);
      lcd.print(" ");
      lcd.setCursor(7, 0);
      lcd.print(">");
      delay(100);
    }

    // Selection of "BACK"
    else if (s == 211 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    //      gcv prev experiments load

    else if (s == 18 && c == PS2_ENTER) {
      s = 21;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 21 && c == PS2_DOWNARROW) {
      s = 22;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 22 && c == PS2_DOWNARROW) {
      s = 23;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 22 && c == PS2_UPARROW) {
      s = 21;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 23 && c == PS2_UPARROW) {
      s = 22;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 23 && c == PS2_ENTER) {
      s = 18;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(7, 0);
      lcd.print(">GCV");
      lcd.setCursor(8, 1);
      lcd.print("GCVC");
      lcd.setCursor(8, 2);
      lcd.print("WEQ");
      lcd.setCursor(8, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 21 && c == PS2_ENTER) {
      s = 24;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 24 && c == PS2_DOWNARROW) {
      s = 25;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 25 && c == PS2_DOWNARROW) {
      s = 26;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 25 && c == PS2_UPARROW) {
      s = 24;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 26 && c == PS2_UPARROW) {
      s = 25;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 26 && c == PS2_ENTER) {
      s = 21;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 24 && c == PS2_ENTER) {
      s = 27;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 27 && c == PS2_LEFTARROW) {
      s = 28;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 28 && c == PS2_RIGHTARROW) {
      s = 27;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 27 || s == 28) && (c >= '0' && c <= '9') && i <= 10) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      Serial.print("input : ");
      Serial.println(setting_Value3);
      Serial.print("count : ");
      Serial.println(valuecount);
      delay(100);
    }

    if ((s == 27 || s == 28) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value3, "") != 0)) {
      s = 27;
      c = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 28 && c == PS2_ENTER) {
      s = 24;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }


    else if (s == 27 && c == PS2_ENTER && valuecount > 0) {

      // Read data from the file
      File dataFile = SD.open("/gcv/gcv_auto/aut" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 13
        while (dataFile.available() && lineCount < NUM_DATA_LINES) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                  // Trim in place
            dataLines[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          Serial.println(dataLines[i]);
        }
        n = 1;
      } else {
        Serial.println("Error opening file for reading.");
        n = -1;
      }
    }

    if (n == -1) {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("Error opening file");
      delay(2000);
      s = 27;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (n == 1) {
      s = 29;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 29 && c == PS2_DOWNARROW) {
      s = 30;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 30 && c == PS2_UPARROW) {
      s = 29;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 30 && c == PS2_DOWNARROW) {
      s = 31;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 31 && c == PS2_UPARROW) {
      s = 30;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 31 && c == PS2_DOWNARROW) {
      s = 32;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 32 && c == PS2_UPARROW) {
      s = 31;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 32 && c == PS2_DOWNARROW) {
      s = 33;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 33 && c == PS2_UPARROW) {
      s = 32;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 33 && c == PS2_DOWNARROW) {
      s = 34;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 34 && c == PS2_UPARROW) {
      s = 33;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 32 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 34 && c == PS2_ENTER) {
      /////////////////////////////////////////////////////////////
      //    will add print functionality here

      Serial2.println(" ");
      Serial2.println("           TOSHNIWAL");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");


      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 33 && c == PS2_ENTER) {
      s = 27;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 25 && c == PS2_ENTER) {
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////
      currentDir = SD.open("/gcv/gcv_auto");
      currentPath = "/gcv/gcv_auto";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }

      s = 25;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SEARCH VIA NUMBER");
      lcd.setCursor(0, 1);
      lcd.print(">VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    /////////////  manual mode experiments //////////////////////
    else if (s == 22 && c == PS2_ENTER) {
      s = 36;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }


    else if (s == 36 && c == PS2_DOWNARROW) {
      s = 37;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 37 && c == PS2_DOWNARROW) {
      s = 38;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 37 && c == PS2_UPARROW) {
      s = 36;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 38 && c == PS2_UPARROW) {
      s = 37;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 37 && c == PS2_ENTER) {
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////
      currentDir = SD.open("/gcv/gcv_manual");
      currentPath = "/gcv/gcv_manual";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }
      s = 22;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("AUTO FIRE EXP.");
      lcd.setCursor(0, 1);
      lcd.print(">MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 38 && c == PS2_ENTER) {
      s = 21;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 36 && c == PS2_ENTER) {
      s = 39;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 39 && c == PS2_LEFTARROW) {
      s = 40;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 40 && c == PS2_RIGHTARROW) {
      s = 39;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 40 || s == 39) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if (s == 40 && c == PS2_ENTER) {
      s = 36;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    if (s == 39 && c == PS2_ENTER && valuecount > 0) {

      // Read data from the file
      File dataFile = SD.open("/gcv/gcv_manual/man" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 13
        while (dataFile.available() && lineCount < NUM_DATA_LINES) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                  // Trim in place
            dataLines[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          Serial.println(dataLines[i]);
        }
        n = 2;
      } else {
        Serial.println("Error opening file for reading.");
        n = -2;
      }
    }

    if (n == -2) {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR OPENING FILE");
      delay(2000);
      s = 39;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }
    if (n == 2) {
      s = 41;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 41 && c == PS2_DOWNARROW) {
      s = 42;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 42 && c == PS2_UPARROW) {
      s = 41;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 42 && c == PS2_DOWNARROW) {
      s = 43;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 43 && c == PS2_UPARROW) {
      s = 42;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 43 && c == PS2_DOWNARROW) {
      s = 44;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 44 && c == PS2_UPARROW) {
      s = 43;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 44 && c == PS2_DOWNARROW) {
      s = 45;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 45 && c == PS2_UPARROW) {
      s = 44;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 45 && c == PS2_DOWNARROW) {
      s = 46;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 46 && c == PS2_UPARROW) {
      s = 45;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 44 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 46 && c == PS2_ENTER) {
      /////////////////////////////////////////////////////////////
      //    will add print functionality here
      Serial2.println(" ");
      Serial2.println("           TOSHNIWAL");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 45 && c == PS2_ENTER) {
      s = 39;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }


    //      GCVC PREVIOUS EXAMPLE LOAD
    ///////////////////////////////////////////////////////////////////////////

    else if (s == 19 && c == PS2_ENTER) {
      s = 101;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 101 && c == PS2_DOWNARROW) {
      s = 102;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 102 && c == PS2_DOWNARROW) {
      s = 103;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 102 && c == PS2_UPARROW) {
      s = 101;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 103 && c == PS2_UPARROW) {
      s = 102;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 103 && c == PS2_ENTER) {
      s = 19;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(8, 0);
      lcd.print("GCV");
      lcd.setCursor(7, 1);
      lcd.print(">GCVC");
      lcd.setCursor(8, 2);
      lcd.print("WEQ");
      lcd.setCursor(8, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 101 && c == PS2_ENTER) {
      s = 104;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 104 && c == PS2_DOWNARROW) {
      s = 105;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 105 && c == PS2_DOWNARROW) {
      s = 106;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 105 && c == PS2_UPARROW) {
      s = 104;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 106 && c == PS2_UPARROW) {
      s = 105;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 106 && c == PS2_ENTER) {
      s = 101;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 104 && c == PS2_ENTER) {
      s = 107;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 107 && c == PS2_LEFTARROW) {
      s = 108;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 108 && c == PS2_RIGHTARROW) {
      s = 107;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 107 || s == 108) && (c >= '0' && c <= '9') && i <= 10) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 107 || s == 108) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value3, "") != 0)) {
      s = 107;
      c = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 108 && c == PS2_ENTER) {
      s = 104;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }


    if (s == 107 && c == PS2_ENTER && valuecount > 0) {

      // Read data from the file
      File dataFile = SD.open("/gcvc/gcvc_auto/aut" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 14
        while (dataFile.available() && lineCount < NUM_DATA_LINES) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                  // Trim in place
            dataLines[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          Serial.println(dataLines[i]);
        }
        n = 1;
      } else {
        Serial.println("Error opening file for reading.");
        n = -1;
      }
    }

    if (n == -1) {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("Error opening file");
      delay(2000);
      s = 107;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    if (n == 1) {
      s = 109;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 109 && c == PS2_DOWNARROW) {
      s = 110;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 110 && c == PS2_UPARROW) {
      s = 109;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 110 && c == PS2_DOWNARROW) {
      s = 111;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 111 && c == PS2_UPARROW) {
      s = 110;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 111 && c == PS2_DOWNARROW) {
      s = 112;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 112 && c == PS2_UPARROW) {
      s = 111;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 112 && c == PS2_DOWNARROW) {
      s = 113;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 113 && c == PS2_UPARROW) {
      s = 112;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 113 && c == PS2_DOWNARROW) {
      s = 114;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 114 && c == PS2_UPARROW) {
      s = 113;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 112 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 114 && c == PS2_ENTER) {
      Serial2.println(" ");
      Serial2.println("           TOSHNIWAL");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(dataLines[13]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");


      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 113 && c == PS2_ENTER) {
      s = 107;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 105 && c == PS2_ENTER) {
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////
      currentDir = SD.open("/gcvc/gcvc_auto");
      currentPath = "/gcvc/gcvc_auto";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }

      s = 105;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SEARCH VIA NUMBER");
      lcd.setCursor(0, 1);
      lcd.print(">VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    ////////  manual mode experiments

    else if (s == 102 && c == PS2_ENTER) {
      s = 116;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }


    else if (s == 116 && c == PS2_DOWNARROW) {
      s = 117;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 117 && c == PS2_DOWNARROW) {
      s = 118;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 117 && c == PS2_UPARROW) {
      s = 116;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 118 && c == PS2_UPARROW) {
      s = 117;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 117 && c == PS2_ENTER) {
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////
      currentDir = SD.open("/gcvc/gcvc_manual");
      currentPath = "/gcvc/gcvc_manual";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }

      s = 117;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SEARCH VIA NUMBER");
      lcd.setCursor(0, 1);
      lcd.print(">VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 118 && c == PS2_ENTER) {
      s = 101;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 116 && c == PS2_ENTER) {
      s = 119;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 119 && c == PS2_LEFTARROW) {
      s = 120;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 120 && c == PS2_RIGHTARROW) {
      s = 119;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 120 || s == 119) && (c >= '0' && c <= '9') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    else if (s == 120 && c == PS2_ENTER) {
      s = 116;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    if (s == 119 && c == PS2_ENTER && valuecount > 0) {

      // Read data from the file
      File dataFile = SD.open("/gcvc/gcvc_manual/man" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 14
        while (dataFile.available() && lineCount < NUM_DATA_LINES) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                  // Trim in place
            dataLines[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES; i++) {
          Serial.println(dataLines[i]);
        }
        n = 2;
      } else {
        Serial.println("Error opening file for reading.");
        n = -2;
      }
    }

    if (n == -2) {
      lcd.clear();
      lcd.setCursor(1, 1);
      lcd.print("ERROR OPENING FILE");
      delay(2000);
      s = 119;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }
    if (n == 2) {
      s = 121;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 121 && c == PS2_DOWNARROW) {
      s = 122;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 122 && c == PS2_UPARROW) {
      s = 121;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[0]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[1]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[2]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[3]);
      delay(100);
    }

    else if (s == 122 && c == PS2_DOWNARROW) {
      s = 123;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 123 && c == PS2_UPARROW) {
      s = 122;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[4]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[5]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[6]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[7]);
      delay(100);
    }

    else if (s == 123 && c == PS2_DOWNARROW) {
      s = 124;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 124 && c == PS2_UPARROW) {
      s = 123;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[8]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[9]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines[10]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines[11]);
      delay(100);
    }

    else if (s == 124 && c == PS2_DOWNARROW) {
      s = 125;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 125 && c == PS2_UPARROW) {
      s = 124;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 125 && c == PS2_DOWNARROW) {
      s = 126;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      lcd.setCursor(0, 3);
      lcd.print(">SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 126 && c == PS2_UPARROW) {
      s = 125;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines[12]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines[13]);
      lcd.setCursor(0, 2);
      lcd.print(">BACK");
      lcd.setCursor(1, 3);
      lcd.print("SAVE, PRINT & MENU");
      delay(100);
    }

    else if (s == 124 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 126 && c == PS2_ENTER) {
      Serial2.println(" ");
      Serial2.println("           TOSHNIWAL");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(dataLines[11]);
      Serial2.println(dataLines[12]);
      Serial2.println(dataLines[13]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 125 && c == PS2_ENTER) {
      s = 118;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 117 && c == PS2_ENTER) {
      s = 129;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      // add logic to view files from sd card
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 129 && c == PS2_LEFTARROW) {
      s = 130;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 130 && c == PS2_RIGHTARROW) {
      s = 129;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    else if (s == 129 && c == PS2_ENTER) {
      s = 117;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 130 && c == PS2_ENTER) {
      s = 117;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }


    /////////////////////////////////////////////////////////
    /////////   weq     ////////////


    else if (s == 20 && c == PS2_ENTER) {
      s = 51;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 51 && c == PS2_DOWNARROW) {
      s = 620;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 620 && c == PS2_DOWNARROW) {
      s = 630;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 620 && c == PS2_UPARROW) {
      s = 51;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 630 && c == PS2_UPARROW) {
      s = 620;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 630 && c == PS2_ENTER) {
      s = 20;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(8, 0);
      lcd.print("GCV");
      lcd.setCursor(8, 1);
      lcd.print("GCVC");
      lcd.setCursor(7, 2);
      lcd.print(">WEQ");
      lcd.setCursor(8, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 51 && c == PS2_ENTER) {
      s = 52;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 52 && c == PS2_DOWNARROW) {
      s = 53;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 53 && c == PS2_DOWNARROW) {
      s = 54;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 53 && c == PS2_UPARROW) {
      s = 52;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 54 && c == PS2_UPARROW) {
      s = 53;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 54 && c == PS2_ENTER) {
      s = 51;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 52 && c == PS2_ENTER) {
      s = 55;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 55 && c == PS2_LEFTARROW) {
      s = 56;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 56 && c == PS2_RIGHTARROW) {
      s = 55;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 55 || s == 56) && (c >= '0' && c <= '9') && i <= 10) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      Serial.println(setting_Value3);
      delay(100);
    }

    if ((s == 55 || s == 56) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value3, "") != 0)) {
      s = 55;
      c = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 56 && c == PS2_ENTER) {
      s = 52;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 55 && c == PS2_ENTER) {
      // Read data from the file
      File dataFile = SD.open("/weq/weq_auto/aut" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 10
        while (dataFile.available() && lineCount < NUM_DATA_LINES1) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                   // Trim in place
            dataLines1[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES1; i++) {
          Serial.println(dataLines1[i]);
        }

        s = 57;
        c = 0;
        b = 0;
        a = 0;
        i = 7;
        n = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines1[0]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines1[1]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines1[2]);
        delay(100);

      } else {
        Serial.println("Error opening file for reading.");
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("ERROR OPENING FILE");
        delay(2000);

        s = 52;
        c = 0;
        b = 0;
        a = 0;
        i = 7;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(">SEARCH VIA NUMBER");
        lcd.setCursor(1, 1);
        lcd.print("VIEW ALL");
        lcd.setCursor(1, 3);
        lcd.print("BACK");
        delay(100);
      }
    }

    else if (s == 57 && c == PS2_DOWNARROW) {
      s = 58;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[3]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[4]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[5]);
      delay(100);
    }

    else if (s == 58 && c == PS2_UPARROW) {
      s = 57;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[0]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[1]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[2]);
      delay(100);
    }

    else if (s == 58 && c == PS2_DOWNARROW) {
      s = 59;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[6].substring(0, 20));
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[6].substring(20));
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[7]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[8]);
      delay(100);
    }

    else if (s == 59 && c == PS2_DOWNARROW) {
      s = 60;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 60 && c == PS2_UPARROW) {
      s = 59;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      n = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[6].substring(0, 20));
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[6].substring(20));
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[7]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[8]);
    }

    else if (s == 59 && c == PS2_UPARROW) {
      s = 58;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[3]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[4]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[5]);
      delay(100);
    }

    else if (s == 60 && c == PS2_DOWNARROW) {
      s = 61;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 61 && c == PS2_DOWNARROW) {
      s = 62;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">BACK");
      delay(100);
    }

    else if (s == 62 && c == PS2_UPARROW) {
      s = 61;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 61 && c == PS2_UPARROW) {
      s = 60;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 60 && c == PS2_ENTER) {
      s = 52;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 61 && c == PS2_ENTER) {
      Serial2.println(" ");
      Serial2.println("          TOSHNIWAL          ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      s = 55;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 62 && c == PS2_ENTER) {
      s = 55;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 53 && c == PS2_ENTER) {
      s = 64;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////

      currentDir = SD.open("/weq/weq_auto");
      currentPath = "/weq/weq_auto";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }

      s = 53;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("SEARCH VIA NUMBER");
      lcd.setCursor(0, 1);
      lcd.print(">VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 620 && c == PS2_ENTER) {
      s = 66;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 66 && c == PS2_DOWNARROW) {
      s = 67;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(">");
    }

    else if (s == 67 && c == PS2_DOWNARROW) {
      s = 68;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
    }

    else if (s == 67 && c == PS2_UPARROW) {
      s = 66;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 0);
      lcd.print(">");
      lcd.setCursor(0, 1);
      lcd.print(" ");
    }

    else if (s == 68 && c == PS2_UPARROW) {
      s = 67;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 3);
      lcd.print(" ");
    }

    else if (s == 68 && c == PS2_ENTER) {
      s = 51;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">AUTO FIRE EXP.");
      lcd.setCursor(1, 1);
      lcd.print("MANUAL FIRE EXP.");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 66 && c == PS2_ENTER) {
      s = 69;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 69 && c == PS2_LEFTARROW) {
      s = 70;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 70 && c == PS2_RIGHTARROW) {
      s = 69;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.setCursor(8, 3);
      lcd.print(">");
      lcd.setCursor(1, 3);
      lcd.print(" ");
    }

    if ((s == 69 || s == 70) && (c >= '0' && c <= '9') && i <= 10) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value3[i - 7] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      Serial.println(setting_Value3);
      delay(100);
    }

    if ((s == 69 || s == 70) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value3, "") != 0)) {
      s = 69;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 70 && c == PS2_ENTER) {
      s = 66;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");  // or may be here
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 69 && c == PS2_ENTER) {
      // Read data from the file
      File dataFile = SD.open("/weq/weq_manual/man" + String(setting_Value3) + ".txt", FILE_READ);
      if (dataFile) {
        int lineCount = 0;
#define NUM_DATA_LINES 10
        while (dataFile.available() && lineCount < NUM_DATA_LINES1) {
          String line = dataFile.readStringUntil('\n');
          // Check for an empty line
          if (!line.isEmpty()) {
            line.trim();                   // Trim in place
            dataLines1[lineCount] = line;  // Assign the trimmed line
            lineCount++;
          }
        }
        dataFile.close();
        Serial.println("Data read from the file.");
        // Print each line
        for (int i = 0; i < NUM_DATA_LINES1; i++) {
          Serial.println(dataLines1[i]);
        }
        s = 71;
        c = 0;
        b = 0;
        a = 0;
        i = 7;
        n = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dataLines1[0]);
        lcd.setCursor(0, 2);
        lcd.print(dataLines1[1]);
        lcd.setCursor(0, 3);
        lcd.print(dataLines1[2]);
        delay(100);

      } else {
        Serial.println("Error opening file for reading.");
        lcd.clear();
        lcd.setCursor(1, 1);
        lcd.print("ERROR OPENING FILE");
        delay(2000);

        s = 66;
        c = 0;
        b = 0;
        a = 0;
        i = 7;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(">SEARCH VIA NUMBER");  // or may be here
        lcd.setCursor(1, 1);
        lcd.print("VIEW ALL");
        lcd.setCursor(1, 3);
        lcd.print("BACK");
        delay(100);
      }
    }

    else if (s == 71 && c == PS2_DOWNARROW) {
      s = 72;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[3]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[4]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[5]);
      delay(100);
    }

    else if (s == 72 && c == PS2_UPARROW) {
      s = 71;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[0]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[1]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[2]);
      delay(100);
    }

    else if (s == 72 && c == PS2_DOWNARROW) {
      s = 730;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[6].substring(0, 20));
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[6].substring(20));
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[7]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[8]);
      delay(100);
    }

    else if (s == 730 && c == PS2_UPARROW) {
      s = 72;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[3]);
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[4]);
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[5]);
      delay(100);
    }

    else if (s == 730 && c == PS2_DOWNARROW) {
      s = 73;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 73 && c == PS2_UPARROW) {
      s = 730;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[6].substring(0, 20));
      lcd.setCursor(0, 1);
      lcd.print(dataLines1[6].substring(20));
      lcd.setCursor(0, 2);
      lcd.print(dataLines1[7]);
      lcd.setCursor(0, 3);
      lcd.print(dataLines1[8]);
      delay(100);
    }

    else if (s == 73 && c == PS2_DOWNARROW) {
      s = 74;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 74 && c == PS2_DOWNARROW) {
      s = 75;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(0, 3);
      lcd.print(">BACK");
      delay(100);
    } else if (s == 75 && c == PS2_UPARROW) {
      s = 74;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(1, 1);
      lcd.print("SAVE & MENU");
      lcd.setCursor(0, 2);
      lcd.print(">SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    } else if (s == 74 && c == PS2_UPARROW) {
      s = 73;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(dataLines1[9]);
      lcd.setCursor(0, 1);
      lcd.print(">SAVE & MENU");
      lcd.setCursor(1, 2);
      lcd.print("SAVE, PRINT & MENU");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 73 && c == PS2_ENTER) {
      s = 1;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">COUNTDOWN TIMER");
      lcd.setCursor(1, 1);
      lcd.print("WEQ");
      lcd.setCursor(1, 2);
      lcd.print("BENZOIC ACID GCV");
      lcd.setCursor(1, 3);
      lcd.print("PREVIOUS EXP");
      delay(100);
    }

    else if (s == 75 && c == PS2_ENTER) {
      s = 69;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 74 && c == PS2_ENTER) {
      Serial2.println(" ");
      Serial2.println("          TOSHNIWAL          ");
      Serial2.println(" ");
      Serial2.println(dataLines[0]);
      Serial2.println(dataLines[1]);
      Serial2.println(dataLines[2]);
      Serial2.println(dataLines[3]);
      Serial2.println(dataLines[4]);
      Serial2.println(dataLines[5]);
      Serial2.println(dataLines[6]);
      Serial2.println(dataLines[7]);
      Serial2.println(dataLines[8]);
      Serial2.println(dataLines[9]);
      Serial2.println(dataLines[10]);
      Serial2.println(" ");
      Serial2.println(" ");
      Serial2.println(" ");

      s = 69;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      valuecount = 0;
      memset(setting_Value3, 0, sizeof(setting_Value3));
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("ENTER NUMBER");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 67 && c == PS2_ENTER) {
      s = 76;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.clear();
      ///////////////////////// view all logic to be added here ////////////////////////////

      currentDir = SD.open("/weq/weq_manual");
      currentPath = "/weq/weq_manual";  // Dynamically store the directory name
      if (!currentDir) {
        lcd.clear();
        lcd.print("Error opening root");
        Serial.println("Error opening root directory.");
      }
      listDirectory(currentDir);
      displayFiles();

      go_out = false;
      while (go_out == false) {
        if (keyboard.available()) {
          c = keyboard.read();
          handleCommand(c);
        }
      }

      s = 66;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">SEARCH VIA NUMBER");  // or may be here
      lcd.setCursor(1, 1);
      lcd.print("VIEW ALL");
      lcd.setCursor(1, 3);
      lcd.print("BACK");
      delay(100);
    }



    //////////// ADD ON GCV SETTINGS ////////////////

    else if (s == 5 && c == PS2_ENTER) {
      s = 78;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(">COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 78 && c == PS2_DOWNARROW) {
      s = 79;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(1, 1);
      lcd.print(">FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 79 && c == PS2_DOWNARROW) {
      s = 80;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(1, 2);
      lcd.print(">CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 80 && c == PS2_DOWNARROW) {
      s = 81;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(1, 3);
      lcd.print(">BACK");
      delay(100);
    }

    else if (s == 81 && c == PS2_UPARROW) {
      s = 80;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(1, 2);
      lcd.print(">CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 80 && c == PS2_UPARROW) {
      s = 79;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(1, 1);
      lcd.print(">FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 79 && c == PS2_UPARROW) {
      s = 78;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(">COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 81 && c == PS2_ENTER) {
      s = 5;
      a = 0;
      b = 0;
      c = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(">ADD ON GCVS");
      lcd.setCursor(1, 2);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 78 && c == PS2_ENTER) {
      s = 82;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      valuecount = 0;
      readFileIntoBuffer("/settings/cotton_gcv.txt", setting_Value5, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER COTTON GCV");
      if (strcmp(setting_Value5, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value5);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 82 && c == PS2_LEFTARROW) {
      s = 83;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 83 && c == PS2_RIGHTARROW) {
      s = 82;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((s == 82 || s == 83) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value5[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 82 || s == 83) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value5, "") != 0)) {
      s = 82;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(setting_Value5, 0, sizeof(setting_Value5));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER COTTON GCV");
      if (strcmp(setting_Value5, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value5);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 82 && c == PS2_ENTER) {
      s = 78;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      writeFile(SD, "/settings/cotton_gcv.txt", setting_Value5);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(">COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 83 && c == PS2_ENTER) {
      s = 78;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      memset(setting_Value5, 0, sizeof(setting_Value5));
      readFileIntoBuffer("/settings/cotton_gcv.txt", setting_Value5, 13);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(">COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 79 && c == PS2_ENTER) {
      s = 84;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      valuecount = 0;
      readFileIntoBuffer("/settings/nichrome_wire_gcv.txt", setting_Value6, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE GCV");
      if (strcmp(setting_Value6, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value6);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 84 && c == PS2_LEFTARROW) {
      s = 85;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 85 && c == PS2_RIGHTARROW) {
      s = 84;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((s == 84 || s == 85) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value6[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 84 || s == 85) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value6, "") != 0)) {
      s = 84;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(setting_Value6, 0, sizeof(setting_Value6));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER FUSE GCV");
      if (strcmp(setting_Value6, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value6);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 84 && c == PS2_ENTER) {
      s = 79;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      writeFile(SD, "/settings/nichrome_wire_gcv.txt", setting_Value6);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(1, 1);
      lcd.print(">FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 85 && c == PS2_ENTER) {
      s = 79;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      memset(setting_Value6, 0, sizeof(setting_Value6));
      readFileIntoBuffer("/settings/nichrome_wire_gcv.txt", setting_Value6, 13);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(1, 1);
      lcd.print(">FUSE WIRE GCV");
      lcd.setCursor(2, 2);
      lcd.print("CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 80 && c == PS2_ENTER) {
      s = 86;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      valuecount = 0;
      readFileIntoBuffer("/settings/capsule_gcv.txt", setting_Value7, 13);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CAPSULE GCV");
      if (strcmp(setting_Value7, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value7);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 86 && c == PS2_LEFTARROW) {
      s = 87;
      a = 0;
      b = 0;
      c = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(">");
      lcd.setCursor(8, 3);
      lcd.print(" ");
    }

    else if (s == 87 && c == PS2_RIGHTARROW) {
      s = 86;
      c = 0;
      b = 0;
      a = 0;
      i = 6;
      lcd.setCursor(1, 3);
      lcd.print(" ");
      lcd.setCursor(8, 3);
      lcd.print(">");
    }

    if ((s == 86 || s == 87) && ((c >= '0' && c <= '9') || c == '.') && i <= 12) {
      lcd.setCursor(i, 1);
      lcd.print(c);
      setting_Value7[i - 6] = c;
      i = i + 1;
      lcd.setCursor(i, 1);
      valuecount++;
      delay(100);
    }

    if ((s == 86 || s == 87) && c == PS2_DELETE && (valuecount > 0 || strcmp(setting_Value7, "") != 0)) {
      s = 86;
      c = 0;
      i = 6;
      valuecount = 0;
      memset(setting_Value7, 0, sizeof(setting_Value7));
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ENTER CAPSULE GCV");
      if (strcmp(setting_Value7, "") != 0) {
        lcd.setCursor(i, 1);
        lcd.print(setting_Value7);
      }
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      lcd.setCursor(8, 3);
      lcd.print(">CONTINUE");
      delay(100);
    }

    else if (s == 86 && c == PS2_ENTER) {
      s = 80;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      writeFile(SD, "/settings/capsule_gcv.txt", setting_Value7);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(1, 2);
      lcd.print(">CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }

    else if (s == 87 && c == PS2_ENTER) {
      s = 80;
      c = 0;
      b = 0;
      a = 0;
      i = 7;
      memset(setting_Value7, 0, sizeof(setting_Value7));
      readFileIntoBuffer("/settings/capsule_gcv.txt", setting_Value7, 13);
      lcd.clear();
      lcd.setCursor(2, 0);
      lcd.print("COTTON GCV");
      lcd.setCursor(2, 1);
      lcd.print("FUSE WIRE GCV");
      lcd.setCursor(1, 2);
      lcd.print(">CAPSULE GCV");
      lcd.setCursor(2, 3);
      lcd.print("BACK");
      delay(100);
    }
  }
}
