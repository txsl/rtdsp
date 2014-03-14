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

double x[N];
double y[N];
double v[N];
double sample;
double output;
int i;
int index=0;

double x_1;
double y_1;

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
	y_1 = input/17.0 + x_1/17.0 + (15/17)*y_1;
	x_1 = input;
	
	return y_1;
}

double IIR_2_noncirc(double input){
	//this function fulfils IIR Direct form ii
	//filter - this version does not implement a circular buffer //a possible efficiency tweak
	//use:reads global variable sample and returns filtered value v[0]=sample; //write input to v[0]
	output = 0.0; //reset output to accumulate result
	v[0] = input;
	//loop for all values of v to accumulate them to output
	for (i=N-1;i>0;i--){
		v[0] -= a[i]*v[i]; //accumulate a coefficients
		output += b[i]*v[i]; //accumulate to output
		v[i] = v[i-1]; //shift v[i] data down to represent the delay elements //in a IIR Direct Form 2 filter
	}
	output += b[0]*v[0]; //write final values to output return output; //return filtered value
	return output;
}

double IIR_2_circ() {
	output = 0.0;
	v[0] = sample;

	for (i=N; i>N-index; i--)
	{
		v[index] -= a[i] * v[index + i - N]; //accumulate a coefficients
		output   += b[i] * v[index + i - N]; //accumulate to output
	}

	for (i=N-index; i>0; i--)
	{
		v[index] -= a[i] * v[index + i]; //accumulate a coefficients
		output   += b[i] * v[index + i]; //accumulate to output
	}

	return output;
}


double IIR_2_trans(){
	//this function reads the global sample variable and returns
	//a filtered value(the output)
	//this function is based on equations derived from a
	//IIR Direct form II transposed filter.
	output = v[0] + b[0]*sample; //calculate output based on buffer data
	//loop to complete each iteration of v[n]=v[n-1]+b_1 x[n-1]-a_1 y[n]
	//to populate buffer
	for(i = 0;i<N-1;i++){
		// add new weighted inputs and outputs to previous buffer value v[n+1]
		v[i] = v[i+1] + b[i+1]*sample - a[i+1]*output;
	}
	//calculate first value of v which does not rely on previous versions
	v[N-2] = b[N-1]*sample - a[N-1]*output;
	return output; //return filtered value
}


void ISR_AIC(void)
{
	//mono_write_16Bit(low_pass_IIR(mono_read_16Bit()));
	mono_write_16Bit(IIR_2_noncirc(mono_read_16Bit()));
	//mono_write_16Bit(low_pass_IIR(mono_read_16Bit()));
	
}

