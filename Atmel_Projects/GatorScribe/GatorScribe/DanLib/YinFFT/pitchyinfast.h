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

/** \file

  Pitch detection using YIN algorithm (fast implementation)

  This algorithm was developed by A. de Cheveigne and H. Kawahara and
  published in:

  De Cheveigné, A., Kawahara, H. (2002) "YIN, a fundamental frequency
  estimator for speech and music", J. Acoust. Soc. Am. 111, 1917-1930.

  This implementation compute the autocorrelation function using time domain
  convolution computed in the spectral domain.

  see http://recherche.ircam.fr/equipes/pcm/pub/people/cheveign.html
      http://recherche.ircam.fr/equipes/pcm/cheveign/ps/2002_JASA_YIN_proof.pdf

*/

#ifndef AUBIO_PITCHYINFAST_H
#define AUBIO_PITCHYINFAST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include "arm_math.h"

extern const float hanning[1024];


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
}yin_t;


yin_t *yin_init (void);

//void del_yin (yin_t * o); 

float yin_get_pitch (yin_t * o, fvec_t * input, arm_rfft_fast_instance_f32 *fftInstance);

float yin_get_confidence (yin_t * o);

#ifdef __cplusplus
}
#endif

#endif /* AUBIO_PITCHYINFAST_H */

