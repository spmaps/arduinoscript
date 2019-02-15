#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
YunServer server;

#include <OneWire.h> 
#include <DallasTemperature.h>

#include <Process.h>

Process date;                 // process used to get the date
int hours, minutes, seconds;  // for the results
int lastSecond = -1;


#define ONE_WIRE_BUS_PIN 4
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress Probe01 = { 0x28, 0x2D, 0xB9, 0x7C, 0x05, 0x00, 0x00, 0xD1 }; 
DeviceAddress Probe02 = { 0x28, 0x69, 0xB5, 0x7D, 0x05, 0x00, 0x00, 0xD6 };
DeviceAddress Probe03 = { 0x28, 0xFF, 0x22, 0x97, 0xA1, 0x15, 0x4, 0x7B };


boolean isWorkingHours = false;  // create boolean to see if working hours of the day
boolean isNightTime = false; // create boolean to set which hours of night we don't need heat

int Relay = 12; //Relay on pin 12
int pirPin = 2; //PIR sensor on digital 2
int pirState = LOW; //start by assuming no motion detection
int val = 0; //variable for reading pin status
int led = 13; //internal led on pin 13
boolean isSomeoneHome = false; //create boolean to see if anyone is home downstairs during working hrs
boolean overrideActivated = false; //create boolean to see if anyone is home downstairs during working hrs
boolean disabled = false; // create boolean to record if script has been disabled using disabled button

float temp1;
float temp2; 
float temp3;

const double setTemp = 21.00;
const double UPPER_OFFSET = 0.5;
const double LOWER_OFFSET = 0.5;
boolean basementTooHot = false;
boolean basementTooCold = false;

unsigned long startTime = 3600000;
unsigned long buttonClick = 7200000;

int startHour;
int startMinute;

void setup() {
  Serial.begin(9600);
  // Bridge startup
  Bridge.begin();
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
  
 
 pinMode(12, OUTPUT);
 pinMode(led, OUTPUT);
 pinMode(pirPin, INPUT);     //Set motion sensor pin as input
 pinMode(Relay, OUTPUT);     //Set Pin12(relay pin) as output
  
  
 // Initialize the Temperature measurement library
  sensors.begin();
  
  // set the resolution to 10 bit (Can be 9 to 12 bits .. lower is faster)
  sensors.setResolution(Probe01, 10);
  sensors.setResolution(Probe02, 10);
  sensors.setResolution(Probe03, 10);

}

void loop() {
  updateTimeParameters();
  updateTemperatures();

  Serial.print("printing temps: ");
  Serial.print(temp1);
  Serial.print(temp2);
  Serial.print(temp3);
  


 val = digitalRead(pirPin); 
  
   if(val == HIGH) {
    startTime = millis();
    startHour = hours;
    startMinute = minutes;      
   
 // digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  if (pirState == LOW) {
      pirState = HIGH;
    }
} else {
  //  digitalWrite(led, LOW);   // turn the LED on (HIGH is the voltage level)
  if (pirState == HIGH) {
    pirState = LOW;
  }
}

   // set both booleans
  if (millis() - startTime < 3600000)
  {
   isSomeoneHome = true;
  }
    
    else
    {
      isSomeoneHome = false; 
    }

    if (millis() - buttonClick < 7200000)
    {
      overrideActivated = true;
    }
    else
    {
      overrideActivated = false;
    }
    
    
 if (!isNightTime && !overrideActivated && !disabled) 
{ 
   if ((!isWorkingHours) && (!basementTooHot))  
   {
     digitalWrite(Relay, HIGH);
   }
   else if ((!basementTooHot) && (isWorkingHours) && (isSomeoneHome))
   {
     digitalWrite(Relay, HIGH);
   }
   else
   {
     digitalWrite(Relay, LOW);
   }
}

else if (isNightTime && !overrideActivated && !disabled)
{
  digitalWrite(Relay, LOW);
}

  // Get clients coming from server
  BridgeClient client = server.accept();

  // There is a new client?
    if (client) {
    // Process request
    process(client);
  // Close bridge connection and free resources.
    client.stop();
  }
 
}


