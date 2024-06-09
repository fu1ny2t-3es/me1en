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

//#define DEBUG_BLIP

#ifdef DEBUG_BLIP
#include <windows.h>
#include <stdio.h>

void debug_me(char *msg, int x)
{
	while(GetModuleHandle(NULL)) {
		if( GetModuleHandle(msg) ) Sleep(1);
		Sleep (1);
	}
}
#endif

#if defined (BLARGG_TEST) && BLARGG_TEST
	#include "blargg_test.h"
#endif

/*
APU = 1789773 NTSC - 1662607 PAL
Crystal = 21.47727 = 6 x 3.57954 MHz NTSC

53693175 clock @ 50 == 1073863.5  [2^20.034]
*/

typedef unsigned long long fixed_t;

enum { time_bits = 64-22 };  /* 22.42 -- 768000 * 2^42 = 2EE0 0000 0000 0000*/
static fixed_t const time_unit = (fixed_t) 1 << time_bits;

typedef signed int buf_t;

struct blip_t
{
	fixed_t factor;
	fixed_t offset;
	int size;
	int clock_rate;
	int sample_rate;
	buf_t integrator;
	buf_t* buffer;
};

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
	
	m = (blip_t*) malloc( sizeof *m );
	
#ifdef DEBUG_BLIP
	printf("[blip_new] %d\n", size); fflush(stdout);
#endif

	if ( m )
	{
		m->buffer = (buf_t*) malloc( size * sizeof (buf_t));
		if (m->buffer == NULL)
		{
			blip_delete(m);
			return 0;
		}
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
		if (m->buffer != NULL)
			free(m->buffer);

		/* Clear fields in case user tries to use after freeing */
		memset( m, 0, sizeof *m );
		free( m );
	}
}

void blip_set_rates( blip_t* m, double clock_rate, double sample_rate )
{
	m->factor = (fixed_t) ((double) time_unit * sample_rate / clock_rate);

	m->clock_rate = (int) clock_rate;
	m->sample_rate = (int) sample_rate;

#ifdef DEBUG_BLIP
	printf("[blip_set_rates] %d %d %lld\n", (int) clock_rate, (int) sample_rate, m->factor); fflush(stdout);
#endif

#ifdef BLIP_ASSERT
	/* Fails if clock_rate exceeds maximum, relative to sample_rate */
	assert( 0 <= factor - m->factor && factor - m->factor < 1 );
#endif
}

void blip_clear( blip_t* m )
{
	m->offset     = 0;
	m->integrator = 0;
	memset( m->buffer, 0, m->size * sizeof (buf_t) );
}

int blip_clocks_needed( const blip_t* m, int samples )
{
	fixed_t needed;

#ifdef BLIP_ASSERT
	/* Fails if buffer can't hold that many more samples */
	assert( samples >= 0 && m->avail + samples <= m->size );
#endif
	
	needed = (fixed_t) samples * time_unit;

#ifdef DEBUG_BLIP
	printf("[blip_clocks_needed] %d\n", samples); fflush(stdout);
#endif

	if ( needed < m->offset )
		return 0;
	
	return (int)((needed - m->offset + m->factor - 1) / m->factor);
}

void blip_end_frame( blip_t* m, unsigned t )
{
#ifdef DEBUG_BLIP
	printf("[blip_end_frame] %lld %d %lld\n", m->offset, t, m->factor); fflush(stdout);
#endif

	m->offset += (fixed_t) t * m->factor;
	
#ifdef DEBUG_BLIP
	printf("[blip_end_frame] %lld %d %lld\n", m->offset, t, m->factor); fflush(stdout);
#endif

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
	buf_t* buf = m->buffer;

	int lpf_taps = blip_lpf_taps(m->sample_rate);

	int remain = (m->offset >> time_bits) - count;
	if( lpf_taps > remain ) remain = lpf_taps;

	m->offset -= count * time_unit;
	
#ifdef DEBUG_BLIP
	printf("[blip_remove_samples] %d %d %d %lld\n", remain, lpf_taps, count, m->offset); fflush(stdout);
#endif

	memmove( &buf [0], &buf [count], remain * sizeof (buf_t) );
	memset( &buf [remain], 0, count * sizeof (buf_t) );
}

int blip_read_samples( blip_t* m, short out [], int count, int stereo )
{
#ifdef DEBUG_BLIP
	printf("[blip_read_samples] %d\n", count); fflush(stdout);
	//debug_me("blip_read_samples", count);
#endif

#ifdef BLIP_ASSERT
	assert( count >= 0 );
#endif
	
	if ( count > blip_samples_avail(m) )
		count = blip_samples_avail(m);
	
	if ( count )
	{
		buf_t const* in = m->buffer;
		buf_t const* end = in + count;
		buf_t sum = m->integrator;
		int step = stereo ? 1 : 0;
		do
		{
			/* Eliminate fraction */
			buf_t s = ARITH_SHIFT( sum, lpf_frac );
#ifdef DEBUG_BLIP
			//printf("%d %d\n", sum, s); fflush(stdout);
#endif

			sum += *in++;
			
			CLAMP( s );
			
			*out++ = s;
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

	//debug_me("blip_add_delta", time);

	fixed_t fixed = (fixed_t) (time * m->factor + m->offset);
	int pos = fixed >> time_bits;

	buf_t* out = m->buffer + pos;

#ifdef DEBUG_BLIP
	printf("[blip_add_delta] %lld %d %d %d %d\n", fixed, pos, time, delta); fflush(stdout);
#endif

#ifdef BLIP_ASSERT
	/* Fails if buffer size was exceeded */
	assert( pos <= m->size );
#endif

	blip_lpf_mono(m->sample_rate, out, delta);
}

void blip_add_delta_fast( blip_t* m, unsigned time, int delta )
{
	blip_add_delta(m, time, delta);
}
