/**********************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		  LAB 3: Interrupt I/O
								   MODIFIED FOR LAB 4!
								   MODIFIED FOR LAB 5!
 				            ********* I N T I O. C **********

  Demonstrates inputing and outputing data from the DSK's audio port using interrupts. 

 ***************************************************************
 				Updated for use on 6713 DSK by Danny Harvey: May-Aug 2006
				Updated for CCS V4 Sept 10
 ***************************************************************/
/*
 *	You should modify the code so that interrupts are used to service the 
 *  audio port.
 */
/********************** Pre-processor statements ********************/

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
/**************** Global declarations ***************/

/* Audio port configuration settings: these values set registers in the AIC23 audio 
   interface to configure it. See TI doc SLWS106D 3-3 to 3-10 for more info. */
DSK6713_AIC23_Config Config = { \
			 /***********************************************/
			 /*   REGISTER	            FUNCTION			      SETTINGS         */ 
			 /**************************************************/\
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
			 /*****************************************/
};

double x[ORDER+1];
double y[ORDER+1];
double d[ORDER+1];
double sample;
double output;
int i;
int index=0;

double x_1 = 0;
double y_1 = 0;

// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

 /************** Function prototypes *******************/
void init_hardware(void);     
void init_HWI(void);            
/****************** Main routine *********************/
void main(){
  // initialize board and the audio port
  init_hardware();
	
  /* initialize hardware interrupts */
  init_HWI();
  	 		
  /* loop indefinitely, waiting for interrupts */  					
  while(1) 
  {};
  
}
        
/**************** init_hardware() ******************/  
void init_hardware()
{
	// We initialise our input buffer here to make sure that it is wiped.
	for (i = 0; i<=ORDER; i++) // for the double-memory implementation, change to i<2N
	{
		x[i] = 0.0;
		d[i] = 0.0;
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

/******************* init_HWI() ************************/  
void init_HWI(void)
{
	IRQ_globalDisable();			// Globally disables interrupts
	IRQ_nmiEnable();				// Enables the NMI interrupt (used by the debugger)
	IRQ_map(IRQ_EVT_RINT1,4);		// Maps an event to a physical interrupt
	IRQ_enable(IRQ_EVT_RINT1);		// Enables the event
	IRQ_globalEnable();				// Globally enables interrupts

} 

/************** WRITE YOUR INTERRUPT SERVICE ROUTINE HERE*******************/  
double low_pass_IIR(double input)
{
	y_1 = input/17.0 + x_1/17.0 + (15.0/17.0)*y_1;
	x_1 = input;
	//y_2 = y_1;
	return y_1;
}

double IIR_2_noncirc(){
	// implementing the basic IIR II filter in a non-circular manner
	// We first set output to 0, and read in the sample to the first block (Delay = 0)
	output = 0.0;
	d[0] = mono_read_16Bit();
	
	for (i=ORDER;i>0;i--){
		// We now go through each order of the filter
		// accumulate all delay items to d[0]
		d[0] -= a[i]*d[i];
		
		// accumulate all delay items to output y_0
		output += b[i]*d[i];
		
		// delay evey item
		d[i] = d[i-1]; 
	}
	
	//add the final item, which doesn't exist for a[0]
	output += b[0]*d[0];
	
	return output;
}

double IIR_2_circ() {
	// very similar to above, but we implement the circular buffer
	// to remove the above d[i] = d[i-1]
	
	output = 0.0;
	d[index] = mono_read_16Bit();
	
	// do this only while index
	for (i=ORDER; i>ORDER-index; i--)
	{
		// accumulate all delay items to d[0], avoiding overflow by doing -N
		d[index] -= a[i] * d[index + i - ORDER - 1];
		// accumulate all delay items to output y_0, avoiding overflow by doing -N
		output   += b[i] * d[index + i - ORDER - 1];
	}

	for (i=ORDER-index; i>=0; i--)
	{
		// accumulate all delay items to d[0]
		d[index] -= a[i] * d[index + i];
		// accumulate all delay items to output y_0
		output   += b[i] * d[index + i];
	}
	
	//update the index - we decrement so to keep the items in d[] ordered
	index--;
	if (index < 0) { index = ORDER; }
	
	return output;
}

double IIR_2_trans(){
	// implement the IIR filter using the transposed form.
	// we save to sample so we can use the item more than once.
	sample = mono_read_16Bit();
	
	// set our current output using previously calculated values.
	output = b[0]*sample + d[0];

	// calculate new values of d
	for(i = 0;i<ORDER;i++){
		d[i] = b[i+1]*sample - a[i+1]*output + d[i+1];
	}
	// created the highest order D value which depends on inputs and outputs
	d[ORDER-1] = b[ORDER]*sample - a[ORDER]*output; // remove the d[i+1] from here
	return output; //return filtered value
}

void ISR_AIC(void)
{
<<<<<<< HEAD
	mono_write_16Bit(mono_read_16Bit());
	//mono_write_16Bit(low_pass_IIR(mono_read_16Bit()));
	// mono_write_16Bit(IIR_2_noncirc());
	//mono_write_16Bit(IIR_2_circ());
	//mono_write_16Bit(low_pass_IIR(mono_read_16Bit()));
	
=======
	mono_write_16Bit(IIR_2_noncirc());
>>>>>>> 61bbc92e200af0ce1a6bd3ac749148dfb1c9c2bb
}

