#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "index.h"
#include <WebSocketsServer.h>
#include "LedMap.h"
#include <NTPClient.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <string.h>

#define USE_SERIAL Serial

unsigned int interval = 200;
volatile unsigned int max_on_leds = 2;
volatile int on_leds[number_of_leds];
volatile bool strobe = false;
const int tmr1_write_count = 10000;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

Ticker timer;
unsigned int p_onLeds = 0; 

const long utcOffsetInSeconds = 0;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// Change this to control hardware
void ledOn(int led){
  // char numstr[21];
  // webSocket.sendTXT(0, "On" + (String) itoa(led, numstr, 10) );
  for(int ch=0;ch<4; ch++)
  {
    digitalWrite(ch, (all_leds[led]->State)>>ch&1);
    pinMode(ch, (all_leds[led]->Direction)>>ch&1);
  }
}

void ledsOn(int leds[], int arraySize)
{
  for(int led=0; led<arraySize; led++)
  {
    ledOn(leds[led]);
  }
}

void ledsOn(std::vector<int> leds)
{
  for(int l : leds)
  {
    ledOn(l);
  }
}

// ISR to Fire when Timer is triggered
volatile int interupt_count=0;
void ICACHE_RAM_ATTR onTime() {
  interupt_count++;
  if(strobe && (interupt_count>100)){
    ledOn(number_of_leds-1);
    if(interupt_count>200)interupt_count=0;
    // Re-Arm the timer as using TIM_SINGLE
    timer1_write(tmr1_write_count); //  5 ticks per us
    return;
  }
  ledOn(on_leds[p_onLeds]);
  p_onLeds++;
  if(p_onLeds+1>max_on_leds) p_onLeds = 0;
	// Re-Arm the timer as using TIM_SINGLE
	timer1_write(tmr1_write_count); //  5 ticks per us
}

void sequence_all()
{
  for(int l=0; l<number_of_leds; l++)
  {
    ledOn(l);
    delay(interval);
    // unsigned long start_time = millis();
    // while((millis()-start_time)<interval)yield(); //delay
  }
}

void displayTime()
{
  strobe = false;
  timeClient.update();
  int hours = timeClient.getHours();
  int mins = timeClient.getMinutes();
  if(mins>30)hours++;
  if(hours>11)hours-=12;  // convert from 24h format to 12h. 
  mins = (int) ceil(mins/5.0);
  if(mins>11)mins-=12;
  // int leds[2] = {hours, mins};
  // ledsOn(leds, 2);

  // ISR uses these values to switch between charlieplexing leds. 
  on_leds[0] = hours;
  on_leds[1] = mins;
  max_on_leds = 2;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
      case WStype_DISCONNECTED:
          Serial.printf("[%u] Disconnected!\n", num);
          break;
      case WStype_CONNECTED: {
          IPAddress ip = webSocket.remoteIP(num);
          Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

          // send message to client
          webSocket.sendTXT(num, "Connected");
      }
          break;
      case WStype_TEXT:
          Serial.printf("[%u] get Text: %s\n", num, payload);
          if(payload[0] == 'L') {
              // we get LED Number data
              int led = strtol((const char *) &payload[1], NULL, 10);
              ledOn(led);
          }
          break;
      default:
          break;
  }

}

void display_percentage(int pc)
{
  if(pc>100)pc=100;
  if(pc<0)pc=0;
  // use std::vector to change array size. 
  max_on_leds = (int)floor((pc/100.0) * 12);
  // on_leds_v.clear();
  if(max_on_leds == 0)
  {
    max_on_leds=1; // to avoid index error at runtime
    on_leds[0]=number_of_leds-1; // last value configured for LEDS_OFF
  }else {
    for (int i=0; i<max_on_leds; i++)
    {
      on_leds[i]=i;
      // on_leds_v.push_back(i);
    }
  }
}

void handleArgs()
{
  String message = "Heya!!";
  if (server.arg("PC")=="")
  {
    message = "Please send correct argv: \"PC\"";
  }else if (server.arg("PC")=="HOME"){
    display_percentage(100);
    strobe = true;
  }else{
    display_percentage(std::atoi(server.arg("PC").c_str()));
    strobe = false;
  }
  server.send(200, "text/plain", message);
}


void setup() {
  //GPIO 1 (TX) swap the pin to a GPIO.
  pinMode(1, FUNCTION_3); 
  //GPIO 3 (RX) swap the pin to a GPIO.
  pinMode(3, FUNCTION_3); 
  // Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    timer1_disable();
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // handle index
  server.on("/", []() {
    // send index.html
    server.sendHeader("Content-Encoding", "gzip");
    server.send(200, "text/html", index_html_gz, index_html_gz_len);
  });

  server.on("/setPC", handleArgs);

  server.begin();

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  timeClient.begin();

  display_percentage(90);

  //Initialize Ticker every 0.5s
	timer1_attachInterrupt(onTime); // Add ISR Function
	timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
	/* Dividers:
		TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
		TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
		TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
	Reloads:
		TIM_SINGLE	0 //on interrupt routine you need to write a new value to start the timer again
		TIM_LOOP	1 //on interrupt the counter will start with the same value again
	*/

	// Arm the Timer for our 0.5s Interval
	timer1_write(tmr1_write_count);
}

unsigned long timerStart = millis();
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  webSocket.loop();

  // sequence_all();

  if((millis()-timerStart)>60000) 
  {
    displayTime();
    timerStart = millis();
  }
}
