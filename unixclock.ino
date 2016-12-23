/*************************************************** 
 *  
 *  Makes a pretty clock that shows UNIX time or the date
 * 
 ****************************************************/

#include <string.h>
#include <stdlib.h>
// For i2C
#include <Wire.h>
// Date and time functions using a DS1307 RTC connected via I2C and Wire lib
#include "RTClib.h"

// For the LED driver
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>

//#include <DateTime.h>
//#include <DateTimeStrings.h>

#ifndef _BV
  #define _BV(bit) (1<<(bit))
#endif
const int  buttonPin = 7;    // the pin that the pushbutton is attached to
const int  MODE_UNIX = 0;
const int  MODE_DATE = 1;
const int  MODE_LOCAL = 2;
const int  MODE_UTC = 3; 

// button 
int buttonMode = 0;   // counter for the number of button presses
int buttonState = 0;         // current state of the button
int lastButtonState = 0;     // previous state of the button


RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();

uint8_t counter = 0;

void setup() {
   // initialize the button pin as a input:
  pinMode(buttonPin, INPUT);
  Wire.begin();
  Serial.begin(9600);
  //while (!Serial);             // Arduino micro: wait for serial monitor COMMENT ON RELEASE
  Serial.println("Start");
  byte error, address;
  int rtcaddr;
  int ledmatrixaddr;
  int nDevices;
  initcharmap();
  
  Serial.println("Scanning...");
  
  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    // I kept getting the backpack changing addressess for some reason so autodetecting it
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
       if (address<16) {
        Serial.print("0");
       }
         
        
      Serial.print(address,HEX);
      Serial.println("  !");
       
      if (address == 104)
        {
          Serial.println("RTC found at 0x68");
          rtcaddr = address;
        }else if (address>111){
        ledmatrixaddr = address;
        Serial.println("Using this address for matrix");
      }
      
      

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  Serial.println("");
  Serial.println("HT16K33 init");
  
  matrix.begin(ledmatrixaddr);  // pass in the address
  int i;
    for ( i = 0; i<8; i++)
    {
      matrix.displaybuffer[i ] = 65535; // blankscreen
    }
    matrix.writeDisplay();
   if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    writestr("no rtc", 6);
    matrix.writeDisplay();
    while (1);
  }

   if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
     writestr("bad rtc", 7);
    matrix.writeDisplay();
    delay(1000);

  }
  Serial.println("End setp");
}


uint8_t charmap[128] = {0};
void initcharmap()
{ /*  This is how my 7 seg displays are wired
        ---1---
       |       |
      32       2
       |       | 
        --64--
       |       |
       16      4
        ---8---   *128
        ---A---
       |       |
      F       B
       |       | 
        --G--
       |       |
       E      C
        ---D---   *H
  
           c       87654321 msbf
                   HGFEDCBA */
  charmap['0'] = 0b00111111;
  charmap['1'] = 0b00000110;
  charmap['2'] = 0b01011011;
  charmap['3'] = 0b01001111;
  charmap['4'] = 0b01100110;
  charmap['5'] = 0b01101101;
  charmap['6'] = 0b01111101;
  charmap['7'] = 0b00000111;
  charmap['8'] = 0b01111111;
  charmap['9'] = 0b01101111;
  charmap['A'] = 0b01110111;
  charmap['a'] = 0b01011111;
  charmap['b'] = 0b01111100;
  charmap['c'] = 0b01011000;
  charmap['C'] = 0b00111001;
  charmap['d'] = 0b01011110;
  charmap['e'] = 0b01111011;
  charmap['E'] = 0b01111001;
  charmap['F'] = 0b01110001;
  charmap['G'] = 0b00111101;
  charmap['g'] = 0b01101111;
  charmap['H'] = 0b01110110;
  charmap['h'] = 0b01110100;
  charmap['I'] = 0b00110000;
  charmap['i'] = 0b00110000;
  charmap['J'] = 0b00011111;
  charmap['j'] = 0b00001110;
  charmap['L'] = 0b00111000;
  charmap['N'] = 0b00110111;
  charmap['n'] = 0b01010100;
  charmap['o'] = 0b01011100;
  charmap['p'] = 0b01110011;
  charmap['r'] = 0b00110001;
  charmap['t'] = 0b01111000;
  charmap['U'] = 0b00111110;
  charmap['u'] = 0b00011100;
  charmap['y'] = 0b01110010;
  charmap['-'] = 0b01000000;
  charmap['!'] = 0b10000010;
}
void writestr(char * s, int l){
  /* using the HTK33 I can write upto 16 chars onto 7 segment displays. 
   *  
   *The first 4 use A0-A7 C0-C3. Second A0-A7 C4-7. 
   *Third used      A8-A15 C0-C3 Fourth A8-15 C4-7
   *
   *
    */
    uint16_t dispybuf[16] = {0};
    bool bitshift = 0;
    int i;
    /*for ( i = 0; i<8; i++)
    {
      matrix.displaybuffer[i ] = 0; // blankscreen
    }*/
    if (l > 16){
      l = 16;
    }
    if (l < 0){
      l = 0;
    }
    for ( i = 0; i<l; i++)
    {
      if (i > 7)
      {
        bitshift = 1;
      }else{
        bitshift = 0;
      }
      
      dispybuf[i % 8] = dispybuf[i % 8] | (charmap[s[i]] << (8*bitshift));
      /*
      Serial.print(s[i]);
      Serial.print(i, DEC);
      Serial.write(":  ");
      
      Serial.print(dispybuf[i % 8], BIN);
      Serial.print(" ");
      */
      
    }
    for ( i = 0; i<8; i++)
    {
      //Serial.print(dispybuf[i]);
      matrix.displaybuffer[i ] = dispybuf[i]; // write to mem
    }
    
    
    
    
}

