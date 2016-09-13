#include <RH_RF69.h> //From: http://www.airspayce.com/mikem/arduino/RadioHead
#include <SPI.h>
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <avr/wdt.h> //We need watch dog for this program

#define ENCRYPTKEY    "CARROTANDTHEASS!" // Use the same 16-byte key on all nodes

#define MAX_TIME_WITHOUT_OK 250L

unsigned long lastCheckin = 0;

RH_RF69 radio;

#define RELAY_CONTROL 9
#define PAUSE_PIN 6

#define LED_RED 5
#define LED_YLW 4
#define LED_GRN 3

//Define the various system states
#define RED 'R'
#define YELLOW 'Y'
#define GREEN 'G'
#define DISCONNECTED 'D'

#define DEBUG false

char systemState;

void setup(){
  Serial.begin(9600);
  
  pinMode(RELAY_CONTROL, OUTPUT);
  turnOffRelay(); //During power up turn off power
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YLW, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(PAUSE_PIN, OUTPUT);
  digitalWrite(PAUSE_PIN, LOW); //Resume
  
  if(!radio.init()) Serial.println("Init Failed");
  if(!radio.setFrequency(434.0)) Serial.println("Set Freq failed");
  radio.setTxPower(20);
  
  radio.setEncryptionKey((uint8_t*)ENCRYPTKEY);
  
  systemState = RED; //On power up start in red state
  setLED(LED_RED);

  Serial.println("Jenny Kill Switch Online");
  
}

void loop(){
  if(!DEBUG){
    if(millis() - lastCheckin > MAX_TIME_WITHOUT_OK){
      if(systemState != DISCONNECTED){
        setLED(LED_RED);
        turnOffRelay();
        systemState = DISCONNECTED;
        Serial.println("Remote failed to check in. Turn off Relay");
      }
    }
    
    if(radio.available()){
      uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if(radio.recv(buf, &len)){
        sendResponse(); //Ack the message
        if (buf[0] == RED || buf[0] == YELLOW || buf[0] == GREEN || buf[0] == DISCONNECTED) lastCheckin = millis(); //Reset timeout
        
        if(buf[0] == RED){
          if(systemState != RED){
            setLED(LED_RED);
            turnOffRelay();
            systemState = RED;
            Serial.println("Kill Jenny");
            Serial.print("RSSI: ");
            Serial.println((radio.lastRssi(), DEC));
          }
        }else if(buf[0] == YELLOW){
          if(systemState != YELLOW){
            setLED(LED_YLW);
            digitalWrite(PAUSE_PIN, HIGH);
            systemState = YELLOW;
            Serial.println("Pause");
            Serial.print("RSSI: ");
            Serial.println((radio.lastRssi(), DEC));
          }
        }else if(buf[0] == GREEN){
          if(systemState != GREEN){
            digitalWrite(PAUSE_PIN, LOW);
            setLED(LED_GRN);
            turnOnRelay();
            systemState = GREEN;
            Serial.println("Enabled");
            Serial.print("RSSI: ");
            Serial.println((radio.lastRssi(), DEC));
          }
        }else if(buf[0] = DISCONNECTED){
          setLED(LED_RED);
          turnOffRelay();
          systemState = DISCONNECTED;
          Serial.println("Reconnecting");
          Serial.print("RSSI: ");
          Serial.println((radio.lastRssi(), DEC));
        }
      }
    }
  }else if(DEBUG){
      if(Serial.available() > 0){
        char input = Serial.read();
        if(input == RED){  
          systemState = RED;
          setLED(LED_RED);
          turnOffRelay();
        }
        else if(input == YELLOW)
        {
          systemState = YELLOW;
          setLED(LED_YLW);
        }
        else if(input == GREEN)
        {
          systemState = GREEN;
          setLED(LED_GRN);
          turnOnRelay();
        }
    }
  }
    
}
  
  //Turns on a given LED
void setLED(byte LEDnumber)
{
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_YLW, HIGH);
  digitalWrite(LED_GRN, HIGH);
  digitalWrite(LEDnumber, LOW);
}

//Turn on the relay
void turnOnRelay()
{
  digitalWrite(RELAY_CONTROL, HIGH);
}

//Turn off the relay
void turnOffRelay()
{
  digitalWrite(RELAY_CONTROL, LOW);
}

//If we receive a system state we send a response
void sendResponse()
{
  uint8_t response[] = "O"; //Send OK
  radio.send(response, sizeof(response));
  radio.waitPacketSent(50); //Block for 50ms before moving on
  Serial.println("Sent a reply");
}
  
  
  
