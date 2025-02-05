/* https://fiiir.com/ */


/*
note: low-pass filter with nearest neighbor downsample  [48K -> 48K = raw]
      frontend does high-quality downsampling  [384K -> 48K]
*/


/* 1.16.15 */
enum { lpf_frac = 15 };
enum { lpf_scale = 32768 };


static int blip_lpf_cutoff = 0;  /* set to nyquist (1/2) of final output sampling rate - 0 = none */


#define LPF_TAPS(x) (buf_t) ((double) (x) * (double) (1UL << lpf_frac) * (double) lpf_scale)


#define BLIP_KAISER


/* excellent quality, brickwall, crisp transitions */
#ifdef BLIP_KAISER
	#include "blip_kaiser_48K.h"
	#include "blip_kaiser_96K.h"
	#include "blip_kaiser_192K.h"
	#include "blip_kaiser_384K.h"
	#include "blip_kaiser_768K.h"
#endif


/* higher quality, brickwall, pretty good but kinda flat */
#ifdef BLIP_BLACKMAN
	#include "blip_blackman_48K.h"
	#include "blip_blackman_96K.h"
	#include "blip_blackman_192K.h"
	#include "blip_blackman_384K.h"
	#include "blip_blackman_768K.h"
#endif


/* good quality, softer edges, 40db stopband, 20% faster */
#ifdef BLIP_KAISER_FAST
	#include "blip_kaiser_fast_48K.h"
	#include "blip_kaiser_fast_96K.h"
	#include "blip_kaiser_fast_192K.h"
	#include "blip_kaiser_fast_384K.h"
	#include "blip_kaiser_fast_768K.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif

void set_blip_cutoff(int rate)
{
	blip_lpf_cutoff = rate;
}

#ifdef __cplusplus
}
#endif


static int blip_lpf_taps(int sample_rate)
{
#ifdef BLIP_KAISER
	switch( sample_rate ) {
	case 48000: return kaiser_48K_taps;
	case 96000: return kaiser_96K_taps;
	case 192000: return kaiser_192K_taps;
	case 384000: return kaiser_384K_taps;
	case 768000: return kaiser_768K_taps;
	}
#endif

#ifdef BLIP_BLACKMAN
	switch( sample_rate ) {
	case 48000: return blackman_48K_taps;
	case 96000: return blackman_96K_taps;
	case 192000: return blackman_192K_taps;
	case 384000: return blackman_384K_taps;
	case 768000: return blackman_768K_taps;
	}
#endif

#ifdef BLIP_KAISER_FAST
	switch( sample_rate ) {
	case 48000: return kaiser_fast_48K_taps;
	case 384000: return kaiser_fast_384K_taps;
	case 768000: return kaiser_fast_768K_taps;
	case 96000: return kaiser_fast_96K_taps;
	case 192000: return kaiser_fast_192K_taps;
	}
#endif

	return 0;
}


#ifdef BLIP_KAISER
static void blip_lpf_kaiser_stereo(int sample_rate, buf_t* out_l, buf_t* out_r, int delta_l, int delta_r)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_768K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_768K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_384K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_384K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_192K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_192K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_96K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_96K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < kaiser_48K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_48K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_48K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_768K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_768K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_384K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_384K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_192K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_192K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_96K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_96K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_768K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_768K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_384K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_384K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_192K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_192K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_768K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_768K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_384K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_384K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_768K_384K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_768K_384K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	out_l [0] += (buf_t) delta_l * (1UL << lpf_frac);
	out_r [0] += (buf_t) delta_r * (1UL << lpf_frac);
}


static void blip_lpf_kaiser_mono(int sample_rate, buf_t* out, int delta)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_768K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_384K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_192K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_96K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < kaiser_48K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_48K_24K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_768K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_384K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_192K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_96K_48K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_768K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_384K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_192K_96K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_768K_192K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_384K_192K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_768K_384K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	out [0] += (buf_t) delta * (1UL << lpf_frac);
}
#endif


#ifdef BLIP_BLACKMAN
static void blip_lpf_blackman_stereo(int sample_rate, buf_t* out_l, buf_t* out_r, int delta_l, int delta_r)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_768K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_768K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_384K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_384K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_192K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_192K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < blackman_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_96K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_96K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < blackman_48K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_48K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_48K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_768K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_768K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_384K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_384K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_192K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_192K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < blackman_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_96K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_96K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_768K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_768K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_384K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_384K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_192K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_192K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_768K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_768K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_384K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_384K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)blackman_768K_384K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)blackman_768K_384K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	out_l [0] += (buf_t) delta_l * (1UL << lpf_frac);
	out_r [0] += (buf_t) delta_r * (1UL << lpf_frac);
}


