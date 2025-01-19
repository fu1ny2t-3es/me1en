/* blip_buf 1.1.0. http://www.slack.net/~ant/ */

#include "stdafx.h"
#include "blip_buf.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

/* Library Copyright (C) 2003-2009 Shay Green. This library is free software;
you can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
library is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#if defined (BLARGG_TEST) && BLARGG_TEST
	#include "blargg_test.h"
#endif

typedef unsigned long long fixed_t;
enum { pre_shift = 0 };

enum { time_bits = pre_shift + 12 };

static fixed_t const time_unit = (fixed_t) 1 << time_bits;

enum { bass_shift  = 9 }; /* affects high-pass filter breakpoint frequency */
enum { end_frame_extra = 2 }; /* allows deltas slightly after frame length */

enum { half_width  = 8 };
enum { buf_extra   = half_width*2 + end_frame_extra };
enum { phase_bits  = 5 };
enum { phase_count = 1 << phase_bits };
enum { delta_bits  = 15 };
enum { delta_unit  = 1 << delta_bits };
enum { frac_bits = time_bits - pre_shift };

/* We could eliminate avail and encode whole samples in offset, but that would
limit the total buffered samples to blip_max_frame. That could only be
increased by decreasing time_bits, which would reduce resample ratio accuracy.
*/

/** Sample buffer that resamples to output rate and accumulates samples
until they're read out */
struct blip_t
{
	fixed_t factor;
	fixed_t offset;
	int avail;
	int size;
	int integrator;
};

typedef int buf_t;

/* probably not totally portable */
#define SAMPLES( buf ) ((buf_t*) ((buf) + 1))

/* Arithmetic (sign-preserving) right shift */
#define ARITH_SHIFT( n, shift ) \
	((n) >> (shift))

enum { max_sample = +32767 };
enum { min_sample = -32768 };

#define CLAMP( n ) \
	{\
		if ( (short) n != n )\
			n = ARITH_SHIFT( n, 16 ) ^ max_sample;\
	}

static double blip_sample_rate = 48000.0;

static void check_assumptions( void )
{
#if 0
	int n;
	
	#if INT_MAX < 0x7FFFFFFF || UINT_MAX < 0xFFFFFFFF
		#error "int must be at least 32 bits"
	#endif
	
	assert( (-3 >> 1) == -2 ); /* right shift must preserve sign */
	
	n = max_sample * 2;
	CLAMP( n );
	assert( n == max_sample );
	
	n = min_sample * 2;
	CLAMP( n );
	assert( n == min_sample );
	
	assert( blip_max_ratio <= time_unit );
	assert( blip_max_frame <= (fixed_t) -1 >> time_bits );
#endif
}

blip_t* blip_new( int size )
{
	blip_t* m;
	assert( size >= 0 );
	
	m = (blip_t*) malloc( sizeof *m + (size + buf_extra) * sizeof (buf_t) );
	if ( m )
	{
		m->factor = time_unit / blip_max_ratio;
		m->size   = size;
		blip_clear( m );
		check_assumptions();
	}
	return m;
}

void blip_delete( blip_t* m )
{
	if ( m != NULL )
	{
		/* Clear fields in case user tries to use after freeing */
		memset( m, 0, sizeof *m );
		free( m );
	}
}

void blip_set_rates( blip_t* m, double clock_rate, double sample_rate )
{
	double factor = time_unit * sample_rate / clock_rate;
	m->factor = (fixed_t) factor;
	
	blip_sample_rate = sample_rate;
	
	/* Fails if clock_rate exceeds maximum, relative to sample_rate */
	assert( 0 <= factor - m->factor && factor - m->factor < 1 );
	
	/* Avoid requiring math.h. Equivalent to
	m->factor = (int) ceil( factor ) */
	if ( m->factor < factor )
		m->factor++;
	
	/* At this point, factor is most likely rounded up, but could still
	have been rounded down in the floating-point calculation. */
}

