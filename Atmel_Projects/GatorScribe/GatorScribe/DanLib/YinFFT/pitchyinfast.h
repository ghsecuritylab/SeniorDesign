#ifndef PITCHYINFAST_H
#define PITCHYINFAST_H

#include <math.h>
#include "arm_math.h"
#include "DMA_Audio.h"

//#define YIN_FFT_SAMPLING_RATE 22900 

// using sine wave to determine best rate 
#define YIN_FFT_SAMPLING_RATE 23250

//#define YIN_FFT_SAMPLING_RATE 46500

typedef struct {
	uint32_t length;  /**< length of buffer */
	float *data;   /**< data vector of length ::fvec_t.length */
} fvec_t;

/** pitch detection object */
typedef struct 
{
	fvec_t *yin;
	float tol;
	uint32_t peak_pos;
	fvec_t *tmpdata;
	fvec_t *sqdiff;
	fvec_t *kernel;
	fvec_t *samples_fft;
	fvec_t *kernel_fft;
	fvec_t *rt_of_tau;
}yin_t;


yin_t *yin_init (void);

//void del_yin (yin_t * o); 

float yin_get_pitch (yin_t * o, fvec_t * input, arm_rfft_fast_instance_f32 *fftInstance);

float yin_get_confidence (yin_t * o);

#endif /* PITCHYINFAST_H */

