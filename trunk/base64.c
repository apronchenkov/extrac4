/* base64.c -- Encode binary data using printable characters.
   Copyright (C) 1999, 2000, 2001, 2004, 2005, 2006 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Simon Josefsson.  Partially adapted from GNU MailUtils
 * (mailbox/filter_trans.c, as of 2004-11-28).  Improved by review
 * from Paul Eggert, Bruno Haible, and Stepan Kasal.
 *
 * See also RFC 3548 <http://www.ietf.org/rfc/rfc3548.txt>.
 *
 * Be careful with error checking.  Here is how you would typically
 * use these functions:
 *
 * bool ok = base64_decode_alloc (in, inlen, &out, &outlen);
 * if (!ok)
 *   FAIL: input was not valid base64
 * if (out == NULL)
 *   FAIL: memory allocation error
 * OK: data in OUT/OUTLEN
 *
 * size_t outlen = base64_encode_alloc (in, inlen, &out);
 * if (out == NULL && outlen == 0 && inlen != 0)
 *   FAIL: input too long
 * if (out == NULL)
 *   FAIL: memory allocation error
 * OK: data in OUT/OUTLEN.
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Get prototype. */
#include "base64.h"

/* Get malloc. */
#include <stdlib.h>

/* Get UCHAR_MAX. */
#include <limits.h>

/* C89 compliant way to cast 'char' to 'unsigned char'. */
static unsigned char to_uchar(char ch) {
		return ch;
}

/* Base64 encode IN array of size INLEN into OUT array of size OUTLEN.
   If OUTLEN is less than BASE64_LENGTH(INLEN), write as many bytes as
   possible. */
