/*
	DJ Table Illumination Controller
	Adafruit Neopixels on an Arduino Micro
	Two pushbuttons for mode control, one potentiometer for color sweep
	
	Design & Code: Presence (Paul Gorman)
	with Tutorials and Tips from Adafruit.com
	for
	Steve Beyer Productions http://stevebeyerproductions.com/
	
	20150429
*/

/* Libraries */
#include <Adafruit_NeoPixel.h>

/* 10k potentiometer on pin 1, connected between 5v and ground ****************/
int potentiometerPin = 0;
int potReadingOld = 0;
float potSmoothing = 0.1; // smaller value filters variances more

/* Three through-hole WS2811 Neopixel LEDs on controller box on Pin 5 *********/
Adafruit_NeoPixel controller = Adafruit_NeoPixel(3, 5, NEO_RGB + NEO_KHZ400);

/* Neopixel WS2812B Top Strip on pin 7 (white**********************************/
Adafruit_NeoPixel stripT = Adafruit_NeoPixel(104, 7, NEO_GRB + NEO_KHZ800);

/* Neopixel WS2812B Bottom Strip on pin 6 (yellow) ****************************/
Adafruit_NeoPixel stripB = Adafruit_NeoPixel(84, 6, NEO_GRB + NEO_KHZ800);


/* Buttons wired to ground from pins 11 and 12 ********************************/
int buttonMode = 11;	// Green Button
int buttonEffect = 12;	// Black Button
int counterModeRead = 0;		// Debouncing - How many cycles button depressed
int counterEffectRead = 0;
int buttonModeRead;			// the current value read from the input pin
int buttonEffectRead;
long previousMillis = 0;

int buttonInterval = 3;           // interval at which to check buttons (milliseconds)
int debounce_count = 50; // number of millis/samples to consider before declaring a debounced input

/* Modes **********************************************************************/
int animationMode = 0;	// 0: static sweepable sparkling color
			// 1: rainbow
			// 2: pacman chase
			// 3: CGA larson scanner
			// 4: Fire Larson Scanner
			// 5: attempt at "plasma" -- circus party

int animationModesTotal = 5;	// total effects available

/* Color Variables ************************************************************/
int displayColor = 0; // temp after randomizations
float displayFade = 0.00; // temp after randomizations, maybe
int brightness = 10;  // 1 low, 10 max // brightness of the lights
int colorFrameRate = 60;		// milliseconds
int plasmaFrameRate = 90;		// milliseconds
int rainbowFrameRate = 3;
int pacmanFrameRate = 20;
int cgaFrameRate = 20;

/* Plasma *********************************************************************/
struct Point {
  float x;
  float y;
};
float phase = 0.0;
float phaseIncrement = 0.01;  // Controls the speed of the moving points. Higher == faster. I like 0.08 ..03 change to .02
float colorStretch = 0.11;    // Higher numbers will produce tighter color bands. I like 0.11 . ok try .11 instead of .03

/* Rainbow ********************************************************************/
int pixelPosition = 0;
int colorValue = 0;
int pixelDirection = 0;

/* CGA ************************************************************************/
int centerPixelT = stripT.numPixels()/2;
int centerPixelB = map(centerPixelT,0,stripT.numPixels(),0,stripB.numPixels())+10;
int pixelPositionBL = centerPixelB;
int pixelPositionBR = centerPixelB;
int pixelPositionTL = centerPixelT;
int pixelPositionTR = centerPixelT;

/* AltCGA ************************************************************************/
int pixelAltPositionBL = map(stripT.numPixels(),0,stripT.numPixels(),0,stripB.numPixels());
int pixelAltPositionBR = 0;
int pixelAltPositionTL = stripT.numPixels();
int pixelAltPositionTR = 0;

/* ****************************************************************************/


void setup() {
	pinMode(buttonMode, INPUT_PULLUP);
	pinMode(buttonEffect, INPUT_PULLUP);
	Serial.begin(9600);
	digitalWrite(13,LOW);
	
	controller.begin();
	controller.setBrightness(100);
	controller.show(); // Initialize all pixels to 'off'
	controller.setPixelColor(0,0,0,255); // starting up blue
	controller.show();
	stripT.begin();
	stripT.setBrightness(255);
	stripT.show();
	stripB.begin();
	stripB.setBrightness(255);
	stripB.show();
}

