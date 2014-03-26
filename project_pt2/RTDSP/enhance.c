/*************************************************************************************
			       DEPARTMENT OF ELECTRICAL AND ELECTRONIC ENGINEERING
					   		     IMPERIAL COLLEGE LONDON 

 				      EE 3.19: Real Time Digital Signal Processing
					       Dr Paul Mitcheson and Daniel Harvey

				        		 PROJECT: Frame Processing

 				            ********* ENHANCE. C **********
							 Shell for speech enhancement 

  		Demonstrates overlap-add frame processing (interrupt driven) on the DSK. 

 *************************************************************************************
 				             By Danny Harvey: 21 July 2006
							 Updated for use on CCS v4 Sept 2010
 ************************************************************************************/
/*
 *	You should modify the code so that a speech enhancement project is built 
 *  on top of this template.
 */
/**************************** Pre-processor statements ******************************/
//  library required when using calloc
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

/* Some functions to help with Complex algebra and FFT. */
#include "cmplx.h"      
#include "fft_functions.h"  

// Some functions to help with writing/reading the audio ports when using interrupts.
#include <helper_functions_ISR.h>

#define WINCONST 0.85185			/* 0.46/0.54 for Hamming window */
#define FSAMP 8000.0		/* sample frequency, ensure this matches Config for AIC */
#define FFTLEN 256					/* fft length = frame length 256/8000 = 32 ms*/
#define NFREQ (1+FFTLEN/2)			/* number of frequency bins from a real FFT */
#define OVERSAMP 4					/* oversampling ratio (2 or 4) */  
#define FRAMEINC (FFTLEN/OVERSAMP)	/* Frame increment */
#define CIRCBUF (FFTLEN+FRAMEINC)	/* length of I/O buffers */

#define OUTGAIN 16000.0				/* Output gain for DAC */
#define INGAIN  (1.0/16000.0)		/* Input gain for ADC  */
// PI defined here for use in your code 
#define PI 3.141592653589793
#define TFRAME FRAMEINC/FSAMP       /* time between calculation of each frame */

#define FRAMES_PER_WINDOW 312
#define WINDOWS 4
#define TIME_CONST .002 // Ie 20ms
float k_filter;

int enable[10];
float LAMBDA = 0.1;
float ALPHA = 20;
float alphamod[2][FFTLEN];

float max(float a, float b) { if (a > b) return a; return b; }
float min(float a, float b) { if (a < b) return a; return b; }

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
    0x008d,  /* 8 SAMPLERATE Sample rate control        8 KHZ-ensure matches FSAMP */\
    0x0001   /* 9 DIGACT     Digital interface activation    On                    */\
			 /**********************************************************************/
};

// Codec handle:- a variable used to identify audio interface  
DSK6713_AIC23_CodecHandle H_Codec;

float *inbuffer, *outbuffer;   		/* Input/output circular buffers */
float *inframe, *outframe;          /* Input and output frames */
float *inwin, *outwin;              /* Input and output windows */
float ingain, outgain;				/* ADC and DAC gains */ 
float cpufrac; 						/* Fraction of CPU time used */
volatile int io_ptr=0;              /* Input/ouput pointer for circular buffers */
volatile int frame_ptr=0;           /* Frame pointer */

complex *inframe_c;
float *fftbin;
int winstage = 0;
int frame_position = 0;

struct transform
{
	float bin[FFTLEN];
};

struct transform min_window[WINDOWS];
struct transform p_fftbin;
struct transform p_power_fftbin;
struct transform p_min_noise;

 /******************************* Function prototypes *******************************/
void init_hardware(void);    	/* Initialize codec */ 
void init_HWI(void);            /* Initialize hardware interrupts */
void ISR_AIC(void);             /* Interrupt service routine for codec */
void process_frame(void);       /* Frame processing routine */
           
/********************************** Main routine ************************************/
void main()
{      

  	int k; // used in various for loops
  	int i; // also used as a counter
  	
  	k_filter = exp(-TFRAME/TIME_CONST);
  
/*  Initialize and zero fill arrays */  

	inbuffer	= (float *) calloc(CIRCBUF, sizeof(float));	/* Input array */
    outbuffer	= (float *) calloc(CIRCBUF, sizeof(float));	/* Output array */
	inframe		= (float *) calloc(FFTLEN, sizeof(float));	/* Array for processing*/
    outframe	= (float *) calloc(FFTLEN, sizeof(float));	/* Array for processing*/
    inwin		= (float *) calloc(FFTLEN, sizeof(float));	/* Input window */
    outwin		= (float *) calloc(FFTLEN, sizeof(float));	/* Output window */
    
    inframe_c = (complex *) calloc(FFTLEN, sizeof(complex));
	fftbin = (float *) calloc(FFTLEN, sizeof(float));
	
	for(i=0;i<WINDOWS;i++)	
		for(k=0;k<FFTLEN;k++)
			min_window[i].bin[k] = FLT_MAX;

	/* initialize board and the audio port */
  	init_hardware();
  
  	/* initialize hardware interrupts */
  	init_HWI();    
  
/* initialize algorithm constants */

	for (k=0;k<10;k++)
	{
		enable[k] = 0;
	}
                       
  	for (k=0;k<FFTLEN;k++)
	{                           
		inwin[k] = sqrt((1.0-WINCONST*cos(PI*(2*k+1)/FFTLEN))/OVERSAMP);
		outwin[k] = inwin[k]; 
		
		// set up alphamodifiers
		alphamod[0][k] = ALPHA;
		
		alphamod[1][k] = 0;
		alphamod[2][k] = 0;
		if (k/FFTLEN > 80/8000 && k/FFTLEN < 7200/8000) //if our k represents > 80Hz @ 8000Hz Sampling, then 1
		{
			alphamod[1][k] = ALPHA;
			alphamod[2][k] = ALPHA;
		}
		if (k/FFTLEN > 3000/8000 && k/FFTLEN < 5000/8000)
		{
			alphamod[2][k] = 0;
		}
			
	}
  	ingain=INGAIN;
  	outgain=OUTGAIN;        

 							
  	/* main loop, wait for interrupt */  
  	while(1) 	process_frame();
}
    
