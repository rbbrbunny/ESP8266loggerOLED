// include all needed libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ThingSpeak.h>
#include <U8g2lib.h>

// define delay between readings
#define setDelay 30000 // in milliseconds

// define display pins
#define OLED_SDA  2
#define OLED_SCL 14
#define OLED_RST  4

// set display config
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA , OLED_RST);

// set credentials
const char *ssid = "WIFI_SSID_GOES_HERE";
const char *password = "WIFI_PASS_GOES_HERE";
unsigned long myChannelNumber = THINGSPEAK_CHANNEL_GOES_HERE;
const char *myWriteAPIKey = "THINGSPEAK_API_KEY_GOES_HERE";

// set the delimiters for our quote
const char *indexOneChar = "oneliner\">"; // read after this
const int indexOneCharLength = 10;        // length of indexOneChar
const char *indexTwoChar = "</div>";      // stop before this
const char *whatToRemove = "<br />";      // unwanted stuff to be removed
const bool wantToRemoveStuff = true;      // if we want to
const int howMuchToRemove = 6;            // then how long is whatToRemove

// set the maximum character amount per line
const int displayLineLength = 25;
const int displaySaveLength = 4; // how much characters to save for the reading

// set the maximum width in pixels for the bottom line reading display
const int displayWidth = 128;

// set the site to connect to and extract the quote to be displayed on the display
const char *whereToConnect = "http://www.onelinerz.net/random-one-liners/1/";

// setup http server
ESP8266WebServer server(80);

// initiating wifi client for ThingSpeak
WiFiClient client;

// container for counting
long timeLast = 0;

// handling of http request for /
void handleRoot(){
  
  server.send(200, "text/plain", String(whereToConnect) + "\n\nis the purveyor of displayed quotes");
}

// handling of wrong directory ie: /somethingNotThere
void handleNotFound(){

  // create string and add all needed stuff to it
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  // add all args in one line
  for(uint8_t i = 0; i < server.args(); i++){
    
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  // print that string
  server.send(404, "text/plain", message);
}


// we initialize stuff here
void setup(void){

  // open serial port
  Serial.begin(115200);

  // connect to wifi
  WiFi.begin(ssid, password);

  // just because
  Serial.println("");

  // wait for connection
  while(WiFi.status() != WL_CONNECTED){

    // display dot every 500ms
    delay(500);
    Serial.print(".");
  }

  // end the dotted line and print ssid and ip
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // check for MDNS
  if(MDNS.begin("esp8266")){
    
    Serial.println("MDNS responder started");
  }

  // initialize root handling
  server.on("/", handleRoot);

  // initialize subdir handling
  // handy trick done there with [](){}
  server.on("/time", [](){
    
    server.send(200, "text/plain", String(millis()));
  });

  // initialize error handling
  server.onNotFound(handleNotFound);

  // and finally initialize the server itself
  server.begin();

  // tell the whole world... hello world! ;)
  Serial.println("HTTP server started");

  // initialize the display
  u8g2.begin();

  // initialize cloud logging
  ThingSpeak.begin(client);
}

// main loop
void loop(void){

  // check if it's time for action or not
  if (millis() - timeLast > setDelay) {

    // to nice to delete, that row length :)
    
    // check the input and save it for later
    // also you could possibly add some more
    // inputs in here or something, you know
    // it's the main stuff. and i need to do
    // it a little bit better soon. with all
    // the bells and whistles and functions.

    // read the freaking sensor
    int reading = analogRead(A0);

    // save the time, so we can compare again, later
    timeLast = millis();

    // send the data to the cloud
    sendTheDataToTheCloud(reading);

    // try and get a quote or some other string of text
    // from specified site and extracted with care
    // also pass the reading to be later displayed
    getDataFromInternet(reading);
  }
  
  // don't forget we have a http server running
  server.handleClient();
}

// handle sending the data to the internet
void sendTheDataToTheCloud(int currentReading){

    // prepare the reading to be sent to the cloud
    // you could utilize more than one field
    ThingSpeak.setField(1, currentReading);
    
    // send that scrumptious data to the cloud
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

// handle retrieving data from internets
void getDataFromInternet(int pipeReading){
  
  // declare an object of class HTTPClient
  HTTPClient http;

  // connect to specified site
  http.begin(whereToConnect);

  // save the GET request response
  int httpCode = http.GET();

  // check if there's any
  if(httpCode > 0){

    // save the tasty tasty data to a string
    String payload = http.getString();

    // search for delimiters and extract text from in between
    int indexOne = payload.indexOf(indexOneChar);
    int indexTwo = payload.indexOf(indexTwoChar, indexOne - 1);
    String tempQuote = payload.substring(indexOne + indexOneCharLength, indexTwo);

    // only from that try and remove stuff, not the whole payload
    int toRemove = tempQuote.indexOf(whatToRemove);

    // if needed remove unwanted stuff
    if(toRemove > 0 && wantToRemoveStuff == true){

      tempQuote.remove(toRemove, howMuchToRemove);
    }

        
    // just for sanity check send it through serial
    Serial.print(String(reading) + " ");
    
    // just because, why not?
    Serial.println(tempQuote);

    // display all that precious data
    displayDataOnDisplay(tempQuote, pipeReading);
  }
  
  // end connection
  http.end();
}

// handle incoming string and display it on OLED display
void displayDataOnDisplay(String incomingData, int currentReading){

  String formattedQuote1 = incomingData.substring(0, displayLineLength - displaySaveLength);
  String formattedQuote2 = incomingData.substring(displayLineLength - displaySaveLength, (displayLineLength * 2) - displaySaveLength);
  String formattedQuote3 = incomingData.substring((displayLineLength * 2) - displaySaveLength, (displayLineLength * 3) - displaySaveLength);
  String formattedQuote4 = incomingData.substring((displayLineLength * 3) - displaySaveLength, (displayLineLength * 4) - displaySaveLength);
  String formattedQuote5 = incomingData.substring((displayLineLength * 4) - displaySaveLength, (displayLineLength * 5) - displaySaveLength);

  // clear display buffer
  u8g2.clearBuffer();

  // set the font to a nice one (and tiny too!)
  u8g2.setFont(u8g2_font_chikita_tf); 

  // set all the data in their places in the buffer
  u8g2.drawStr(0,5,String(currentReading).c_str());
  u8g2.drawStr(25,5,formattedQuote1.c_str());
  u8g2.drawStr(0,11,formattedQuote2.c_str());
  u8g2.drawStr(0,17,formattedQuote3.c_str());
  u8g2.drawStr(0,23,formattedQuote4.c_str());
  u8g2.drawStr(0,29,formattedQuote5.c_str());

  // map the reading to the display width
  int mappedReading = map(currentReading, 0, 1024, 0, displayWidth);

  // draw a box on the bottom of the display with current mapped value
  u8g2.drawBox(0, 30, mappedReading, 2);

  // draw the bloody image
  u8g2.sendBuffer();
}
