//A2
//IO Dashboard: https://io.adafruit.com/colej5/dashboards/a2
//This code uses a light sensor, location API, and IO dashboard together to
//output some basic information.

#include <Adafruit_TSL2591.h> //light sensor
#include <Wire.h>
#include <Adafruit_Sensor.h> //light sensor
#include <ESP8266WiFi.h> //wifi
#include <ESP8266HTTPClient.h> //wifi
#include "config.h" //IO
#include <ArduinoJson.h> //provides the ability to parse and construct JSON objects

//WiFi, password, and key for API use
const char* ssid = "Sif";
const char* pass = "appletree3";
const char* key = "4eedd73e52133aedc410d57c74cb697f";

//typedef struct, specialized to hold the information retrieved by API calls
typedef struct {
  String city;
  String country;
} ApiInfo;
ApiInfo location;

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass in a number for the sensor identifier 

//Initialize API feed elements
AdafruitIO_Feed *light = io.feed("light");
AdafruitIO_Feed *slider = io.feed("slider");
AdafruitIO_Feed *text = io.feed("text");

void setup() {
  
  //This block of code starts up the connection to the Dashboard

  // start the serial connection and output the file name and the time and date
  Serial.begin(115200);
  delay(10);
  Serial.print("This board is running: ");
  Serial.println(F(__FILE__));
  Serial.print("Compiled: ");
  Serial.println(F(__DATE__ " " __TIME__));
  // wait for serial monitor to open
  while(! Serial);
  // initialize dht22
  //dht.begin();
  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //Output that we are connected
  Serial.println();
  Serial.println(io.statusText());

  //This block of code connects to WiFi to work with an API
  
  //We output a "connecting to ___" message, and attempt to connect to WiFi. While we aren't connected,
  //a while loop outputs a "." every half second so we know the machine is in the process of connecting.
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  //Once we connect, we output a confirmation and the device IP address.
  Serial.println(); Serial.println("WiFi connected"); Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());

  //This part of the code checks the light sensor, and configures it

  //make sure our light sensor is working, if not, let the user know something is wrong
    if (tsl.begin()) 
  {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else 
  {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }
  //Configure the sensor
  configureSensor();

  //call our API function
  getApi();

  //when we get a message from the slider on the Dashboard, use this method
  slider->onMessage(message);
  slider->get();
}

void loop() {
  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 100-600 milliseconds! Uncomment whichever of the following you want to read
  uint16_t x = tsl.getLuminosity(TSL2591_VISIBLE);

  //Output location and luminosity information
  Serial.print("Luminosity in ");
  Serial.print(location.city);
  Serial.print(", ");
  Serial.print(location.country);
  Serial.print(": ");
  Serial.println(x, DEC);

  //Send location and luminosity information to Dashboard
  light->save(x);
  String words = location.city;
  words += ", ";
  words += location.country;
  text->save(words);
  delay(2000); 
}

//This method reads the value of a slider on our dashboard and lets us know if it is too low
void message(AdafruitIO_Data *data) {
  // convert the data to integer
  int reading = data->toInt();
  Serial.print(reading);
  Serial.println(" received from slider");

  if (reading == 0) {
    Serial.println("Your slider is low :O");
  }
}

//This method calls an API and collects some information from the JSON output
void getApi() {
  //Start it up
  HTTPClient theClient;
  Serial.println("Making HTTP request");

  //Collect the information from the URL, and check to make sure we haven't encountered any errors.
  theClient.begin("http://api.ipstack.com/" + getIP() + "?access_key=" + key);
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {

      //Print a line that lets us know the correct payload has been received, and organize the information
      //we've grabbed from online in JSON format in the same way we did for getIP().
      Serial.println("Received HTTP payload.");
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      Serial.println("Parsing...");
      JsonObject& root = jsonBuffer.parse(payload);

      // Test if parsing succeeds, and if it didn't, output a notice.
      if (!root.success()) {
        Serial.println("parseObject() failed");
        Serial.println(payload);
        return;
      }

      //Grab some specific info for our location typedef struct
      location.country = root["country_name"].as<String>();
      location.city = root["city"].as<String>();

    //Output a line in case httpCode is the incorrect value.
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
    }
  }
}

//This method grabs our IP address. Used previously in various other assignments
String getIP() {
  HTTPClient theClient;
  String ipAddress;
  theClient.begin("http://api.ipify.org/?format=json");
  int httpCode = theClient.GET();
  if (httpCode > 0) {
    if (httpCode == 200) {
      DynamicJsonBuffer jsonBuffer;
      String payload = theClient.getString();
      JsonObject& root = jsonBuffer.parse(payload);
      ipAddress = root["ip"].as<String>();
    } else {
      Serial.println("Something went wrong with connecting to the endpoint.");
      return "error";
    }
  }
  return ipAddress;
}

//This method sets certain variables for our light sensor when called
void configureSensor(void) {
  tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
}
