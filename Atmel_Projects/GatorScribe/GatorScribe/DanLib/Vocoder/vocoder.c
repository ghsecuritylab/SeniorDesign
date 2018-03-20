/*
 * Vocoder.c
 *
 * Created: 1/29/2018 5:06:45 PM
 *  Author: Daniel Gonzalez
 */ 

#include "vocoder.h"
#include <stdlib.h>

extern const float hanning[1024];

static const float OneOverPi = 1.0f/3.14159f;
static const float OneOverTwoPi = 0.15915494309f; 
static const float TwoPi = 2.0f * M_PI;
static const float Overlap_x_OneOverTwoPi = (float)NUM_OF_OVERLAPS * 0.15915494309f;
static const float TwoPi_d_Overlap =  2.0f * M_PI / (float)NUM_OF_OVERLAPS;

static const float ifft_scale = 4.0f / (float)NUM_OF_OVERLAPS; // 4.0 = 2 (only using half the spectrum) * 2 (removed scaling from original fft) 

static const float freqPerBin = (float)FFT_SAMPLE_RATE/(float)WIN_SIZE;
static const float oneOverFreqPerBin = (float)WIN_SIZE / (float)FFT_SAMPLE_RATE; 
static const float expct = 2.0f * M_PI / (float)NUM_OF_OVERLAPS;


