#include <RFM69.h>
#include <SPI.h>
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <avr/wdt.h> //We need watch dog for this program

#define NETWORKID 42   // Must be the same for all nodes (0 to 255)
#define MYNODEID 2   // My node ID (0 to 255) 1 = RCU | 2 = VCU 
#define TONODEID 1   // Destination node ID (0 to 254, 255 = broadcast)

#define FREQUENCY   RF69_433MHZ

#define ENCRYPT       true // Set to "true" to use encryption
#define ENCRYPTKEY    "CARROTANDTHEASS!" // Use the same 16-byte key on all nodes

#define MAX_TIME_WITHOUT_OK 250L

unsigned long lastCheckin = 0;

RFM69 radio;

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

char systemState;

void setup(){
  wdt_reset();
  wdt_disable();
  
  Serial.begin(9600);
  
  pinMode(RELAY_CONTROL, OUTPUT);
  turnOffRelay(); //During power up turn off power
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YLW, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(PAUSE_PIN, OUTPUT);
  digitalWrite(PAUSE_PIN, LOW); //Resume
  
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower();
  if(ENCRYPT){
    radio.encrypt(ENCRYPTKEY);
  }
  
  systemState = RED; //On power up start in red state
  setLED(LED_RED);

  Serial.println("Jenny Kill Switch Online");
  
  wdt_reset();
  wdt_enable(WDTO_1S);
}

void loop(){
  if(millis() - lastCheckin > MAX_TIME_WITHOUT_OK){
    if(systemState != DISCONNECTED){
      setLED(LED_RED);
      turnOffRelay();
      systemState = DISCONNECTED;
      Serial.println("Remote failed to check in. Turn off Relay");
    }
  }
  
  if(radio.receiveDone()){
    if(radio.ACKRequested())
    {
      radio.sendACK();
      Serial.println("ACK sent");
    }
    if(radio.DATALEN > 1){
      Serial.print("Unknown Message: ");
      for (byte i = 0; i < radio.DATALEN; i++){
        Serial.print((char)radio.DATA[i]);
      }
      Serial.println("");
    }else{
      char message = radio.DATA[0];
      if (message == RED || message == YELLOW || message == GREEN || message == DISCONNECTED) lastCheckin = millis(); //Reset timeout
      
      if(message == RED){
        if(systemState != RED){
          setLED(LED_RED);
          turnOffRelay();
          systemState = RED;
          Serial.println("Kill Jenny");
          Serial.print("RSSI: ");
          Serial.println(radio.RSSI);
        }
      }else if(message == YELLOW){
        if(systemState != YELLOW){
          setLED(LED_YLW);
          digitalWrite(PAUSE_PIN, HIGH);
          systemState = YELLOW;
          Serial.println("Pause");
          Serial.print("RSSI: ");
          Serial.println(radio.RSSI);
        }
      }else if(message == GREEN){
        if(systemState != GREEN){
          setLED(LED_GRN);
          turnOnRelay();
          systemState = GREEN;
          Serial.println("Enabled");
          Serial.print("RSSI: ");
          Serial.println(radio.RSSI);
        }
      }else if(message = DISCONNECTED){
        setLED(LED_RED);
        turnOffRelay();
        systemState = DISCONNECTED;
        Serial.println("Reconnecting");
        Serial.print("RSSI: ");
        Serial.println(radio.RSSI);
      }
    }
  }
}
  
  //Turns on a given LED
void setLED(byte LEDnumber)
{
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_YLW, LOW);
  digitalWrite(LED_GRN, LOW);
  digitalWrite(LEDnumber, HIGH);
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
  
  
  