void blip_clear( blip_t* m )
{
	/* We could set offset to 0, factor/2, or factor-1. 0 is suitable if
	factor is rounded up. factor-1 is suitable if factor is rounded down.
	Since we don't know rounding direction, factor/2 accommodates either,
	with the slight loss of showing an error in half the time. Since for
	a 64-bit factor this is years, the halving isn't a problem. */
	
	m->offset     = m->factor / 2;
	m->avail      = 0;
	m->integrator = 0;
	memset( SAMPLES( m ), 0, (m->size + buf_extra) * sizeof (buf_t) );
}

int blip_clocks_needed( const blip_t* m, int samples )
{
	fixed_t needed;
	
	/* Fails if buffer can't hold that many more samples */
	assert( samples >= 0 && m->avail + samples <= m->size );
	
	needed = (fixed_t) samples * time_unit;
	if ( needed < m->offset )
		return 0;
	
	return (int)((needed - m->offset + m->factor - 1) / m->factor);
}

void blip_end_frame( blip_t* m, unsigned t )
{
	fixed_t off = t * m->factor + m->offset;
	m->avail += off >> time_bits;
	m->offset = off & (time_unit - 1);
	
	/* Fails if buffer size was exceeded */
	assert( m->avail <= m->size );
}

int blip_samples_avail( const blip_t* m )
{
	return m->avail;
}

static void remove_samples( blip_t* m, int count )
{
	buf_t* buf = SAMPLES( m );
	int remain = m->avail + buf_extra - count;
	m->avail -= count;
	
	memmove( &buf [0], &buf [count], remain * sizeof buf [0] );
	memset( &buf [remain], 0, count * sizeof buf [0] );
}

int blip_read_samples( blip_t* m, short out [], int count, int stereo )
{
	assert( count >= 0 );
	
	if ( count > m->avail )
		count = m->avail;
	
	if ( count )
	{
		int const step = stereo ? 2 : 1;
		buf_t const* in  = SAMPLES( m );
		buf_t const* end = in + count;
		int sum = m->integrator;
		do
		{
			/* Eliminate fraction */
			int s = ARITH_SHIFT( sum, delta_bits );
			
			sum += *in++;
			
			CLAMP( s );
			
			*out = s;
			out += step;
			
			/* High-pass filter */
			/* sum -= s << (delta_bits - bass_shift); */
		}
		while ( in != end );
		m->integrator = sum;
		
		remove_samples( m, count );
	}
	
	return count;
}

/* Things that didn't help performance on x86:
	__attribute__((aligned(128)))
	#define short int
	restrict
*/

/* https://fiiir.com/ */

