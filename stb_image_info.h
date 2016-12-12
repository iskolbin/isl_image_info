#ifndef STBII_INCLUDE_STB_IMAGE_INFO_H
#define STBII_INCLUDE_STB_IMAGE_INFO_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	STBII_UNKNOWN,
	STBII_BMP,
	STBII_PNG,
	STBII_JPEG,
	STBII_GIF,
	STBII_TIFF,
} stbii_format;

typedef enum {
	STBII_RESULT_OK,
	STBII_RESULT_UNKNOWN_FORMAT,
	STBII_RESULT_FILE_OPEN_ERROR,
	STBII_RESULT_BAD_JPEG_MARKER,
	STBII_RESULT_BAD_TIFF,
} stbii_result;

typedef enum {
	STBII_UNKNOWN_COLORTYPE,
	STBII_INDEX,
	STBII_GRAYSCALE,
	STBII_GRAYSCALE_ALPHA,
	STBII_RGB,
	STBII_RGB_ALPHA,
} stbii_colortype;

#ifdef __cplusplus
extern "C" {
#endif

extern stbii_result stbii_info( const char *filename, unsigned long int *pwidth, unsigned long int *pheight, stbii_format *pformat, unsigned int *pbpp, stbii_colortype *pcolortype );
extern const char *stbii_strformat( stbii_format format );
extern const char *stbii_strresult( stbii_result result );
extern const char *stbii_strcolortype( stbii_colortype colortype );

#ifdef __cplusplus
}
#endif

static unsigned char fread_u8( FILE *f ) {
	unsigned char a;
	fread( &a, 1, 1, f );
	return a;
}

static unsigned int fread_u16_little( FILE *f ) {
	return fread_u8(f) + (fread_u8(f)<<8);
}

static unsigned int fread_u32_little( FILE * f ) {
	return fread_u8(f) + (fread_u8(f)<<8) + (fread_u8(f)<<16) + (fread_u8(f)<<24);
}

static unsigned int fread_u16_big( FILE *f ) {
	return (fread_u8(f)<<8) + fread_u8(f);
}

static unsigned long int fread_u32_big( FILE *f ) {
	return (fread_u8(f)<<24) + (fread_u8(f)<<16) + (fread_u8(f)<<8) + fread_u8(f);
}

static unsigned int fread_u16( FILE *f, int big ) {
	return big ? fread_u16_big(f) : fread_u16_little(f);
}

static unsigned long int fread_u32( FILE *f, int big ) {
	return big ? fread_u32_big(f) : fread_u32_little(f);
}

static stbii_result fread_png( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format, unsigned int *bpp, stbii_colortype *colortype ) {
	unsigned char third;
	fread( &third, 1, 1, f );
	if ( third == 78 ) {
		unsigned char ch;
		fseek( f, 13, SEEK_CUR );
		*width = fread_u32_big( f );
		*height = fread_u32_big( f );
		*format = STBII_PNG;
		fread( &ch, 1, 1, f );
		*bpp = (unsigned int) ch;
		fread( &ch, 1, 1, f );
		switch (ch) {
			case 0: *colortype = STBII_GRAYSCALE; break;
			case 2: *colortype = STBII_RGB; break;
			case 3: *colortype = STBII_INDEX; break;
			case 4: *colortype = STBII_GRAYSCALE_ALPHA; break;
			case 6: *colortype = STBII_RGB_ALPHA; break;
		}
		return STBII_RESULT_OK;
	} else {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}
}

static stbii_result fread_bmp( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format, unsigned int *bpp, stbii_colortype *colortype ) {
	fseek( f, 16, SEEK_CUR );
	*width = fread_u32_little( f );
	*height = fread_u32_little( f );
	*format = STBII_BMP;
	fread_u16_little( f );
	*bpp = fread_u16_little( f );
	switch ( *bpp ) {
		case 1: case 2: case 4: case 8: *colortype = STBII_INDEX; break;
		case 24: *colortype = STBII_RGB; break;
		case 16: case 32: *colortype = STBII_RGB_ALPHA; break;
	}
	return STBII_RESULT_OK;
}

static stbii_result fread_gif( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format , unsigned int *bpp, stbii_colortype *colortype ) {
	unsigned char third;
	fread( &third, 1, 1, f );
	if ( third == 'F' ) {
		fseek( f, 3, SEEK_CUR );			
		*width = fread_u16_little( f );
		*height = fread_u16_little( f );
		*format = STBII_GIF;
		*bpp = 8;
		*colortype = STBII_INDEX;
		return STBII_RESULT_OK;
	} else {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}
}

static stbii_result fread_jpeg( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format, unsigned int *bpp, stbii_colortype *colortype ) {
	*format = STBII_JPEG;
	for(;;) switch (fread_u16_big( f )) {
		case 0xffd8: case 0xffd0: case 0xffd1: case 0xffd2: case 0xffd3:
		case 0xffd4: case 0xffd5: case 0xffd6: case 0xffd7: case 0xffd9:
			break;

		case 0xffdd:
			fread_u16_big( f );
			break;

		case 0xffe0: case 0xffe1: case 0xffe2: case 0xffe3: case 0xffe4:
		case 0xffe5: case 0xffe6: case 0xffe7: case 0xffe8: case 0xffe9: 
		case 0xffea: case 0xffeb: case 0xffec: case 0xffed: case 0xffee:
		case 0xffef: case 0xfffe: case 0xffdb: case 0xffc4: case 0xffda: 
			fseek( f, ((long int) fread_u16_big( f )) - 2, SEEK_CUR );
			break;

		case 0xffc0: case 0xffc2:
			fseek( f, 2, SEEK_CUR );
			*bpp = (unsigned int) fread_u8( f );
			*width = fread_u16_big( f );
			*height = fread_u16_big( f );
			switch (fread_u8(f)) {
				case 1: *colortype = STBII_GRAYSCALE; break;
				case 3: case 4: *colortype = STBII_RGB; break;
			}
			return STBII_RESULT_OK;

		default:
			return STBII_RESULT_BAD_JPEG_MARKER;
	}
}

