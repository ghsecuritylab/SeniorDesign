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

COMPILER_ALIGNED(WIN_SIZE>>1) static smpl_t _yin[WIN_SIZE>>1]; 
COMPILER_ALIGNED(WIN_SIZE) static smpl_t _tmpdata[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE>>1) static smpl_t _sqdiff[WIN_SIZE>>1];
COMPILER_ALIGNED(WIN_SIZE) static smpl_t _kernel[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static smpl_t _samples_fft[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static smpl_t _kernel_fft[WIN_SIZE];

aubio_pitchyinfast_t * new_aubio_pitchyinfast (void)
{
	uint32_t i; 
	aubio_pitchyinfast_t *o = AUBIO_NEW (aubio_pitchyinfast_t);
	/*
	o->yin = new_fvec (WIN_SIZE / 2);
	o->tmpdata = new_fvec (WIN_SIZE);
	o->sqdiff = new_fvec (WIN_SIZE / 2);
	o->kernel = new_fvec (WIN_SIZE);
	o->samples_fft = new_fvec (WIN_SIZE);
	o->kernel_fft = new_fvec (WIN_SIZE);
	o->fft = new_aubio_fft (WIN_SIZE);
	*/
	o->tol = 0.05; // changed from 0.15
	o->peak_pos = 0;
	
	o->yin = AUBIO_NEW(fvec_t);
	o->yin->data = _yin; 
	o->yin->length = sizeof(_yin)/sizeof(_yin[0]);
	for(i = 0; i < o->yin->length; i++) o->yin->data[i] = 0.0; 
	
	o->tmpdata = AUBIO_NEW(fvec_t);
	o->tmpdata->data = _tmpdata; 
	o->tmpdata->length = sizeof(_tmpdata)/sizeof(_tmpdata[0]);
	for(i = 0; i < o->tmpdata->length; i++) o->tmpdata->data[i] = 0.0;

	o->sqdiff = AUBIO_NEW(fvec_t);
	o->sqdiff->data = _sqdiff; 
	o->sqdiff->length = sizeof(_sqdiff)/sizeof(_sqdiff[0]);
	for(i = 0; i < o->sqdiff->length; i++) o->sqdiff->data[i] = 0.0;
	
	o->kernel = AUBIO_NEW(fvec_t);
	o->kernel->data = _kernel;
	o->kernel->length = sizeof(_kernel)/sizeof(_kernel[0]);
	for(i = 0; i < o->kernel->length; i++) o->kernel->data[i] = 0.0;
	
	o->samples_fft = AUBIO_NEW(fvec_t);
	o->samples_fft->data = _samples_fft;
	o->samples_fft->length = sizeof(_samples_fft)/sizeof(_samples_fft[0]);
	for(i = 0; i < o->samples_fft->length; i++) o->samples_fft->data[i] = 0.0;
	
	o->kernel_fft = AUBIO_NEW(fvec_t);
	o->kernel_fft->data = _kernel_fft;
	o->kernel_fft->length = sizeof(_kernel_fft)/sizeof(_kernel_fft[0]);
	for(i = 0; i < o->kernel_fft->length; i++) o->kernel_fft->data[i] = 0.0;

	return o;
}

/*
void del_aubio_pitchyinfast (aubio_pitchyinfast_t * o)
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

float aubio_pitchyinfast_do (aubio_pitchyinfast_t * o, fvec_t * input, arm_rfft_fast_instance_f32 *fftInstance)
{
	uint_t W = o->yin->length; // WIN_SIZE / 2
	fvec_t kernel_ptr;
	uint_t tau;
	uint_t period;
	smpl_t tmp2;
	uint32_t i; 

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
	tmp2 = o->sqdiff->data[0]; // compiler acted a bit funny here. had to store the value in a local variable 
	for (i = 0; i < o->sqdiff->length; i++)
		o->sqdiff->data[i] += tmp2; 
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
	o->kernel->data[0] = 0; 
	arm_fill_f32(0.0, &o->kernel->data[kernel_ptr.length], kernel_ptr.length); 
	
    // compute fft(kernel)	
	arm_rfft_fast_f32(fftInstance, o->kernel->data, o->kernel_fft->data, 0);

    // compute complex product		
	arm_cmplx_mult_cmplx_f32(o->kernel_fft->data, o->samples_fft->data, compmul->data, compmul->length); 

    // compute inverse fft
	arm_rfft_fast_f32(fftInstance, compmul->data, rt_of_tau->data, 1); 

    // compute square difference r_t(tau) = sqdiff - 2 * r_t_tau[W-1:-1]
    for (tau = 0; tau < W; tau++) 
		o->yin->data[tau] = o->sqdiff->data[tau] - 2. * rt_of_tau->data[tau+W];
  }

	// now build yin and look for first minimum
	o->yin->data[0] = 1.0;
	tmp2 = 0.0; 
	for (tau = 1; tau < 5; tau++)
	{
		tmp2 += o->yin->data[tau];
		if (abs(tmp2) > 0.00001)
		{
			o->yin->data[tau] *= tau / tmp2;
		}
		else
		{
			o->yin->data[tau] = 1.0;
		}
		o->yin->data[tau] *= tau / tmp2;
	}
	for (tau = 5; tau < o->yin->length; tau++) 
	{
		tmp2 += o->yin->data[tau];
		if (abs(tmp2) > 0.00001)
		{
			o->yin->data[tau] *= tau / tmp2;
		}
		else
		{
			o->yin->data[tau] = 1.0;
		}
		o->yin->data[tau] *= tau / tmp2;

		period = tau - 3;
		if ((o->yin->data[period] < o->tol) && (o->yin->data[period] < o->yin->data[period + 1]))
		{
			o->peak_pos = (uint_t)period;
			return YIN_FFT_SAMPLING_RATE / fvec_quadratic_peak_pos (o->yin, o->peak_pos);
		}
	}
	
	// use global minimum
	arm_min_f32(o->yin->data, o->yin->length, &tmp2, &o->peak_pos); 
	tmp2 = fvec_quadratic_peak_pos (o->yin, (uint_t)o->peak_pos);
	if (tmp2 < 1)
		return 0; 
	else 
		return YIN_FFT_SAMPLING_RATE / tmp2; 
}

smpl_t aubio_pitchyinfast_get_confidence (aubio_pitchyinfast_t * o) 
{
	return 1. - o->yin->data[o->peak_pos];
}

uint_t aubio_pitchyinfast_set_tolerance (aubio_pitchyinfast_t * o, smpl_t tol)
{
	o->tol = tol;
	return 0;
}

smpl_t aubio_pitchyinfast_get_tolerance (aubio_pitchyinfast_t * o)
{
	return o->tol;
}
