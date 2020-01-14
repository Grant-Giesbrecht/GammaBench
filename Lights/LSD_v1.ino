/* Light Strip Display v1
*
*/

#include <FastLED.h>

#define NUM_1A 64
#define NUM_1B 32
#define NUM_2A 16
#define NUM_2B 32
#define NUM_2C 16

//Display program
#define PRGM_WHITE 0
#define PRGM_RAINBOW 1
#define PRGM_RED_BLINK 2

#define LSD_1A 3
#define LSD_1B 4
#define LSD_2A 5
#define LSD_2B 6
#define LSD_2C 7

#define POT_PIN A3

//This is an array of leds.  One item for each led in your strip.
CRGB leds_1A[NUM_1A];
CRGB leds_1B[NUM_1B];
CRGB leds_2A[NUM_2A];
CRGB leds_2B[NUM_2B];
CRGB leds_2C[NUM_2C];

//Get maximum index value
size_t max_idx = NUM_1A;

int program = 0;

//This function sets up the leds and tells the controller about them
void setup() {

    max_idx = (NUM_1B > max_idx)? NUM_1B : max_idx;
    max_idx = (NUM_2A > max_idx)? NUM_2A : max_idx;
    max_idx = (NUM_2B > max_idx)? NUM_2B : max_idx;
    max_idx = (NUM_2C > max_idx)? NUM_2C : max_idx;

    // sanity check delay - allows reprogramming if accidently blowing power w/leds
    delay(2000);

    //Set up library
    FastLED.addLeds<WS2812B, LSD_1A, RGB>(leds_1A, NUM_1A);
    FastLED.addLeds<WS2812B, LSD_1B, RGB>(leds_1B, NUM_1B);
    FastLED.addLeds<WS2812B, LSD_2A, RGB>(leds_2A, NUM_2A);
    FastLED.addLeds<WS2812B, LSD_2B, RGB>(leds_2B, NUM_2B);
    FastLED.addLeds<WS2812B, LSD_2C, RGB>(leds_2C, NUM_2C);
}

float intensity;

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop() {

    for (int i = 0 ; i < 255 ; i++){
        ledAll(i, 0, 0);
        FastLED.show();
        delay(10);
    }
    for (int i = 255 ; i >= 0 ; i--){
        ledAll(i, 0, 0);
        FastLED.show();
        delay(10);
    }
//    delay(500);
//
//    blackout();
//    FastLED.show();
//    delay(500);

//    ledAll(50, 50, 50);
//    ledIntval(0, 0, 150, 2, 0);
//    FastLED.show();
//    delay(1000);
//
//
//    blackout();
//    FastLED.show();
//    delay(1000);

//    ledIntval(250, 0, 0, 3, 0);
//    ledIntval(0, 0, 250, 3, 1);
//    ledIntval(0, 250, 0, 3, 2);
//    FastLED.show();
//    delay(1000);
//
//    ledIntval(250, 0, 0, 3, 1);
//    ledIntval(0, 0, 250, 3, 2);
//    ledIntval(0, 250, 0, 3, 0);
//    FastLED.show();
//    delay(1000);
//
//    ledIntval(250, 0, 0, 3, 2);
//    ledIntval(0, 0, 250, 3, 0);
//    ledIntval(0, 250, 0, 3, 1);
//    FastLED.show();
//    delay(1000);

    // //Read intensity knob
    // intensity = analogRead(POT_PIN)/1024;
    //
    // if (program == PRGM_WHITE){ //All white mode
    //
    // }else if(program == PRGM_RAINBOW){ //Pride mode
    //
    // }else{ //
    //     //Turn error status on
    // }
    //
    // ledAll(0, 0, 150);
    //
    // for(int whiteLed = 0; whiteLed < 20; whiteLed +=2) {
    //     led(50, 50, 50, whiteLed);
    // }
    //
    // // Show the leds (only one of which is set to white, from above)
    // FastLED.show();
    //
    // // Wait a little bit
    // delay(1000);
    //
    // for(int whiteLed = 0; whiteLed < 20; whiteLed ++) {
    //     leds[whiteLed] = CRGB(50, 50, 50);
    // }
    //
    // for(int whiteLed = 0; whiteLed < 20; whiteLed +=2) {
    //     leds[whiteLed] = CRGB(0, 0, 150);
    // }



    // Wait a little bit
//    delay(1000);
}

/*
Writes RGB values to a single LED specified by section and index.
*/
void led(int r, int g, int b, String section, int idx){

    if (section == "1A"){
        if (idx >= NUM_1A) return;
        leds_1A[idx] = CRGB(g, r, b);
    }else if(section == "1B"){
        if (idx >= NUM_1B) return;
        leds_1B[idx] = CRGB(g, r, b);
    }else if(section == "2A"){
        if (idx >= NUM_2A) return;
        leds_2A[idx] = CRGB(g, r, b);
    }else if (section == "2B"){
        if (idx >= NUM_2B) return;
        leds_2B[idx] = CRGB(g, r, b);
    }else if (section == "2C"){
        if (idx >= NUM_2C) return;
        leds_2C[idx] = CRGB(g, r, b);
    }else{
        //do nothing
    }

}

/*
Writes an RGB value to 1 LED (specified by index) in every section.
*/
void led(int r, int g, int b, int idx){
    led(r, g, b, "1A", idx);
    led(r, g, b, "1B", idx);
    led(r, g, b, "2A", idx);
    led(r, g, b, "2B", idx);
    led(r, g, b, "2C", idx);
}

/*
Writes an RGB value to every LED in every section.
*/
void ledAll(int r, int g, int b){
    for (int lidx = 0 ; lidx < max_idx ; lidx++){
        led(r, g, b, lidx);
    }
}

/*
Writes an RGB value to every 'n'th LED in every section, where 'n' is specified
by interval. Offset shifts the first LED back.
*/
void ledIntval(int r, int g, int b, int interval, int offset){

    if (offset < 0){
        offset = 0;
    }

    if (interval < 1){
        interval = 1;
    }

    for (int lidx = offset ; lidx < max_idx ; lidx += interval){
        led(r, g, b, lidx);
    }
}

/*
Turns off all LEDs
*/
void blackout(){
    ledAll(0, 0, 0);
}





void red_slowup_flash(){
  for (int i = 0 ; i < 255 ; i++){
        ledAll(i, 0, 0);
        FastLED.show();
        delay(8);
    }
    delay(500);

    blackout();
    FastLED.show();
    delay(500);
}
