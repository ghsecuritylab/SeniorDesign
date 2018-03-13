/*
 * psola.c
 *
 * Created: 3/13/2018 3:53:58 PM
 *  Author: Daniel Gonzalez
 */ 

#include "psola.h"
#include <stdlib.h>

uint32_t _bufferLen;
float *_workingBuffer;
float *_storageBuffer;
float *_window; 
float window_buffer[2048];

static void hanning_win(float32_t *window, int length)
{
	for (int i = 0; i < length-1; i++) {
		window[i] = 0.5f * (1.0f - cosf(2.0f*PI*i/(length-1)));
	}
}

void PSOLA_init(uint32_t bufferLen)
{
	if (bufferLen < 1) {
		_bufferLen = DEFAULT_BUFFER_SIZE;
		} else {
		_bufferLen = bufferLen;
	}
	// allow for twice the room to deal with the case when the end of the buffer may not be sufficient
	_workingBuffer = malloc(2*bufferLen*sizeof(float));
	// allow for twice the room so we can move new data into this buffer
	_storageBuffer = malloc(2*bufferLen*sizeof(float)); ;
	// allocates maximum size for window to avoid reinitialization cost
	_window = window_buffer; // malloc(bufferLen*sizeof(float));
}

void pitchCorrect(float* input, float *output, int Fs, float inputPitch, float desiredPitch) {
	// Move things into the storage buffer
	for (uint32_t i = 0; i < _bufferLen; i++) 
	{
		//slide the past data into the front
		_storageBuffer[i] = _storageBuffer[i + _bufferLen];
		//load up next set of data
		_storageBuffer[i + _bufferLen] = input[i];
	}
	// Percent change of frequency
	float32_t scalingFactor = 1.0f; // 1.0f + (inputPitch - desiredPitch)/desiredPitch;
	// PSOLA constants
	uint32_t analysisShift = ceil(Fs/inputPitch);
	uint32_t analysisShiftHalfed = round(analysisShift/2);
	int32_t synthesisShift = round(analysisShift*scalingFactor);
	uint32_t analysisIndex = 0;
	uint32_t synthesisIndex = 0;
	int32_t analysisBlockStart;
	uint32_t analysisBlockEnd;
	uint32_t synthesisBlockEnd;
	uint32_t analysisLimit = _bufferLen - analysisShift - 1;
	// Window declaration
	int winLength = analysisShift + analysisShiftHalfed + 1;
	int windowIndex;
	//bartlett(_window,winLength);
	hanning_win(_window, winLength);
	// PSOLA Algorithm
	while (analysisIndex < analysisLimit) {
		// Analysis blocks are two pitch periods long
		analysisBlockStart = (int32_t)analysisIndex - (int32_t)analysisShiftHalfed;
		if (analysisBlockStart < 0) {
			analysisBlockStart = 0;
		}
		analysisBlockEnd = analysisBlockStart + analysisShift + analysisShiftHalfed;
		if (analysisBlockEnd > _bufferLen - 1) {
			analysisBlockEnd = _bufferLen - 1;
		}
		// Overlap and add
		synthesisBlockEnd = synthesisIndex + analysisBlockEnd - analysisBlockStart;
		uint32_t inputIndex = analysisBlockStart;
		windowIndex = 0;
		for (uint32_t j = synthesisIndex; j <= synthesisBlockEnd; j++) {
			_workingBuffer[j] += input[inputIndex]*_window[windowIndex];
			inputIndex++;
			windowIndex++;
		}
		// Update pointers
		analysisIndex += analysisShift;
		synthesisIndex += synthesisShift;
	}

	arm_copy_f32(_workingBuffer, output, WIN_SIZE); 
	arm_fill_f32(0.0f, _workingBuffer, _bufferLen); 
}


