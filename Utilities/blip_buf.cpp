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

/*
YM2612 = 7.67 Mhz NTSC
Z80 = 3.579545 MHz
Crystal = 53.693175 = 15 x 3.57954 MHz NTSC

53693175 clock @ 50 == 1073863.5  [2^20.034]
*/

typedef unsigned long long fixed_t;

enum { blip_max_rate = (int) (3579545.0 / 50.0) };
enum { time_bits = 64-22 };  /* 22.42 -- 768000 * 2^42 = 2EE0 0000 0000 0000*/
static fixed_t const time_unit = (fixed_t) 1 << time_bits;

typedef signed int buf_t;

/** Sample buffer that resamples to output rate and accumulates samples
until they're read out */
struct blip_t
{
	fixed_t factor;
	fixed_t offset;
	int avail;
	int size;
	buf_t integrator;
};

/* probably not totally portable */
#define SAMPLES( buf ) ((buf_t*) ((buf) + 1))

/* Arithmetic (sign-preserving) right shift */
#define ARITH_SHIFT( n, shift ) \
	((n) >> (shift))

enum { max_sample = +32767 };
enum { min_sample = -32768 };

#define CLAMP( n ) \
{\
	if ( n > max_sample ) n = max_sample;\
	else if ( n < min_sample) n = min_sample;\
}

static int blip_sample_rate = 48000;

#include "blip_lpf.h"

#ifdef BLIP_ASSERT
static void check_assumptions( void )
{
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
}
#endif

blip_t* blip_new( int size )
{
	blip_t* m;
#ifdef BLIP_ASSERT
	assert( size >= 0 );
#endif
	
	m = (blip_t*) malloc( sizeof *m + size * sizeof (buf_t) );
	if ( m )
	{
		m->factor = time_unit;
		m->size   = size;
		blip_clear( m );
#ifdef BLIP_ASSERT
		check_assumptions();
#endif
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
	m->factor = (fixed_t) ((double) time_unit * sample_rate / clock_rate);
	blip_sample_rate = (int) sample_rate;
	
#ifdef BLIP_ASSERT
	/* Fails if clock_rate exceeds maximum, relative to sample_rate */
	assert( 0 <= factor - m->factor && factor - m->factor < 1 );
#endif
}

void blip_clear( blip_t* m )
{
	m->offset     = 0;
	m->avail      = 0;
	m->integrator = 0;
	memset( SAMPLES( m ), 0, m->size * sizeof (buf_t) );
}

int blip_clocks_needed( const blip_t* m, int samples )
{
	fixed_t needed;

#ifdef BLIP_ASSERT
	/* Fails if buffer can't hold that many more samples */
	assert( samples >= 0 && m->avail + samples <= m->size );
#endif
	
	needed = (fixed_t) samples * time_unit;
	if ( needed < m->offset )
		return 0;
	
	return (int)((needed - m->offset + m->factor - 1) / m->factor);
}

void blip_end_frame( blip_t* m, unsigned t )
{
	m->offset += (fixed_t) t * m->factor;
	
#ifdef BLIP_ASSERT
	/* Fails if buffer size was exceeded */
	assert( m->avail <= m->size );
#endif
}

int blip_samples_avail( const blip_t* m )
{
	return (m->offset >> time_bits);
}

static void remove_samples( blip_t* m, int count )
{
	buf_t* buf = SAMPLES( m );

	int lpf_taps;
	switch( blip_sample_rate ) {
	case 768000: lpf_taps = lpf_step_768_taps; break;
	case 384000: lpf_taps = lpf_step_384_taps; break;
	case 192000: lpf_taps = lpf_step_192_taps; break;
	case 96000: lpf_taps = lpf_step_96_taps; break;
	case 48000: lpf_taps = lpf_step_48_taps; break;
	default: lpf_taps = 0; break;
	}

	int remain = (m->offset >> time_bits) - count;
	if( lpf_taps > remain ) remain = lpf_taps;

	m->offset -= count * time_unit;
	
	memmove( &buf [0], &buf [count], remain * sizeof (buf_t) );
	memset( &buf [remain], 0, count * sizeof (buf_t) );
}

int blip_read_samples( blip_t* m, short out [], int count, int stereo )
{
#ifdef BLIP_ASSERT
	assert( count >= 0 );
	
	if ( count > m->avail )
		count = m->avail;
	
	if ( count )
#endif
	{
		int const step = stereo ? 2 : 1;
		buf_t const* in  = SAMPLES( m );
		buf_t const* end = in + count;
		buf_t sum = m->integrator;
		do
		{
			/* Eliminate fraction */
			buf_t s = ARITH_SHIFT( sum, lpf_frac );
			
			sum += *in++;
			
			CLAMP( s );
			
			*out = s;
			out += step;
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

void blip_add_delta( blip_t* m, unsigned time, int delta )
{
	if (!delta) return;

	fixed_t fixed = (fixed_t) (time * m->factor + m->offset);
	int pos = fixed >> time_bits;

	buf_t* out = SAMPLES( m ); + pos;

#ifdef BLIP_ASSERT
	/* Fails if buffer size was exceeded */
	/* assert( out <= &SAMPLES( m ) [m->size + end_frame_extra] ); */
#endif

	/* 31-bit * 15-bit = 46-bit >> 15 = 31-bit */
	switch( blip_sample_rate ) {
#if 1
	case 768000:
		for( int lcv = 0; lcv < lpf_step_768_taps; lcv++ ) {
			out [lcv] += ((signed long long)lpf_step_768[lcv] * delta) / lpf_scale;
		}
		break;

	case 384000:
		for( int lcv = 0; lcv < lpf_step_384_taps; lcv++ ) {
			out [lcv] += ((signed long long)lpf_step_384[lcv] * delta) / lpf_scale;
		}
		break;

	case 192000:
		for( int lcv = 0; lcv < lpf_step_192_taps; lcv++ ) {
			out [lcv] += ((signed long long)lpf_step_192[lcv] * delta) / lpf_scale;
		}
		break;

	case 96000:
		for( int lcv = 0; lcv < lpf_step_96_taps; lcv++ ) {
			out [lcv] += ((signed long long)lpf_step_96[lcv] * delta) / lpf_scale;
		}
		break;

	case 48000:
		for( int lcv = 0; lcv < lpf_step_48_taps; lcv++ ) {
			out [lcv] += ((signed long long)lpf_step_48[lcv] * delta) / lpf_scale;
		}
		break;
#endif

	default:
		out [0] += (buf_t) delta * (1UL << lpf_frac);
		break;
	}
}

void blip_add_delta_fast( blip_t* m, unsigned time, int delta )
{
	blip_add_delta(m, time, delta);
}
