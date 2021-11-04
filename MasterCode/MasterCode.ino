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

#define DATA_PIN    3
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    16
#define BUTTON_PIN 4
#define xres 16            // Total number of  columns in the display
#define yres 16            // Total number of  rows in the display

CRGB leds[NUM_LEDS];
arduinoFFT FFT = arduinoFFT();

#define BRIGHTNESS          200
#define FRAMES_PER_SECOND  120

int inPin = 7; 
int val = 0;     // variable for reading the pin status
int past_output = HIGH;
int counter = 0;
int check = 0;

int powerPin = 7; //TODO need to change
int pVal = 0;
int pPast = HIGH;
int pCheck = 0;
int pSwitch = 0;
double vReal[SAMPLES];
double vImag[SAMPLES];

int Intensity[xres] = { }; // initialize Frequency Intensity to zero
int Displacement = 1;


void setup() {
  pinMode(MIC_IN, INPUT);
  Serial.begin(115200);         //Initialize Serial
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, soundReactive };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
 
  pVal = digitalRead(powerPin);  // read input value

  if (pPast == pVal) {
    pCheck = 1;
  }
  else if (pVal == HIGH and pPast == LOW) {
    pPast = HIGH;
  }
  else if (pVal == LOW and pPast == HIGH and pCheck == 1) {
    delayMicroseconds(450);
    pSwitch = (pSwitch + 1) % 2;
    pPast = LOW;
    pCheck = 0;
    FastLED.clear();
    FastLED.show();
    delayMicroseconds(450);
  }
  
  if (pSwitch == 1) {
    val = digitalRead(inPin);  // read input value
  
    if (past_output == val) {
      check = 1;
    }
    else if (val == HIGH and past_output == LOW) {
      past_output = HIGH;
    }
    else if (val == LOW and past_output == HIGH and check == 1) {
      gCurrentPatternNumber = (gCurrentPatternNumber + 1)%sizeof
      delayMicroseconds(450);
      past_output = LOW;
      check = 0;
    }
    delayMicroseconds(1000);
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
  
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 
  }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void soundReactive()
{
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
    
    //Update Display
    int color = 0;
    for(int i = 0; i < xres; i++){
      for(int j = 0; j < yres; j++){
        if(j <= Intensity[i]){                                // Light everything within the intensity range
          if(j%2 == 0){
            leds[(xres*(j+1))-i-1] = CHSV(color, 255, BRIGHTNESS);
          }
          else{
            leds[(xres*j)+i] = CHSV(color, 255, BRIGHTNESS);
          }
        }
        else{                                                  // Everything outside the range goes dark
          if(j%2 == 0){
            leds[(xres*(j+1))-i-1] = CHSV(color, 255, 0);
          }
          else{
            leds[(xres*j)+i] = CHSV(color, 255, 0);
          }
        }
      }
      color += 255/xres;                                      // Increment the Hue to get the Rainbow
    }
    
    FastLED.show();
  }
  
    


void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void logo() 
{
  
}

