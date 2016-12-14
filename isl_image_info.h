#ifndef ISLII_INCLUDE_ISL_IMAGE_INFO_H
#define ISLII_INCLUDE_ISL_IMAGE_INFO_H

#define ISLII_VERSION 1

#ifdef ISLII_STATIC
#define ISLIIDEF static
#else
#define ISLIIDEF extern
#endif

typedef enum {
	ISLII_UNKNOWN,
	ISLII_BMP,
	ISLII_PNG,
	ISLII_JPEG,
	ISLII_GIF,
} islii_format;

typedef enum {
	ISLII_ERROR_OK,
	ISLII_ERROR_UNKNOWN_FORMAT,
	ISLII_ERROR_BAD_FILE,
	ISLII_ERROR_BAD_JPEG_MARKER,
} islii_error;

typedef enum {
	ISLII_UNKNOWN_COLORTYPE,
	ISLII_INDEX,
	ISLII_GRAYSCALE,
	ISLII_GRAYSCALE_ALPHA,
	ISLII_RGB,
	ISLII_RGB_ALPHA,
} islii_colortype;

typedef struct {
	islii_error error;
	islii_format format;
	islii_colortype colortype;
	unsigned long int width;
	unsigned long int height;
	unsigned int bpp;
} islii_result;

typedef void* islii_file;
typedef size_t (*islii_fread)(void *,size_t,size_t,islii_file); 

#ifdef __cplusplus
extern "C" {
#endif

ISLIIDEF islii_result islii_info( islii_file, islii_fread );
ISLIIDEF const char *islii_strformat( islii_format format );
ISLIIDEF const char *islii_strerror( islii_error error );
ISLIIDEF const char *islii_strcolortype( islii_colortype colortype );

#ifndef ISLII_NO_STDIO
#include <stdio.h>
extern islii_result islii_info_file( FILE *f );
#endif

#ifdef __cplusplus
}
#endif

#endif // ISLII_INCLUDE_ISL_IMAGE_INFO_H


#ifdef ISL_IMAGE_INFO_IMPLEMENTATION

#ifndef ISLII_NO_STDIO
static size_t fread_stdio_wrapper(void *ptr, size_t size, size_t nmemb, islii_file stream) {
	return fread( ptr, size, nmemb, (FILE *) stream );
}

islii_result islii_info_file( FILE *f ) {
	return islii_info( (islii_file)f, fread_stdio_wrapper );
}
#endif

static unsigned char fread_u8( islii_file f, islii_fread read ) {
	unsigned char a;
	read( &a, 1, 1, f );
	return a;
}

static unsigned int fread_u16_little( islii_file f, islii_fread read ) {
	return fread_u8(f,read) + (fread_u8(f,read)<<8);
}

static unsigned int fread_u32_little( islii_file f, islii_fread read ) {
	return fread_u8(f,read) + (fread_u8(f,read)<<8) + (fread_u8(f,read)<<16) + (fread_u8(f,read)<<24);
}

static unsigned int fread_u16_big( islii_file f, islii_fread read ) {
	return (fread_u8(f,read)<<8) + fread_u8(f,read);
}

static unsigned long int fread_u32_big( islii_file f, islii_fread read ) {
	return (fread_u8(f,read)<<24) + (fread_u8(f,read)<<16) + (fread_u8(f,read)<<8) + fread_u8(f,read);
}

static unsigned int fread_u16( islii_file f, islii_fread read, int big ) {
	return big ? fread_u16_big(f,read) : fread_u16_little(f,read);
}

static unsigned long int fread_u32( islii_file f, islii_fread read, int big ) {
	return big ? fread_u32_big(f,read) : fread_u32_little(f,read);
}

static void fskip( islii_file f, islii_fread read, int count ) {
	int i;
	for ( i = 0; i < count; i++ ) {
		fread_u8( f, read );
	}
}

static islii_result fread_png( islii_file f, islii_fread read ) {
	islii_result result = { ISLII_ERROR_OK, ISLII_UNKNOWN, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0 };
	unsigned char third;
	fread( &third, 1, 1, f );
	if ( third == 78 ) {
		fskip( f, read, 13 );
		result.width = fread_u32_big( f, read );
		result.height = fread_u32_big( f, read );
		result.format = ISLII_PNG;
		result.bpp = (unsigned int) fread_u8( f, read );
		switch ( fread_u8( f, read )) {
			case 0: result.colortype = ISLII_GRAYSCALE; break;
			case 2: result.colortype = ISLII_RGB; break;
			case 3: result.colortype = ISLII_INDEX; break;
			case 4: result.colortype = ISLII_GRAYSCALE_ALPHA; break;
			case 6: result.colortype = ISLII_RGB_ALPHA; break;
		}
	} else {
		result.error = ISLII_ERROR_UNKNOWN_FORMAT;
	}
	return result;
}

