#include "DHT.h" //sensor lib
#include <LiquidCrystal.h> //LCD lib
#include <Wire.h> //lib to communocate with I2C/TWI devices
#include "SD.h" //logging lib
#include "RTClib.h" //clock lib
#include <SdFat.h> //lib needed to create a CSV file with a correct time/date

RTC_PCF8523 rtc; // define the Real Time Clock object

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

LiquidCrystal lcd(9, 8, 5, 4, 3, 2); //LCD display pins - don't use 10, 11, 12, or 13: used by data logger

#define LOG_INTERVAL  60000 // mills between entries
#define SYNC_INTERVAL 60000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define DHTPIN 7     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE); //initialize DHT sensor

const int chipSelect = 10; //data logging shield digital pin (also uses 11, 12, and 13)

File logfile; //the logging file

void error(char *str) //error function if problem with SD card
{
  Serial.print("error: ");
  Serial.println(str);

  while (1);
}

void dateTime(uint16_t* date, uint16_t* time) { //function to create an accurately timed/dated file
  DateTime now = rtc.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

void setup()
{
  Serial.begin(9600);

  lcd.begin(16, 2); //number of columns and rows of the lcd display

  Serial.print("Initializing SD card...");
  pinMode(10, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return;
  }
  Serial.println("card initialized.");

  char filename[] = "LOGGER00.CSV"; //creates a file
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (! SD.exists(filename)) {
      SdFile::dateTimeCallback(dateTime);  //properly time/date the new file
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }

  if (! logfile) {
    char moo1 [] = "Card failed, or not present" ;
    error(moo1);
  }

  Serial.print("Logging to: ");
  Serial.println(filename);


  dht.begin(); //initiate the sensor
  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }

  Wire.begin(); //initiate communication with the clock
  if (! rtc.begin()) {
    Serial.println("RTC failed");
  }

  DateTime now = rtc.now();  //measure the time and date

  logfile.print(now.year(), DEC);  //print the following in the new file. once.
  logfile.print('/');
  logfile.print(now.month(), DEC);
  logfile.print('/');
  logfile.print(now.day(), DEC);
  logfile.print(" - ");
  logfile.print(daysOfTheWeek[now.dayOfTheWeek()]);
  logfile.print(", ");
  logfile.println("Time,Temperature (C),Humidity (%),Heat Index (C)");

}

void loop() {

  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);

  lcd.print("T=");  //display the following info on the LCD
  lcd.setCursor(2, 0);
  lcd.print(t);
  lcd.print(" Celsius");
  lcd.print("                  ");
  lcd.setCursor(0, 1);
  lcd.print("Humid=");
  lcd.print(h);
  lcd.print("%");
  lcd.print("                  ");

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");

    return;
  }

  DateTime now = rtc.now();  //measure the time

  logfile.print(", ");  //DONT FORGET THE COMAS TO CHANGE CELLS IN THE CSV FILE
  logfile.print(now.hour(), DEC);
  logfile.print(':');
  logfile.print(now.minute(), DEC);
  logfile.print(':');
  logfile.print(now.second(), DEC);

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  logfile.print(", ");
  logfile.print(t);
  logfile.print(", ");
  logfile.print(h);
  logfile.print(", ");
  logfile.print(hic);

  logfile.println();

  if ((millis() - syncTime) < SYNC_INTERVAL) return; //NOT SURE WHAT THIS DOES
  syncTime = millis();

  logfile.flush();  //Actually writes in the file

  Serial.println("Data written in file...");

}
