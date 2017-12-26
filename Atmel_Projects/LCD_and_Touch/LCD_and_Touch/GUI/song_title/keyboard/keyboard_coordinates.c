//
//  keyboard_coordinates.c
//  
//
//  Created by Daniel Gonzalez on 12/24/17.
//

#include "asf.h"
#include "keyboard_coordinates.h"
#include "fastmath.h"

#define BACKSPACE   0x08
#define RETURN      0x0D
#define SHIFT       0x0F
#define SPACE       0x20

static char upper_chars[33] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P' , BACKSPACE,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', RETURN,
    SHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '!', '?', SHIFT,
    SPACE
}; 
static char lower_chars[33] = {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p' , BACKSPACE,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', RETURN,
    SHIFT, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', SHIFT,
    SPACE
}; 

static enum letter_idx
{
    Q = 0, W = 1, E = 2, R = 3, T = 4, Y = 5, U = 6, I = 7, O = 8, P = 9, BS = 10,
    A = 11, S = 12, D = 13, F = 14, G = 15, H = 16, J = 17, K = 18, L = 19, RET = 20,
    SH1 = 21, Z = 22, X = 23, C = 24, V = 25, B = 26, N = 27, M = 28, COM = 29, PER = 30, SH2 = 31,
    SP = 32
};

static key_coord_t key_coordinates[] = {
	{20, 175},  // Q
	{63, 175},  // W
	{105, 175}, // E
	{150, 175}, // R
	{193, 175}, // T
	{236, 175}, // Y
	{279, 175}, // U
	{324, 175}, // I
    {367, 175}, // O
    {410, 175}, // P
    {455, 175}, // BACKSPACE
    
    {37,  214},  // A
    {80,  214},  // S
    {123, 214}, // D
    {167, 214}, // F
    {210, 214}, // G
    {252, 214}, // H
    {296, 214}, // J
    {339, 214}, // K
    {382, 214}, // L
    {440, 214}, // RET
    
    {19,  253},  // SH1
    {60,  253},  // Z
    {103, 253}, // X
    {145, 253}, // C
    {188, 253}, // V
    {232, 253}, // B
    {274, 253}, // N
    {317, 253}, // M
    {358, 253}, // C
    {401, 253}, // P
    {450, 253}, // SH2
    {252, 294}  // SPACE
}; 

void get_key(int16_t x, int16_t y, uint32_t case_option, char *key_pressed)
{
	uint32_t min = 0xFFFF; 
	int32_t x_diff; 
	int32_t y_diff; 
	uint32_t dist; 
	
	char *keys; 
	
	if (case_option) 
	{
		keys = upper_chars; 
	}
	else
	{
		keys = lower_chars; 
	}
	
	for (int i = 0; i < 33; i++)
	{
		x_diff = x - key_coordinates[i].x; 
		y_diff = y - key_coordinates[i].y; 
		dist = sqrt(x_diff*x_diff + y_diff*y_diff); 
		if (dist < min)
		{
			min = dist; 
			*key_pressed = keys[i]; 
		}
	}
}