void chess() {
  fadeToBlackBy( leds, NUM_LEDS, 20);
  leds[0] = CRGB(0, 0, 0);
  leds[1] = CRGB(0, 0, 0);
  leds[2] = CRGB(255, 255, 255);
  leds[3] = CRGB(255, 255, 255);
  leds[4] = CRGB(0, 0, 0);
  leds[5] = CRGB(0, 0, 0);
  leds[6] = CRGB(255, 255, 255);
  leds[7] = CRGB(255, 255, 255);
  leds[8] = CRGB(0, 0, 0);
  leds[9] = CRGB(0, 0, 0);
  leds[10] = CRGB(255, 255, 255);
  leds[11] = CRGB(255, 255, 255);
  leds[12] = CRGB(0, 0, 0);
  leds[13] = CRGB(0, 0, 0);
  leds[14] = CRGB(255, 255, 255);
  leds[15] = CRGB(255, 255, 255);
  leds[16] = CRGB(0, 0, 0);
  leds[17] = CRGB(0, 0, 0);
  leds[18] = CRGB(255, 255, 255);
  leds[19] = CRGB(255, 255, 255);
  leds[20] = CRGB(0, 0, 0);
  leds[21] = CRGB(0, 0, 0);
  leds[22] = CRGB(255, 255, 255);
  leds[23] = CRGB(255, 255, 255);
  leds[24] = CRGB(0, 0, 0);
  leds[25] = CRGB(0, 0, 0);
  leds[26] = CRGB(255, 255, 255);
  leds[27] = CRGB(255, 255, 255);
  leds[28] = CRGB(0, 0, 0);
  leds[29] = CRGB(0, 0, 0);
  leds[30] = CRGB(255, 255, 255);
  leds[31] = CRGB(255, 255, 255);
  leds[32] = CRGB(255, 255, 255);
  leds[33] = CRGB(255, 255, 255);
  leds[34] = CRGB(0, 0, 0);
  leds[35] = CRGB(0, 0, 0);
  leds[36] = CRGB(255, 255, 255);
  leds[37] = CRGB(255, 255, 255);
  leds[38] = CRGB(0, 0, 0);
  leds[39] = CRGB(0, 0, 0);
  leds[40] = CRGB(255, 255, 255);
  leds[41] = CRGB(255, 255, 255);
  leds[42] = CRGB(0, 0, 0);
  leds[43] = CRGB(0, 0, 0);
  leds[44] = CRGB(255, 255, 255);
  leds[45] = CRGB(255, 255, 255);
  leds[46] = CRGB(0, 0, 0);
  leds[47] = CRGB(0, 0, 0);
  leds[48] = CRGB(255, 255, 255);
  leds[49] = CRGB(255, 255, 255);
  leds[50] = CRGB(0, 0, 0);
  leds[51] = CRGB(0, 0, 0);
  leds[52] = CRGB(255, 255, 255);
  leds[53] = CRGB(255, 255, 255);
  leds[54] = CRGB(0, 0, 0);
  leds[55] = CRGB(0, 0, 0);
  leds[56] = CRGB(255, 255, 255);
  leds[57] = CRGB(255, 255, 255);
  leds[58] = CRGB(0, 0, 0);
  leds[59] = CRGB(0, 0, 0);
  leds[60] = CRGB(255, 255, 255);
  leds[61] = CRGB(255, 255, 255);
  leds[62] = CRGB(0, 0, 0);
  leds[63] = CRGB(0, 0, 0);
  leds[64] = CRGB(0, 0, 0);
  leds[65] = CRGB(0, 0, 0);
  leds[66] = CRGB(255, 255, 255);
  leds[67] = CRGB(255, 255, 255);
  leds[68] = CRGB(0, 0, 0);
  leds[69] = CRGB(0, 0, 0);
  leds[70] = CRGB(255, 255, 255);
  leds[71] = CRGB(255, 255, 255);
  leds[72] = CRGB(0, 0, 0);
  leds[73] = CRGB(0, 0, 0);
  leds[74] = CRGB(255, 255, 255);
  leds[75] = CRGB(255, 255, 255);
  leds[76] = CRGB(0, 0, 0);
  leds[77] = CRGB(0, 0, 0);
  leds[78] = CRGB(255, 255, 255);
  leds[79] = CRGB(255, 255, 255);
  leds[80] = CRGB(0, 0, 0);
  leds[81] = CRGB(0, 0, 0);
  leds[82] = CRGB(255, 255, 255);
  leds[83] = CRGB(255, 255, 255);
  leds[84] = CRGB(0, 0, 0);
  leds[85] = CRGB(0, 0, 0);
  leds[86] = CRGB(255, 255, 255);
  leds[87] = CRGB(255, 255, 255);
  leds[88] = CRGB(0, 0, 0);
  leds[89] = CRGB(0, 0, 0);
  leds[90] = CRGB(255, 255, 255);
  leds[91] = CRGB(255, 255, 255);
  leds[92] = CRGB(0, 0, 0);
  leds[93] = CRGB(0, 0, 0);
  leds[94] = CRGB(255, 255, 255);
  leds[95] = CRGB(255, 255, 255);
  leds[96] = CRGB(255, 255, 255);
  leds[97] = CRGB(255, 255, 255);
  leds[98] = CRGB(0, 0, 0);
  leds[99] = CRGB(0, 0, 0);
  leds[100] = CRGB(255, 255, 255);
  leds[101] = CRGB(255, 255, 255);
  leds[102] = CRGB(0, 0, 0);
  leds[103] = CRGB(0, 0, 0);
  leds[104] = CRGB(255, 255, 255);
  leds[105] = CRGB(255, 255, 255);
  leds[106] = CRGB(0, 0, 0);
  leds[107] = CRGB(0, 0, 0);
  leds[108] = CRGB(255, 255, 255);
  leds[109] = CRGB(255, 255, 255);
  leds[110] = CRGB(0, 0, 0);
  leds[111] = CRGB(0, 0, 0);
  leds[112] = CRGB(255, 255, 255);
  leds[113] = CRGB(255, 255, 255);
  leds[114] = CRGB(0, 0, 0);
  leds[115] = CRGB(0, 0, 0);
  leds[116] = CRGB(255, 255, 255);
  leds[117] = CRGB(255, 255, 255);
  leds[118] = CRGB(0, 0, 0);
  leds[119] = CRGB(0, 0, 0);
  leds[120] = CRGB(255, 255, 255);
  leds[121] = CRGB(255, 255, 255);
  leds[122] = CRGB(0, 0, 0);
  leds[123] = CRGB(0, 0, 0);
  leds[124] = CRGB(255, 255, 255);
  leds[125] = CRGB(255, 255, 255);
  leds[126] = CRGB(0, 0, 0);
  leds[127] = CRGB(0, 0, 0);
  leds[128] = CRGB(0, 0, 0);
  leds[129] = CRGB(0, 0, 0);
  leds[130] = CRGB(255, 255, 255);
  leds[131] = CRGB(255, 255, 255);
  leds[132] = CRGB(0, 0, 0);
  leds[133] = CRGB(0, 0, 0);
  leds[134] = CRGB(255, 255, 255);
  leds[135] = CRGB(255, 255, 255);
  leds[136] = CRGB(0, 0, 0);
  leds[137] = CRGB(0, 0, 0);
  leds[138] = CRGB(255, 255, 255);
  leds[139] = CRGB(255, 255, 255);
  leds[140] = CRGB(0, 0, 0);
  leds[141] = CRGB(0, 0, 0);
  leds[142] = CRGB(255, 255, 255);
  leds[143] = CRGB(255, 255, 255);
  leds[144] = CRGB(0, 0, 0);
  leds[145] = CRGB(0, 0, 0);
  leds[146] = CRGB(255, 255, 255);
  leds[147] = CRGB(255, 255, 255);
  leds[148] = CRGB(0, 0, 0);
  leds[149] = CRGB(0, 0, 0);
  leds[150] = CRGB(255, 255, 255);
  leds[151] = CRGB(255, 255, 255);
  leds[152] = CRGB(0, 0, 0);
  leds[153] = CRGB(0, 0, 0);
  leds[154] = CRGB(255, 255, 255);
  leds[155] = CRGB(255, 255, 255);
  leds[156] = CRGB(0, 0, 0);
  leds[157] = CRGB(0, 0, 0);
  leds[158] = CRGB(255, 255, 255);
  leds[159] = CRGB(255, 255, 255);
  leds[160] = CRGB(255, 255, 255);
  leds[161] = CRGB(255, 255, 255);
  leds[162] = CRGB(0, 0, 0);
  leds[163] = CRGB(0, 0, 0);
  leds[164] = CRGB(255, 255, 255);
  leds[165] = CRGB(255, 255, 255);
  leds[166] = CRGB(0, 0, 0);
  leds[167] = CRGB(0, 0, 0);
  leds[168] = CRGB(255, 255, 255);
  leds[169] = CRGB(255, 255, 255);
  leds[170] = CRGB(0, 0, 0);
  leds[171] = CRGB(0, 0, 0);
  leds[172] = CRGB(255, 255, 255);
  leds[173] = CRGB(255, 255, 255);
  leds[174] = CRGB(0, 0, 0);
  leds[175] = CRGB(0, 0, 0);
  leds[176] = CRGB(255, 255, 255);
  leds[177] = CRGB(255, 255, 255);
  leds[178] = CRGB(0, 0, 0);
  leds[179] = CRGB(0, 0, 0);
  leds[180] = CRGB(255, 255, 255);
  leds[181] = CRGB(255, 255, 255);
  leds[182] = CRGB(0, 0, 0);
  leds[183] = CRGB(0, 0, 0);
  leds[184] = CRGB(255, 255, 255);
  leds[185] = CRGB(255, 255, 255);
  leds[186] = CRGB(0, 0, 0);
  leds[187] = CRGB(0, 0, 0);
  leds[188] = CRGB(255, 255, 255);
  leds[189] = CRGB(255, 255, 255);
  leds[190] = CRGB(0, 0, 0);
  leds[191] = CRGB(0, 0, 0);
  leds[192] = CRGB(0, 0, 0);
  leds[193] = CRGB(0, 0, 0);
  leds[194] = CRGB(255, 255, 255);
  leds[195] = CRGB(255, 255, 255);
  leds[196] = CRGB(0, 0, 0);
  leds[197] = CRGB(0, 0, 0);
  leds[198] = CRGB(255, 255, 255);
  leds[199] = CRGB(255, 255, 255);
  leds[200] = CRGB(0, 0, 0);
  leds[201] = CRGB(0, 0, 0);
  leds[202] = CRGB(255, 255, 255);
  leds[203] = CRGB(255, 255, 255);
  leds[204] = CRGB(0, 0, 0);
  leds[205] = CRGB(0, 0, 0);
  leds[206] = CRGB(255, 255, 255);
  leds[207] = CRGB(255, 255, 255);
  leds[208] = CRGB(0, 0, 0);
  leds[209] = CRGB(0, 0, 0);
  leds[210] = CRGB(255, 255, 255);
  leds[211] = CRGB(255, 255, 255);
  leds[212] = CRGB(0, 0, 0);
  leds[213] = CRGB(0, 0, 0);
  leds[214] = CRGB(255, 255, 255);
  leds[215] = CRGB(255, 255, 255);
  leds[216] = CRGB(0, 0, 0);
  leds[217] = CRGB(0, 0, 0);
  leds[218] = CRGB(255, 255, 255);
  leds[219] = CRGB(255, 255, 255);
  leds[220] = CRGB(0, 0, 0);
  leds[221] = CRGB(0, 0, 0);
  leds[222] = CRGB(255, 255, 255);
  leds[223] = CRGB(255, 255, 255);
  leds[224] = CRGB(255, 255, 255);
  leds[225] = CRGB(255, 255, 255);
  leds[226] = CRGB(0, 0, 0);
  leds[227] = CRGB(0, 0, 0);
  leds[228] = CRGB(255, 255, 255);
  leds[229] = CRGB(255, 255, 255);
  leds[230] = CRGB(0, 0, 0);
  leds[231] = CRGB(0, 0, 0);
  leds[232] = CRGB(255, 255, 255);
  leds[233] = CRGB(255, 255, 255);
  leds[234] = CRGB(0, 0, 0);
  leds[235] = CRGB(0, 0, 0);
  leds[236] = CRGB(255, 255, 255);
  leds[237] = CRGB(255, 255, 255);
  leds[238] = CRGB(0, 0, 0);
  leds[239] = CRGB(0, 0, 0);
  leds[240] = CRGB(255, 255, 255);
  leds[241] = CRGB(255, 255, 255);
  leds[242] = CRGB(0, 0, 0);
  leds[243] = CRGB(0, 0, 0);
  leds[244] = CRGB(255, 255, 255);
  leds[245] = CRGB(255, 255, 255);
  leds[246] = CRGB(0, 0, 0);
  leds[247] = CRGB(0, 0, 0);
  leds[248] = CRGB(255, 255, 255);
  leds[249] = CRGB(255, 255, 255);
  leds[250] = CRGB(0, 0, 0);
  leds[251] = CRGB(0, 0, 0);
  leds[252] = CRGB(255, 255, 255);
  leds[253] = CRGB(255, 255, 255);
  leds[254] = CRGB(0, 0, 0);
  leds[255] = CRGB(0, 0, 0);
}
