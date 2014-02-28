/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O

 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 *************************************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ************************************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/**************************** Pre-processor statements ******************************/

#include <stdlib.h>
//  Included so program can make use of DSP/BIOS configuration tool.  
#include "dsp_bios_cfg.h"

/* The file dsk6713.h must be included in every program that uses the BSL.  This 
   example also includes dsk6713_aic23.h because it uses the 
   AIC23 codec module (audio interface). */
#include "dsk6713.h"
#include "dsk6713_aic23.h"

// math library (trig functions)
#include <math.h>

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

// Constants needed for our sine wave generation
#define PI 3.141592653 // define PI for sine table generation
#define SINE_TABLE_SIZE 256 // define size of the table

/******************************* Global declarations ********************************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /**********************************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**********************************************************************/\
    0x0017,  /* 0 LEFTINVOL  Left line input channel volume  0dB                   */\
    0x0017,  /* 1 RIGHTINVOL Right line input channel volume 0dB                   */\
    0x01f9,  /* 2 LEFTHPVOL  Left channel headphone volume   0dB                   */\
    0x01f9,  /* 3 RIGHTHPVOL Right channel headphone volume  0dB                   */\
    0x0011,  /* 4 ANAPATH    Analog audio path control       DAC on, Mic boost 20dB*/\
    0x0000,  /* 5 DIGPATH    Digital audio path control      All Filters off       */\
    0x0000,  /* 6 DPOWERDOWN Power down control              All Hardware on       */\
    0x0043,  /* 7 DIGIF      Digital audio interface format  16 bit                */\
    0x008d,  /* 8 SAMPLERATE Sample rate control             8 KHZ                 */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};


// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

// Our global variables for the output
float sine_freq = 1000; //let's default to 1000 
float table[SINE_TABLE_SIZE]; // table to store sine wave
float index = 0; // our current position in the table - used to generate sine waves
float gain = 32767; // output must be 16 BIT SGN = 15BIT USGN.
					// Sine is between 0-1, so gain*1 must be below max(15bit USGN)
					// (2^15) - 1 is biggest int we can represent
					// = 32767! 

 /******************************* Function prototypes ********************************/
void init_sine(void);
void init_hardware(void);
void init_HWI(void);
/********************************** Main routine ************************************/
void main(){

	// initliase sine
	init_sine();

	// initialize board and the audio port
	init_hardware();

	// initialize hardware interrupts
	init_HWI();
			
	// loop indefinitely, waiting for interrupts
	while(1) 
	{};
}
        
/********************************** init_hardware() **********************************/  
void init_hardware()
{
    // Initialize the board support library, must be called first 
    DSK6713_init();
    
    // Start the AIC23 codec using the settings defined above in config 
    H_Codec = DSK6713_AIC23_openCodec(0, &Config);

	/* Function below sets the number of bits in word used by MSBSP (serial port) for 
	receives from AIC23 (audio port). We are using a 32 bit packet containing two 
	16 bit numbers hence 32BIT is set for  receive */
	MCBSP_FSETS(RCR1, RWDLEN1, 32BIT);	

	/* Configures interrupt to activate on each consecutive available 32 bits 
	from Audio port hence an interrupt is generated for each L & R sample pair */	
	MCBSP_FSETS(SPCR1, RINTM, FRM);

	/* These commands do the same thing as above but applied to data transfers to  
	the audio port */
	MCBSP_FSETS(XCR1, XWDLEN1, 32BIT);	
	MCBSP_FSETS(SPCR1, XINTM, FRM);	
	

}

/********************************** init_sine() **********************************/  
void init_sine()
{
	// initialising our sinusoid at this point.
	// To save time at the output stage, we're creating an "absolute" sine table
	// i.e. our sin does NOT have negative values!

    int i;
	for (i = 0; i < SINE_TABLE_SIZE; i++)
	{
		float temp = sin(2*PI*i / SINE_TABLE_SIZE);
		
		// get absolute value...
		if (temp < 0) {
			temp = -temp;
		}
		
		// save it up!
		table[i] = temp;
	}
}


/********************************** init_HWI() **************************************/  
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts

} 

/******************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE***********************/  

void ISR_AIC(void)
{
	// We save the position in the sine table through the global variable 'index'
	// On every call, we add to the % of the sine table we go through at each sample, which is SIZE_OF_TABLE/(sample_rate/sine_freq)
	// However, we hardcore our sample rate at 8000 Hz 
	index += (SINE_TABLE_SIZE)/(8000/sine_freq);

	// we need to make sure we're in the correct index
	while(index > SINE_TABLE_SIZE) {
		index -= SINE_TABLE_SIZE;
	}

	// and let's output our number magic!
	// This is the value from the table selected based on the index, multiplied by our gain, and converted to an integer.
	mono_write_16Bit((int)(table[(int)index] * gain));
}