/********************************** init_hardware() *********************************/  
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

	/* These commands do the same thing as above but applied to data transfers to the 
	audio port */
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
      
/******************************** process_frame() ***********************************/  
void process_frame(void)
{
	int i, k, m; 
	int io_ptr0;
	struct transform g, min_noise;

	// We always use FLT_MAX because we are looking for the minimum value, so we start at the highest possible value as a default.
	for(k=0;k<FFTLEN;k++) // Set g ready for use later on
	{
		g.bin[k] = FLT_MAX;
		min_noise.bin[k] = FLT_MAX;
	}

	/* work out fraction of available CPU time used by algorithm */    
	cpufrac = ((float) (io_ptr & (FRAMEINC - 1)))/FRAMEINC;  
	
	/* wait until io_ptr is at the start of the current frame */ 	
	while((io_ptr/FRAMEINC) != frame_ptr); 
	
	/* then increment the framecount (wrapping if required) */ 
	if (++frame_ptr >= (CIRCBUF/FRAMEINC)) frame_ptr=0;
 	
 	/* save a pointer to the position in the I/O buffers (inbuffer/outbuffer) where the 
 	data should be read (inbuffer) and saved (outbuffer) for the purpose of processing */
 	io_ptr0=frame_ptr * FRAMEINC;
	
	/* copy input data from inbuffer into inframe (starting from the pointer position) */ 
	 
	m=io_ptr0;
    for (k=0;k<FFTLEN;k++)
	{                           
		inframe[k] = inbuffer[m] * inwin[k]; 
		if (++m >= CIRCBUF) m=0; /* wrap if required */
	} 
	
	/************************* DO PROCESSING OF FRAME  HERE **************************/
	
	// Put our samples in the correct (complex) format for the FFT
	for (k=0;k<FFTLEN;k++)
	{                           
		inframe_c[k].r = inframe[k]; 
		inframe_c[k].i = 0;
	}

	// Take the FFT of our 256 samples
	fft(FFTLEN, inframe_c);

	// Since we have no information on phase, we take the absolute value of each frequency bin
	for (k=0;k<FFTLEN;k++)
	{
		fftbin[k] = cabs(inframe_c[k]);
	}
	
	// Go through all frequency bins in turn, and find the minimum value. This is the basic spectral subtraction routine.
 	for (k=0;k<FFTLEN;k++)
	{
		// since we need P(x) at a later time anyway, let's keep it here.
		p_fftbin.bin[k] = (((1 - k_filter) * fftbin[k]) + (k_filter * p_fftbin.bin[k]));
		if (enable[1] == 1)
		{
			// set fftbin to p_fftbin
			fftbin[k] = p_fftbin.bin[k];
		} else if (enable[1] == 2)
		{
			// Update the value of the previous fftbin, so that we can keep this for later...
			p_power_fftbin.bin[k] = sqrt(((1 - k_filter) * fftbin[k] * fftbin[k]) + (k_filter * p_power_fftbin.bin[k] * p_power_fftbin.bin[k]));
			
			// set fftbin to p_fftbin
			fftbin[k] = p_power_fftbin.bin[k];
		}
		
		// Update the current 2.5s window minimum value if the current FFT sample is lower in amplitude than the one stored
		if (fftbin[k] < min_window[winstage].bin[k]) //ENH1
			min_window[winstage].bin[k] = fftbin[k]; //ENH1
		
		// We iterate through all 2.5s windows to find the minimum noise amongst them, and set it as the variable min_noise
		for (i=0;i<WINDOWS;i++)
		{
			if (min_noise.bin[k] > min_window[i].bin[k])
				min_noise.bin[k] = min_window[i].bin[k];
		}
		
		if (enable[2] == 1)
		{
			// Update the value of the previous fftbin, so that we can keep this for later...
			p_min_noise.bin[k] = (((1 - k_filter) * min_noise.bin[k]) + (k_filter * p_min_noise.bin[k]));
			
			// set min_noise to p_min_noise
			min_noise.bin[k] = p_min_noise.bin[k];
		}
		
		
		// this is messy and needs to be re-written
		// We either take the estimated noise, or if our LAMBDA value is bigger, we use that instead.
		if (enable[4] == 0)
		{
			switch (enable[3]) {
				case 1:
					g.bin[k] = max (LAMBDA * ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]), 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]));
					break;
				case 2:
					g.bin[k] = max (LAMBDA * p_fftbin.bin[k]/fftbin[k], 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]));
					break;
				case 3:
					g.bin[k] = max (LAMBDA*(alphamod[enable[5]][k] * min_noise.bin[k])/p_fftbin.bin[k] , 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / p_fftbin.bin[k]));
					break;
				case 4:
					g.bin[k] = max (LAMBDA, 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / p_fftbin.bin[k]));
					break;
				default: 
					g.bin[k] = max (LAMBDA, 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]));	
			}
		}
		else if (enable[4] == 1)
		{
			switch (enable[3]) {
				case 1:
					g.bin[k] = max (LAMBDA * ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]), 1 - ((alphamod[enable[5]][k] * min_noise.bin[k]) / fftbin[k]));
					break;
				case 2:
					g.bin[k] = max (LAMBDA * p_fftbin.bin[k]/fftbin[k], sqrt(1 - ((alphamod[enable[5]][k] * min_noise.bin[k] * alphamod[enable[5]][k] * min_noise.bin[k]) / (fftbin[k]* fftbin[k]))));
					break;
				case 3:
					g.bin[k] = max (LAMBDA*(alphamod[enable[5]][k] * min_noise.bin[k])/p_fftbin.bin[k] , sqrt(1 - ((alphamod[enable[5]][k] * min_noise.bin[k] * alphamod[enable[5]][k] * min_noise.bin[k]) / (p_fftbin.bin[k]*p_fftbin.bin[k]))));
					break;
				case 4:
					g.bin[k] = max (LAMBDA, sqrt(1 - ((alphamod[enable[5]][k] * min_noise.bin[k] * alphamod[enable[5]][k] * min_noise.bin[k]) / (p_fftbin.bin[k]*p_fftbin.bin[k]))));
					break;
				default: 
					g.bin[k] = max (LAMBDA, sqrt(1 - (alphamod[enable[5]][k]*min_noise.bin[k]*alphamod[enable[5]][k]*min_noise.bin[k] / (fftbin[k]*fftbin[k]))) );	
			}
		}
		
		
	
		// Multiply the complex FFT (X(w) with our value of G(w))
		
		if (enable[0] == 1)
			inframe_c[k] = rmul(g.bin[k], inframe_c[k]);
	}
	
	
	
	
	
	
	

	// Move the frame position along one, and if at the end, reset and check if we need to use a different 2.5s window
	if (frame_position < FRAMES_PER_WINDOW)
	{
		frame_position++; // frame_position is only used to tell if we need to move to a new 2.5s interval.
	}
	else // We need to do some specicial stuff if we've gone through a 2.5 second segment
	{
		frame_position = 0; // reset the frame position to 0 for the next window stage
		if(winstage<WINDOWS) // Either move on to the next 2.5s window
			winstage++;
		else				// Or reset it to 0
			winstage = 0; 

		for(k=0;k<FFTLEN;k++) // Then clear the old data away
		{
			min_window[winstage].bin[k] = FLT_MAX;
		}
	}
	
	ifft(FFTLEN, inframe_c);
									
    for (k=0;k<FFTLEN;k++)
	{                           
		outframe[k] = inframe_c[k].r;/* copy input straight into output */ 
	} 

	/********************************************************************************/
	
    /* multiply outframe by output window and overlap-add into output buffer */  
                           
	m=io_ptr0;
    
    for (k=0;k<(FFTLEN-FRAMEINC);k++) 
	{    										/* this loop adds into outbuffer */                       
	  	outbuffer[m] = outbuffer[m]+outframe[k]*outwin[k];   
		if (++m >= CIRCBUF) m=0; /* wrap if required */
	}         
    for (;k<FFTLEN;k++) 
	{                           
		outbuffer[m] = outframe[k]*outwin[k];   /* this loop over-writes outbuffer */        
	    m++;
	}	                                   
}        
/*************************** INTERRUPT SERVICE ROUTINE  *****************************/

// Map this to the appropriate interrupt in the CDB file
   
void ISR_AIC(void)
{       
	short sample;
	/* Read and write the ADC and DAC using inbuffer and outbuffer */
	
	sample = mono_read_16Bit();
	inbuffer[io_ptr] = ((float)sample)*ingain;
		/* write new output data */
	mono_write_16Bit((int)(outbuffer[io_ptr]*outgain)); 
	
	/* update io_ptr and check for buffer wraparound */    
	
	if (++io_ptr >= CIRCBUF) io_ptr=0;
}

/************************************************************************************/
