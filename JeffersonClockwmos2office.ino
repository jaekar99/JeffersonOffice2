//************* Wol Clock by Jon Fuge ******************************

//************* Declare included libraries ******************************
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//************* Declare structures ******************************
//Create structure for LED RGB information
struct RGB {
  byte r, g, b;
};

//Create structure for time information
struct TIME {
  byte Hour, Minute;
};

//************* Editable Options ******************************
//The colour of the "12" to give visual reference to the top
const RGB Twelve = { 40, 0, 5 }; //purple
//The colour of the "quarters" 3, 6 & 9 to give visual reference
const RGB Quarters = { 27, 0, 13 }; //purple
//The colour of the "divisions" 1,2,4,5,7,8,10 & 11 to give visual reference
const RGB Divisions = { 5, 0, 12 }; //purple
//All the other pixels with no information
const RGB Background = { 0, 0, 0 };//blue

//The Hour hand
const RGB Hour = { 0, 70, 0 };//orange maximum
//The Minute hand
const RGB Minute = { 0, 0, 70 };//orange medium
//The Second hand
const RGB Second = { 32, 16, 0 };//orange dim

// Make clock go forwards or backwards (dependant on hardware)
const char ClockGoBackwards = 1;

//Set brightness by time for night and day mode
const TIME WeekNight = {21, 30}; // Night time to go dim
const TIME WeekMorning = {6, 15}; //Morning time to go bright
const TIME WeekendNight = {21, 30}; // Night time to go dim
const TIME WeekendMorning = {9, 30}; //Morning time to go bright

const int day_brightness = 255;
const int night_brightness = 180;

//Set your timezone in hours difference rom GMT
const int hours_Offset_From_GMT = -7;

//Set your wifi details so the board can connect and get the time from the internet
const char *ssid      = "staff";    //  your network SSID (name)
const char *password  = "I need server access!"; // your network password

byte SetClock;

// By default 'time.nist.gov' is used.
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Which pin on the ESP8266 is connected to the NeoPixels?
#define PIN            14 // This is the D5 pin

//************* Declare user functions ******************************
void Draw_Clock(time_t t, byte Phase);
int ClockCorrect(int Pixel);
void SetBrightness(time_t t);
void SetClockFromNTP ();
bool IsDst();

//************* Declare NeoPixel ******************************
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

//************* Setup function for Wol_Clock ******************************
void setup() {
  pixels.begin(); // This initializes the NeoPixel library.
  Draw_Clock(0, 1); // Just draw a blank clock

  WiFi.begin(ssid, password); // Try to connect to WiFi
  Draw_Clock(0, 2); // Draw the clock background

  while ( WiFi.status() != WL_CONNECTED )
    delay ( 500 ); // keep waiging until we successfully connect to the WiFi

  Draw_Clock(0, 3); // Add the quater hour indicators

  SetClockFromNTP(); // get the time from the NTP server with timezone correction
}

void SetClockFromNTP ()
{
  timeClient.update(); // get the time from the NTP server
  setTime(timeClient.getEpochTime()); // Set the system yime from the clock
  if (IsDst())
    adjustTime((hours_Offset_From_GMT + 1) * 3600); // offset the system time with the user defined timezone (3600 seconds in an hour)
  else
    adjustTime(hours_Offset_From_GMT * 3600); // offset the system time with the user defined timezone (3600 seconds in an hour)
}

bool IsDst()
{
  if (month() < 3 || month() > 10)  return false; 
  if (month() > 3 && month() < 10)  return true; 

  int previousSunday = day() - weekday();

  if (month() == 3) return previousSunday >= 24;
  if (month() == 10) return previousSunday < 24;

  return false; // this line never gonna happend
}
        
//************* Main program loop for Wol_Clock ******************************
void loop() {
  time_t t = now(); // Get the current time

  Draw_Clock(t, 4); // Draw the whole clock face with hours minutes and seconds
  if (minute(t) == 0) // at the start of each hour, update the time from the time server
    if (SetClock == 1)
    {
      SetClockFromNTP(); // get the time from the NTP server with timezone correction
      SetClock = 0;
    }
  else
  {
    delay(200); // Just wait for 0.1 seconds
    SetClock = 1;
  }
}

//************* Functions to draw the clock ******************************
void Draw_Clock(time_t t, byte Phase)
{
  if (Phase <= 0)
    for (int i = 0; i < 60; i++)
      pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // for Phase = 0 or less, all pixels are black

  if (Phase >= 1)
    for (int i = 0; i < 60; i++)
      pixels.setPixelColor(i, pixels.Color(Background.r, Background.g, Background.b)); // for Phase = 1 or more, draw minutes with Background colour

  if (Phase >= 2)
    for (int i = 0; i < 60; i = i + 5)
      pixels.setPixelColor(i, pixels.Color(Divisions.r, Divisions.g, Divisions.b)); // for Phase = 2 or more, draw 5 minute divisions

  if (Phase >= 3) {
    for (int i = 0; i < 60; i = i + 15)
      pixels.setPixelColor(ClockCorrect(i), pixels.Color(Quarters.r, Quarters.g, Quarters.b)); // for Phase = 3 or more, draw 15 minute divisions
    pixels.setPixelColor(ClockCorrect(0), pixels.Color(Twelve.r, Twelve.g, Twelve.b)); // for Phase = 3 and above, draw 12 o'clock indicator
  }

  if (Phase >= 4) {
    pixels.setPixelColor(ClockCorrect(second(t)), pixels.Color(Second.r, Second.g, Second.b)); // draw the second hand first
    if (second() % 2)
      pixels.setPixelColor(ClockCorrect(minute(t)), pixels.Color(Minute.r, Minute.g, Minute.b)); // to help identification, minute hand flshes between normal and half intensity
    else
      pixels.setPixelColor(ClockCorrect(minute(t)), pixels.Color(Minute.r / 2, Minute.g / 2, Minute.b / 2)); // lower intensity minute hand

    pixels.setPixelColor(ClockCorrect(((hour(t) % 12) * 5) + minute(t) / 12), pixels.Color(Hour.r, Hour.g, Hour.b)); // draw the hour hand last
  }

  SetBrightness(t); // Set the clock brightness dependant on the time
  pixels.show(); // show all the pixels
}

//************* Function to set the clock brightness ******************************
void SetBrightness(time_t t)
{
  int NowHour = hour(t);
  int NowMinute = minute(t);

  if ((weekday() >= 2) && (weekday() <= 6))
    if ((NowHour > WeekNight.Hour) || ((NowHour == WeekNight.Hour) && (NowMinute >= WeekNight.Minute)) || ((NowHour == WeekMorning.Hour) && (NowMinute <= WeekMorning.Minute)) || (NowHour < WeekMorning.Hour))
      pixels.setBrightness(night_brightness);
    else
      pixels.setBrightness(day_brightness);
  else if ((NowHour > WeekendNight.Hour) || ((NowHour == WeekendNight.Hour) && (NowMinute >= WeekendNight.Minute)) || ((NowHour == WeekendMorning.Hour) && (NowMinute <= WeekendMorning.Minute)) || (NowHour < WeekendMorning.Hour))
    pixels.setBrightness(night_brightness);
  else
    pixels.setBrightness(day_brightness);
}

//************* This function reverses the pixel order ******************************
int ClockCorrect(int Pixel)
{
  if (ClockGoBackwards == 1)
    return ((60 - Pixel +30) % 60); // my first attempt at clock driving had it going backwards :)
  else
    return (Pixel);
}