/* ****************************************************************************/

void loop() {
	unsigned long currentMillis = millis();
	//if((millis() % buttonInterval) == 0 ) {
	if(currentMillis - previousMillis > buttonInterval) {
		// Mode Button Debouncing
		buttonModeRead = digitalRead(buttonMode);
		if (buttonModeRead == HIGH && counterModeRead > 0) {
			/* button not pressed but it had been like a cycle ago... */
			counterModeRead--;
		}
		if (buttonModeRead == LOW) { /* button pressed */
			counterModeRead++;
		}
		// Effect Button Debouncing
		buttonEffectRead = digitalRead(buttonEffect);
		if (buttonEffectRead == HIGH && counterEffectRead > 0) {
			/* button not pressed, but it had been earlier */
			counterEffectRead--;
		}
		if (buttonEffectRead == LOW) { /* button pressed */
			counterEffectRead++;
		}
		previousMillis = currentMillis;
	}
	if (counterModeRead > 0 || counterEffectRead > 0) {
		digitalWrite(13, HIGH);
	}
	// If the button depressed long enough, switch it
	if (counterModeRead >= debounce_count) {
		digitalWrite(13, LOW);
		counterModeRead = 0;
		if (animationMode == 0) {
			/* if static, go into animated mode */
			animationMode++;
		} else {
			/* if animated, go back to static color */
			animationMode = 0;
		}
		Serial.print("Mode Toggle: ");
		Serial.println(animationMode);

	}
	if (counterEffectRead >= debounce_count) {
		digitalWrite(13,LOW);
		counterEffectRead = 0;
		/* if we're animated, go into new effect */
		if (animationMode > 0) {
			animationMode++;
		}
		if (animationMode > animationModesTotal) {
			animationMode = 1;
		}
		Serial.print("Effect Toggle: ");
		Serial.println(animationMode);
	}
	
	if (animationMode == 0) {
		/* Static Color, Set by Potentiometer */
		/* read the potentiometer */
		int potReadingNew = analogRead(potentiometerPin);
		/* smooth potentiometer bouncing */
		int potReading = ((1 - potSmoothing) * potReadingOld) + (potSmoothing * potReadingNew);
		/* illuminate onboard LED if pot moves */
		if (potReadingOld != potReading) {
			digitalWrite(13,HIGH);
		} else {
			digitalWrite(13,LOW);
		}		
		potReadingOld = potReadingNew;
		byte colorWheel = map(potReading, 0, 1020, 0, 255);  // convert analogIn value to colorwheel value
		if ((millis() % colorFrameRate) == 0 ) { /* update LEDs every 60 milliseconds */
			Sparkle(colorWheel);
		}
	}

	if (animationMode == 1) {
		if ((millis() % rainbowFrameRate) == 0) {
			RainbowCycle();
		}
	}

	if (animationMode == 2) {
		if ((millis() % pacmanFrameRate) == 0) {
			Pacman();
		}
	}

	if (animationMode == 3) {
		if ((millis() % cgaFrameRate) == 0) {
			CgaCycle();
		}
	}

	if (animationMode == 4) {
		if ((millis() % cgaFrameRate) == 0) {
			AltCycle();
		}
	}

	if (animationMode == 5) {
		if ((millis() % plasmaFrameRate) == 0) {
			PlasmaController();
			PlasmaStrip();
		}
	}

}

/* ****************************************************************************/


/* Color Wheel 
 * Input a value 0 to 255 to get a color value.
 * The colours are a transition r - g - b - back to r.
 */
