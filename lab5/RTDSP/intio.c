/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O
								   MODIFIED FOR LAB 4!
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

#include "coeffs.txt" // this holds our coefficients used for the FIR filter.
/****************** Global declarations ********************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /*****************************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /*******************************************************/\
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
			 /***********************************************/
};

#define N 78 // our number of samples is order of filter + 1. This value comes from matlab

double x[N]; // for double memory circular IIR, change this to x[2N];
double sample;
double output;
int i;
int circIndex = 0;
int circIndex2 = N; //used for double memory circular IIR - see notes

// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

 /**************** Function prototypes **********************/
void init_hardware(void);     
void init_HWI(void);                   
/********************* Main routine *************************/
void main(){
  // initialize board and the audio port
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {};
  
}
        
/******************* init_hardware() *********************/  
void init_hardware()
{
	// We initialise our input buffer here to make sure that it is wiped.
	for (i = 0; i<N; i++) // for the double-memory implementation, change to i<2N
	{
		x[i] = 0.0;
	}
	
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
double low_pas_IIR(double input)
{
	
}

double non_circ_FIR(void)
{
	// Let's first shift all the values along one
	// i.e. x[4] = x[3], ..., x[1] = x[0]
	for (i=N-1; i>0; i--)
	{
		x[i] = x[i-1];
	}
	//and now we put the new value in
	x[0] = mono_read_16Bit();
	
	output = 0.0; //reset output & perform MAC on all i
	for (i = N-1; i>=0; i--)
	{
		output += x[i] * b[i];
	}

	return output; //return the output here
}

double basic_circ_FIR(void)
{
	//circIndex points to the oldest value, so lets overwrite that!
	x[circIndex] = mono_read_16Bit();

	output = 0.0; //reset output
	
	// loop until we reach the end of the input buffer (circIndex + i overflows)
	for( i=0; i<N-circIndex; i++ ) {
		output += x[circIndex+i] * b[i];
	}
	
	// continue the MAC from where we left off, negating N though.
	// This effectively implements MOD N, but without the overhead!
	for (i = N-circIndex; i<N; i++){
		output += x[circIndex+i-N]*b[i];
	}
	
	// decrement circIndex, so that it points to the new 'oldest' x-value
	circIndex--;
	if (circIndex < 0) { circIndex=N-1; } //loop it around
	return output;
}


double symmetrical_circ_FIR_even(void)
{
	x[circIndex] = mono_read_16Bit();
	output = 0.0;

	// This is assuming out filter is EVEN
	// If our index is in the lower half of the input buffer,
	// we need to make sure that the x[index-1-i] doesn't go below -1.
	if (circIndex < (N/2))
	{
		// here we make sure x[index-1-i] stays above 0
		for( i=0; i<circIndex; i++ ) {
			output += b[i] * (x[circIndex+i] + x[circIndex - 1 - i]);
		}
		for (i=circIndex; i<N/2; i++){
			output += b[i] * (x[circIndex+i] + x[circIndex - 1 - i + N]);
		}
	}
	else // here x[index+1] will overload
	{
		// Here we make sure that x[index+i] stays below N-1
		for( i=0; i<N-circIndex; i++ ) {
			output += b[i] * (x[circIndex+i] + x[circIndex - 1 - i]);
		}
		for (i=N-circIndex; i<N/2; i++){
			output += b[i] * (x[circIndex + i - N] + x[circIndex - 1 - i]);
		}
	}
	
	//decrement index to make sure that the buffers are kept in order (as above)
	circIndex--;
	if (circIndex < 0) { circIndex=N-1; } //loop it around, ernie!
	return output;
}

double symmetrical_circ_doublememory_FIR(void)
{
	// Use double the memory to make things pretty cool!

	x[circIndex] = mono_read_16Bit(); //put input into the current index..
	x[circIndex2] = x[circIndex];	// .. and copy it to the secnod index.
	// we cant use the mono_read_16Bit(), as this might read the next value!


	output = 0.0; // reset output

	// now, let's loop through our filter coefficients, and multiply by the
	// correct x inputs, avoiding the fear of overloading!
	for (i=0; i<N/2; i++)
	{
		// x[index+i] can grow up to 2N-index, which is plenty
		// x[index2-1-i] can go down to 2N-index, again, loads.
		output += b[i] * ( x[circIndex+i] + x[circIndex2-1-i]);
	}

	//to update the indexes, we look at performing the logic on circIndex
	circIndex --;
	if (circIndex < 0) { circIndex=N-1; }
	//and then propagating this value to circIndex2!
	circIndex2 = circIndex + N;
	return output; //Done!
}

void ISR_AIC(void)
{
	mono_write_16Bit(non_circ_FIR());
	//mono_write_16Bit(basic_circ_FIR());
	//mono_write_16Bit(symmetrical_circ_FIR_even());

	// MAKE SURE YOU MAKE THE NECCESSARY CHANGES BEFORE USING
	// 1.) INCREASE the size of X to X[2*N]
	// 2.) INCREASE the for loop in the main() to initialise x properly.

	//mono_write_16Bit(symmetrical_circ_doublememory_FIR());
}

