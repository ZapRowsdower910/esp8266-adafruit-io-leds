/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <FastLED.h>

#define GREENPIN   5
#define BLUEPIN 14
#define REDPIN  0

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       ""
#define WLAN_PASS       ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Store the MQTT server, username, and password in flash memory.
// This is required for using the Adafruit MQTT library.
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//const char PHOTOCELL_FEED[] PROGMEM = AIO_USERNAME "/feeds/something-else";
//Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, PHOTOCELL_FEED);

// Setup a feed called 'onoff' for subscribing to changes.
const char LEDBRIGHTNESS_FEED[] PROGMEM = AIO_USERNAME "/feeds/3dprinter-leds";
Adafruit_MQTT_Subscribe ledBrightness = Adafruit_MQTT_Subscribe(&mqtt, LEDBRIGHTNESS_FEED);

const char LEDCOLOR_FEED[] PROGMEM = AIO_USERNAME "/feeds/leds-color";
Adafruit_MQTT_Subscribe ledColor = Adafruit_MQTT_Subscribe(&mqtt, LEDCOLOR_FEED);

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(9600);

  delay( 3000 ); // power-up safety delay
  
  pinMode(REDPIN,   OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN,  OUTPUT);
  
  Serial.println("testing colors");
  
  showAnalogRGB(CRGB::Red);
  delay(3500);
  showAnalogRGB(CRGB::Green);
  delay(3500);
  showAnalogRGB(CRGB::Blue);

  Serial.println(F("Leds setup, starting WiFi.."));

  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&ledBrightness);
  mqtt.subscribe(&ledColor);

}

uint32_t x = 0;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &ledBrightness) {
      Serial.print(F("Got new brightness: "));
//      currentLedBrightness = atoi((char *)ledBrightness.lastread);
//      FastLED.setBrightness(currentLedBrightness);
//      FastLED.show();
//      Serial.println((char *)ledBrightness.lastread);

    } else if (subscription == &ledColor) {
      char* data = (char*)ledColor.lastread;
      int dataLen = strlen(data);
      Serial.print("Got: ");
      Serial.println(data);
      if (dataLen < 1) {
        // Stop processing if not enough data was received.
        continue;
      }
      
      int r = parseHexByte(&data[0]);
      int g = parseHexByte(&data[2]);
      int b = parseHexByte(&data[4]);
      if ((r < 0) || (g < 0) || (b < 0)) {
        // Couldn't parse the color, stop processing.
        break; 
      }

      showAnalogRGB(CRGB(r, g, b));
//      setLedColor(ledColor.lastread);
      //      currentLedBrightness = atoi((char *)ledBrightness.lastread);
      //      FastLED.setBrightness(currentLedBrightness);
//      Serial.println((char *)ledColor.lastread);
    }
  }

  //  animateLEDs();

  // Now we can publish stuff!
  //  Serial.print(F("\nSending photocell val "));
  //  Serial.print(x);
  //  Serial.print("...");
  //  if (! photocell.publish(x++)) {
  //    Serial.println(F("Failed"));
  //  } else {
  //    Serial.println(F("OK!"));
  //  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

// showAnalogRGB: this is like FastLED.show(), but outputs on 
// analog PWM output pins instead of sending data to an intelligent,
// pixel-addressable LED strip.
// 
// This function takes the incoming RGB values and outputs the values
// on three analog PWM output pins to the r, g, and b values respectively.
void showAnalogRGB(const CRGB& rgb)
{
  Serial.print(F("Red: ")); Serial.println(rgb.r);
  Serial.print(F("Green: ")); Serial.println(rgb.g);
  Serial.print(F("Blue: ")); Serial.println(rgb.b);
  analogWrite(REDPIN,   rgb.r );
  analogWrite(GREENPIN, rgb.g );
  analogWrite(BLUEPIN,  rgb.b );
}

uint8_t charToInt(char data){
  uint8_t result = 0;
  
  if ((data >= '0') && (data <= '9')) {
    result += 16*(data-'0');
  }
  else if ((data >= 'a') && (data <= 'f')) {
    result += 16*(10+(data-'a'));
  }

  return result;
}


// Function to parse a hex byte value from a string.
// The passed in string MUST be at least 2 characters long!
// If the value can't be parsed then -1 is returned, otherwise the
// byte value is returned.
int parseHexByte(char* data) {
  char high = tolower(data[0]);
  char low = tolower(data[1]);
  uint8_t result = 0;
  // Parse the high nibble.
  if ((high >= '0') && (high <= '9')) {
    result += 16*(high-'0');
  }
  else if ((high >= 'a') && (high <= 'f')) {
    result += 16*(10+(high-'a'));
  }
  else {
    // Couldn't parse the high nibble.
    return -1;
  }
  // Parse the low nibble.
  if ((low >= '0') && (low <= '9')) {
    result += low-'0';
  }
  else if ((low >= 'a') && (low <= 'f')) {
    result += 10+(low-'a');
  }
  else {
    // Couldn't parse the low nibble.
    return -1;
  }
  return result;
}