uint32_t Wheel(byte WheelPos, float opacity) {
	WheelPos = 255 - WheelPos;
	if(WheelPos < 85) {
		return controller.Color((WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity, 0);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return controller.Color((255 - WheelPos * 3) * opacity, 0, (WheelPos * 3) * opacity);
	} else {
		WheelPos -= 170;
		return controller.Color(0, (WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity);
	}
}

void CgaCycle() {
	//set base color
	for (int pixel=0; pixel < stripT.numPixels(); pixel++) {
		stripT.setPixelColor(pixel,76,0,153);
	}
	for (int pixel=0; pixel < stripB.numPixels(); pixel++) {
		stripB.setPixelColor(pixel,76,0,153);
	}
	controller.setPixelColor(0,76,0,153);
	controller.setPixelColor(1,76,0,153);
	controller.setPixelColor(2,76,0,153);
	controller.setPixelColor((pixelPosition % 20) -1,76,0,153);
	controller.setPixelColor((pixelPosition % 20),255,255,255);
	// move block forwards
	stripT.setPixelColor(pixelPositionTL,0,255,255);
	stripT.setPixelColor(pixelPositionTL+8,0,255,255);
	for (int group = 1; group < 8; group++) {
		stripT.setPixelColor(pixelPositionTL+group,255,255,255);
		stripT.setPixelColor(pixelPositionTL-group,76,0,153);
	}
	stripB.setPixelColor(map(pixelPositionBL,0,stripT.numPixels(),0,stripB.numPixels()),0,255,255);
	stripB.setPixelColor(map(pixelPositionBL,0,stripT.numPixels(),0,stripB.numPixels())+8,0,255,255);
	for (int group = 1; group < 8; group++) {
		stripB.setPixelColor(map(pixelPositionBL,0,stripT.numPixels(),0,stripB.numPixels())+group,255,255,255);
		stripB.setPixelColor(map(pixelPositionBL,0,stripT.numPixels(),0,stripB.numPixels())-group,76,0,153);
	}
	// move block backwards
	stripT.setPixelColor(pixelPositionTR,0,255,255);
	stripT.setPixelColor(pixelPositionTR-8,0,255,255);
	for (int group = 7; group > 0; group--) {
		stripT.setPixelColor(pixelPositionTR-group,255,255,255);
		stripT.setPixelColor(pixelPositionTR+group,76,0,153);
	}
	stripB.setPixelColor(map(pixelPositionBR,0,stripT.numPixels(),0,stripB.numPixels()),0,255,255);
	stripB.setPixelColor(map(pixelPositionBR,0,stripT.numPixels(),0,stripB.numPixels())-8,0,255,255);
	for (int group = 7; group > 0; group--) {
		stripB.setPixelColor(map(pixelPositionBR,0,stripT.numPixels(),0,stripB.numPixels())-group,255,255,255);
		stripB.setPixelColor(map(pixelPositionBR,0,stripT.numPixels(),0,stripB.numPixels())+group,76,0,153);
	}
	stripT.show();
	stripB.show();
	controller.show();
	pixelPosition++;
	pixelPositionTL++;
	pixelPositionTR--;
	pixelPositionBL++;
	pixelPositionBR--;
	if (pixelPositionTL > stripT.numPixels()) {
		pixelPositionTL = centerPixelT;
		pixelPositionBL = centerPixelB;
	}
	if (pixelPositionTR < 0) {
		pixelPositionTR = centerPixelT;
		pixelPositionBR = centerPixelB;
	}
	if (pixelPosition > 255) {
		pixelPosition = 0;
	}
}

void AltCycle() {
	//set base color
	for (int pixel=0; pixel < stripT.numPixels(); pixel++) {
		stripT.setPixelColor(pixel,255,0,0);
	}
	for (int pixel=0; pixel < stripB.numPixels(); pixel++) {
		stripB.setPixelColor(pixel,255,0,0);
	}
	controller.setPixelColor(0,255,0,0);
	controller.setPixelColor(1,255,0,0);
	controller.setPixelColor(2,255,0,0);
	controller.setPixelColor((pixelPosition % 20) -1,255,128,0);
	controller.setPixelColor((pixelPosition % 20),255,255,0);
	// move block forwards
	stripT.setPixelColor(pixelAltPositionTL,255,128,0);
	stripT.setPixelColor(pixelAltPositionTL+8,255,255,0);
	for (int group = 1; group < 8; group++) {
		stripT.setPixelColor(pixelAltPositionTL+group,255,128,0);
		stripT.setPixelColor(pixelAltPositionTL-group,255,255,0);
	}
	stripB.setPixelColor(map(pixelAltPositionBL,0,stripT.numPixels(),0,stripB.numPixels()),255,128,0);
	stripB.setPixelColor(map(pixelAltPositionBL,0,stripT.numPixels(),0,stripB.numPixels())+8,255,255,0);
	for (int group = 1; group < 8; group++) {
		stripB.setPixelColor(map(pixelAltPositionBL,0,stripT.numPixels(),0,stripB.numPixels())+group,255,128,0);
		stripB.setPixelColor(map(pixelAltPositionBL,0,stripT.numPixels(),0,stripB.numPixels())-group,255,255,0);
	}
	// move block backwards
	stripT.setPixelColor(pixelAltPositionTR,255,128,0);
	stripT.setPixelColor(pixelAltPositionTR-8,255,255,0);
	for (int group = 7; group > 0; group--) {
		stripT.setPixelColor(pixelAltPositionTR-group,255,128,0);
		stripT.setPixelColor(pixelAltPositionTR+group,255,128,0);
	}
	stripB.setPixelColor(map(pixelAltPositionBR,0,stripT.numPixels(),0,stripB.numPixels()),255,128,0);
	stripB.setPixelColor(map(pixelAltPositionBR,0,stripT.numPixels(),0,stripB.numPixels())-8,255,255,0);
	for (int group = 7; group > 0; group--) {
		stripB.setPixelColor(map(pixelAltPositionBR,0,stripT.numPixels(),0,stripB.numPixels())-group,255,128,0);
		stripB.setPixelColor(map(pixelAltPositionBR,0,stripT.numPixels(),0,stripB.numPixels())+group,255,128,0);
	}
	stripT.show();
	stripB.show();
	controller.show();
	pixelPosition++;
	pixelAltPositionTL++;
	pixelAltPositionTR--;
	pixelAltPositionBL++;
	pixelAltPositionBR--;
	if (pixelAltPositionTL > stripT.numPixels()) {
		pixelAltPositionTL = 0;
		pixelAltPositionBL = 0;
	}
	if (pixelAltPositionTR < 0) {
		pixelAltPositionTR = stripT.numPixels();
		pixelAltPositionBR = stripB.numPixels()+14;
	}
	if (pixelPosition > 255) {
		pixelPosition = 0;
	}
}

void Pacman() {
	if (pixelDirection == 0) {
		controller.setPixelColor(0,0,0,255);
		controller.setPixelColor(1,0,0,255);
		controller.setPixelColor(2,255,165,0);
		for (int pixel=0; pixel < stripT.numPixels(); pixel++) {
			if (pixel == pixelPosition) {
				stripT.setPixelColor(pixel,255,255,255);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),255,255,255);
			} else if (pixel == pixelPosition + 1 || pixel == pixelPosition -1) {
				stripT.setPixelColor(pixel,255,255,8);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),255,255,8);
			} else if (pixel == pixelPosition +2 || pixel == pixelPosition - 2) {
				stripT.setPixelColor(pixel,165,165,0);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),165,165,0);
			} else if ( (pixel == pixelPosition + 20) || (pixel == pixelPosition - 20) ) {
				stripT.setPixelColor(pixel,70,30,180);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),70,30,180);
			} else if ( (pixel == pixelPosition + 50) || (pixel == pixelPosition - 50) ) {
				stripT.setPixelColor(pixel,30,144,0);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),30,144,0);
			} else {
				stripT.setPixelColor(pixel, 0, 0, 255);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()), 0, 0, 255);
			}
		}
		pixelPosition++;
	} else {
		controller.setPixelColor(0,255,165,0);
		controller.setPixelColor(1,0,0,255);
		controller.setPixelColor(2,0,0,255);
		for (int pixel=stripT.numPixels(); pixel > 0; pixel--) {
			if (pixel == pixelPosition) {
				stripT.setPixelColor(pixel,255,255,255);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),255,255,255);
			} else if (pixel == pixelPosition + 1 || pixel == pixelPosition -1) {
				stripT.setPixelColor(pixel,255,255,8);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),255,255,8);
			} else if (pixel == pixelPosition + 2 || pixel == pixelPosition -2) {
				stripT.setPixelColor(pixel,165,165,0);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),165,165,0);
			} else if ( (pixel == pixelPosition + 20) || (pixel == pixelPosition - 20) ) {
				stripT.setPixelColor(pixel,70,130,180);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),70,130,180);
			} else if ( (pixel == pixelPosition + 50) || (pixel == pixelPosition - 50) ) {
				stripT.setPixelColor(pixel,30,144,255);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()),30,144,255);
			} else {
				stripT.setPixelColor(pixel, 0, 0, 255);
				stripB.setPixelColor(map(pixel,0,stripT.numPixels(),0,stripB.numPixels()), 0, 0, 255);
			}
		}
		pixelPosition--;
	}		
	stripT.show();
	stripB.show();
	controller.show();
	if (pixelPosition > stripT.numPixels()) {
		pixelDirection = 1;
	}
	if (pixelPosition < 0 && pixelDirection == 1) {
		pixelDirection = 0;
	}
}