void read_button() {
  // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button
      // wend from off to on:
      buttonMode++;
      if (buttonMode == 4){
       buttonMode = 0;
      }
      Serial.println(buttonMode);
    } else {
      // if the current state is LOW then the button
      // wend from on to off:
      Serial.println("off");
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state,
  //for next time through the loop
  lastButtonState = buttonState;




}


char *i2str(int i, char *buf){
  byte l=0;
  if(i<0) buf[l++]='-';
  boolean leadingZ=true;
  for(int div=10000, mod=0; div>0; div/=10){
    mod=i%div;
    i/=div;
    if(!leadingZ || i!=0){
       leadingZ=false;
       buf[l++]=i+'0';
    }
    i=mod;
  }
  buf[l]=0;
  return buf;
}

void loop() {
  read_button();
  
  if (! rtc.isrunning()) {
      writestr("bad rtc", 7);
    matrix.writeDisplay();
    delay(1000);
  }
  DateTime now = rtc.now();
  
  //char teststring[] = "tHi5 i5 A tE5t 0F the LEd 75EG di5pLay! -1234567890!--    ";
  //char teststring[] = "1234567890AbCdEF ";
  
  char buf[16];
  memset(buf, 0, sizeof buf);
  //itoa(now.unixtime(), buf, 10);
  if (buttonMode == MODE_UNIX){
    // UNIX MODE
        String tstr = String(now.unixtime());
        tstr.toCharArray(buf, 16);
        // i want to move it up 2
        
        for(int i = 0; i<2; i++){
          char c = 0;
          char d = 0;
          for(int j = 0; j<16; j++){
            d = buf[j];
            buf[j] = c;
            c = d;
          }
        }
      writestr(buf, 16);
  }else if (buttonMode == MODE_DATE){
    String tstr = String(now.year());
    tstr = tstr+ " ";
    tstr = tstr+ now.month();
    tstr = tstr+ " ";
    tstr = tstr+ now.day();
    tstr.toCharArray(buf, 16);
    writestr(buf, 16);
  }else if (buttonMode == MODE_LOCAL){
    String tstr = String("");
    if (now.hour()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+String(now.hour());
    if (now.minute()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+ String(now.minute());
    if (now.second()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+ String(now.second());
    tstr = tstr+ String("   utc");
    tstr.toCharArray(buf, 16);
    
   
    writestr(buf, 16);
    matrix.displaybuffer[1] = matrix.displaybuffer[1] | 128; //dots
    matrix.displaybuffer[3] = matrix.displaybuffer[3] | 128;
    
  }else if (buttonMode == MODE_UTC){
    DateTime past (now + TimeSpan(0,1,0,0));
    String tstr = String("");
    if (past.hour()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+String(past.hour());
    if (past.minute()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+ String(past.minute());
    if (past.second()<10){
    tstr = tstr+  String("0");
    }
    tstr = tstr+ String(past.second());
    tstr = tstr+ String("   cet");
    tstr.toCharArray(buf, 16);
    writestr(buf, 16);
    matrix.displaybuffer[1] = matrix.displaybuffer[1] | 128; //dots
    matrix.displaybuffer[3] = matrix.displaybuffer[3] | 128;
    
  }

  
  
  //matrix.displaybuffer[counter % 8 ] = matrix.displaybuffer[counter % 8 ] | 128 <<((counter/8)*8);
  /*
  Serial.println("");
  Serial.print(counter, DEC);
  Serial.print("/");
  Serial.print(counter % 8, DEC);
  Serial.print("/");
  Serial.print((counter/8)*8, DEC);
  Serial.print("/");
  Serial.print(teststring[counter]);
  Serial.println("");
  */
  // write the changes we just made to the display
  matrix.writeDisplay();
  
  delay(250);
  Serial.println(buf);
  //Serial.println(strlen(teststring));
  //counter++;
  //if (counter >= strlen(teststring)) counter = 0;  
}
