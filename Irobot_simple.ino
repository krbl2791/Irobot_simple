#include "Arduino.h"
//The setup function is called once at startup of the sketch
/*
  Oct 9,2013 Jim Isaak (Additional notes added 10/10)
  v2 --
  This program takes single key entries though the serial monitor to the Ardunio,
  then using a Switch-Case statement, initiates a specific action.

  Roomba works with the software serial connection at Baud
  (need to hold down power key on power on for a bit to force it to this mode)

Key strokes in Serial monitor result in actions:
I - Initialization  P - Power Down  R - Turn Right  L- Turn Left F-Go Forward B-Go backwards
Space to stop all motors/drivers, m to download songs, M to play song zero
numbers 0,1,2,3,9 to play songs associated with those numbers
8 to try to do a figure 8 while playing music
(ok, so using the same strategy for songs is a bad idea)
The above all "stop" after a few seconds ... a safety consideration
"(" for circle to the right (does not stop, be ready to use space or pick up Roomba)
")" for circle to the left  (ditto)
C - hit virtual "Clean" button, starts cleaning (ignores commands, may need to start/command again)

Not yet implemented:
s-switch the state of the "spot" 	, d- switch the state of the dirt LED

The nice thing about having a menu of Switch-Case selections is you can test how the
beast really behaves.  Try a new option, and build a personal understanding. as well
as test things (like how many seconds at this speed/curve radius does it take to circle)
Feedback via the USB to the serial monitor provides confirmation of key input,
the default is used two ways -- to catch song requests, and to provide feedback on
key strokes that are not associated with an action.


  The software serial circuit:
 * RX is digital pin 10 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 ================================ end of intro comments =================
  */
#include <SoftwareSerial.h>;

//Drive Commands
 byte Rstop[6]={137,0,0,0,0}; //stopping the drive motors is just a "zero" speed selection

 byte Rright[6]={137,0,100,0,1}; //plus 1 radius means turn in place - right
 byte Rleft[6]={137,0,100,255,255};  //-1 radius means turn in place - left

 byte Rfwd[6]={137,0,100,0x80,0}  ;    //radius hex 8000 is straight
 byte Rback[6]= {137,255,-100,0x80,0}; //But back is not as "reliable" as forward (battery power sensitive?)

 byte RCircleR[6]={137,0,100,0,128}; //Circle Right
 byte RCircleL[6]= {137,0,100,-1,-128}; //Circle Left


 byte Song0[17]={140,0,7,31,60,43,55,55,50,67,45,79,40,91,35,103,30};

 byte Song1[35]={140,1,16, 36,20, 37,20, 0,40,  36,20, 37,20, 0,30,  36,20, 37,20, 0,20,  36,20, 37,20, 0,10,  36,20, 37,20, 0,5,  38,64}; //Jaws
 byte Song9[33]={140,9,15, 36,20, 37,20, 0,65,  36,20, 37,20, 0,60,  36,20, 37,20, 0,55,  36,20, 37,20, 0,50,  36,20, 37,20, 0,45};

 byte Song2[13]={140,2,5,79,48,81,48,77,48,65,50,72,64}; //Strange Encounters 1
 byte Song3[13]={140,3,5,69,48,71,48,67,48,55,48,62,64}; //Strange Encounters 2

 byte Song15[11]={140,15,4,33,16,45,20,43,12,31,40};

 byte RoombaPlay[2] = {141,15};  //Play song 0 to 15
 byte RoombaMotorsOff[2] = {138,0};  //motors off code (brushes, vac)
 byte RoombaOff = 133;
 byte zero = 0;
 int ptr, cnt, cmd;
 char inbuf[128], cin;  //I tried buffering input from Roomba, but that dropped characters, the tight loop works better
 unsigned long t1,t2;
 boolean play = false;

SoftwareSerial mySerial(10, 11); // RX, TX (instance of serial object)

//KRBL
int ddPin=5; //device detect
// End KRBL

// Initialization Code ============================================
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(19200);
  Serial.println("Menu 1");  //tell monitor we are alive

  // set the data rate for the SoftwareSerial port
   mySerial.begin(19200); //Softserial works at 19200!! 530 series runs at 1115200
   ptr=0;  //Pointer in InBuf as needed
   cnt=0;  //counter when needed
   t1=millis();  //T1 to keep track of time intervals
}