/* A static color is so boring, so color shift a little */
void Sparkle(int color) {
	for(int pixel=0; pixel < controller.numPixels(); pixel++) {
		if (random(0,1) == 1) {
			displayColor = color + random(5);  // color hue sparkles
			displayFade = brightness * 100 * 0.001; // brightness 
		} else {
			displayColor = color - random(5);
			displayFade = brightness * 100 * 0.001; // static brightness
		}
		controller.setPixelColor(pixel, Wheel(displayColor, displayFade));
		controller.show();
	}
	for (int pixel=0; pixel < stripT.numPixels(); pixel++) {
		if (random(0,1) == 1) {
			displayColor = color + random(5);  // color hue sparkles
			displayFade = brightness * 100 * 0.001; // brightness 
		} else {
			displayColor = color - random(5);
			displayFade = brightness * 100 * 0.001; // static brightness
		}
		stripT.setPixelColor(pixel, Wheel(displayColor, displayFade));
		stripB.setPixelColor(pixel, Wheel(displayColor, displayFade));
	}
	stripT.show();
	stripB.show();
}

void RainbowCycle() {
 	for (pixelPosition = 0; pixelPosition <= stripT.numPixels(); pixelPosition++) {
 		//int pixelColor = map(colorValue+pixelPosition,0,(stripT.numPixels() + 255),0,stripT.numPixels());
		//int pixelColor = ((colorValue * 256 / stripT.numPixels()) + pixelPosition);
		int pixelColor = (colorValue + pixelPosition);
		stripT.setPixelColor(pixelPosition, Wheel(pixelColor,displayFade));
		stripB.setPixelColor(pixelPosition, Wheel(pixelColor,displayFade));
		//Serial.println(pixelColor);
	}
	controller.setPixelColor(0, Wheel((colorValue+10), displayFade));
	controller.setPixelColor(1, Wheel((colorValue+20), displayFade));
	controller.setPixelColor(2, Wheel((colorValue+30), displayFade));
	stripT.show();
	stripB.show();
	controller.show();
	colorValue++;
	if (colorValue == 256) {
		colorValue = 0;
	}
}