static stbii_result fread_tiff( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format, unsigned int *bpp, stbii_colortype *colortype, int big ) {
	int found = 0;
	unsigned int i;
	unsigned int entries;
	unsigned int rest = fread_u16_big( f );

	if ((big && rest != 0x2A) || (!big && rest != 0x2A00)) {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}

	*bpp = 1;
	*format = STBII_TIFF;
	fseek( f, fread_u32( f, big ) - 8, SEEK_CUR );
	entries = fread_u16( f, big );
	for ( i = 1; i <= entries && found != 3; i++ ) {
		unsigned int tag = fread_u16( f, big );
		unsigned int fieldType = fread_u16( f, big );
		unsigned long int count = fread_u32( f, big );
		unsigned int offset;

		if (( fieldType == 3 || fieldType == 8 )) {
			offset = fread_u16( f, big );
			fseek( f, 2, SEEK_CUR );
		} else {
			offset = fread_u16( f, big );
		}

		switch ( tag ) {
			case 0x100: *width = offset; found |= 1; break;
			case 0x101: *height = offset; found |= 2; break;
			case 0x102: *bpp = offset; break;
			case 0x106:
				found |= 4;
				switch ( offset ) {
					case 0: case 1: *colortype = STBII_GRAYSCALE; break;
					case 3: case 4: *colortype = STBII_INDEX; break;
					case 2: case 5: case 6: case 8: case 9: case 10:
					case 0x8023: case 0x884c: case 0x804c: case 0x804d: *colortype = STBII_RGB; break;
				}
		}
	}

	return (found == 7) ? STBII_RESULT_OK : STBII_RESULT_BAD_TIFF;
}

stbii_result stbii_info( const char *filename, unsigned long int *pwidth, unsigned long int *pheight, stbii_format *pformat, unsigned int *pbpp, stbii_colortype *pcolortype ) {
	FILE *f = fopen( filename, "r" );
	unsigned long int width = 0;
	unsigned long int height = 0;
	unsigned int bpp = 0;
	stbii_format format = STBII_UNKNOWN;
	stbii_result result = STBII_RESULT_OK;
	stbii_colortype colortype = STBII_UNKNOWN_COLORTYPE;

	if ( !f ) {
		result = STBII_RESULT_FILE_OPEN_ERROR;
	} else {
		unsigned int header = fread_u16_big( f );
		switch ( header ) {
			case 0x8950: result = fread_png( f, &width, &height, &format, &bpp, &colortype ); break;
			case 0x424D: result = fread_bmp( f, &width, &height, &format, &bpp, &colortype ); break;
			case 0x4749: result = fread_gif( f, &width, &height, &format, &bpp, &colortype ); break;
			case 0xFFD8: result = fread_jpeg( f, &width, &height, &format, &bpp, &colortype ); break;
			case 0x4D4D: result = fread_tiff( f, &width, &height, &format, &bpp, &colortype, 1 ); break;
			case 0x4949: result = fread_tiff( f, &width, &height, &format, &bpp, &colortype, 0 ); break;
			default: result = STBII_RESULT_UNKNOWN_FORMAT;
		}
	}

	if ( result == STBII_RESULT_OK ) {
		if ( pwidth != NULL ) *pwidth = width;
		if ( pheight != NULL ) *pheight = height;
		if ( pformat != NULL ) *pformat = format;
		if ( pbpp != NULL ) *pbpp = bpp;
		if ( pcolortype != NULL ) *pcolortype = colortype;
	}

	return result;
}

const char *stbii_strformat( stbii_format format ) {
	switch ( format ) {
		case STBII_BMP: return "BMP";
		case STBII_PNG: return "PNG";
		case STBII_JPEG: return "JPEG";
		case STBII_GIF: return "GIF";
		case STBII_TIFF: return "TIFF";
		default: return "UNKNOWN";
	}
}

const char *stbii_strresult( stbii_result result ) {
	switch( result ) {
		case STBII_RESULT_OK: return "OK";
		case STBII_RESULT_UNKNOWN_FORMAT: return "UNKNOWN_FORMAT";
		case STBII_RESULT_FILE_OPEN_ERROR: return "FILE_OPEN_ERROR";
		case STBII_RESULT_BAD_JPEG_MARKER: return "BAD_JPEG_MARKER";
		case STBII_RESULT_BAD_TIFF: return "BAD_TIFF";
		default: return "UNKNOWN_ERROR";
	}
}

const char *stbii_strcolortype( stbii_colortype colortype ) {
	switch( colortype ) {
		case STBII_INDEX: return "INDEX";
		case STBII_GRAYSCALE: return "GRAYSCALE";
		case STBII_GRAYSCALE_ALPHA: return "GRAYSCALE_ALPHA";
		case STBII_RGB: return "RGB";
		case STBII_RGB_ALPHA: return "RGB_ALPHA";
		default: return "UNKNOWN_COLORTYPE";
	}
}

#endif