// Main Loop ======================================================
void loop() // run over and over
{
  if (mySerial.available())   //First order of business: listen to Roomba
    Serial.write(mySerial.read());   //writes to USB input from soft serial

  t2=millis()-t1;  //T2 is time since last t1 reset
  if (play)
   {
     mySerial.write(RoombaPlay,2);
     delay(3000);
   }


  if (Serial.available())  //check for command from computer USB
  {
  cmd = Serial.read();  //get the character
  switch (cmd)
  {
    case ')':
      CircleRight();
      Serial.println("circle to the right");
    break;
    case '(':
      CircleLeft();
      Serial.println("circle to the left");
    break;
    case ' ':  //space to stop all motors
       StopIt();
    break;
    case '\I':  // Send start and command initialization
    	wakeUp();
    	mySerial.write(128);  //start (goes to Passive)
      delay(20);  //delay 20 milliseconds after state change
      mySerial.write(130);  //Command mode - goes to safe
      mySerial.write(132);  //Full mode This command gives you complete control over Roomba by putting the OI into Full mode, and turning off the cliff, wheel-drop and internal charger safety features.*/

      delay(20);  //delay 20 milliseconds after state change
      Serial.println("Roomba Initialized (start, command)");
    break;
    //-----------switching LEDs
    case 's': //turn on spot LED
      Serial.println("s done");
      mySerial.write(RoombaOff);  //Not the right commands
    break;
    case '\d': // turn on dirt LED
        Serial.println("d done");
      mySerial.write(RoombaOff); //Not the right commands
    break;
    //---------------- Motors & virtual buttons
    case '\C': //turn on clean Operation (turn off alt motors)

      mySerial.write(135); //start clean operation
      mySerial.write(RoombaMotorsOff,2);  //This does NOT work - once you start "Clean" mode, it ignores you
      play=true;                          //It is possible that it would work if you reset to SAFE mode.
      Serial.println("Clean, no brushes done");
    break;
//---------------------------
    case '\P':  // send power down
      Serial.println("P done");
      mySerial.write(RoombaOff);
    break;
      case '\m':  // send music initialization
      mySerial.write(Song0,17);
      mySerial.write(Song1,35);
      mySerial.write(Song9,33);
      mySerial.write(Song2,13);
      mySerial.write(Song3,13);
      mySerial.write(Song15,11);
      Serial.println("Songs Set");
    break;
    case 'M':   //send Music play
      Serial.println("M done");
      mySerial.write(RoombaPlay,2);  //Play
    break;
    case 'R':   //Turn Right
       TurnRight();
    break;
    case 'L':   //Turn left
       TurnLeft();
    break;
     case 'F':   //Turn left
       Go(2);
    break;
    case 'B':   //Turn left
       Back(2);
    break;
    case '8':   //Figure 8
       Fig8();
       Serial.println("so much for 8");
    break;
    default:
      if ((cmd<'0') || (cmd>'9'))
       { Serial.print("Not recognized: ");
       Serial.println(cmd, DEC);  //this give you the ASCII decimal value for the key
       }
       {
        mySerial.write(141);
        mySerial.write(cmd-48);
        Serial.print("Playing Song :");
        Serial.println(cmd-48);
       };
  }}
}
// ============= Useful Control Subroutines ============

// wake up the robot
void wakeUp ()
{
  // setWarningLED(ON);
	// digitalWrite(13, LOW);
	Serial.println("ddPin High and low");
  digitalWrite(ddPin, HIGH);
  delay(100);
  digitalWrite(ddPin, LOW);
  delay(500);
  digitalWrite(ddPin, HIGH);
  delay(2000);
  digitalWrite(13, LOW);
	Serial.println("ddPin High");
}


void TurnRight()
{
       mySerial.write(Rright,6);
       Serial.println("turning right");
       delay(2000);
       mySerial.write(Rstop,6);
}

void TurnLeft()
{
       mySerial.write(Rleft,6);
       Serial.println("turning left");
       delay(2000);
       mySerial.write(Rstop,6);
}
void Go(int seconds)  //Go forwward for some number of seconds
{

       mySerial.write(Rfwd,6);
       Serial.println("going forward");
       delay(seconds*1000);  //beware: using delay means that "space" won't stop it!
       mySerial.write(Rstop,6);
}
void Back(int seconds)
{
       mySerial.write(Rback,6);
       Serial.println("Backing up");
       delay(seconds*1000);
       mySerial.write(Rstop,6);
}
void Fig8()
{
 int t8 = 4500;
 TurnLeft();
 PlayIt(2);
 CircleRight();
 PlayIt(2);
 delay(t8);
 CircleRight();
 PlayIt(3);
 delay(t8);
 Go(1);   //=====================
 TurnRight();
 PlayIt(9);
 CircleLeft();
 delay(t8);
 PlayIt(1);
 CircleLeft();
 delay(t8);
 Go(1);
 TurnLeft();
 Go(2);
 TurnRight();
 StopIt();
}
void CircleRight()
{
  mySerial.write(RCircleR,6);
}
void CircleLeft()
{
  mySerial.write(RCircleL,6);
}
void StopIt()
{
      mySerial.write(Rstop,6);
      mySerial.write(RoombaMotorsOff,2);
      Serial.println("All Stopped");
}
void PlayIt(byte s)
{
 mySerial.write(141);
 mySerial.write(s);
}