size_t base64_encode(const char * in, size_t insize, char * out, size_t outsize) {
		static const char b64str[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		
		const char * inmax = in + insize;
		const char * outmin = out;
		const char * outmax = out + outsize;

		while( in < inmax && out < outmax ) {
		  
				*out++ = b64str[ to_uchar(in[0]) >> 2 ];

				if( outmax == out )
						break;

				if( inmax == in + 1 ) {
						*out++ = b64str[ ( to_uchar(in[0] ) << 4) & 0x3f ];
						++in;
				} else {
						*out++ = b64str[ ( ( to_uchar(in[0] ) << 4 ) | ( to_uchar(in[1]) >> 4 ) ) & 0x3f ];
						++in;
				}

				if( outmax == out )
						break;

				if( inmax == in ) {
						*out++ = '=';
				} else if( inmax == in + 1 ) {
						*out++ = b64str[ ( to_uchar(in[0]) << 2 ) & 0x3f ];
						++in;
				} else {
						*out++ = b64str[ ( ( to_uchar(in[0]) << 2 ) | ( to_uchar(in[1]) >> 6 ) ) & 0x3f ];
						++in;
				}

				if( outmax == out )
						break;

				if( inmax == in )
						*out++ = '=';
				else {
						*out++ = b64str[ to_uchar(in[0]) & 0x3f ];
						++in;
				}
		}

		return out - outmin;
}

/* Allocate a buffer and store zero terminated base64 encoded data
   from array IN of size INLEN, returning BASE64_LENGTH(INLEN), i.e.,
   the length of the encoded data, excluding the terminating zero.  On
   return, the OUT variable will hold a pointer to newly allocated
   memory that must be deallocated by the caller.  If output string
   length would overflow, 0 is returned and OUT is set to NULL.  If
   memory allocation failed, OUT is set to NULL, and the return value
   indicates length of the requested memory block, i.e.,
   BASE64_LENGTH(inlen) + 1. */
size_t base64_encode_alloc(const char *in, size_t inlen, char **out) {
		size_t outlen = base64_length(inlen);

							/* Check for overflow in outlen computation.
							 *
							 * If there is no overflow, outlen >= inlen.
							 *
							 * If the operation (inlen + 2) overflows then it yields at most +1, so
							 * outlen is 0.
							 *
							 * If the multiplication overflows, we lose at least half of the
							 * correct value, so the result is < ((inlen + 2) / 3) * 2, which is
							 * less than (inlen + 2) * 0.66667, which is less than inlen as soon as
							 * (inlen > 4).
							 */
		if( inlen > outlen ) {
				*out = NULL;
				return 0;
		}

		*out = (char*)malloc(outlen);
		if( NULL == *out )
				return outlen;

		base64_encode (in, inlen, *out, outlen);

		return outlen;
}

/* With this approach this file works independent of the charset used
   (think EBCDIC).  However, it does assume that the characters in the
   Base64 alphabet (A-Za-z0-9+/) are encoded in 0..255.  POSIX
   1003.1-2001 require that char and unsigned char are 8-bit
   quantities, though, taking care of that problem.  But this may be a
   potential problem on non-POSIX C99 platforms. 

   IBM C V6 for AIX mishandles "#define B64(x) ...'x'...", so use "_"
   as the formal parameter rather than "x".  */
#define B64(_)					\
    ((_) == 'A' ? 0				\
   : (_) == 'B' ? 1				\
   : (_) == 'C' ? 2				\
   : (_) == 'D' ? 3				\
   : (_) == 'E' ? 4				\
   : (_) == 'F' ? 5				\
   : (_) == 'G' ? 6				\
   : (_) == 'H' ? 7				\
   : (_) == 'I' ? 8				\
   : (_) == 'J' ? 9				\
   : (_) == 'K' ? 10				\
   : (_) == 'L' ? 11				\
   : (_) == 'M' ? 12				\
   : (_) == 'N' ? 13				\
   : (_) == 'O' ? 14				\
   : (_) == 'P' ? 15				\
   : (_) == 'Q' ? 16				\
   : (_) == 'R' ? 17				\
   : (_) == 'S' ? 18				\
   : (_) == 'T' ? 19				\
   : (_) == 'U' ? 20				\
   : (_) == 'V' ? 21				\
   : (_) == 'W' ? 22				\
   : (_) == 'X' ? 23				\
   : (_) == 'Y' ? 24				\
   : (_) == 'Z' ? 25				\
   : (_) == 'a' ? 26				\
   : (_) == 'b' ? 27				\
   : (_) == 'c' ? 28				\
   : (_) == 'd' ? 29				\
   : (_) == 'e' ? 30				\
   : (_) == 'f' ? 31				\
   : (_) == 'g' ? 32				\
   : (_) == 'h' ? 33				\
   : (_) == 'i' ? 34				\
   : (_) == 'j' ? 35				\
   : (_) == 'k' ? 36				\
   : (_) == 'l' ? 37				\
   : (_) == 'm' ? 38				\
   : (_) == 'n' ? 39				\
   : (_) == 'o' ? 40				\
   : (_) == 'p' ? 41				\
   : (_) == 'q' ? 42				\
   : (_) == 'r' ? 43				\
   : (_) == 's' ? 44				\
   : (_) == 't' ? 45				\
   : (_) == 'u' ? 46				\
   : (_) == 'v' ? 47				\
   : (_) == 'w' ? 48				\
   : (_) == 'x' ? 49				\
   : (_) == 'y' ? 50				\
   : (_) == 'z' ? 51				\
   : (_) == '0' ? 52				\
   : (_) == '1' ? 53				\
   : (_) == '2' ? 54				\
   : (_) == '3' ? 55				\
   : (_) == '4' ? 56				\
   : (_) == '5' ? 57				\
   : (_) == '6' ? 58				\
   : (_) == '7' ? 59				\
   : (_) == '8' ? 60				\
   : (_) == '9' ? 61				\
   : (_) == '+' ? 62				\
   : (_) == '/' ? 63				\
   : -1)

static const signed char b64[0x100] = {
		B64(0x00), B64(0x01), B64(0x02), B64(0x03), B64(0x04), B64(0x05), B64(0x06), B64(0x07),
		B64(0x08), B64(0x09), B64(0x0a), B64(0x0b), B64(0x0c), B64(0x0d), B64(0x0e), B64(0x0f), 
		B64(0x10), B64(0x11), B64(0x12), B64(0x13), B64(0x14), B64(0x15), B64(0x16), B64(0x17),
		B64(0x18), B64(0x19), B64(0x1a), B64(0x1b), B64(0x1c), B64(0x1d), B64(0x1e), B64(0x1f), 
		B64(0x20), B64(0x21), B64(0x22), B64(0x23), B64(0x24), B64(0x25), B64(0x26), B64(0x27),
		B64(0x28), B64(0x29), B64(0x2a), B64(0x2b), B64(0x2c), B64(0x2d), B64(0x2e), B64(0x2f), 
		B64(0x30), B64(0x31), B64(0x32), B64(0x33), B64(0x34), B64(0x35), B64(0x36), B64(0x37),
		B64(0x38), B64(0x39), B64(0x3a), B64(0x3b), B64(0x3c), B64(0x3d), B64(0x3e), B64(0x3f), 
		B64(0x40), B64(0x41), B64(0x42), B64(0x43), B64(0x44), B64(0x45), B64(0x46), B64(0x47),
		B64(0x48), B64(0x49), B64(0x4a), B64(0x4b), B64(0x4c), B64(0x4d), B64(0x4e), B64(0x4f), 
		B64(0x50), B64(0x51), B64(0x52), B64(0x53), B64(0x54), B64(0x55), B64(0x56), B64(0x57),
		B64(0x58), B64(0x59), B64(0x5a), B64(0x5b), B64(0x5c), B64(0x5d), B64(0x5e), B64(0x5f), 
		B64(0x60), B64(0x61), B64(0x62), B64(0x63), B64(0x64), B64(0x65), B64(0x66), B64(0x67),
		B64(0x68), B64(0x69), B64(0x6a), B64(0x6b), B64(0x6c), B64(0x6d), B64(0x6e), B64(0x6f), 
		B64(0x70), B64(0x71), B64(0x72), B64(0x73), B64(0x74), B64(0x75), B64(0x76), B64(0x77),
		B64(0x78), B64(0x79), B64(0x7a), B64(0x7b), B64(0x7c), B64(0x7d), B64(0x7e), B64(0x7f), 
		B64(0x80), B64(0x81), B64(0x82), B64(0x83), B64(0x84), B64(0x85), B64(0x86), B64(0x87),
		B64(0x88), B64(0x89), B64(0x8a), B64(0x8b), B64(0x8c), B64(0x8d), B64(0x8e), B64(0x8f), 
		B64(0x90), B64(0x91), B64(0x92), B64(0x93), B64(0x94), B64(0x95), B64(0x96), B64(0x97),
		B64(0x98), B64(0x99), B64(0x9a), B64(0x9b), B64(0x9c), B64(0x9d), B64(0x9e), B64(0x9f), 
		B64(0xa0), B64(0xa1), B64(0xa2), B64(0xa3), B64(0xa4), B64(0xa5), B64(0xa6), B64(0xa7),
		B64(0xa8), B64(0xa9), B64(0xaa), B64(0xab), B64(0xac), B64(0xad), B64(0xae), B64(0xaf), 
		B64(0xb0), B64(0xb1), B64(0xb2), B64(0xb3), B64(0xb4), B64(0xb5), B64(0xb6), B64(0xb7),
		B64(0xb8), B64(0xb9), B64(0xba), B64(0xbb), B64(0xbc), B64(0xbd), B64(0xbe), B64(0xbf), 
		B64(0xc0), B64(0xc1), B64(0xc2), B64(0xc3), B64(0xc4), B64(0xc5), B64(0xc6), B64(0xc7),
		B64(0xc8), B64(0xc9), B64(0xca), B64(0xcb), B64(0xcc), B64(0xcd), B64(0xce), B64(0xcf), 
		B64(0xd0), B64(0xd1), B64(0xd2), B64(0xd3), B64(0xd4), B64(0xd5), B64(0xd6), B64(0xd7),
		B64(0xd8), B64(0xd9), B64(0xda), B64(0xdb), B64(0xdc), B64(0xdd), B64(0xde), B64(0xdf), 
		B64(0xe0), B64(0xe1), B64(0xe2), B64(0xe3), B64(0xe4), B64(0xe5), B64(0xe6), B64(0xe7),
		B64(0xe8), B64(0xe9), B64(0xea), B64(0xeb), B64(0xec), B64(0xed), B64(0xee), B64(0xef), 
		B64(0xf0), B64(0xf1), B64(0xf2), B64(0xf3), B64(0xf4), B64(0xf5), B64(0xf6), B64(0xf7),
		B64(0xf8), B64(0xf9), B64(0xfa), B64(0xfb), B64(0xfc), B64(0xfd), B64(0xfe), B64(0xff)
};

#if UCHAR_MAX == 255
# define uchar_in_range(c) true
#else
# define uchar_in_range(c) ((c) <= 255)
#endif

/* Return true if CH is a character from the Base64 alphabet, and
   false otherwise.  Note that '=' is padding and not considered to be
   part of the alphabet.  */
bool isbase64 (char ch) {
		return uchar_in_range( to_uchar(ch) ) && 0 <= b64[ to_uchar(ch) ];
}

/* Decode base64 encoded input array IN of length INLEN to output
   array OUT that can hold *OUTLEN bytes.  Return true if decoding was
   successful, i.e. if the input was valid base64 data, false
   otherwise.  If *OUTLEN is too small, as many bytes as possible will
   be written to OUT.  On return, *OUTLEN holds the length of decoded
   bytes in OUT.  Note that as soon as any non-alphabet characters are
   encountered, decoding is stopped and false is returned.  This means
   that, when applicable, you must remove any line terminators that is
   part of the data stream before calling this function.  */
bool base64_decode(const char * in, size_t insize, char * out, size_t *outsize) {
		const char * inmax_basic = in + insize - 4;
		const char * inmax = in + insize;
		const char * outmin = out;
		const char * outmax = out + *outsize;

		if( 0 != insize%4 )
				return false;
		
		while( in < inmax_basic && out < outmax ) {

				if( !isbase64(in[0]) || !isbase64(in[1]) ||
					!isbase64(in[2]) || !isbase64(in[3]) )
						break;
				
				*out++ = ( b64[ to_uchar(in[0]) ] << 2 ) | ( b64[ to_uchar(in[1]) ] >> 4 );
				++in;

				if( outmax == out )
						break;

				*out++ = ( b64[ to_uchar(in[0]) ] << 4 ) | ( b64[ to_uchar(in[1]) ] >> 2);
				++in;

				if( outmax == out )
						break;

				*out++ = ( b64[ to_uchar(in[0]) ] << 6 ) | ( b64[ to_uchar(in[1]) ] );
				in += 2;
		}

		do {
				if( in != inmax_basic || out == outmax ) 
						break;
				
				if( !isbase64(in[0]) || !isbase64(in[1]) )
						break;

				*out++ = ( b64[ to_uchar(in[0]) ] << 2 ) | ( b64[ to_uchar(in[1]) ] >> 4 );
				++in;
				if( outmax == out )
						break;

				if( '=' == in[1] ) {
						in += ( '=' == in[2] ) ? 3 : 0;
						break;
				}

				if( !isbase64 (in[1]) )
						break;

				*out++ = ( b64[ to_uchar(in[0]) ] << 4 ) | ( b64[ to_uchar(in[1]) ] >> 2);
				++in;
				if( outmax == out )
						break;
						
				if( in[1] == '=' ) {
						++in;
						break;
				}
				
				if( !isbase64( in[1] ) )
						break;
								
				*out++ = ( b64[ to_uchar(in[0]) ] << 6 ) | ( b64[ to_uchar(in[1]) ] );
				in += 2;

		} while( false );

		if( in != inmax )
				return false;

		*outsize = out - outmin;

		return true;
}

/* Allocate an output buffer in *OUT, and decode the base64 encoded
   data stored in IN of size INLEN to the *OUT buffer.  On return, the
   size of the decoded data is stored in *OUTLEN.  OUTLEN may be NULL,
   if the caller is not interested in the decoded length.  *OUT may be
   NULL to indicate an out of memory error, in which case *OUTLEN
   contains the size of the memory block needed.  The function returns
   true on successful decoding and memory allocation errors.  (Use the
   *OUT and *OUTLEN parameters to differentiate between successful
   decoding and memory error.)  The function returns false if the
   input was invalid, in which case *OUT is NULL and *OUTLEN is
   undefined. */
bool base64_decode_alloc(const char *in, size_t inlen, char **out, size_t *outlen) {
							/* This may allocate a few bytes too much, depending on input,
							   but it's not worth the extra CPU time to compute the exact amount.
							   The exact amount is 3 * inlen / 4, minus 1 if the input ends
							   with "=" and minus another 1 if the input ends with "==".
							   Dividing before multiplying avoids the possibility of overflow.  */
		const size_t needlen = 3 * (inlen / 4) + 2;
		
		*out = (char*)malloc(needlen);
		if( !*out )
				return true;
		
		if( !base64_decode(in, inlen, *out, outlen) ) {
				free(*out);
				*out = NULL;
				return false;
		}

		return true;
}