/* Kaiser( 768000 sampling, 20000 cutoff, 40000 transition, 60 stopband ) */
static int const bl_step_768[71] = {
(int) (-0.000098092726162600 * 32767.0),
(int) (-0.000189528252561278 * 32767.0),
(int) (-0.000320447705586071 * 32767.0),
(int) (-0.000494151121613073 * 32767.0),
(int) (-0.000710737007201466 * 32767.0),
(int) (-0.000966136824648772 * 32767.0),
(int) (-0.001251251759043275 * 32767.0),
(int) (-0.001551273009943655 * 32767.0),
(int) (-0.001845264742358474 * 32767.0),
(int) (-0.002106080689066819 * 32767.0),
(int) (-0.002300671401260667 * 32767.0),
(int) (-0.002390817273091819 * 32767.0),
(int) (-0.002334300387735019 * 32767.0),
(int) (-0.002086497990422328 * 32767.0),
(int) (-0.001602351186412020 * 32767.0),
(int) (-0.000838633648428096 * 32767.0),
(int) (0.000243582159112557 * 32767.0),
(int) (0.001676385967545950 * 32767.0),
(int) (0.003482558509245782 * 32767.0),
(int) (0.005673316699617247 * 32767.0),
(int) (0.008246422710621456 * 32767.0),
(int) (0.011184793914599036 * 32767.0),
(int) (0.014455730788425389 * 32767.0),
(int) (0.018010851418428311 * 32767.0),
(int) (0.021786785652236615 * 32767.0),
(int) (0.025706641170947998 * 32767.0),
(int) (0.029682210202755622 * 32767.0),
(int) (0.033616841947171135 * 32767.0),
(int) (0.037408864810365025 * 32767.0),
(int) (0.040955406963715003 * 32767.0),
(int) (0.044156435951209876 * 32767.0),
(int) (0.046918820060101271 * 32767.0),
(int) (0.049160207312117649 * 32767.0),
(int) (0.050812522908097915 * 32767.0),
(int) (0.051824902686163472 * 32767.0),
(int) (0.052165907786116301 * 32767.0),
(int) (0.051824902686163472 * 32767.0),
(int) (0.050812522908097915 * 32767.0),
(int) (0.049160207312117649 * 32767.0),
(int) (0.046918820060101271 * 32767.0),
(int) (0.044156435951209876 * 32767.0),
(int) (0.040955406963715003 * 32767.0),
(int) (0.037408864810365025 * 32767.0),
(int) (0.033616841947171107 * 32767.0),
(int) (0.029682210202755622 * 32767.0),
(int) (0.025706641170947980 * 32767.0),
(int) (0.021786785652236615 * 32767.0),
(int) (0.018010851418428311 * 32767.0),
(int) (0.014455730788425389 * 32767.0),
(int) (0.011184793914599036 * 32767.0),
(int) (0.008246422710621456 * 32767.0),
(int) (0.005673316699617252 * 32767.0),
(int) (0.003482558509245782 * 32767.0),
(int) (0.001676385967545950 * 32767.0),
(int) (0.000243582159112557 * 32767.0),
(int) (-0.000838633648428096 * 32767.0),
(int) (-0.001602351186412019 * 32767.0),
(int) (-0.002086497990422328 * 32767.0),
(int) (-0.002334300387735019 * 32767.0),
(int) (-0.002390817273091819 * 32767.0),
(int) (-0.002300671401260667 * 32767.0),
(int) (-0.002106080689066819 * 32767.0),
(int) (-0.001845264742358474 * 32767.0),
(int) (-0.001551273009943655 * 32767.0),
(int) (-0.001251251759043275 * 32767.0),
(int) (-0.000966136824648772 * 32767.0),
(int) (-0.000710737007201466 * 32767.0),
(int) (-0.000494151121613072 * 32767.0),
(int) (-0.000320447705586071 * 32767.0),
(int) (-0.000189528252561278 * 32767.0),
(int) (-0.000098092726162600 * 32767.0)
};


/* Kaiser( 384000 sampling, 20000 cutoff, 40000 transition, 60 stopband ) */
static int const bl_step_384[35] = {
(int) (-0.000175812880091685 * 32767.0),
(int) (-0.000635812044189314 * 32767.0),
(int) (-0.001479279206595866 * 32767.0),
(int) (-0.002675642074653213 * 32767.0),
(int) (-0.004005148822996397 * 32767.0),
(int) (-0.005016801022270830 * 32767.0),
(int) (-0.005039854685094183 * 32767.0),
(int) (-0.003263360451153054 * 32767.0),
(int) (0.001118636250893612 * 32767.0),
(int) (0.008718423971787051 * 32767.0),
(int) (0.019761811783614903 * 32767.0),
(int) (0.033939358466188771 * 32767.0),
(int) (0.050346538899363219 * 32767.0),
(int) (0.067541651161594218 * 32767.0),
(int) (0.083724439471463283 * 32767.0),
(int) (0.097009046687554987 * 32767.0),
(int) (0.105739753754647753 * 32767.0),
(int) (0.108784101479873621 * 32767.0),
(int) (0.105739753754647753 * 32767.0),
(int) (0.097009046687554987 * 32767.0),
(int) (0.083724439471463283 * 32767.0),
(int) (0.067541651161594218 * 32767.0),
(int) (0.050346538899363219 * 32767.0),
(int) (0.033939358466188771 * 32767.0),
(int) (0.019761811783614892 * 32767.0),
(int) (0.008718423971787051 * 32767.0),
(int) (0.001118636250893612 * 32767.0),
(int) (-0.003263360451153054 * 32767.0),
(int) (-0.005039854685094183 * 32767.0),
(int) (-0.005016801022270830 * 32767.0),
(int) (-0.004005148822996397 * 32767.0),
(int) (-0.002675642074653213 * 32767.0),
(int) (-0.001479279206595866 * 32767.0),
(int) (-0.000635812044189314 * 32767.0),
(int) (-0.000175812880091685 * 32767.0)
};


