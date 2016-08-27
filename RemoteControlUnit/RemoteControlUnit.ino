#include <RFM69.h>
#include <SPI.h>
#include <SimpleTimer.h> //https://github.com/jfturcot/SimpleTimer
#include <avr/wdt.h> //We need watch dog for this program

#define NETWORKID 42   // Must be the same for all nodes (0 to 255)
#define MYNODEID 1   // My node ID (0 to 255) 1 = RCU | 2 = VCU 
#define TONODEID 2   // Destination node ID (0 to 254, 255 = broadcast)

#define FREQUENCY   RF69_433MHZ

#define ENCRYPT       true // Set to "true" to use encryption
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
#define BLOCKING_WAIT_TIME 10L
#define MAX_DELIVERY_FAILURES 3

#define DEBUG true

byte failCount = 0;
char systemState;

unsigned long lastBlink = 0;
#define BLINK_RATE 500 //Amount of milliseconds for LEDs to toggle when disconnected

RFM69 radio;

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
  
  radio.initialize(FREQUENCY, MYNODEID, NETWORKID);
  radio.setHighPower();
  
  if(ENCRYPT){
    radio.encrypt(ENCRYPTKEY);
  }
  
  wdt_reset();
  wdt_enable(WDTO_1S);
  Serial.print("Remote Control Unit Ready");
}

void loop(){
  timer.run();
  wdt_reset();
  
  
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
  wdt_reset();
  
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
  Serial.print("Sending: ");
  Serial.println(thingToSend);
  
  boolean ackReceived = radio.sendWithRetry(TONODEID, thingToSend, 1); //Only send 1 byte messages
  
  if(ackReceived){
    Serial.println("ACK Received");
    failCount = 0;
    if(systemState == DISCONNECTED){
      Serial.println("Connection Reestablished");
      setLED(LED_RED);
      systemState = RED; //Default to stop
    }
  }else{
    Serial.println("No ACK");
    if(systemState != DISCONNECTED){
      if(failCount++ > MAX_DELIVERY_FAILURES){
        failCount = MAX_DELIVERY_FAILURES;
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_YLW, HIGH);
        digitalWrite(LED_GRN, HIGH);
        systemState = DISCONNECTED;
      }
    }
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

