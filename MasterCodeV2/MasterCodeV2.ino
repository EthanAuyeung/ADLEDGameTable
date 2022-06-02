#include <arduinoFFT.h>
#include <FastLED.h>

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define SAMPLES 64        // Must be a power of 2
#define MIC_IN A0         // Use A0 for mic input

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    576
#define xres 16            // Total number of  columns in the display
#define yres 36            // Total number of  rows in the display
#define DATA_PIN2    4


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

// led grid runtime consts
#define BRIGHTNESS         150
#define FRAMES_PER_SECOND  30

// old grid to new grid consts
#define LED_DIST 5/8
#define GRID_DIST 1.5
#define BORDER_DIST 0.28
int mapper[36];

CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS];

// music reactive globals
arduinoFFT FFT = arduinoFFT();
double vReal[SAMPLES];
double vImag[SAMPLES];
int Intensity[xres] = { }; // initialize Frequency Intensity to zero
int Displacement = 1;

// led state vars
int cyclePin = 2;
int cyclePinVal = 0;
int cyclePinValPrev = 0;

// power state vars
int powerPin = 3;
int powerState = 0;
int powerPinVal = 0;
int powerPinValPrev = 0;

void setup() {
  pinMode(MIC_IN, INPUT);
  Serial.begin(57600);         //Initialize Serial
  delay(3000); // 3 second delay for recovery
  Serial.print("Setup started...");
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<LED_TYPE,DATA_PIN2,COLOR_ORDER>(leds2, NUM_LEDS).setCorrection(TypicalLEDStrip);


  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  uint16_t sampleIndex;
  for( uint16_t i = 0 ; i < 36; i++ )
  {
      mapper[i] = indexToSampleIndex(i);
  }
  // todo: make sure these are correct
  mapper[35] = 15;
  mapper[2] = 255;
  mapper[34]= 255;

  Serial.println("  done.");
}

// led grid state patterns
typedef void (*SimplePatternList[])();

SimplePatternList gPatterns = {chess, soundReactive, movingRainbow, rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm};
int gCurrentPatternNumber = 0;
int gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{

    // update power button state
    powerPinVal = digitalRead(powerPin);
    if (powerPinVal == HIGH && powerPinValPrev == LOW) {
      // invert on rising edge of press
      powerState = 1 - powerState;
      Serial.println("power switch pressed.");
      delay(250);
    }
    powerPinValPrev = powerPinVal;
    
    if (powerState == LOW) {
      // turn off grid when powered off
      FastLED.clear();
    } else {
      // power is on

      // check if cycle button pressed
      cyclePinVal = digitalRead(cyclePin);
      if (cyclePinVal == HIGH && cyclePinValPrev == LOW) {
        nextPattern();
        Serial.print("pattern cycle button pressed. patternIndex=");
        Serial.println(gCurrentPatternNumber);
        delay(250);
      }
      cyclePinValPrev = cyclePinVal;

      // update leds
      // Call the current pattern function once, updating the 'leds' array
      gPatterns[gCurrentPatternNumber]();
    }
    FastLED.show();
    delay((int) 1000/FRAMES_PER_SECOND);
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof(A[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
//  gCurrentPatternNumber++;
//  if (gCurrentPatternNumber >= NUM_PATTERNS) {
//    gCurrentPatternNumber = 0;
//  }
gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void soundReactive()
{
  for(int i = 0; i < SAMPLES; i++){
      vReal[i] = analogRead(MIC_IN);
//      Serial.println(vReal[i]);
      vImag[i] = 0;
    }
    //FFT
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  
    //Update Intensity Array
  for(int i = 2; i < (xres*Displacement)+2; i+=Displacement){
      vReal[i] = constrain(vReal[i],0 ,2047);            // set max value for input data
      vReal[i] = map(vReal[i], 0, 2047, 0, yres);        // map data to fit our display
  
      Intensity[(i/Displacement)-2] --;                      // Decrease displayed value
      if (vReal[i] > Intensity[(i/Displacement)-2])          // Match displayed value to measured value
        Intensity[(i/Displacement)-2] = vReal[i];
    
  }
    
  int color = 0;
  for(int i = 0; i < xres; i++){
    for(int j = 0; j < yres; j++){
        if(j <= Intensity[i]){                                // Light everything within the intensity range
          if(i%2 == 0){
//            mapToGrid((xres*(j+1))-i-1, CHSV(color, 255, BRIGHTNESS));
            leds[(yres*(i+1))-j-1] = CHSV(color, 255, BRIGHTNESS);
            leds2[(yres*(i+1))-j-1] = CHSV(color, 255, BRIGHTNESS);

          }
          else{
//              mapToGrid((xres*j)+i, CHSV(color, 255, BRIGHTNESS));
            leds[(yres*i)+j] = CHSV(color, 255, BRIGHTNESS);
            leds2[(yres*i)+j] = CHSV(color, 255, BRIGHTNESS);
          }
        }
        else{                                                  // Everything outside the range goes dark
          if(i%2 == 0){
//            mapToGrid((xres*(j+1))-i-1, CHSV(color, 255, 0));
              leds[(yres*(i+1))-j-1] = CHSV(color, 255, 0);
              leds2[(yres*(i+1))-j-1] = CHSV(color, 255, 0);
          }
          else{
//            mapToGrid((xres*j)+i, CHSV(color, 255, 0));
            leds[(yres*i)+j] = CHSV(color, 255, 0);
            leds2[(yres*i)+j] = CHSV(color, 255, 0);
          }
      }
    }
    color += 255/xres;                                      // Increment the Hue to get the Rainbow
  }
}

int hi = 0;
int rainbowCounter = 0;

void movingRainbow() 
{
    hi = (hi + 2) % 256;
    CHSV color = CHSV(hi, 255, 200);
    mapToGrid(rainbowCounter,color);
    rainbowCounter = (rainbowCounter + 1 ) % 256;
}

void rainbow()
{
  fill_rainbow( leds, NUM_LEDS, gHue, 255/NUM_LEDS);
  fill_rainbow( leds2, NUM_LEDS, gHue, 255/NUM_LEDS);
  gHue = (gHue + 6) % 256;
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter(fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS) ] += CRGB::White;
    leds2[random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  fadeToBlackBy( leds2, NUM_LEDS, 10);

  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds2[pos] += CHSV( gHue + random8(64), 200, 255);
  pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds2[pos] += CHSV( gHue + random8(64), 200, 255);
  pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
  leds2[pos] += CHSV( gHue + random8(64), 200, 255);

}

void sinelon() // change to mapping
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  fadeToBlackBy( leds2, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);       
  leds2[pos] += CHSV( gHue, 255, 192);

}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
//    mapToGrid(i, ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10)));
//    mapToGrid(i, ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10)));

    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    leds2[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  fadeToBlackBy( leds2, NUM_LEDS, 20);

  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    leds2[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);

    dothue += 32;
  }
}

void chess() {
  fadeToBlackBy( leds, NUM_LEDS, 40);
  fadeToBlackBy( leds2, NUM_LEDS, 40);
  makeGrid();
}

void makeGrid() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      makeCell(i,j);
    }
  }
}

