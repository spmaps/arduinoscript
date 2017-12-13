#include <OneWire.h> 
#include <DallasTemperature.h>
//LCD SETUP
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>  // F Malpartida's NewLiquidCrystal library
//Download: https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
/*-----( Declare Constants )-----*/
#define I2C_ADDR    0x27  // Define I2C Address for the PCF8574T 
//---(Following are the PCF8574 pin assignments to LCD connections )----
// This are different than earlier/different I2C LCD displays
#define BACKLIGHT_PIN  3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

#define  LED_OFF  1
#define  LED_ON  0

/*-----( Declare objects )-----*/  
LiquidCrystal_I2C  lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
//LCD END


#include <Time.h>

#define ONE_WIRE_BUS_PIN 4
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress Probe01 = { 0x28, 0x2D, 0xB9, 0x7C, 0x05, 0x00, 0x00, 0xD1 }; 
DeviceAddress Probe02 = { 0x28, 0x69, 0xB5, 0x7D, 0x05, 0x00, 0x00, 0xD6 };


int lastSecond = -1;          // need an impossible value for comparison
boolean isWorkingHours = false;  // create boolean to see if working hours of the day
boolean isNightTime = false; // create boolean to set which hours of night we don't need heat

int Relay = 12; //Relay on pin 12
int pirPin = 2; //PIR sensor on digital 2
int led = 13; //internal led on pin 13
boolean isSomeoneHome = false; //create boolean to see if anyone is home downstairs during working hrs

float temp1;
float temp2;
const double setTemp = 20.00;
const double UPPER_OFFSET = 0.5;
const double LOWER_OFFSET = 0.5;
boolean upstairsTooHot = false;
boolean upstairsTooCold = false;

unsigned long startTime = 3600000;
int startHour;
int startMinute;
char buffer[6];

void setup() {
  
  
 setTime(10,35,0,13,12,17); // set time to 10:35 on 13 Dec 2017 (24hr clock)
 pinMode(led, OUTPUT);
 pinMode(pirPin, INPUT);     //Set motion sensor pin as input
 pinMode(Relay, OUTPUT);     //Set Pin12(relay pin) as output
 delay(2000); // it takes the sensor 2 seconds to scan the area around it before it can 
 lcd.begin (20,2);  // initialize the lcd 
// Switch on the backlight
 lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
 lcd.setBacklight(LED_ON);
  
  
 // Initialize the Temperature measurement library
  sensors.begin();
  
  // set the resolution to 10 bit (Can be 9 to 12 bits .. lower is faster)
  sensors.setResolution(Probe01, 10);
  sensors.setResolution(Probe02, 10);

}


void loop() { 
//clear lcd
  lcd.clear();
 delay(1000);
 lcd.home();
 lcd.backlight();  //Backlight ON if under program control

  
  updateTimeParameters();
  updateTemperatures();
 
  
  int pirVal = digitalRead(pirPin); 
  
   if(pirVal == LOW)
    {
    startTime = millis();
    startHour = hour();
    startMinute = minute();
   
     
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);               // wait for a second
   
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);               // wait for a second
  
    }
   
  
  
  if (millis() - startTime < 3600000)
  {
  
   isSomeoneHome = true;
  }
    
    else
    {
      isSomeoneHome = false; 
      
    }
  
  
 sprintf(buffer, "%02d:%02d", (hour()), (minute()));
  
 lcd.setCursor(0,0); //Start at character 0 on line 0
 lcd.print("Time:");
 lcd.setCursor(11,0);
 lcd.print(buffer);
 
 
 lcd.setCursor(0,1); //Start at character 0 on line 1
 lcd.print("S-Time:");
 lcd.setCursor(11,1);


 sprintf(buffer, "%02d:%02d", (startHour), (startMinute));
 
 lcd.print(buffer);
 delay(5000); 
   
 lcd.clear();
 delay(100);
 lcd.home(); 
 
 lcd.setCursor(0,0); //Start at character 0 on line 0
 lcd.print("Upstairs:");
 lcd.setCursor(11,0);
 lcd.print(temp1);
 
 lcd.setCursor(0,1); //Start at character 0 on line 1
 lcd.print("Basement:");
 lcd.setCursor(11,1);
 lcd.print(temp2);
 delay(5000);
  
 
// ssummer script: comes on when upstairs flat is below lower offset and goes off when temp above upper offset  
 if (!isNightTime) 
{ 
   if (upstairsTooCold)
   {
     digitalWrite(Relay, HIGH);
   }
   else if (upstairsTooHot)
   {
     digitalWrite(Relay, LOW);
   }
}

else if (isNightTime)
{
  digitalWrite(Relay, LOW);
}
 
}


void updateTemperatures()
{
  sensors.requestTemperatures();  
  temp1 = sensors.getTempC(Probe01);
  temp2 = sensors.getTempC(Probe02);
 
 
  //Check to see if upstairs flat too hot      
  if (temp1 > (setTemp + UPPER_OFFSET))
  {
    upstairsTooHot = true;
  }
  else
  {
    upstairsTooHot = false;
  }
 
  //Check to see if upstairs flat too cold    
  if (temp1 < (setTemp - LOWER_OFFSET))
  {
    upstairsTooCold = true;
  }  
  else
 { 
  upstairsTooCold = false;
 } 
} 

  
  //find out what time it is and set booleans 'isWorkingHours' and 'isNightTime':
void updateTimeParameters()
{
  
 

    
    if (((hour()) >= 9) && ((hour()) < 19)) //choose the time on the left when the boiler goes off in the daytime, then on the right set the time it comes back on in evening.
    {
      isWorkingHours = true;
    }
    else
    {
      isWorkingHours = false;
    }
    
    if (((hour()) >= 23) || ((hour()) < 7)) //this are hours that boiler is turned off during night. If u want it to go off from midnight to 7am write: (((hour()) >= 0) && ((hour()) < 7))
    {
      isNightTime = true;
    }
    else 
    {
     isNightTime = false;
   }
 
   
}