/* Kaiser( 192000 sampling, 20000 cutoff, 40000 transition, 60 stopband ) */
static int const bl_step_192[19] = {
(int) (-0.000276028503720261 * 32767.0),
(int) (-0.002477319816001691 * 32767.0),
(int) (-0.007094444123606007 * 32767.0),
(int) (-0.010405788713147471 * 32767.0),
(int) (-0.003535920313012242 * 32767.0),
(int) (0.023405680658299382 * 32767.0),
(int) (0.073197876839255563 * 32767.0),
(int) (0.135275284222901887 * 32767.0),
(int) (0.187736879467335943 * 32767.0),
(int) (0.208347560563390016 * 32767.0),
(int) (0.187736879467335943 * 32767.0),
(int) (0.135275284222901887 * 32767.0),
(int) (0.073197876839255563 * 32767.0),
(int) (0.023405680658299382 * 32767.0),
(int) (-0.003535920313012242 * 32767.0),
(int) (-0.010405788713147471 * 32767.0),
(int) (-0.007094444123606014 * 32767.0),
(int) (-0.002477319816001691 * 32767.0),
(int) (-0.000276028503720261 * 32767.0)
};


/* Kaiser( 96000 sampling, 20000 cutoff, 40000 transition, 60 stopband ) */
static int const bl_step_96[11] = {
(int) (0.000335234619452467 * 32767.0),
(int) (-0.009448347470590280 * 32767.0),
(int) (-0.027215990885137889 * 32767.0),
(int) (0.051865459369631738 * 32767.0),
(int) (0.276611612110899019 * 32767.0),
(int) (0.415704064511489890 * 32767.0),
(int) (0.276611612110899019 * 32767.0),
(int) (0.051865459369631738 * 32767.0),
(int) (-0.027215990885137861 * 32767.0),
(int) (-0.009448347470590280 * 32767.0),
(int) (0.000335234619452467 * 32767.0)
};


/* Kaiser( 48000 sampling, 20000 cutoff, 23250 transition, 50 stopband ) */
static int const bl_step_48[7] = {
(int) (0.005910687029449499 * 32767.0),
(int) (-0.051208526065244689 * 32767.0),
(int) (0.127203093247553328 * 32767.0),
(int) (0.836189491576483590 * 32767.0),
(int) (0.127203093247553328 * 32767.0),
(int) (-0.051208526065244689 * 32767.0),
(int) (0.005910687029449499 * 32767.0)
};


/* Shifting by pre_shift allows calculation using unsigned int rather than
possibly-wider fixed_t. On 32-bit platforms, this is likely more efficient.
And by having pre_shift 32, a 32-bit platform can easily do the shift by
simply ignoring the low half. */