void updateTemperatures()
{
  sensors.requestTemperatures();  
  temp1 = sensors.getTempC(Probe01);
  temp2 = sensors.getTempC(Probe02);
  temp3 = sensors.getTempC(Probe03);

 
 
  //Check to see if basement (temp2) too hot - (temporarily swapped for living room [temp1])      
  if (temp1 > (setTemp + UPPER_OFFSET))
  {
    basementTooHot = true;
  }
  else
  {
    basementTooHot = false;
  }
 
  //Check to see if basement (temp2) too cold (temporarily swapped for living rm[temp1])   
  if (temp1 < (setTemp - LOWER_OFFSET))
  {
    basementTooCold = true;
  }  
  else
 { 
  basementTooCold = false;
 } 
} 

  
  //find out what time it is and set booleans 'isWorkingHours' and 'isNightTime':
void updateTimeParameters()
{

 // start the date process:
  if (!date.running())  {
    date.begin("date");
    date.addParameter("+%T");
    date.run();


    while (date.available() > 0) {
      // get the result of the date process (should be hh:mm:ss):
      String timeString = date.readString();

      // find the colons:
      int firstColon = timeString.indexOf(":");
      int secondColon = timeString.lastIndexOf(":");

      // get the substrings for hour, minute second:
      String hourString = timeString.substring(0, firstColon);
      String minString = timeString.substring(firstColon + 1, secondColon);
      String secString = timeString.substring(secondColon + 1);

      // convert to ints,saving the previous second:
      hours = hourString.toInt();
      minutes = minString.toInt();
      lastSecond = seconds;          // save to do a time comparison
      seconds = secString.toInt();

  
    if ((hours >= 9) && (hours < 18)) //choose the time on the left when the boiler goes off in the daytime, then on the right set the time it comes back on in evening.
    {
      isWorkingHours = true;
    }
    else
    {
      isWorkingHours = false;
    }
    
    if ((hours >= 23) || (hours < 6)) //this are hours that boiler is turned off during night. If u want it to go off from midnight to 7am write: (((hour()) >= 0) && ((hour()) < 7))
    {
      isNightTime = true;
    }
    else 
    {
     isNightTime = false;
   }
}
  }
}

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('/');


  // is "digital" command?
  if (command == "digital") {
    int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
        
  } else {
    value = digitalRead(pin);
  }

// Send feedback to client
 client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);


  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
  }

// is 'override' command?
if (command == "override") {
  int v;
    v = client.parseInt();
    if (v == 1) {
     digitalWrite(Relay, HIGH);
     buttonClick = millis();
    }
    else if (v == 0) {
           digitalWrite(Relay, LOW);
           buttonClick = millis();
    }
}

// is 'disable' command?
if (command == "disable") {
  int x;
    x = client.parseInt();
    if (x == 1) {
     disabled = true;
     digitalWrite(Relay, LOW);
    }
    else if (x == 0) {
           disabled = false;
    }
}
  
  // is 'temperature' command?
   if (command == "temp") {
    int room;
 // updateTemperatures();

  // Read room number
  room = client.parseInt();
  
  if (room == 1) {
     client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
      client.print(temp1);
      client.print(" °C");
     }  
  if (room == 2) {
    client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
      client.print(temp2);
      client.print(" °C");
     }
   if (room == 3) {
        client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
      client.print(temp3);
      client.print(" °C");
     }
  }
    // is 'timenow' command?
   if (command == "timenow") {
     client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
    client.print("Time now: ");
    client.print(hours);
    client.print("h");
    client.print(minutes);
    client.print("m");
  }
     // is 'stime' command?
   if (command == "stime") {
     client.println("Status: 200");
  client.println("Access-Control-Allow-Origin: *");   
  client.println("Access-Control-Allow-Methods: GET");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
    client.print("STime: ");
    client.print(startHour);
    client.print("h");
    client.print(startMinute);
    client.print("m");
    client.print(startTime);
  }
    
  }