void makeCell(int i, int j) {
  bool isWhiteCell = (i % 2) ^ (j % 2);
  CRGB color;
  int rowIndex;
  int colIndex;
  
  if (isWhiteCell) {
    color = CRGB(10,0,150);
  } else {
    color = CHSV(160,255,50);
  }

  rowIndex = i*2;
  colIndex = j*2;

//  mapToGrid((rowIndex+0)*16 + colIndex+0, color);
//  mapToGrid((rowIndex+0)*16 + colIndex+1, color);
//  mapToGrid((rowIndex+1)*16 + colIndex+0, color);
//  mapToGrid((rowIndex+1)*16 + colIndex+1, color);
    mapToGrid((rowIndex+0)*16 + colIndex+0, color);
    mapToGrid((rowIndex+0)*16 + colIndex+1, color);
    mapToGrid((rowIndex + 2)*16 - colIndex-1 , color);
    mapToGrid((rowIndex + 2)*16 - colIndex-2, color);
}

// mapping functions

void mapToGrid(int LEDNum, CRGB color) { 
    int row  = LEDNum / 16 ;
    int starting_led = row * 36;
    for (int i = starting_led; i < starting_led + 36; i++) {
      row = i / 36;
      int index = i % 36;
      if ((row % 2) == 1) {
         if (((15 - mapper[35 - index]) + 16 * row) == LEDNum) {
            leds[i] = color;
            leds2[i] = color;
            
         }
      }
      else {
        if ((mapper[index] + 16 * row) == LEDNum) {
          leds[i] = color;
          leds2[i] = color;
          
        }
      }
    }
}

float indexToDeltaInches(uint8_t index) {
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

uint8_t deltaInchesToOldIndex(float deltaInches)
{
    float gridIndexFloat = deltaInches / GRID_DIST;
    uint8_t gridIndex = round(gridIndexFloat);
    return gridIndex;
}

// THIS IS THE TOP LEVEL FUNCTION, ONLY CALL THIS
uint8_t indexToSampleIndex(uint8_t index)
{
    float deltaInches;
    float absoluteInches;
    bool isTooCloseToGrid;
    uint8_t oldIndex;

    deltaInches = indexToDeltaInches(index);
    absoluteInches = deltaInchesToAbsoluteInches(deltaInches);
    isTooCloseToGrid = tooCloseToGrid(absoluteInches);

    if ( isTooCloseToGrid )
    {
        return (uint8_t) -1;
    }

    oldIndex = deltaInchesToOldIndex(deltaInches);

    return oldIndex;
}
