/*
  Copyright (C) 2003-2017 Paul Brossier <piem@aubio.org>

  This file is part of aubio.

  aubio is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  aubio is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with aubio.  If not, see <http://www.gnu.org/licenses/>.

*/

/* This algorithm was developed by A. de Cheveigné and H. Kawahara and
 * published in:
 *
 * de Cheveigné, A., Kawahara, H. (2002) "YIN, a fundamental frequency
 * estimator for speech and music", J. Acoust. Soc. Am. 111, 1917-1930.
 *
 * see http://recherche.ircam.fr/equipes/pcm/pub/people/cheveign.html
 */

#include <asf.h>

#include "pitchyinfast.h"
#include "DMA_Audio.h"

COMPILER_ALIGNED(WIN_SIZE>>1) static float _yin[WIN_SIZE>>1]; 
COMPILER_ALIGNED(WIN_SIZE) static float _tmpdata[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE>>1) static float _sqdiff[WIN_SIZE>>1];
COMPILER_ALIGNED(WIN_SIZE) static float _kernel[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float _samples_fft[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float _kernel_fft[WIN_SIZE];

static float fvec_quadratic_peak_pos (const fvec_t * x, uint32_t pos) {
	float s0, s1, s2; uint32_t x0, x2;
	float half = .5, two = 2.;
	if (pos == 0 || pos == x->length - 1) return (float)pos;
	x0 = (pos < 1) ? pos : pos - 1;
	x2 = (pos + 1 < x->length) ? pos + 1 : pos;
	if (x0 == pos) return (x->data[pos] <= x->data[x2]) ? (float)pos : (float)x2;
	if (x2 == pos) return (x->data[pos] <= x->data[x0]) ? (float)pos : (float)x0;
	s0 = x->data[x0];
	s1 = x->data[pos];
	s2 = x->data[x2];
	return (float)(pos + half * (s0 - s2 ) / (s0 - two * s1 + s2));
}

yin_t * yin_init (void)
{
	uint32_t i; 
	yin_t *o = (yin_t*)calloc(sizeof(yin_t), 1);
	/*
	o->yin = new_fvec (WIN_SIZE / 2);
	o->tmpdata = new_fvec (WIN_SIZE);
	o->sqdiff = new_fvec (WIN_SIZE / 2);
	o->kernel = new_fvec (WIN_SIZE);
	o->samples_fft = new_fvec (WIN_SIZE);
	o->kernel_fft = new_fvec (WIN_SIZE);
	*/
	o->tol = 0.05; // changed from 0.15
	o->peak_pos = 0;
	
	o->yin = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->yin->data = _yin; 
	o->yin->length = sizeof(_yin)/sizeof(_yin[0]);
	for(i = 0; i < o->yin->length; i++) o->yin->data[i] = 0.0; 
	
	o->tmpdata = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->tmpdata->data = _tmpdata; 
	o->tmpdata->length = sizeof(_tmpdata)/sizeof(_tmpdata[0]);
	for(i = 0; i < o->tmpdata->length; i++) o->tmpdata->data[i] = 0.0;

	o->sqdiff = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->sqdiff->data = _sqdiff; 
	o->sqdiff->length = sizeof(_sqdiff)/sizeof(_sqdiff[0]);
	for(i = 0; i < o->sqdiff->length; i++) o->sqdiff->data[i] = 0.0;
	
	o->kernel = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->kernel->data = _kernel;
	o->kernel->length = sizeof(_kernel)/sizeof(_kernel[0]);
	for(i = 0; i < o->kernel->length; i++) o->kernel->data[i] = 0.0;
	
	o->samples_fft = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->samples_fft->data = _samples_fft;
	o->samples_fft->length = sizeof(_samples_fft)/sizeof(_samples_fft[0]);
	for(i = 0; i < o->samples_fft->length; i++) o->samples_fft->data[i] = 0.0;
	
	o->kernel_fft = (fvec_t*)calloc(sizeof(fvec_t), 1);
	o->kernel_fft->data = _kernel_fft;
	o->kernel_fft->length = sizeof(_kernel_fft)/sizeof(_kernel_fft[0]);
	for(i = 0; i < o->kernel_fft->length; i++) o->kernel_fft->data[i] = 0.0;

	return o;
}

/*
void del_yin (yin_t * o)
{
	del_fvec (o->yin);
	del_fvec (o->tmpdata);
	del_fvec (o->sqdiff);
	del_fvec (o->kernel);
	del_fvec (o->samples_fft);
	del_fvec (o->kernel_fft);
	AUBIO_FREE (o);
}
*/

float yin_get_pitch (yin_t * o, fvec_t * input, arm_rfft_fast_instance_f32 *fftInstance)
{
	uint32_t W = o->yin->length; // WIN_SIZE / 2
	fvec_t kernel_ptr;
	uint32_t tau, period, i;
	float tmp_f;

  // compute r_t(0) + r_t + tau(0)
  // actually computing r_t(0) 
  // r_t+tau(0) -> zero lag term done in the for loop for every tau
  {
    fvec_t *squares = o->tmpdata;
	arm_mult_f32(input->data, input->data, squares->data, input->length);
	
	o->sqdiff->data[0] = 0; 
	for(i = 0; i < W; i++)
		o->sqdiff->data[0] += squares->data[i];
	
    for (tau = 1; tau < W; tau++) 
	{
		o->sqdiff->data[tau] = o->sqdiff->data[tau-1];
		o->sqdiff->data[tau] -= squares->data[tau-1];
		o->sqdiff->data[tau] += squares->data[W+tau-1];
    }
	tmp_f = o->sqdiff->data[0]; // compiler acted a bit funny here. had to store the value in a local variable 
	for (i = 0; i < o->sqdiff->length; i++)
		o->sqdiff->data[i] += tmp_f; 
  }
  // compute r_t(tau) = -2.*ifft(fft(samples)*fft(samples[W-1::-1]))
  // https://stackoverflow.com/questions/3949324/calculate-autocorrelation-using-fft-in-matlab
  {
    fvec_t *compmul = o->tmpdata;
    fvec_t *rt_of_tau = o->samples_fft;
	
    // build kernel, take a copy of first half of samples, other half are zeros. need to fill them since fft changes the values >:( 
    kernel_ptr.data = o->kernel->data + 1;
    kernel_ptr.length = W;
	for (i = 0; i < kernel_ptr.length; i++)
	{
		kernel_ptr.data[i] = input->data[kernel_ptr.length-i-1]; 
	}
	o->kernel->data[0] = 0.0; 
	arm_fill_f32(0.0, &o->kernel->data[kernel_ptr.length], kernel_ptr.length); 
	
    // compute fft(kernel)	
	arm_rfft_fast_f32(fftInstance, o->kernel->data, o->kernel_fft->data, 0);

    // compute complex product		
	arm_cmplx_mult_cmplx_f32(o->kernel_fft->data, o->samples_fft->data, compmul->data, compmul->length); 

    // compute inverse fft
	arm_rfft_fast_f32(fftInstance, compmul->data, rt_of_tau->data, 1); 

    // compute square difference r_t(tau) = sqdiff - 2 * r_t_tau[W-1:-1]
	fvec_t *scaled_rt = o->tmpdata;
	arm_scale_f32(&rt_of_tau->data[W], 2.0, scaled_rt->data, W);
	arm_sub_f32(o->sqdiff->data, scaled_rt->data, o->yin->data, o->sqdiff->length); 
  }

	// now compute the cumulative mean normalized difference function and look for first minimum
	o->yin->data[0] = 1.0;
	tmp_f = 0.0; 
	for (tau = 1; tau < 5; tau++)
	{
		tmp_f += o->yin->data[tau];
		if (abs(tmp_f) > 0.00001)
		{
			o->yin->data[tau] *= tau / tmp_f;
		}
		else
		{
			o->yin->data[tau] = 1.0;
		}
		o->yin->data[tau] *= tau / tmp_f;
	}
	for (tau = 5; tau < o->yin->length; tau++) 
	{
		tmp_f += o->yin->data[tau];
		if (abs(tmp_f) > 0.00001)
		{
			o->yin->data[tau] *= tau / tmp_f;
		}
		else
		{
			o->yin->data[tau] = 1.0;
		}
		o->yin->data[tau] *= tau / tmp_f;

		period = tau - 3;
		if ((o->yin->data[period] < o->tol) && (o->yin->data[period] < o->yin->data[period + 1]))
		{
			o->peak_pos = (uint32_t)period;
			return YIN_FFT_SAMPLING_RATE / fvec_quadratic_peak_pos (o->yin, o->peak_pos);
		}
	}
	
	// use global minimum
	arm_min_f32(o->yin->data, o->yin->length, &tmp_f, &o->peak_pos); 
	tmp_f = fvec_quadratic_peak_pos (o->yin, (uint32_t)o->peak_pos);
	if (tmp_f < 1)
		return 0; 
	else 
		return YIN_FFT_SAMPLING_RATE / tmp_f; 
}

float yin_get_confidence (yin_t * o) 
{
	return 1. - o->yin->data[o->peak_pos];
}
