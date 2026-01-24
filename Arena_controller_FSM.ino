//adafruit neopixel library setup
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN        9
#define NUMPIXELS 197
#define team1Length 97
#define team2Length 100


Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 10

  /* welcome to the V1 beetle arena controller code if you are an arena controller here is what you need to know:
  serial input is enabled and can at as an arena controller all on its own! 
  here are the keys you need to know:

  f: fight start, also unpause when in unstick state (state 2)
  spacebar: end match
  r: reset
  u: unstick

note: these are case sensitive
 */

//definitions
  /*code is setup for an arduino mega atm -  if you change you will need to change this variable, set pullup to pull down and change the button
  input side to 3.3v */
  bool ESP = false;

  //color variables
    int red = 0;
    int green = 0;
    int blue = 0;

  //computer inputs
    char receivedChar;
    boolean newData = false;
  
  //declare audio pins
    int audio_Ready_team1 = 3;
    int audio_Ready_team2 = 4;
    int audio_fight_pin = 5;
    int audio_1_minute = 6;
    int audio_30_seconds = 7;
    int audio_fight_end = 8;


  // declare button inputs
    //gnd pins bc im short on time and dont want to solder even more shit
    int team_1_green = 22;
    int team_1_green_gnd = 23;

    int team_2_green = 24;
    int team_2_green_gnd = 25;

    int team_1_red = 26;
    int team_1_red_gnd = 27;

    int team_2_red = 28;
    int team_2_red_gnd = 29;

  //override controller buttons
    //this system just uses an arduino gnd pin for the moment
    int controller_start = 33;
    int controller_pause = 35;
    int controller_stop = 37;

    int controller_timer_add = 32;
    int controller_timer_subtract = 34;
    int controller_reset = 36;

  // tracks game state
    int state = 0;

  // memory storage for ready buttons
    bool ready1 = false;
    bool ready2 = false;

  //memory for timer announcments
    bool min_triggered = false;
    bool half_min_triggered = false;

  // start time of the timer
    long timer_last_millis = 0;
    long current_millis = 0;
    long timer = 0;

  // set match length
    long match_lenght_millis = 60000*3;




void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);
    Serial.println("<Arduino is ready>");

  //audio plays when a pin is pulled low - set all audio control pins high
    pinMode(audio_Ready_team1, OUTPUT);
    pinMode(audio_Ready_team2, OUTPUT);
    pinMode(audio_fight_pin, OUTPUT);
    pinMode(audio_1_minute, OUTPUT);
    pinMode(audio_30_seconds, OUTPUT);
    pinMode(audio_fight_end, OUTPUT); 

    digitalWrite(audio_Ready_team1, HIGH);
    digitalWrite(audio_Ready_team2, HIGH);
    digitalWrite(audio_fight_pin, HIGH);
    digitalWrite(audio_1_minute, HIGH);
    digitalWrite(audio_30_seconds, HIGH);
    digitalWrite(audio_fight_end, HIGH);

    pinMode(team_1_green,INPUT_PULLUP);
    pinMode(team_2_green,INPUT_PULLUP);
    pinMode(team_1_red,INPUT_PULLUP);
    pinMode(team_2_red,INPUT_PULLUP);

  //when using pullup! - arduino 
    pinMode(team_1_green_gnd,OUTPUT);
    pinMode(team_2_green_gnd,OUTPUT);
    pinMode(team_1_red_gnd,OUTPUT);
    pinMode(team_2_red_gnd,OUTPUT);

    pinMode(controller_start,INPUT_PULLUP);
    pinMode(controller_pause,INPUT_PULLUP);
    pinMode(controller_stop,INPUT_PULLUP);

    pinMode(controller_reset,INPUT_PULLUP);
    pinMode(controller_timer_subtract,INPUT_PULLUP);
    pinMode(controller_timer_add,INPUT_PULLUP);

  //led initialization
    #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
      clock_prescale_set(clock_div_1);
    #endif
      pixels.begin();
      
    pixels.setBrightness(255);
    color_fill(255,255,255);
}