static islii_result fread_bmp( islii_file f, islii_fread read ) {
	islii_result result = { ISLII_ERROR_OK, ISLII_BMP, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0 };
	fskip( f, read, 16 );
	result.width = fread_u32_little( f, read );
	result.height = fread_u32_little( f, read );
	fread_u16_little( f, read );
	result.bpp = fread_u16_little( f, read );
	switch ( result.bpp ) {
		case 1: case 2: case 4: case 8: result.colortype = ISLII_INDEX; break;
		case 24: result.colortype = ISLII_RGB; break;
		case 16: case 32: result.colortype = ISLII_RGB_ALPHA; break;
	}
	return result;
}

static islii_result fread_gif( islii_file f, islii_fread read ) {
	islii_result result = { ISLII_ERROR_OK, ISLII_UNKNOWN, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0 };
	if ( fread_u8( f, read ) == 'F' ) {
		fskip( f, read, 3 );
		result.width = fread_u16_little( f, read );
		result.height = fread_u16_little( f, read );
		result.format = ISLII_GIF;
		result.bpp = 8;
		result.colortype = ISLII_INDEX;
	} else {
		result.error = ISLII_ERROR_UNKNOWN_FORMAT;
	}
	return result;
}

static islii_result fread_jpeg( islii_file f, islii_fread read ) {
	islii_result result = { ISLII_ERROR_OK, ISLII_JPEG, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0 };
	for(;;) switch (fread_u16_big( f, read )) {
		case 0xffd8: case 0xffd0: case 0xffd1: case 0xffd2: case 0xffd3:
		case 0xffd4: case 0xffd5: case 0xffd6: case 0xffd7: case 0xffd9:
			break;

		case 0xffdd:
			fread_u16_big( f, read );
			break;

		case 0xffe0: case 0xffe1: case 0xffe2: case 0xffe3: case 0xffe4:
		case 0xffe5: case 0xffe6: case 0xffe7: case 0xffe8: case 0xffe9: 
		case 0xffea: case 0xffeb: case 0xffec: case 0xffed: case 0xffee:
		case 0xffef: case 0xfffe: case 0xffdb: case 0xffc4: case 0xffda: 
			fskip( f, read, ((long int) fread_u16_big( f, read )) - 2 );
			break;

		case 0xffc0: case 0xffc2:
			fskip( f, read, 2 );
			result.bpp = (unsigned int) fread_u8( f, read );
			result.width = fread_u16_big( f, read );
			result.height = fread_u16_big( f, read );
			switch ( fread_u8( f, read )) {
				case 1: result.colortype = ISLII_GRAYSCALE; break;
				case 3: case 4: result.colortype = ISLII_RGB; break;
			}
			break;

		default:
			result.error = ISLII_ERROR_BAD_JPEG_MARKER;
			break;
	}
	return result;
}

islii_result islii_info( islii_file f, islii_fread read ) {
	if ( !f ) {
		islii_result result = { ISLII_ERROR_BAD_FILE, ISLII_UNKNOWN, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0 };
		return result;
	} else {
		islii_result result = { ISLII_ERROR_UNKNOWN_FORMAT, ISLII_UNKNOWN, ISLII_UNKNOWN_COLORTYPE, 0, 0, 0};
		switch ( fread_u16_big( f, read )) {
			case 0x8950: result = fread_png( f, read ); break;
			case 0x424D: result = fread_bmp( f, read ); break;
			case 0x4749: result = fread_gif( f, read ); break;
			case 0xFFD8: result = fread_jpeg( f, read ); break;
		}
		return result;
	}
}

const char *islii_strformat( islii_format format ) {
	switch ( format ) {
		case ISLII_BMP: return "BMP";
		case ISLII_PNG: return "PNG";
		case ISLII_JPEG: return "JPEG";
		case ISLII_GIF: return "GIF";
		default: return "UNKNOWN";
	}
}

const char *islii_strerror( islii_error error ) {
	switch( error ) {
		case ISLII_ERROR_OK: return "OK";
		case ISLII_ERROR_UNKNOWN_FORMAT: return "UNKNOWN_FORMAT";
		case ISLII_ERROR_BAD_FILE: return "BAD_FILE";
		case ISLII_ERROR_BAD_JPEG_MARKER: return "BAD_JPEG_MARKER";
		default: return "UNKNOWN_ERROR";
	}
}

const char *islii_strcolortype( islii_colortype colortype ) {
	switch( colortype ) {
		case ISLII_INDEX: return "INDEX";
		case ISLII_GRAYSCALE: return "GRAYSCALE";
		case ISLII_GRAYSCALE_ALPHA: return "GRAYSCALE_ALPHA";
		case ISLII_RGB: return "RGB";
		case ISLII_RGB_ALPHA: return "RGB_ALPHA";
		default: return "UNKNOWN_COLORTYPE";
	}
}

#endif // ISL_IMAGE_INFO_IMPLEMENTATION
