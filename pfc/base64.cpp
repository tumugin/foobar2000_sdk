﻿#include "pfc.h"

namespace bitWriter {
	static void set_bit(t_uint8 * p_stream,size_t p_offset, bool state) {
		t_uint8 mask = 1 << (7-(p_offset&7));
		t_uint8 & byte = p_stream[p_offset>>3];
		byte = (byte & ~mask) | (state ? mask : 0);
	}
	static void set_bits(t_uint8 * stream, t_size offset, t_size word, t_size bits) {
		for(t_size walk = 0; walk < bits; ++walk) {
			t_uint8 bit = (t_uint8)((word >> (bits - walk - 1))&1);
			set_bit(stream, offset+walk, bit != 0);
		}
	}
};

namespace pfc {
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	static const uint8_t alphabetRev[256] = 
		{0x40,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x3E,0xFF,0xFF,0xFF,0x3F
		,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E
		,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28
		,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
		,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
	};

	size_t base64_strlen( const char * text ) {
		size_t ret = 0;
		for ( size_t w = 0; ;++w ) {
			uint8_t c = (uint8_t) text[w];
			if (c == 0) break;
			if ( alphabetRev[c] != 0xFF ) ++ret;
		}
		return ret;
	}
	t_size base64_decode_estimate(const char * text) {
		t_size textLen = base64_strlen(text);
		return textLen * 3 / 4;
	}

	

	void base64_decode(const char * text, void * out) {
		
		size_t outWritePtr = 0;
		size_t textWalk = 0;
		size_t inTemp = 0;
		uint8_t temp[3];
		for ( size_t textWalk = 0 ; ; ++ textWalk ) {
			const uint8_t c = (uint8_t) text[textWalk];
			if (c == 0) break;
			const uint8_t v = alphabetRev[ c ];
			if ( v != 0xFF ) {
				PFC_ASSERT( inTemp + 6 <= 24 );
				bitWriter::set_bits(temp,inTemp,v,6);
				inTemp += 6;
				if ( inTemp == 24 ) {
					memcpy( (uint8_t*) out + outWritePtr, temp, 3 );
					outWritePtr += 3;
					inTemp = 0;
				}
			}
		}
		if ( inTemp > 0 ) {
			size_t delta = inTemp / 8;
			memcpy( (uint8_t*) out + outWritePtr, temp, delta );
			outWritePtr += delta;
			inTemp = 0;
		}
#if PFC_DEBUG
		size_t estimated = base64_decode_estimate( text ) ;
		PFC_ASSERT( outWritePtr == estimated );
#endif
	}
	void base64_encode(pfc::string_base & out, const void * in, t_size inSize) {
		out.reset(); base64_encode_append(out, in, inSize);
	}
	void base64_encode_append(pfc::string_base & out, const void * in, t_size inSize) {
		int shift = 0;
		int accum = 0;
		const t_uint8 * inPtr = reinterpret_cast<const t_uint8*>(in);

		for(t_size walk = 0; walk < inSize; ++walk) {
			accum <<= 8;
			shift += 8;
			accum |= inPtr[walk];
			while ( shift >= 6 ) {
				shift -= 6;
				out << format_char( alphabet[(accum >> shift) & 0x3F] );
			}
		}
		if (shift == 4) {
			out << format_char( alphabet[(accum & 0xF)<<2] ) << "=";
		} else if (shift == 2) {
			out << format_char( alphabet[(accum & 0x3)<<4] ) << "==";
		}
	}

	void base64_decode_to_string( pfc::string_base & out, const char * text ) {
		char * ptr = out.lock_buffer( base64_decode_estimate( text ) );
		base64_decode(text, ptr);
		out.unlock_buffer();
	}
	void base64_encode_from_string( pfc::string_base & out, const char * in ) {
		base64_encode( out, in, strlen(in) );
	}
}
