/*
 * key_signature.h
 *
 * Created: 12/27/2017 6:41:24 PM
 *  Author: Daniel Gonzalez
 */ 


#ifndef KEY_SIGNATURE_H_
#define KEY_SIGNATURE_H_

enum majorKeys {
	C_MAJOR = 0, 
	G_MAJOR = 1, 
	D_MAJOR = 2, 
	A_MAJOR = 3,
	E_MAJOR = 4,
	B_MAJOR = 5,
	Fs_MAJOR = 6,
	Cs_MAJOR = 7,
	F_MAJOR = -1,
	Bb_MAJOR = -2,
	Eb_MAJOR = -3,
	Ab_MAJOR = -4,
	Db_MAJOR = -5,
	Gb_MAJOR = -6,
	Cb_MAJOR = -7
};
enum minorKeys {
	A_MINOR = 0,
	E_MINOR = 1,
	B_MINOR = 2,
	Fs_MINOR = 3,
	Cs_MINOR = 4,
	Gs_MINOR = 5,
	Ds_MINOR = 6,
	As_MINOR = 7,
	D_MINOR = -1,
	G_MINOR = -2,
	C_MINOR = -3,
	F_MINOR = -4,
	Bb_MINOR = -5,
	Eb_MINOR = -6,
	Ab_MINOR = -7
};

typedef enum {
	MAJOR = 0, 
	MINOR = 1
} key_mode_t;

typedef struct key_signature
{
	int8_t key; 
	key_mode_t mode;
} key_signature_t;

void key_signature_menu(key_signature_t *key_sig);


#endif /* KEY_SIGNATURE_H_ */