void PlasmaStrip () {
	phase += phaseIncrement;
	// The two points move along Lissajious curves, see: http://en.wikipedia.org/wiki/Lissajous_curve
	// We want values that fit the LED grid: x values between 0..13, y values between 0..8 .
	// The sin() function returns values in the range of -1.0..1.0, so scale these to our desired ranges.
	// The phase value is multiplied by various constants; I chose these semi-randomly, to produce a nice motion.
	Point p1 = { (sin(phase*1.000)+1.0) * 4.5, (sin(phase*1.310)+1.0) * 4.0 };
	Point p2 = { (sin(phase*1.770)+1.0) * 4.5, (sin(phase*2.865)+1.0) * 4.0 };
	Point p3 = { (sin(phase*0.250)+1.0) * 4.5, (sin(phase*0.750)+1.0) * 4.0 };

	byte row, col;
  
	// For each row...
	for( row=0; row<2; row++ ) {
	    float row_f = float(row);  // Optimization: Keep a floating point value of the row number, instead of recasting it repeatedly.
    
		// For each column...
		for( col=0; col<stripT.numPixels(); col++ ) {
			float col_f = float(col);  // Optimization.

			// Calculate the distance between this LED, and p1.
			Point dist1 = { col_f - p1.x, row_f - p1.y };  // The vector from p1 to this LED.
			float distance1 =sqrt( dist1.x*dist1.x + dist1.y*dist1.y );

			// Calculate the distance between this LED, and p2.
			Point dist2 = { col_f - p2.x, row_f - p2.y };  // The vector from p2 to this LED.
			float distance2 = sqrt( dist2.x*dist2.x + dist2.y*dist2.y );

			// Calculate the distance between this LED, and p3.
			Point dist3 = { col_f - p3.x, row_f - p3.y };  // The vector from p3 to this LED.
			float distance3 = sqrt( dist3.x*dist3.x + dist3.y*dist3.y );

			// Warp the distance with a sin() function. As the distance value increases, the LEDs will get light,dark,light,dark,etc...
			// You can use a cos() for slightly different shading, or experiment with other functions. Go crazy!
			float color_1 = distance1;  // range: 0.0...1.0
			float color_2 = distance2;
			float color_3 = distance3;
			float color_4 = (sin( distance1 * distance2 * colorStretch )) + 2.0 * 0.5;

			// Square the color_f value to weight it towards 0. The image will be darker and have higher contrast.
			color_1 *= color_1 * color_4;
			color_2 *= color_2 * color_4;
			color_3 *= color_3 * color_4;
			color_4 *= color_4;
			// Scale the color up to 0..7 . Max brightness is 7.
			//stripT.setPixelColor(col + (8 * row), stripT.Color(color_4, 0, 0) );
			//stripT.setPixelColor(col + (2 * row), stripT.Color(color_1, color_2, color_3));
			if (row == 0) {
				stripT.setPixelColor(col, stripT.Color(color_1, color_2, color_3));      	
			} else {
				stripB.setPixelColor(col, stripB.Color(color_1, color_2, color_3));
			}
		}
	}
	stripT.show();
	stripB.show();
}

