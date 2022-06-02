/*
    author: atteAhma
    can be used anywhere without restriction
*/

#include <FastLED.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#define LED_DIST 5/8
#define GRID_DIST 1.5
#define BORDER_DIST 0.28

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    576
CRGB leds[NUM_LEDS];
int mapper[36];

#define BRIGHTNESS         200
#define FRAMES_PER_SECOND  

float indexToDeltaInches(uint16_t index)
{
    float deltaInches = ((float) index) * LED_DIST;
    return deltaInches;
}

float deltaInchesToAbsoluteInches(float deltaInches)
{
    float absoluteInches = deltaInches + (GRID_DIST/2);
    return absoluteInches;
}

bool tooCloseToGrid(float absoluteInches)
{
    float relGridInches = fmod(absoluteInches, GRID_DIST);

    if ( (relGridInches < BORDER_DIST) || (GRID_DIST - relGridInches < BORDER_DIST) )
    {
        // too close to border
        return true;
    }

    return false;
}

uint16_t deltaInchesToOldIndex(float deltaInches)
{
    float gridIndexFloat = deltaInches / GRID_DIST;
    uint16_t gridIndex = round(gridIndexFloat);
    return gridIndex;
}

// THIS IS THE TOP LEVEL FUNCTION, ONLY CALL THIS
uint16_t indexToSampleIndex(uint16_t index)
{
    float deltaInches;
    float absoluteInches;
    bool isTooCloseToGrid;
    uint16_t oldIndex;

    deltaInches = indexToDeltaInches(index);
    absoluteInches = deltaInchesToAbsoluteInches(deltaInches);
    isTooCloseToGrid = tooCloseToGrid(absoluteInches);

    if ( isTooCloseToGrid )
    {
        return (uint16_t) -1;
    }

    oldIndex = deltaInchesToOldIndex(deltaInches);

    return oldIndex;
}
void setup()
{
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

    uint16_t sampleIndex;
    Serial.begin(9600);
    for( uint16_t i = 0 ; i < 36; i++ )
    {
        mapper[i] = indexToSampleIndex(i);
    }
    mapper[35] = 15;
    mapper[2] = 255;
    mapper[34]= 255;
    
}

int counter = 0l;
int hi = 0;
void loop() {
    for (int i = 0; i < 256; i++) {
      hi = (hi + random(1,5)) % 256;
      CHSV color = CHSV(hi, 255, 200);
      mapToGrid(i,color); 
    FastLED.show();

    delay(25);

    }

//    
//    for (int i = starting_led; i < starting_led + 36; i++) {
//      int row = i / 36;
//      int index = i % 36;
//      if ((row % 2) == 1) {
//         if (((15 - mapper[35 - index]) + 16 * row) == (counter)) {
//            leds[i] = CHSV (50, 255, 100);
//         }
//      }
//      else {
//        if ((mapper[index] + 16 * row) == counter) {
//          leds[i] = CHSV (50, 255, 100);
//        }
//      }
//    }
//    counter = (counter + 1) % 16 + (starting_led / 36) * 16;
//    FastLED.show();
//    delay(500);
//    FastLED.clear();
//    FastLED.show();
//    starting_led += 36;
}

void mapToGrid(int LEDNum, CRGB color) { 
    int row  = LEDNum / 16 ;
    int starting_led = row * 36;
    for (int i = starting_led; i < starting_led + 36; i++) {
      row = i / 36;
      int index = i % 36;
      if ((row % 2) == 1) {
         if (((15 - mapper[35 - index]) + 16 * row) == LEDNum) {
            leds[i] = color;
         }
      }
      else {
        if ((mapper[index] + 16 * row) == LEDNum) {
          leds[i] = color;
        }
      }
    }
}
