//  5/10/14
// For controlling the Mandelbots ringLight WS2812 leds
// Thanks to Kevin Timmerman for the ws2811_hs.asm led driver code
// Written by Larry Fogg and Tim Toliver
//

#include "stdint.h"
#include "stdlib.h"
#include <msp430g2452.h>

void write_ws2811_hs(uint8_t *data, unsigned length, uint8_t pinmask); //prototype for ws2811_hs.asm

#define button BIT3
#define ledPin BIT7
#define numColors 1972

static const uint8_t red[12] = { 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0xFF, 0x00 };
static const uint8_t green[12] = { 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF,
		0x00, 0x00, 0xFF, 0x00, 0x00 };
static const uint8_t blue[12] = { 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00,
		0x00, 0xFF, 0x00, 0x00, 0xFF };
static const uint8_t purple[12] = { 0, 128, 128, 0, 128, 128, 0, 128, 128, 0,
		128, 128 };
static const uint8_t yellow[12] = { 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0xFF,
		0xFF, 0x00, 0xFF, 0xFF, 0x00 };
static const uint8_t orange[12] = { 0x80, 0xFF, 0x00, 0x80, 0xFF, 0x00, 0x80,
		0xFF, 0x00, 0x80, 0xFF, 0x00 };
static const uint8_t steelBlue[12] = { 130, 70, 180, 130, 70, 180, 130, 70, 180,
		130, 70, 180 };
static const uint8_t pink[12] = { 20, 255, 147, 20, 255, 147, 20, 255, 147, 20,
		255, 147 };
static const uint8_t aqua[12] = { 255, 0, 255, 255, 0, 255, 255, 0, 255, 255, 0,
		255 };

static uint8_t z[12]; //GRB values for 4 LEDs
uint8_t pressCount;   //keep track of button presses
uint8_t buttonPressed = 1; //start with true so stored pressCount is executed

//for writing flash
uint8_t * Flash_ptr = (unsigned char *) 0x1040; //location to write in flash info segment C

void millisecDelay(int delay) {
	int i;

	for (i = 0; i < delay; i++)
		_delay_cycles(10000); //roughly 1 msec. calibrate this
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	_disable_interrupts();
	pressCount++;                    //increment button presses
	buttonPressed = 1;				//true
	millisecDelay(300); 			    //for button debounce
	P1IFG &= ~button;                //clear interrupt flag
	_enable_interrupts();
}

void writeFlash(unsigned char data) {
	_disable_interrupts();
	FCTL1 = FWKEY + ERASE;                    //Set Erase bit
	FCTL3 = FWKEY;                            //Clear Lock bit

	*Flash_ptr = 0;                         //Dummy write to erase Flash segment

	FCTL1 = FWKEY + WRT;                      //Set WRT bit for write operation

	*Flash_ptr = data;                        //Write value to flash

	FCTL1 = FWKEY;                            //Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     //Set LOCK bit
	_enable_interrupts();
}

void loadLED(int ledNum, uint8_t R, uint8_t G, uint8_t B) //load z[] for one LED
{
	z[ledNum * 3] = G;
	z[ledNum * 3 + 1] = R;
	z[ledNum * 3 + 2] = B;
}

void writeLEDs() {
	_disable_interrupts();
	write_ws2811_hs(z, sizeof(z), ledPin);
	_enable_interrupts();
}

void getRGB(int color, uint8_t *R, uint8_t *G, uint8_t *B) //this generates RGB values for 1972 rainbow colors from red to violet and back to red
{
	float brightness = 0.3; //scale factor to dim LEDs. if they are too bright, color washes out

	if (color >= (numColors / 2)) //adjust color for return from violet to red
		color = numColors - color;
	if (color < 199) {
		*R = 255;
		*G = 56 + color;
		*B = 57;
	} else if (color < 396) {
		*R = 254 - (color - 199);
		*G = 255;
		*B = 57;
	} else if (color < 592) {
		*R = 57;
		*G = 255;
		*B = 58 + (color - 396);
	} else if (color < 789) {
		*R = 57;
		*G = 254 - (color - 592);
		*B = 255;
	} else {
		*R = 58 + (color - 789);
		*G = 57;
		*B = 255;
	}
	*R *= brightness; //apply brightness modification
	*G *= brightness;
	*B *= brightness;
}

void allWhite(uint8_t intensity) //all LEDs stay white
{
	int i;

	for (i = 0; i < 12; i++)
		z[i] = intensity;
	writeLEDs();
}

void allBlack() //all LEDs off (black)
{
	int i;

	for (i = 0; i < 12; i++)
		z[i] = 0x00;
	writeLEDs();
}

void lightning() {
	uint8_t R, G, B;
	int color; //numeric index into color palette
	int flashDuration;

	while (!buttonPressed) {
		allBlack(); //clear LEDs
		color = rand() % numColors;
		if (color % 7 == 0) //every so often, throw in a bright white flash
				{
			R = G = B = 0xff;
			flashDuration = 10; //quicker for white flash
		} else {
			getRGB(color, &R, &G, &B); //get random color
			flashDuration = 50; //slower for color flash. makes color more apparent
		}
		loadLED(rand() % 4, R, G, B); //load color into random LED
		writeLEDs(); //flash the LED
		millisecDelay(flashDuration);
		allBlack(); //clear LEDs
		millisecDelay(rand() % 500); //wait for next flash
	}
}