static void blip_lpf_blackman_mono(int sample_rate, buf_t* out, int delta)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_768K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_384K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_192K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < blackman_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_96K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < blackman_48K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_48K_24K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_768K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_384K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_192K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < blackman_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_96K_48K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_768K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_384K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < blackman_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_192K_96K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_768K_192K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < blackman_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_384K_192K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < blackman_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)blackman_768K_384K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	out [0] += (buf_t) delta * (1UL << lpf_frac);
}
#endif


#ifdef BLIP_KAISER_FAST
static void blip_lpf_kaiser_fast_stereo(int sample_rate, buf_t* out_l, buf_t* out_r, int delta_l, int delta_r)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_768K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_768K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_384K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_384K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_192K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_192K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_fast_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_96K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_96K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < kaiser_fast_48K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_48K_24K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_48K_24K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_768K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_768K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_384K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_384K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_192K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_192K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_fast_96K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_96K_48K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_96K_48K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_768K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_768K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_384K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_384K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_192K_96K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_192K_96K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_768K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_768K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_384K_192K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_384K_192K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out_l [lcv] += ((signed long long)kaiser_fast_768K_384K[lcv] * delta_l) / lpf_scale;
				out_r [lcv] += ((signed long long)kaiser_fast_768K_384K[lcv] * delta_r) / lpf_scale;
			}
			return;
		}
	}


	out_l [0] += (buf_t) delta_l * (1UL << lpf_frac);
	out_r [0] += (buf_t) delta_r * (1UL << lpf_frac);
}


static void blip_lpf_kaiser_fast_mono(int sample_rate, buf_t* out, int delta)
{
	if( blip_lpf_cutoff == 24000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_768K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_384K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_192K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_fast_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_96K_24K[lcv] * delta) / lpf_scale;
			}
			return;

		case 48000:
			for( int lcv = 0; lcv < kaiser_fast_48K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_48K_24K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 48000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_768K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_384K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_192K_48K[lcv] * delta) / lpf_scale;
			}
			return;

		case 96000:
			for( int lcv = 0; lcv < kaiser_fast_96K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_96K_48K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 96000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_768K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_384K_96K[lcv] * delta) / lpf_scale;
			}
			return;

		case 192000:
			for( int lcv = 0; lcv < kaiser_fast_192K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_192K_96K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 192000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_768K_192K[lcv] * delta) / lpf_scale;
			}
			return;

		case 384000:
			for( int lcv = 0; lcv < kaiser_fast_384K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_384K_192K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	if( blip_lpf_cutoff == 384000 ) {
		switch( sample_rate ) {
		case 768000:
			for( int lcv = 0; lcv < kaiser_fast_768K_taps; lcv++ ) {
				out [lcv] += ((signed long long)kaiser_fast_768K_384K[lcv] * delta) / lpf_scale;
			}
			return;
		}
	}


	out [0] += (buf_t) delta * (1UL << lpf_frac);
}
#endif


static void blip_lpf_stereo(int sample_rate, buf_t* out_l, buf_t* out_r, int delta_l, int delta_r)
{
	/* 31-bit * 15-bit = 46-bit >> 15 = 31-bit */

#ifdef BLIP_KAISER
	blip_lpf_kaiser_stereo(sample_rate, out_l, out_r, delta_l, delta_r);
#endif

#ifdef BLIP_BLACKMAN
	blip_lpf_blackman_stereo(sample_rate, out_l, out_r, delta_l, delta_r);
#endif

#ifdef BLIP_KAISER_FAST
	blip_lpf_kaiser_fast_stereo(sample_rate, out_l, out_r, delta_l, delta_r);
#endif
}


static void blip_lpf_mono(int sample_rate, buf_t* out, int delta)
{
	/* 31-bit * 15-bit = 46-bit >> 15 = 31-bit */

#ifdef BLIP_KAISER
	blip_lpf_kaiser_mono(sample_rate, out, delta);
#endif

#ifdef BLIP_BLACKMAN
	blip_lpf_blackman_mono(sample_rate, out, delta);
#endif

#ifdef BLIP_KAISER_FAST
	blip_lpf_kaiser_fast_mono(sample_rate, out, delta);
#endif
}
