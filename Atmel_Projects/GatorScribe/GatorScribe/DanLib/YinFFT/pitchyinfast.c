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

#include "aubio_priv.h"
#include "fvec.h"
#include "mathutils.h"
#include "cvec.h"
#include "pitchyinfast.h"

aubio_pitchyinfast_t * new_aubio_pitchyinfast (uint_t bufsize)
{
	aubio_pitchyinfast_t *o = AUBIO_NEW (aubio_pitchyinfast_t);
	o->yin = new_fvec (bufsize / 2);
	o->tmpdata = new_fvec (bufsize);
	o->sqdiff = new_fvec (bufsize / 2);
	o->kernel = new_fvec (bufsize);
	o->samples_fft = new_fvec (bufsize);
	o->kernel_fft = new_fvec (bufsize);
	o->fft = new_aubio_fft (bufsize);
	o->tol = 0.05; // changed from 0.15
	o->peak_pos = 0;
	return o;
}

void del_aubio_pitchyinfast (aubio_pitchyinfast_t * o)
{
	del_fvec (o->yin);
	del_fvec (o->tmpdata);
	del_fvec (o->sqdiff);
	del_fvec (o->kernel);
	del_fvec (o->samples_fft);
	del_fvec (o->kernel_fft);
	del_aubio_fft (o->fft);
	AUBIO_FREE (o);
}

float32_t aubio_pitchyinfast_do (aubio_pitchyinfast_t * o, fvec_t * input)
{
	const smpl_t tol = o->tol;
	fvec_t* yin = o->yin;
	const uint_t length = yin->length;
	uint_t B = o->tmpdata->length;
	uint_t W = o->yin->length; // B / 2
	fvec_t tmp_slice, kernel_ptr;
	uint_t tau;
	sint_t period;
	smpl_t tmp2 = 0.0;

  // compute r_t(0) + r_t + tau(0)
  // actually computing r_t(0) -> constant on line 109
  // r_t+tau(0) -> zero lag term done in the for loop for every tau
  {
    fvec_t *squares = o->tmpdata;
    fvec_weighted_copy(input, input, squares);
    tmp_slice.data = squares->data;
    tmp_slice.length = W;
    o->sqdiff->data[0] = fvec_sum(&tmp_slice);
    for (tau = 1; tau < W; tau++) 
	{
		o->sqdiff->data[tau] = o->sqdiff->data[tau-1];
		o->sqdiff->data[tau] -= squares->data[tau-1];
		o->sqdiff->data[tau] += squares->data[W+tau-1];
    }
    fvec_add(o->sqdiff, o->sqdiff->data[0]);
  }
  // compute r_t(tau) = -2.*ifft(fft(samples)*fft(samples[W-1::-1]))
  // https://stackoverflow.com/questions/3949324/calculate-autocorrelation-using-fft-in-matlab
  {
    fvec_t *compmul = o->tmpdata;
    fvec_t *rt_of_tau = o->samples_fft;
	
    // build kernel, take a copy of first half of samples
    tmp_slice.data = input->data;
    tmp_slice.length = W;
    kernel_ptr.data = o->kernel->data + 1;
    kernel_ptr.length = W;
    fvec_copy(&tmp_slice, &kernel_ptr);
    // reverse them
    fvec_rev(&kernel_ptr);
    // compute fft(kernel)
    aubio_fft_do_complex(o->fft, o->kernel, o->kernel_fft);
    // compute complex product
    compmul->data[0] = o->kernel_fft->data[0] * o->samples_fft->data[0];
    for (tau = 1; tau < W; tau++) 
	{
		compmul->data[tau] = o->kernel_fft->data[tau] * o->samples_fft->data[tau];
		compmul->data[tau] -= o->kernel_fft->data[B-tau] * o->samples_fft->data[B-tau];
    }
    compmul->data[W] = o->kernel_fft->data[W] * o->samples_fft->data[W];
    for (tau = 1; tau < W; tau++) 
	{
		compmul->data[B-tau] = o->kernel_fft->data[B-tau] * o->samples_fft->data[tau];
		compmul->data[B-tau] += o->kernel_fft->data[tau] * o->samples_fft->data[B-tau];
    }
    // compute inverse fft
    aubio_fft_rdo_complex(o->fft, compmul, rt_of_tau);
    // compute square difference r_t(tau) = sqdiff - 2 * r_t_tau[W-1:-1]
    for (tau = 0; tau < W; tau++) 
	{
		yin->data[tau] = o->sqdiff->data[tau] - 2. * rt_of_tau->data[tau+W];
    }
  }

	// now build yin and look for first minimum
	yin->data[0] = 1.0;
	for (tau = 1; tau < length; tau++) 
	{
		tmp2 += yin->data[tau];
		if (abs(tmp2) > 0.00001) 
		{
			yin->data[tau] *= tau / tmp2;
		}
		else
		{
			yin->data[tau] = 1.0;
		}
		period = tau - 3;
		if (tau > 4 && (yin->data[period] < tol) && (yin->data[period] < yin->data[period + 1]))
		{
			o->peak_pos = (uint_t)period;
			return YIN_FFT_SAMPLING_RATE / fvec_quadratic_peak_pos (yin, o->peak_pos);
		}
	}
		
	// use global minimum
	o->peak_pos = (uint_t)fvec_min_elem (yin);
	return YIN_FFT_SAMPLING_RATE / fvec_quadratic_peak_pos (yin, o->peak_pos);
	//return -1; 
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