void blip_add_delta( blip_t* m, unsigned time, int delta )
{
#if 0
	blip_add_delta_fast(m, time, delta);
	return;
#endif

	if (!delta) return;

	unsigned fixed = (unsigned) ((time * m->factor + m->offset) >> pre_shift);
	buf_t* out = SAMPLES( m ) + m->avail + (fixed >> frac_bits);

	/* Fails if buffer size was exceeded */
	/* assert( out <= &SAMPLES( m ) [m->size + end_frame_extra] ); */
	
	if( blip_sample_rate == 768000.0 ) {
		out [0] += bl_step_768[0]*delta;
		out [1] += bl_step_768[1]*delta;
		out [2] += bl_step_768[2]*delta;
		out [3] += bl_step_768[3]*delta;
		out [4] += bl_step_768[4]*delta;
		out [5] += bl_step_768[5]*delta;
		out [6] += bl_step_768[6]*delta;
		out [7] += bl_step_768[7]*delta;
		out [8] += bl_step_768[8]*delta;
		out [9] += bl_step_768[9]*delta;
		out [10] += bl_step_768[10]*delta;
		out [11] += bl_step_768[11]*delta;
		out [12] += bl_step_768[12]*delta;
		out [13] += bl_step_768[13]*delta;
		out [14] += bl_step_768[14]*delta;
		out [15] += bl_step_768[15]*delta;
		out [16] += bl_step_768[16]*delta;
		out [17] += bl_step_768[17]*delta;
		out [18] += bl_step_768[18]*delta;
		out [19] += bl_step_768[19]*delta;
		out [20] += bl_step_768[20]*delta;
		out [21] += bl_step_768[21]*delta;
		out [22] += bl_step_768[22]*delta;
		out [23] += bl_step_768[23]*delta;
		out [24] += bl_step_768[24]*delta;
		out [25] += bl_step_768[25]*delta;
		out [26] += bl_step_768[26]*delta;
		out [27] += bl_step_768[27]*delta;
		out [28] += bl_step_768[28]*delta;
		out [29] += bl_step_768[29]*delta;
		out [30] += bl_step_768[30]*delta;
		out [31] += bl_step_768[31]*delta;
		out [32] += bl_step_768[32]*delta;
		out [33] += bl_step_768[33]*delta;
		out [34] += bl_step_768[34]*delta;
		out [35] += bl_step_768[35]*delta;
		out [36] += bl_step_768[36]*delta;
		out [37] += bl_step_768[37]*delta;
		out [38] += bl_step_768[38]*delta;
		out [39] += bl_step_768[39]*delta;
		out [40] += bl_step_768[40]*delta;
		out [41] += bl_step_768[41]*delta;
		out [42] += bl_step_768[42]*delta;
		out [43] += bl_step_768[43]*delta;
		out [44] += bl_step_768[44]*delta;
		out [45] += bl_step_768[45]*delta;
		out [46] += bl_step_768[46]*delta;
		out [47] += bl_step_768[47]*delta;
		out [48] += bl_step_768[48]*delta;
		out [49] += bl_step_768[49]*delta;
		out [50] += bl_step_768[50]*delta;
		out [51] += bl_step_768[51]*delta;
		out [52] += bl_step_768[52]*delta;
		out [53] += bl_step_768[53]*delta;
		out [54] += bl_step_768[54]*delta;
		out [55] += bl_step_768[55]*delta;
		out [56] += bl_step_768[56]*delta;
		out [57] += bl_step_768[57]*delta;
		out [58] += bl_step_768[58]*delta;
		out [59] += bl_step_768[59]*delta;
		out [60] += bl_step_768[60]*delta;
		out [61] += bl_step_768[61]*delta;
		out [62] += bl_step_768[62]*delta;
		out [63] += bl_step_768[63]*delta;
		out [64] += bl_step_768[64]*delta;
		out [65] += bl_step_768[65]*delta;
		out [66] += bl_step_768[66]*delta;
		out [67] += bl_step_768[67]*delta;
		out [68] += bl_step_768[68]*delta;
		out [69] += bl_step_768[69]*delta;
		out [70] += bl_step_768[70]*delta;
	}

	else if( blip_sample_rate == 384000.0 ) {
		out [0] += bl_step_384[0]*delta;
		out [1] += bl_step_384[1]*delta;
		out [2] += bl_step_384[2]*delta;
		out [3] += bl_step_384[3]*delta;
		out [4] += bl_step_384[4]*delta;
		out [5] += bl_step_384[5]*delta;
		out [6] += bl_step_384[6]*delta;
		out [7] += bl_step_384[7]*delta;
		out [8] += bl_step_384[8]*delta;
		out [9] += bl_step_384[9]*delta;
		out [10] += bl_step_384[10]*delta;
		out [11] += bl_step_384[11]*delta;
		out [12] += bl_step_384[12]*delta;
		out [13] += bl_step_384[13]*delta;
		out [14] += bl_step_384[14]*delta;
		out [15] += bl_step_384[15]*delta;
		out [16] += bl_step_384[16]*delta;
		out [17] += bl_step_384[17]*delta;
		out [18] += bl_step_384[18]*delta;
		out [19] += bl_step_384[19]*delta;
		out [20] += bl_step_384[20]*delta;
		out [21] += bl_step_384[21]*delta;
		out [22] += bl_step_384[22]*delta;
		out [23] += bl_step_384[23]*delta;
		out [24] += bl_step_384[24]*delta;
		out [25] += bl_step_384[25]*delta;
		out [26] += bl_step_384[26]*delta;
		out [27] += bl_step_384[27]*delta;
		out [28] += bl_step_384[28]*delta;
		out [29] += bl_step_384[29]*delta;
		out [30] += bl_step_384[30]*delta;
		out [31] += bl_step_384[31]*delta;
		out [32] += bl_step_384[32]*delta;
		out [33] += bl_step_384[33]*delta;
		out [34] += bl_step_384[34]*delta;
	}

	else if( blip_sample_rate == 192000.0 ) {
		out [0] += bl_step_192[0]*delta;
		out [1] += bl_step_192[1]*delta;
		out [2] += bl_step_192[2]*delta;
		out [3] += bl_step_192[3]*delta;
		out [4] += bl_step_192[4]*delta;
		out [5] += bl_step_192[5]*delta;
		out [6] += bl_step_192[6]*delta;
		out [7] += bl_step_192[7]*delta;
		out [8] += bl_step_192[8]*delta;
		out [9] += bl_step_192[9]*delta;
		out [10] += bl_step_192[10]*delta;
		out [11] += bl_step_192[11]*delta;
		out [12] += bl_step_192[12]*delta;
		out [13] += bl_step_192[13]*delta;
		out [14] += bl_step_192[14]*delta;
		out [15] += bl_step_192[15]*delta;
		out [16] += bl_step_192[16]*delta;
		out [17] += bl_step_192[17]*delta;
		out [18] += bl_step_192[18]*delta;
	}

	else if( blip_sample_rate == 96000.0 ) {
		out [0] += bl_step_96[0]*delta;
		out [1] += bl_step_96[1]*delta;
		out [2] += bl_step_96[2]*delta;
		out [3] += bl_step_96[3]*delta;
		out [4] += bl_step_96[4]*delta;
		out [5] += bl_step_96[5]*delta;
		out [6] += bl_step_96[6]*delta;
		out [7] += bl_step_96[7]*delta;
		out [8] += bl_step_96[8]*delta;
		out [9] += bl_step_96[9]*delta;
		out [10] += bl_step_96[10]*delta;
	}

	else if( blip_sample_rate == 48000.0 ) {
		out [0] += bl_step_48[0]*delta;
		out [1] += bl_step_48[1]*delta;
		out [2] += bl_step_48[2]*delta;
		out [3] += bl_step_48[3]*delta;
		out [4] += bl_step_48[4]*delta;
		out [5] += bl_step_48[5]*delta;
		out [6] += bl_step_48[6]*delta;
	}
}

void blip_add_delta_fast( blip_t* m, unsigned time, int delta )
{
#if 1
	blip_add_delta(m, time, delta);
	return;
#endif

	fixed_t fixed = (fixed_t) ((time * m->factor + m->offset) >> pre_shift);
	buf_t* out = SAMPLES( m ) + m->avail + (fixed >> frac_bits);
	
	int interp = fixed >> (frac_bits - delta_bits) & (delta_unit - 1);
	int delta2 = delta * interp;
	
	/* Fails if buffer size was exceeded */
	/* assert( out <= &SAMPLES( m ) [m->size + end_frame_extra] ); */
	
	out [7] += delta * delta_unit - delta2;
	out [8] += delta2;
}