void loop() 
{
  recvOneChar();
  //debug statement
  Serial.println(state);
  

  // Finite state machine
  switch (state){
    case  0:
      // start state 0
            
      // team 1 ready up
      if((digitalRead(team_1_green) == ESP) && (ready1 == false))
      {
        ready1 = true;
        pixels.fill(pixels.Color(255,0,0),0,team1Length);
        pixels.show();
        Serial.println("team 1 ready");
        triggerAudio(audio_Ready_team1,3000);
      }

      //team 2 ready up
      if((digitalRead(team_2_green) == ESP) && (ready2 == false))
      {
        ready2 = true;
        pixels.fill(pixels.Color(0,255,0),team1Length,team2Length);
        pixels.show();
        Serial.println("team 2 ready");
        triggerAudio(audio_Ready_team2,3000);
      }

      //check start conditon
      if ((((ready1 == true) && (ready2 == true)) || (digitalRead(controller_start) == ESP)) || (receivedChar == 'f'))
      {
        state = 1;
        // STATE 0->1 transition logic
        Serial.println("0->1 transition start");
        timer_last_millis = millis();
        timer = 0;
        min_triggered = false;
        half_min_triggered = false;

        //reset ready 1 & 2 to be used later
        ready1 = false;
        ready2 = false;
        delay(1000);
        for (int i = 0; i < 3; i++) {
          delay(1000);
          color_fill(255,0,0);
          delay(1000);
          pixels.clear();
          pixels.show();
        }
        triggerAudio(audio_fight_pin,2000);
        Serial.println("match start");
        color_fill(0,0,255);
        delay(2000);
      }
      break;


    case 1:
      //in match - countdown active
      color_fill(255,255,255);
      //delta time implemetation - in short a time tracker
      current_millis = millis();
      timer += current_millis - timer_last_millis;
      timer_last_millis = current_millis;

      //timer debug
      Serial.println(timer/1000);

      if (timer >= match_lenght_millis)
      {
        //if the timer is up transition to match end
      state = 3;
        // state 1->3 transition logic
      triggerAudio(audio_fight_end,1000);
      Serial.println("match time over");
      fightEndAnim();
      }


      //1 minute left
      if ( (timer >= match_lenght_millis-(60000*1)) && (min_triggered == false) )
        {
          min_triggered = true;
          triggerAudio(audio_1_minute,3000);
          Serial.println("1 minute left");
        }

      //30 seconds left
      if ( (timer >= match_lenght_millis-(60000*.5)) && (half_min_triggered == false) )
        {
          half_min_triggered = true;
          triggerAudio(audio_30_seconds,3000);
          Serial.println("30 seconds left");
        }

      //unstick
      if ((digitalRead(team_1_green) == ESP) || (digitalRead(team_2_green) == ESP) || (digitalRead(controller_pause) == ESP)|| (receivedChar == 'u'))
        {
          //pause match for unstick or override
          state = 2;
          //state 1->2 transition logic
          //led to purple
          color_fill(255,255,0);

          Serial.print("fight paused");
          triggerAudio(audio_fight_end,3000);
        }
        

      //tapout
      if ((digitalRead(team_1_red) == ESP) || (digitalRead(team_2_red) == ESP) || (digitalRead(controller_stop) == ESP) || (receivedChar == ' '))
        {
          //end match for tapout or override
          state = 3;
          //state 1->3 transition logic
          Serial.print("tapout");
          triggerAudio(audio_fight_end,1000);
          fightEndAnim();
        }
        break;



    case 2:
      // match paused
      if (digitalRead(controller_start) == ESP || (receivedChar == 'f'))
      {
        // unpause 2->1
        state = 1;
        Serial.print("fight unpaused");

        //redo start sequence
        for (int i = 0; i < 3; i++) {
          delay(1000);
          color_fill(255,0,0);
          delay(1000);
          pixels.clear();
          pixels.show();
        }
        triggerAudio(audio_fight_pin,2000);
        Serial.println("match start");
        color_fill(0,0,255);
        delay(2000);
        timer_last_millis = millis();
        }


      if (digitalRead(controller_stop) == ESP || (receivedChar == ' '))
      {
        // stop match 2->3 transition
        timer_last_millis = millis();
        state = 3;
        Serial.print("controller override stop");
        fightEndAnim();
      }
      break;


    case 3:
      //match complete
      if(digitalRead(controller_reset) == ESP || (receivedChar == 'r'))
      state = 0;
      color_fill(255,255,255);

  }
  showNewData();
  receivedChar = "o";
}

void triggerAudio(int fileNumber,int Tdelay)
{
  digitalWrite(fileNumber, LOW);
  delay(Tdelay);
  digitalWrite(fileNumber, HIGH);
}


void color_fill(int color1, int color2, int color3)
{
  pixels.fill(pixels.Color(color1,color2,color3));
  pixels.show();
}

void recvOneChar() {
    if (Serial.available() > 0) {
        receivedChar = Serial.read();
        Serial.println("recieved");
        newData = true;
    }
}

void showNewData() {
    if (newData == true) {
        Serial.print("This just in ... ");
        Serial.println(receivedChar);
        newData = false;
    }
}

void fightEndAnim(){
  //fight stop anim
    color_fill(255,0,0);
    delay(5000);
}
