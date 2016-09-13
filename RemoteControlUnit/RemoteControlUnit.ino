#include <RH_RF69.h> //From: http://www.airspayce.com/mikem/arduino/RadioHead/
#include <SPI.h>
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <avr/wdt.h> //We need watch dog for this program

#define ENCRYPTKEY    "CARROTANDTHEASS!" // Use the same 16-byte key on all nodes

#define BUTTON_RED 9
#define BUTTON_GND 8
#define BUTTON_GRN 7
#define BUTTON_YLW 6

#define LED_RED 5
#define LED_YLW 4
#define LED_GRN 3

//Define the various system states
#define RED 'R'
#define YELLOW 'Y'
#define GREEN 'G'
#define DISCONNECTED 'D'

#define CHECKIN_PERIOD 25L
#define BLOCKING_WAIT_TIME 10
#define MAX_DELIVERY_FAILURES 3
byte failCount = 0;

#define DEBUG false

volatile char systemState;

unsigned long lastBlink = 0;
#define BLINK_RATE 500 //Amount of milliseconds for LEDs to toggle when disconnected

RH_RF69 radio;

SimpleTimer timer;
long secondTimerID;

void setup(){
  wdt_reset();
  wdt_disable();
  
  Serial.begin(9600);
  
  
  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_YLW, INPUT_PULLUP);
  pinMode(BUTTON_GRN, INPUT_PULLUP);
  pinMode(BUTTON_GND, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YLW, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  digitalWrite(BUTTON_GND, LOW);
  
  systemState = RED; //On power up start in red state
  setLED(LED_RED);

  secondTimerID = timer.setInterval(CHECKIN_PERIOD, checkIn);
  
  if(!radio.init()) Serial.println("init failed");
  if(!radio.setFrequency(434.0)) Serial.println("Set Freq failed");
  radio.setTxPower(20);
  radio.setEncryptionKey((uint8_t*)ENCRYPTKEY);
  
  Serial.println("Remote Control Unit Ready");
}

void loop(){
  timer.run();
  
  if(!DEBUG){
    if(digitalRead(BUTTON_RED) == HIGH) //Top priority (Red is NC to ground so high = pressed)
    {
      systemState = RED;
      setLED(LED_RED); //Turn on LED
      //Check the special case of hitting all three buttons
      if (digitalRead(BUTTON_YLW) == LOW && digitalRead(BUTTON_GRN) == LOW) shutDown(); //Go into low power sleep mode
    }
    else if (digitalRead(BUTTON_YLW) == LOW)
    {
      systemState = YELLOW;
      setLED(LED_YLW); //Turn on LED
    }
    else if (digitalRead(BUTTON_GRN) == LOW)
    {
      systemState = GREEN;
      setLED(LED_GRN); //Turn on LED
    }
  }
  
  if (Serial.available() > 0){
    char input = Serial.read();
    if(input == RED){  
      systemState = RED;
      setLED(LED_RED);
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
    }
  }
}

void shutDown(){
  //Might use this for some special command later
  if(DEBUG){
    Serial.println("Shutdown Issued");
  }
}

void checkIn(){
  
  if(systemState == RED)
  {
    sendPacket("R");
  }
  else if (systemState == YELLOW)
  {
    sendPacket("Y");
  }
  else if (systemState == GREEN)
  {
    sendPacket("G");
  }
  else if (systemState == DISCONNECTED)
  {
    if(millis() - lastBlink > BLINK_RATE)
    {
      lastBlink = millis();
      digitalWrite(LED_RED, !digitalRead(LED_RED));
      digitalWrite(LED_YLW, !digitalRead(LED_YLW));
      digitalWrite(LED_GRN, !digitalRead(LED_GRN));
      sendPacket("D"); //Attempt to re-establish connection
    }
  }
}

//Sends a packet
//If we fail to send packet or fail to get a response, time out and go to DISCONNECTED system state
void sendPacket(char* thingToSend)
{
  //Serial.print("Sending: ");
  //Serial.println(thingToSend);
  radio.send((uint8_t*)thingToSend, sizeof(thingToSend));
  radio.waitPacketSent(BLOCKING_WAIT_TIME);
  boolean ackReceived = radio.waitAvailableTimeout(BLOCKING_WAIT_TIME); //wait for response
  
  //Read in any response from jenny
  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  if(radio.recv(buf, &len)){
//    Serial.print("Got ACK: ");
//    Serial.println((char*)buf);
  }
  if(ackReceived){
    failCount = 0;
    if(systemState == DISCONNECTED){
      Serial.println("Connection Reestablished");
      //Serial.print("RSSI: ");
      //Serial.println(radio.RSSI);
      setLED(LED_RED);
      systemState = RED; //Default to stop
    }
  }else{
    //Serial.println("No ACK");
    failCount++;
    if(systemState != DISCONNECTED){
      if(failCount > MAX_DELIVERY_FAILURES){
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_YLW, HIGH);
        digitalWrite(LED_GRN, HIGH);
        systemState = DISCONNECTED;
        Serial.println("Setting Disconnected");
      }
    }
    Serial.print("Fail Count: ");
    Serial.println(failCount);
    radio.setModeIdle(); //This clears the buffer so that rf69.send() does not lock up
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

