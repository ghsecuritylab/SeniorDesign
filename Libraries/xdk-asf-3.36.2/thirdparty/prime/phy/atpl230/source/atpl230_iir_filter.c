/**
 * \file
 *
 * \brief ATPL230 Physical layer - IIR Filter
 *
 * Copyright (c) 2014-2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#include "compiler.h"
#include "atpl230_reg.h"
#include "atpl230_iir_filter.h"

/**
 * \weakgroup phy_plc_group
 * @{
 */

/* Filter IIR initialization */
const uint8_t uc_data_filter_IIR [LENGTH_DATA_FILTER_IIR] = {
	0x12, 0x5A,
	0x0D, 0x40,
	0x40, 0x00,
	0x40, 0x00,
	0x40, 0x00,
	0x40, 0x00,
	0x40, 0x00,
	0x40, 0x00,
	0x13, 0xC0,
	0x0E, 0x43,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0xE2, 0xD7,
	0xFC, 0xBC,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x7F, 0xFF,
	0x7F, 0xFF,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x40, 0x00,
	0x40, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00,
	0x00, 0x00
};

/* Chirp initialization */
const uint8_t uc_data_chirp [LENGTH_DATA_CHIRP] = {
	0x09, 0x6B, 0x7F, 0xA7,
	0x9D, 0xA6, 0xAE, 0x15,
	0x7E, 0xA8, 0xED, 0x82,
	0xBD, 0x26, 0x6D, 0x28,
	0xD2, 0x7B, 0x88, 0x5E,
	0x7B, 0xC9, 0x20, 0x93,
	0xA0, 0x92, 0x55, 0x4E,
	0xE8, 0xCF, 0x82, 0x1E,
	0x79, 0x2A, 0x29, 0x44,
	0xA4, 0x03, 0x59, 0x02,
	0xD6, 0x44, 0x86, 0xFF,
	0x7F, 0xB5, 0x08, 0xAE,
	0xCA, 0x85, 0x74, 0x4B,
	0xA2, 0xFE, 0xA8, 0x10,
	0x6E, 0xD5, 0xBF, 0xF8,
	0x22, 0x49, 0x7B, 0x53,
	0x80, 0x32, 0x07, 0x11,
	0x0F, 0xD0, 0x80, 0xFB,
	0x7B, 0x7E, 0x21, 0xAA,
	0xD1, 0x86, 0x77, 0x44,
	0x8C, 0x34, 0xC9, 0x74,
	0x3A, 0x34, 0x8E, 0x00,
	0x72, 0x4A, 0x39, 0xA3,
	0xCB, 0x31, 0x74, 0x99,
	0x87, 0x9D, 0xD4, 0x83,
	0x1D, 0x56, 0x83, 0x68,
	0x7F, 0x9A, 0x0A, 0x19,
	0x0E, 0x10, 0x7F, 0x3A,
	0x87, 0x28, 0x2A, 0x33,
	0xB7, 0xDE, 0x96, 0x43,
	0x4F, 0xC0, 0x9B, 0xE1,
	0x78, 0xD4, 0x2A, 0x3E,
	0x04, 0xB7, 0x7F, 0xEA,
	0x8C, 0x91, 0x37, 0x4E,
	0x9C, 0x40, 0xAF, 0xCA,
	0x18, 0xA2, 0x82, 0x64,
	0x79, 0x3A, 0xD6, 0xEA,
	0x62, 0xFA, 0x51, 0x2A,
	0xF4, 0x24, 0x7F, 0x73,
	0x92, 0x16, 0x41, 0x99,
	0x88, 0x6F, 0xD2, 0x4D,
	0xD5, 0x62, 0x87, 0x4D,
	0x3C, 0x40, 0x8F, 0x11,
	0x7A, 0xBA, 0xDB, 0xA4,
	0x72, 0x3F, 0x39, 0xB9,
	0x2F, 0xF1, 0x76, 0xAF,
	0xDA, 0x60, 0x7A, 0x58,
	0x98, 0x59, 0x4B, 0x1A,
	0x80, 0x06, 0x02, 0x5C,
	0x93, 0x1A, 0xBC, 0xBB,
	0xC4, 0x9D, 0x8E, 0x9C,
	0x02, 0x03, 0x80, 0x04,
	0x3A, 0xC6, 0x8E, 0x4A,
	0x64, 0x64, 0xB0, 0x98,
	0x7A, 0xF6, 0xDC, 0x71,
	0x7F, 0xB1, 0x08, 0xE0,
	0x76, 0xB4, 0x2F, 0xE4,
	0x65, 0x10, 0x4E, 0x8E,
	0x4F, 0x72, 0x64, 0x5C,
	0x39, 0x82, 0x72, 0x5B,
	0x25, 0xB9, 0x7A, 0x51,
	0x15, 0x8C, 0x7E, 0x2C,
	0x09, 0xB6, 0x7F, 0xA2,
	0x02, 0x81, 0x7F, 0xFA,
	0x00, 0x01, 0x7F, 0xFF
};