void drawSnake(int headLoc) //draw snake starting at headLoc
{
	int shoulderLoc, torsoLoc, tailLoc;
	uint8_t R, G, B; //color of head

	getRGB(rand() % numColors, &R, &G, &B);
	shoulderLoc = (headLoc + 1) % 4; //find correct array locations for body parts. keep within 0-3 range
	torsoLoc = (headLoc + 2) % 4;
	tailLoc = (headLoc + 3) % 4;
	loadLED(headLoc, R, G, B); //random color head
	loadLED(shoulderLoc, 0, 0, 0);
	loadLED(torsoLoc, 0, 0, 0);
	loadLED(tailLoc, 0, 0, 0); //black tail
	writeLEDs(); //draw the snake
}

void chaseSnakeTail() //white head, black tail. Snake goes in circles
{
	int headLocation = 0; //which LED is the head (white)
	int crawlSpeed; //delay between snake moves
	int slowSpeed = 150;
	int fastSpeed = 10;
	int CWdirection = 1; //start with clockwise direction = true

	while (!buttonPressed) {
		for (crawlSpeed = slowSpeed; crawlSpeed > fastSpeed; crawlSpeed -= 2) //speed up tail chasing
				{
			drawSnake(headLocation);
			millisecDelay(crawlSpeed);
			if (CWdirection)
				headLocation += 3;
			else
				headLocation++;
			headLocation %= 4; //keep LED in range (0-3)
			if (buttonPressed)
				break;
		}

		CWdirection = !CWdirection; //reverse direction for next cycle

		for (crawlSpeed = fastSpeed; crawlSpeed < slowSpeed; crawlSpeed += 2) //slow down tail chasing
				{
			drawSnake(headLocation);
			millisecDelay(crawlSpeed);
			if (CWdirection)
				headLocation += 3;
			else
				headLocation++;
			headLocation %= 4; //keep LED in range (0-3)
			if (buttonPressed)
				break;
		}
	}
}

void allColors() //display rainbow color progression, each LED out of phase with the others
{
	uint8_t R, G, B;
	int color;
	int ledNum; //0-3

	while (!buttonPressed)
		for (color = 0; color < numColors; color++) //progress from red to violet and back to red
				{
			for (ledNum = 0; ledNum < 4; ledNum++) {
				getRGB((color + ledNum * 200) % numColors, &R, &G, &B); //offset LED colors by 200 from neighbors
				loadLED(ledNum, R, G, B);
			}
			writeLEDs();
			millisecDelay(4);
			if (buttonPressed) //allow exit from function
				break;
		}
}

void writeLeds(const uint8_t color[12]) {

	unsigned j;
	for (j = 0; j < 12; j++) {
		z[j] = color[j];
		write_ws2811_hs(z, sizeof(z), ledPin);
	}

}
void main(void) {
	WDTCTL = WDTPW + WDTHOLD;            // No watchdog reset

	DCOCTL = 0;
	BCSCTL1 = CALBC1_12MHZ; // Run at 12 MHz
	DCOCTL = CALDCO_12MHZ;

	//P1SEL &= ~ledPin | button;
	P1DIR |= ledPin;   // ledPin is an output
	P1OUT = 0;         // set port 1 low

	P1DIR &= ~button;  // button is an input
	P1REN |= button;   // pull down on button
	P1OUT &= ~button;  // set pull down
	P1IES &= ~button; // Interrupt Edge Select - 0: trigger on rising edge, 1: trigger on falling edge
	P1IFG &= ~button;  // interrupt flag for p1.3 is off
	P1IE |= button;    // enable interrupt

	FCTL2 = FWKEY + FSSEL_1 + FN3; // MCLK/32 for Flash Timing Generator 12mhz/32 = 375hz  // these both work has to be between 257-476 hz
	//FCTL2 = FWKEY + FSSEL_1 + 0x1A;             // MCLK/27 for Flash Timing Generator 12mhz/27 = 444hz

	pressCount = *Flash_ptr; // load value written in flash

	_enable_interrupts();

	while (1) {
		if (buttonPressed) {
			buttonPressed = 0; //reset

			switch (pressCount) {
			case 0: //bright white
				allWhite(0x80);
				break;
			case 1: //dim white
				allWhite(0x40);
				break;
			case 2:
				lightning();
				break;
			case 3:
				allColors();
				break;
			case 4:
				chaseSnakeTail();
				break;
			case 5:
				writeLeds(red);
				break;
			case 6:
				writeLeds(blue);
				break;
			case 7:
				writeLeds(green);
				break;
			case 8:
				writeLeds(purple);
				break;
			case 9:
				writeLeds(yellow);
				break;
			case 10:
				writeLeds(orange);
				break;
			case 11:
				writeLeds(steelBlue);
				break;
			case 12:
				writeLeds(pink);
				break;
			case 13:
				writeLeds(aqua);
				break;
			default:
				pressCount = 0; //default to 0
				buttonPressed = 1; //force execution of case 0
				break;
			}
			writeFlash(pressCount);  // write to flash for poweroff
		}
	}
}