COMPILER_ALIGNED(WIN_SIZE_D2) static float prevAnaPhase[WIN_SIZE_D2];
COMPILER_ALIGNED(WIN_SIZE_D2) static float gSumPhase[WIN_SIZE_D2];
COMPILER_ALIGNED(2*WIN_SIZE) float gFFTworksp[2*WIN_SIZE];
COMPILER_ALIGNED(2*WIN_SIZE) static float gOutputAccum[2*WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gAnaFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynFreq[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float gSynMagn[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float scaled_hanning[WIN_SIZE];
COMPILER_ALIGNED(WIN_SIZE) static float ifft_real_values[WIN_SIZE];

/* Spectral envelope */ 
COMPILER_ALIGNED(WIN_SIZE) static float shift_envelope[WIN_SIZE];
extern float envelope_filter[];
extern uint32_t envelope_filter_length;

static const float log_scale[] = {0,
	0.056534141,
	0.090010598,
	0.11404676,
	0.13291016,
	0.14850168,
	0.16183531,
	0.17351632,
	0.18393508,
	0.19335817,
	0.20197576,
	0.20992818,
	0.21732207,
	0.22424027,
	0.23074852,
	0.23689976,
	0.24273728,
	0.24829699,
	0.25360894,
	0.25869855,
	0.26358756,
	0.26829463,
	0.27283591,
	0.27722549,
	0.28147587,
	0.28559786,
	0.28960121,
	0.29349452,
	0.29728562,
	0.30098128,
	0.30458787,
	0.30811095,
	0.31155577,
	0.31492698,
	0.31822878,
	0.32146513,
	0.32463965,
	0.32775560,
	0.33081600,
	0.33382368,
	0.33678123,
	0.33969113,
	0.34255558,
	0.34537664,
	0.34815636,
	0.35089642,
	0.35359859,
	0.35626450,
	0.35889560,
	0.36149332,
	0.36405897,
	0.36659381,
	0.36909899,
	0.37157562,
	0.37402478,
	0.37644744,
	0.37884453,
	0.38121694,
	0.38356552,
	0.38589108,
	0.38819438,
	0.39047614,
	0.39273703,
	0.39497775,
	0.39719889,
	0.39940101,
	0.40158474,
	0.40375066,
	0.40589926,
	0.40803099,
	0.41014636,
	0.41224581,
	0.41432980,
	0.41639876,
	0.41845307,
	0.42049316,
	0.42251939,
	0.42453206,
	0.42653155,
	0.42851824,
	0.43049234,
	0.43245426,
	0.43440425,
	0.43634260,
	0.43826964,
	0.44018555,
	0.44209063,
	0.44398510,
	0.44586921,
	0.44774321,
	0.44960731,
	0.45146176,
	0.45330676,
	0.45514244,
	0.45696908,
	0.45878682,
	0.46059588,
	0.46239638,
	0.46418855,
	0.46597254,
	0.46774858,
	0.46951669,
	0.47127715,
	0.47303000,
	0.47477546,
	0.47651365,
	0.47824469,
	0.47996876,
	0.48168597,
	0.48339641,
	0.48510021,
	0.48679751,
	0.48848844,
	0.49017307,
	0.49185157,
	0.49352396,
	0.49519044,
	0.49685103,
	0.49850589,
	0.50015503,
	0.50179869,
	0.50343686,
	0.50506961,
	0.50669706,
	0.50831932,
	0.50993645,
	0.51154852,
	0.51315558,
	0.51475781,
	0.51635516,
	0.51794785,
	0.51953578,
	0.52111918,
	0.52269804,
	0.52427244,
	0.52584237,
	0.52740806,
	0.52896947,
	0.53052664,
	0.53207964,
	0.53362858,
	0.53517348,
	0.53671443,
	0.53825146,
	0.53978461,
	0.54131395,
	0.54283953,
	0.54436135,
	0.54587960,
	0.54739410,
	0.54890513,
	0.55041265,
	0.55191672,
	0.55341733,
	0.55491459,
	0.55640852,
	0.55789912,
	0.55938649,
	0.56087065,
	0.56235158,
	0.56382948,
	0.56530422,
	0.56677592,
	0.56824464,
	0.56971031,
	0.57117307,
	0.57263297,
	0.57408994,
	0.57554412,
	0.57699543,
	0.57844394,
	0.57988977,
	0.58133286,
	0.58277327,
	0.58421105,
	0.58564621,
	0.58707887,
	0.58850884,
	0.58993638,
	0.59136134,
	0.59278387,
	0.59420395,
	0.59562159,
	0.59703684,
	0.59844977,
	0.59986031,
	0.60126853,
	0.60267448,
	0.60407811,
	0.60547954,
	0.60687876,
	0.60827577,
	0.60967058,
	0.61106324,
	0.61245382,
	0.61384225,
	0.61522865,
	0.61661291,
	0.61799514,
	0.61937541,
	0.62075365,
	0.62212986,
	0.62350416,
	0.62487650,
	0.62624687,
	0.62761539,
	0.62898201,
	0.63034672,
	0.63170964,
	0.63307071,
	0.63442993,
	0.63578737,
	0.63714302,
	0.63849688,
	0.63984901,
	0.64119935,
	0.64254802,
	0.64389503,
	0.64524031,
	0.64658391,
	0.64792591,
	0.64926624,
	0.65060490,
	0.65194201,
	0.65327752,
	0.65461141,
	0.65594375,
	0.65727454,
	0.65860379,
	0.65993148,
	0.66125768,
	0.66258234,
	0.66390556,
	0.66522729,
	0.66654760,
	0.66786641,
	0.66918373,
	0.67049968,
	0.67181420,
	0.67312729,
	0.67443907,
	0.67574942,
	0.67705840,
	0.67836601,
	0.67967230,
	0.68097728,
	0.68228090,
	0.68358320,
	0.68488425,
	0.68618393,
	0.68748236,
	0.68877953,
	0.69007540,
	0.69137007,
	0.69266349,
	0.69395566,
	0.69524664,
	0.69653636,
	0.69782490,
	0.69911224,
	0.70039839,
	0.70168334,
	0.70296711,
	0.70424974,
	0.70553124,
	0.70681161,
	0.70809084,
	0.70936888,
	0.71064591,
	0.71192175,
	0.71319652,
	0.71447021,
	0.71574283,
	0.71701437,
	0.71828485,
	0.71955425,
	0.72082257,
	0.72208989,
	0.72335613,
	0.72462136,
	0.72588563,
	0.72714877,
	0.72841096,
	0.72967219,
	0.73093235,
	0.73219156,
	0.73344982,
	0.73470700,
	0.73596328,
	0.73721862,
	0.73847300,
	0.73972642,
	0.74097890,
	0.74223047,
	0.74348110,
	0.74473083,
	0.74597961,
	0.74722749,
	0.74847448,
	0.74972057,
	0.75096577,
	0.75221008,
	0.75345349,
	0.75469607,
	0.75593776,
	0.75717860,
	0.75841856,
	0.75965768,
	0.76089597,
	0.76213342,
	0.76336998,
	0.76460576,
	0.76584071,
	0.76707482,
	0.76830816,
	0.76954067,
	0.77077240,
	0.77200329,
	0.77323341,
	0.77446276,
	0.77569133,
	0.77691907,
	0.77814609,
	0.77937233,
	0.78059775,
	0.78182250,
	0.78304642,
	0.78426963,
	0.78549212,
	0.78671384,
	0.78793484,
	0.78915507,
	0.79037458,
	0.79159337,
	0.79281145,
	0.79402882,
	0.79524553,
	0.79646146,
	0.79767668,
	0.79889125,
	0.80010509,
	0.80131823,
	0.80253077,
	0.80374253,
	0.80495369,
	0.80616415,
	0.80737394,
	0.80858308,
	0.80979151,
	0.81099933,
	0.81220645,
	0.81341296,
	0.81461883,
	0.81582403,
	0.81702858,
	0.81823254,
	0.81943583,
	0.82063848,
	0.82184052,
	0.82304192,
	0.82424277,
	0.82544297,
	0.82664251,
	0.82784146,
	0.82903981,
	0.83023757,
	0.83143473,
	0.83263129,
	0.83382726,
	0.83502269,
	0.83621746,
	0.83741170,
	0.83860528,
	0.83979839,
	0.84099084,
	0.84218276,
	0.84337419,
	0.84456497,
	0.84575516,
	0.84694487,
	0.84813398,
	0.84932250,
	0.85051054,
	0.85169798,
	0.85288495,
	0.85407132,
	0.85525715,
	0.85644251,
	0.85762727,
	0.85881150,
	0.85999519,
	0.86117834,
	0.86236107,
	0.86354321,
	0.86472487,
	0.86590600,
	0.86708659,
	0.86826670,
	0.86944634,
	0.87062538,
	0.87180406,
	0.87298214,
	0.87415975,
	0.87533683,
	0.87651342,
	0.87768960,
	0.87886524,
	0.88004035,
	0.88121510,
	0.88238925,
	0.88356298,
	0.88473624,
	0.88590902,
	0.88708133,
	0.88825315,
	0.88942450,
	0.89059544,
	0.89176583,
	0.89293581,
	0.89410537,
	0.89527446,
	0.89644307,
	0.89761126,
	0.89877898,
	0.89994627,
	0.90111309,
	0.90227950,
	0.90344548,
	0.90461099,
	0.90577608,
	0.90694070,
	0.90810496,
	0.90926874,
	0.91043210,
	0.91159505,
	0.91275758,
	0.91391963,
	0.91508132,
	0.91624254,
	0.91740340,
	0.91856384,
	0.91972387,
	0.92088348,
	0.92204261,
	0.92320138,
	0.92435974,
	0.92551774,
	0.92667532,
	0.92783254,
	0.92898929,
	0.93014568,
	0.93130165,
	0.93245727,
	0.93361247,
	0.93476731,
	0.93592173,
	0.93707579,
	0.93822944,
	0.93938273,
	0.94053566,
	0.94168818,
	0.94284028,
	0.94399208,
	0.94514346,
	0.94629449,
	0.94744515,
	0.94859546,
	0.94974536,
	0.95089489,
	0.95204407,
	0.95319283,
	0.95434129,
	0.95548946,
	0.95663720,
	0.95778459,
	0.95893162,
	0.96007830,
	0.96122462,
	0.96237057,
	0.96351618,
	0.96466148,
	0.96580642,
	0.96695101,
	0.96809524,
	0.96923918,
	0.97038269,
	0.97152591,
	0.97266883,
	0.97381139,
	0.97495359,
	0.97609544,
	0.97723699,
	0.97837824,
	0.97951907,
	0.98065966,
	0.98179990,
	0.98293978,
	0.98407936,
	0.98521858,
	0.98635751,
	0.98749614,
	0.98863441,
	0.98977238,
	0.99091005,
	0.99204743,
	0.99318445,
	0.99432117,
	0.99545753,
	0.99659365,
	0.99772942,
	0.99886489,
	1}; 

void Vocoder_init(void)
{
	uint32_t i; 
	for (i = 0; i < WIN_SIZE; i++)
	{
		gAnaFreq[i] = 0.0f;
		gSynFreq[i] = 0.0f; 
		gSynMagn[i] = 0.0f; 
	}
	for (i = 0; i < 2*WIN_SIZE; i++)
	{
		gFFTworksp[i] = 0.0f; 
		gOutputAccum[i] = 0.0f; 
	}
	for(i = 0; i < WIN_SIZE_D2; i++)
	{
		prevAnaPhase[i] = 0.0f;
		gSumPhase[i] = 0.0f;
	}
	
	/* Create ifft hanning window with appropriate scaling factor */ 
	arm_scale_f32((float *)hanning, ifft_scale, scaled_hanning, WIN_SIZE);
}

static inline float princarg(float inPhase)
{
	return (inPhase - (float)(round(inPhase*OneOverTwoPi)) * TwoPi); 
}

void pitch_shift_do(float shift_amount, cvec_t *mags_and_phases)
{
	float tmp; 
	uint32_t k, target;
	/* ***************** ANALYSIS ******************* */
	/* this is the analysis step */
	for (k = 0; k < WIN_SIZE_D2; k++) {
		/* compute phase difference */
		tmp = mags_and_phases->phas[k] - prevAnaPhase[k]; 
		
		/* subtract expected phase difference */
		tmp -= (float)k * expct; 
		
		/* map delta phase into +/- Pi interval */
 		tmp = princarg(tmp); 

		// get deviation from bin frequency from the +/- Pi interval
		tmp = Overlap_x_OneOverTwoPi*tmp;
		
		// compute the k-th partials' true frequency 
		tmp = ((float)k + tmp)*freqPerBin;
		
		/* store true frequency in analysis arrays */
		gAnaFreq[k] = tmp; 
	}
	
	/* ***************** PROCESSING ******************* */
	arm_fill_f32(0.0f, gSynFreq, WIN_SIZE_D2); 
	arm_scale_f32(gAnaFreq, shift_amount, gAnaFreq, WIN_SIZE_D2);
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		target = (float)k * shift_amount; 
		if (target <= WIN_SIZE_D2) 
		{
			gSynMagn[target] += mags_and_phases->norm[k];// * mags_and_phases->env[target] / mags_and_phases->env[k];
			gSynFreq[target] = gAnaFreq[k]; //  * shift_amount;
		}
	}
	
	/* ***************** SYNTHESIS ******************* */
	for (k = 0; k < WIN_SIZE_D2; k++) 
	{
		// subtract bin mid frequency from true frequency from synthesis arrays 
		tmp = gSynFreq[k] - (float)k * freqPerBin;

		// get bin deviation from freq deviation 
		tmp *= oneOverFreqPerBin;

		// take number of overlaps into account 
		tmp = TwoPi_d_Overlap*tmp;

		// add the overlap phase advance back in 
		tmp += (float)k * expct; 

		// accumulate delta phase to get bin phase 
		gSumPhase[k] += tmp;
	}
}
static uint32_t phase_cnt = 0; 
void get_harmonized_output(float * outData, cvec_t *mags_and_phases, arm_rfft_fast_instance_f32 *fftInstance)
{
	uint32_t k; 
	float sin_value, cos_value;

	/* calculate shift envelope -- todo: try different filter cutoffs */ 
	arm_conv_f32(gSynMagn, WIN_SIZE_D2, (float *)envelope_filter, envelope_filter_length, shift_envelope);
	float *shift_env = &shift_envelope[envelope_filter_length>>1];
	arm_mult_f32(gSynMagn, mags_and_phases->env, gSynMagn, WIN_SIZE_D2); // scaling from original envelope	arm_mult_f32((float *)log_scale, gSynMagn, gSynMagn, WIN_SIZE_D2); 
	for (k = 0; k < WIN_SIZE_D2; k++)
	{
		/* scale by synth envelope - adding small term to avoid dividing by zero */ 
		gSynMagn[k] /= (shift_env[k] + 0.000000001f); //Abs(mags_and_phases->env[k] - shift_env[k]) / shift_env[k];  //Abs(2.0f*mags_and_phases->env[k] - shift_env[k]) / shift_env[k];
				
		/* Get real and imag part and interleave */ 
		//gSumPhase[k] = princarg(gSumPhase[k]); 
		arm_sin_cos_f32(gSumPhase[k], &sin_value, &cos_value);
		gFFTworksp[2*k] = gSynMagn[k]*cos_value;
		gFFTworksp[2*k+1] = gSynMagn[k]*sin_value;
	}

	/* do inverse transform */
	arm_rfft_fast_f32(fftInstance, gFFTworksp, ifft_real_values, 1);
	
	arm_scale_f32(ifft_real_values, 5.0f, ifft_real_values, WIN_SIZE); 
	
	/* Window and overlap & add */ 
	arm_mult_f32(scaled_hanning, ifft_real_values, ifft_real_values, WIN_SIZE);
 	arm_add_f32(gOutputAccum, ifft_real_values, gOutputAccum, WIN_SIZE);

	/* Copy data to output buffer */ 
	arm_copy_f32(gOutputAccum, outData, STEP_SIZE);
		
	/* shift accumulator */
	arm_copy_f32(&gOutputAccum[STEP_SIZE], gOutputAccum, WIN_SIZE);
	
	/* zero out synthesis mags */ 
	arm_fill_f32(0.0f, gSynMagn, WIN_SIZE_D2); 
	
	/* Save previous phases */ 
	arm_copy_f32(mags_and_phases->phas, prevAnaPhase, WIN_SIZE_D2); 
	
// 	if (++phase_cnt >= 20)	{
// 		arm_fill_f32(0.0f, gSumPhase, WIN_SIZE_D2); 	
// 		phase_cnt = 0; 
// 	}
}