/* AngleRealImagComp initialization */
const uint8_t uc_data_angle_real_imag_comp [LENGTH_DATA_ANGLE_REAL_IMAG_COMP] = {
	0x6F, 0x6E,
	0x68, 0xA1,
	0x63, 0x5E,
	0x5F, 0x5C,
	0x5C, 0x3E,
	0x59, 0xF0,
	0x58, 0x2D,
	0x56, 0xB2,
	0x55, 0x96,
	0x54, 0xC1,
	0x54, 0x1B,
	0x53, 0xB4,
	0x53, 0x5E,
	0x53, 0x3A,
	0x53, 0x32,
	0x53, 0x25,
	0x53, 0x30,
	0x53, 0x87,
	0x53, 0xAA,
	0x53, 0xAB,
	0x53, 0xBE,
	0x53, 0xA5,
	0x53, 0x99,
	0x53, 0x79,
	0x53, 0x4F,
	0x53, 0x16,
	0x52, 0xCD,
	0x52, 0x6E,
	0x52, 0x2C,
	0x51, 0xF5,
	0x51, 0xC0,
	0x51, 0xA4,
	0x51, 0xB0,
	0x51, 0xD9,
	0x51, 0xDB,
	0x52, 0x0D,
	0x52, 0x05,
	0x52, 0x59,
	0x52, 0x5A,
	0x52, 0x65,
	0x52, 0x73,
	0x52, 0x55,
	0x52, 0x0C,
	0x51, 0xFA,
	0x51, 0xAF,
	0x51, 0x9C,
	0x51, 0x6A,
	0x51, 0x5B,
	0x51, 0x48,
	0x51, 0x57,
	0x51, 0x72,
	0x51, 0x96,
	0x51, 0xCD,
	0x51, 0xFE,
	0x52, 0x30,
	0x52, 0x4F,
	0x52, 0x38,
	0x52, 0x82,
	0x52, 0x51,
	0x52, 0x56,
	0x52, 0x1F,
	0x52, 0x16,
	0x52, 0x06,
	0x51, 0xE5,
	0x51, 0xC6,
	0x51, 0xC7,
	0x51, 0xA7,
	0x51, 0xFB,
	0x52, 0x16,
	0x52, 0x66,
	0x52, 0xB4,
	0x53, 0x06,
	0x53, 0x3E,
	0x53, 0x5C,
	0x53, 0x8E,
	0x53, 0xB3,
	0x53, 0xAB,
	0x53, 0xCF,
	0x53, 0x91,
	0x53, 0x4B,
	0x53, 0x2C,
	0x53, 0x18,
	0x53, 0x21,
	0x53, 0x3B,
	0x53, 0x41,
	0x53, 0xA8,
	0x54, 0x25,
	0x54, 0xB0,
	0x55, 0x6B,
	0x56, 0xB2,
	0x57, 0xFB,
	0x59, 0xE4,
	0x5C, 0x49,
	0x5F, 0x54,
	0x63, 0x7C,
	0x68, 0x58,
	0x6F, 0x82
};

const uint32_t ul_data_offset_correction [NUM_ROWS_DATA_OFFSET_CORRECTION] = {
	0xC15319F6,
	0x2FF8D008,
	0xE60A3EAD,
	0x0000BC29,
	0x19F63EAD,
	0xD008D008,
	0x3EAD19F6,
	0xBC290000,
	0x3EADE60A,
	0xD0082FF8,
	0x19F6C153,
	0x000043D7,
	0xE60AC153,
	0x2FF82FF8,
	0xC153E60A,
	0x43D70000,
};

/* @} */