void PlasmaController () {
	phase += phaseIncrement;
	// The two points move along Lissajious curves, see: http://en.wikipedia.org/wiki/Lissajous_curve
	// We want values that fit the LED grid: x values between 0..13, y values between 0..8 .
	// The sin() function returns values in the range of -1.0..1.0, so scale these to our desired ranges.
	// The phase value is multiplied by various constants; I chose these semi-randomly, to produce a nice motion.
	Point p1 = { (sin(phase*1.000)+1.0) * 4.5, (sin(phase*1.310)+1.0) * 4.0 };
	Point p2 = { (sin(phase*1.770)+1.0) * 4.5, (sin(phase*2.865)+1.0) * 4.0 };
	Point p3 = { (sin(phase*0.250)+1.0) * 4.5, (sin(phase*0.750)+1.0) * 4.0 };

	byte row, col;

	// For each row...
	for( row=0; row<1; row++ ) {
		float row_f = float(row);  // Optimization: Keep a floating point value of the row number, instead of recasting it repeatedly.

		// For each column...
		for( col=0; col<controller.numPixels(); col++ ) {
			float col_f = float(col);  // Optimization.

			// Calculate the distance between this LED, and p1.
			Point dist1 = { col_f - p1.x, row_f - p1.y };  // The vector from p1 to this LED.
			float distance1 = sqrt( dist1.x*dist1.x + dist1.y*dist1.y );

			// Calculate the distance between this LED, and p2.
			Point dist2 = { col_f - p2.x, row_f - p2.y };  // The vector from p2 to this LED.
			float distance2 = sqrt( dist2.x*dist2.x + dist2.y*dist2.y );

			// Calculate the distance between this LED, and p3.
			Point dist3 = { col_f - p3.x, row_f - p3.y };  // The vector from p3 to this LED.
			float distance3 = sqrt( dist3.x*dist3.x + dist3.y*dist3.y );

			// Warp the distance with a sin() function. As the distance value increases, the LEDs will get light,dark,light,dark,etc...
			// You can use a cos() for slightly different shading, or experiment with other functions. Go crazy!
			float color_1 = distance1;  // range: 0.0...1.0
			float color_2 = distance2;
			float color_3 = distance3;
			float color_4 = (sin( distance1 * distance2 * colorStretch )) + 2.0 * 0.5;

			// Square the color_f value to weight it towards 0. The image will be darker and have higher contrast.
			color_1 *= color_1 * color_4;
			color_2 *= color_2 * color_4;
			color_3 *= color_3 * color_4;
			color_4 *= color_4;

			// Scale the color up to 0..7 . Max brightness is 7.
			//controller.setPixelColor(col + (8 * row), controller.Color(color_4, 0, 0) );
			controller.setPixelColor(col + (3 * row), controller.Color(color_1, color_2, color_3));
		}
	}
	controller.show();
}

