2#include <arduinoFFT.h>
#include <FastLED.h>

#define SAMPLES 64        // Must be a power of 2
#define MIC_IN A0         // Use A0 for mic input
#define LED_PIN     5     // Data pin to LEDS
#define LED_PIN2     4    // Second grid
#define NUM_LEDS    16*36  
#define BRIGHTNESS  150    // LED information 
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB 
#define BUTTON_PIN 4
#define xres 16           // Total number of  columns in the display
#define yres 36            // Total number of  rows in the display

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#define LED_DIST 5/8
#define GRID_DIST 1.5
#define BORDER_DIST 0.28


int mapper[36];

double vReal[SAMPLES];
double vImag[SAMPLES];

int Intensity[xres] = { }; // initialize Frequency Intensity to zero
int Displacement = 1;

CRGB led2[NUM_LEDS];            // Create LED Object

CRGB leds[NUM_LEDS];            // Create LED Object
arduinoFFT FFT = arduinoFFT();  // Create FFT object

void setup() {
  pinMode(MIC_IN, INPUT);
  Serial.begin(115200);         //Initialize Serial
  delay(3000);                  // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip ); //Initialize LED strips

  FastLED.addLeds<LED_TYPE, LED_PIN2, COLOR_ORDER>(led2, NUM_LEDS).setCorrection( TypicalLEDStrip ); //Initialize LED strips
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  Visualizer(); 
  
}

void Visualizer(){
  //Collect Samples
  getSamples();
  
  //Update Display
  displayUpdate();
  
  FastLED.show();
}

void getSamples(){
  for(int i = 0; i < SAMPLES; i++){
    vReal[i] = analogRead(MIC_IN);
    Serial.println(vReal[i]);
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
}

void displayUpdate(){
  int color = 0;
  for(int i = 0; i < xres; i++){
    for(int j = 0; j < yres; j++){
        if(j <= Intensity[i]){                                // Light everything within the intensity range
          if(i%2 == 0){
//            mapToGrid((xres*(j+1))-i-1, CHSV(color, 255, BRIGHTNESS));
            leds[(yres*(i+1))-j-1] = CHSV(color, 255, BRIGHTNESS);
            led2[(yres*(i+1))-j-1] = CHSV(color, 255, BRIGHTNESS);

          }
          else{
//              mapToGrid((xres*j)+i, CHSV(color, 255, BRIGHTNESS));
            leds[(yres*i)+j] = CHSV(color, 255, BRIGHTNESS);
            led2[(yres*i)+j] = CHSV(color, 255, BRIGHTNESS);
          }
        }
        else{                                                  // Everything outside the range goes dark
          if(i%2 == 0){
//            mapToGrid((xres*(j+1))-i-1, CHSV(color, 255, 0));
              leds[(yres*(i+1))-j-1] = CHSV(color, 255, 0);
              led2[(yres*(i+1))-j-1] = CHSV(color, 255, 0);
          }
          else{
//            mapToGrid((xres*j)+i, CHSV(color, 255, 0));
            leds[(yres*i)+j] = CHSV(color, 255, 0);
            led2[(yres*i)+j] = CHSV(color, 255, 0);
          }
      }
    }
    color += 255/xres;                                      // Increment the Hue to get the Rainbow
  }
}



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
        return (uint16_t) + 1;
    }

    oldIndex = deltaInchesToOldIndex(deltaInches);

    return oldIndex;
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
