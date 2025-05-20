/*
uPNG -- derived from LodePNG version 20100808

Copyright (c) 2005-2010 Lode Vandevenne
Copyright (c) 2010 Sean Middleditch

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

		1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.

		2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.

		3. This notice may not be removed or altered from any source
		distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upng.h"

// TODO: These are ESP-IDF specific, they should be abstracted or provided by Pebble's OS layer
#include "esp_heap_caps.h"
#include "esp_log.h"

#define UPNG_MALLOC(size) heap_caps_malloc(size, MALLOC_CAP_DEFAULT)
#define UPNG_FREE(ptr) heap_caps_free(ptr)
#define UPNG_REALLOC(ptr, size) heap_caps_realloc(ptr, size, MALLOC_CAP_DEFAULT)

#define MAKE_DWORD(a,b,c,d) (((unsigned long)(a) << 24) | ((unsigned long)(b) << 16) | ((unsigned long)(c) << 8) | (unsigned long)(d))
#define MAKE_DWORD_PTR(ptr) MAKE_DWORD((ptr)[0], (ptr)[1], (ptr)[2], (ptr)[3])

static const char *TAG = "upng";

struct upng_t {
	unsigned width;
	unsigned height;

	upng_format format;
	unsigned bpp; /* bits per pixel */
	unsigned bitdepth;
	unsigned components;
    unsigned pixelsize; /* bytes per pixel */

	unsigned char* buffer;
	unsigned long size;

	upng_error error;
	unsigned long error_line;

	// File buffer if loaded from file
	unsigned char *file_buffer;
	unsigned long file_size;
};

/* create a new upng structure */
static upng_t* upng_new(void)
{
	upng_t* upng;

	upng = (upng_t*)UPNG_MALLOC(sizeof(upng_t));
	if (upng == NULL) {
		return NULL;
	}

	memset(upng, 0, sizeof(upng_t));
	return upng;
}

/* read the a byte from the buffer */
static unsigned char upng_get_byte(const unsigned char* buffer, unsigned long offset)
{
	return buffer[offset];
}

/* read a 32-bit unsigned int from the buffer */
static unsigned long upng_get_dword(const unsigned char* buffer, unsigned long offset)
{
	return ((unsigned long)(buffer[offset + 0]) << 24) |
		   ((unsigned long)(buffer[offset + 1]) << 16) |
		   ((unsigned long)(buffer[offset + 2]) <<  8) |
		   ((unsigned long)(buffer[offset + 3]) <<  0);
}

/* set an error */
#define upng_set_error(upng, code) do { (upng)->error = (code); (upng)->error_line = __LINE__; } while (0)

/* note that this is a dummy implementation of inflate. It does not actually decompress anything. 
 * We need a proper zlib implementation for this. This is just to make it compile for now.
 */
static int uz_inflate(unsigned char *out, unsigned long *outlen, const unsigned char *in, unsigned long inlen) {
    ESP_LOGW(TAG, "STUB: uz_inflate called. PNG decompression will not work correctly.");
    if (*outlen < inlen) return -1; // Z_BUF_ERROR if output buffer is too small
    memcpy(out, in, inlen);
    *outlen = inlen;
    return 0; // Z_OK
}


upng_t* upng_new_from_bytes(const unsigned char* bytes, unsigned long size)
{
	upng_t* upng;

	if (bytes == NULL) {
		return NULL;
	}

	upng = upng_new();
	if (upng == NULL) {
		return NULL;
	}

	upng->file_buffer = (unsigned char*)UPNG_MALLOC(size);
	if (upng->file_buffer == NULL) {
		upng_set_error(upng, UPNG_ENOMEM);
		return upng;
	}
	memcpy(upng->file_buffer, bytes, size);
	upng->file_size = size;

	return upng;
}

upng_t* upng_new_from_file(const char* path)
{
	upng_t* upng;
	unsigned char* buffer;
	long size;
	FILE* fp;

	fp = fopen(path, "rb");
	if (fp == NULL) {
        // upng = upng_new(); // Can't set error if upng is NULL
        // if(upng) upng_set_error(upng, UPNG_ENOTFOUND);
		return NULL; /* No upng object to set error on yet */
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size <= 0) {
		fclose(fp);
        // upng = upng_new();
        // if(upng) upng_set_error(upng, UPNG_ENOTFOUND); /* Or some other error */
		return NULL;
	}

	buffer = (unsigned char*)UPNG_MALLOC(size);
	if (buffer == NULL) {
		fclose(fp);
        // upng = upng_new();
        // if(upng) upng_set_error(upng, UPNG_ENOMEM);
		return NULL;
	}

	if (fread(buffer, 1, size, fp) != (unsigned long)size) {
		UPNG_FREE(buffer);
		fclose(fp);
        // upng = upng_new();
        // if(upng) upng_set_error(upng, UPNG_ENOTFOUND); /* Or some other error */
		return NULL;
	}

	fclose(fp);

	upng = upng_new_from_bytes(buffer, size);
	UPNG_FREE(buffer);		/* upng_new_from_bytes makes a copy */

	return upng;
}

void upng_free(upng_t* upng)
{
	if (upng == NULL) {
		return;
	}

	UPNG_FREE(upng->buffer);
	UPNG_FREE(upng->file_buffer);
	UPNG_FREE(upng);
}

upng_error upng_header(upng_t* upng)
{
	const unsigned char *fbuffer;
	unsigned long fsize;

	unsigned long offset = 0;
	unsigned long length;
	unsigned long type;

	if (upng == NULL) {
		return UPNG_EPARAM;
	}
	fbuffer = upng->file_buffer;
	fsize = upng->file_size;

	/* check if we have a PGN file */
	if (fsize < 8 || upng_get_dword(fbuffer, 0) != 0x89504E47UL || upng_get_dword(fbuffer, 4) != 0x0D0A1A0AUL) {
		upng_set_error(upng, UPNG_ENOTPNG);
		return upng->error;
	}
	offset += 8;

	/* find IHDR */
	while (offset < fsize) {
		length = upng_get_dword(fbuffer, offset + 0);
		type   = upng_get_dword(fbuffer, offset + 4);

		if (type == MAKE_DWORD('I','H','D','R')) {
			break;
		}

		offset += 4 + 4 + length + 4;
	}

	/* check that we found an IHDR chunk */
	if (type != MAKE_DWORD('I','H','D','R')) {
		upng_set_error(upng, UPNG_EMALFORMED);
		return upng->error;
	}
	offset += 8; /* Add chunk type and length fields */

	/* read IHDR */
	upng->width = upng_get_dword(fbuffer, offset + 0);
	upng->height = upng_get_dword(fbuffer, offset + 4);
	upng->bitdepth = upng_get_byte(fbuffer, offset + 8);

	unsigned color_type = upng_get_byte(fbuffer, offset + 9);
	unsigned compression_method = upng_get_byte(fbuffer, offset + 10);
	unsigned filter_method = upng_get_byte(fbuffer, offset + 11);
	unsigned interlace_method = upng_get_byte(fbuffer, offset + 12);

	/* check for bad parameters */
	if (upng->width == 0 || upng->height == 0 || compression_method != 0 || filter_method != 0) {
		upng_set_error(upng, UPNG_EMALFORMED);
		return upng->error;
	}

	if (interlace_method != 0 && interlace_method != 1) { /* 0 = no interlace, 1 = Adam7 interlace */
        /* Adam7 interlacing is not supported by this simplified decoder, but LodePNG (original source) might handle it.
           For now, we treat it as unsupported. */
		upng_set_error(upng, UPNG_EUNINTERLACED);
		return upng->error;
	}

	/* set parameters based on color type */
	switch (color_type) {
	case 0: /* greyscale */
		upng->components = 1;
		switch (upng->bitdepth) {
		case 1: upng->format = UPNG_LUMINANCE1; break;
		case 2: upng->format = UPNG_LUMINANCE2; break;
		case 4: upng->format = UPNG_LUMINANCE4; break;
		case 8: upng->format = UPNG_LUMINANCE8; break;
		default: upng_set_error(upng, UPNG_EUNFORMAT); return upng->error;
		}
		break;
	case 2: /* rgb */
		upng->components = 3;
		switch (upng->bitdepth) {
		case 8: upng->format = UPNG_RGB8; break;
		case 16: upng->format = UPNG_RGB16; break;
		default: upng_set_error(upng, UPNG_EUNFORMAT); return upng->error;
		}
		break;
	case 4: /* greyscale with alpha */
		upng->components = 2;
		switch (upng->bitdepth) {
        // Note: LodePNG supported these. This simplified version might not fully.
        case 1: upng->format = UPNG_LUMINANCE_ALPHA1; break; 
        case 2: upng->format = UPNG_LUMINANCE_ALPHA2; break;
        case 4: upng->format = UPNG_LUMINANCE_ALPHA4; break;
		case 8: upng->format = UPNG_LUMINANCE_ALPHA8; break;
		default: upng_set_error(upng, UPNG_EUNFORMAT); return upng->error;
		}
		break;
	case 6: /* rgb with alpha */
		upng->components = 4;
		switch (upng->bitdepth) {
		case 8: upng->format = UPNG_RGBA8; break;
		case 16: upng->format = UPNG_RGBA16; break;
		default: upng_set_error(upng, UPNG_EUNFORMAT); return upng->error;
		}
		break;
	case 3: /* paletted */
        // Paletted images are not supported in this simplified decoder.
		upng_set_error(upng, UPNG_EUNSUPPORTED); // Or UPNG_EUNFORMAT
		return upng->error;
	default:
		upng_set_error(upng, UPNG_EUNFORMAT);
		return upng->error;
	}

	upng->bpp = upng->bitdepth * upng->components;
    upng->pixelsize = (upng->bpp + 7) / 8; /* bytes per pixel, rounded up */

	return UPNG_EOK;
}

upng_error upng_decode(upng_t* upng) {
    const unsigned char *fbuffer;
    unsigned long fsize;
    unsigned char *compressed_data = NULL, *current_compressed_data = NULL;
    unsigned long compressed_size = 0;
    unsigned long offset = 8; // Start after PNG signature
    int ret;

    if (upng == NULL || upng->file_buffer == NULL) {
        return UPNG_EPARAM;
    }

    if (upng->error != UPNG_EOK) {
        return upng->error;
    }

    if (upng->format == UPNG_BAD) {
        upng_error error = upng_header(upng);
        if (error != UPNG_EOK) return error;
    }

    fbuffer = upng->file_buffer;
    fsize = upng->file_size;

    // Calculate required buffer size
    // For non-interlaced, this is (width * bpp + 7) / 8 * height
    // Add 1 byte per scanline for filter type
    unsigned long scanline_bytes = (upng->width * upng->bpp + 7) / 8;
    unsigned long raw_buffer_size = (scanline_bytes + 1) * upng->height;
    unsigned char* raw_buffer = (unsigned char*)UPNG_MALLOC(raw_buffer_size);
    if (!raw_buffer) {
        upng_set_error(upng, UPNG_ENOMEM);
        return upng->error;
    }

    // Concatenate all IDAT chunks
    while (offset < fsize) {
        unsigned long length = upng_get_dword(fbuffer, offset + 0);
        unsigned long type = upng_get_dword(fbuffer, offset + 4);

        if (type == MAKE_DWORD('I', 'D', 'A', 'T')) {
            unsigned char *new_compressed_data = (unsigned char*)UPNG_REALLOC(compressed_data, compressed_size + length);
            if (!new_compressed_data) {
                UPNG_FREE(compressed_data);
                UPNG_FREE(raw_buffer);
                upng_set_error(upng, UPNG_ENOMEM);
                return upng->error;
            }
            compressed_data = new_compressed_data;
            memcpy(compressed_data + compressed_size, fbuffer + offset + 8, length);
            compressed_size += length;
        } else if (type == MAKE_DWORD('I', 'E', 'N', 'D')) {
            break;
        }
        offset += 4 + 4 + length + 4; // length, type, data, crc
    }

    if (!compressed_data || compressed_size == 0) {
        UPNG_FREE(raw_buffer);
        UPNG_FREE(compressed_data);
        upng_set_error(upng, UPNG_EMALFORMED); // No IDAT chunks found
        return upng->error;
    }

    // Decompress
    unsigned long decompressed_len = raw_buffer_size;
    ret = uz_inflate(raw_buffer, &decompressed_len, compressed_data, compressed_size);
    UPNG_FREE(compressed_data);

    if (ret != 0 /* Z_OK */) {
        UPNG_FREE(raw_buffer);
        upng_set_error(upng, UPNG_EMALFORMED); // Decompression error
        // Could also map zlib errors to upng errors if needed
        // e.g. if (ret == -3 /* Z_DATA_ERROR */) { ... }
        return upng->error;
    }

    // Unfilter (this is a STUB - does not actually unfilter)
    // For simplicity, we'll just copy scanline data, ignoring filter bytes.
    // A real implementation needs to handle Paeth, Sub, Up, Average, None filters.
    upng->size = upng->width * upng->height * upng->pixelsize;
    upng->buffer = (unsigned char*)UPNG_MALLOC(upng->size);
    if (!upng->buffer) {
        UPNG_FREE(raw_buffer);
        upng_set_error(upng, UPNG_ENOMEM);
        return upng->error;
    }

    ESP_LOGW(TAG, "STUB: PNG unfiltering is not implemented. Image will likely be corrupted.");
    unsigned char *dst = upng->buffer;
    unsigned char *src = raw_buffer;
    for (unsigned y = 0; y < upng->height; ++y) {
        // unsigned filter_type = *src++; // Read filter type byte - STUBBED
        src++; // Skip filter type byte
        memcpy(dst, src, scanline_bytes);
        dst += scanline_bytes;
        src += scanline_bytes;
    }

    UPNG_FREE(raw_buffer);
    return UPNG_EOK;
}


upng_error upng_get_error(const upng_t* upng)
{
	return upng->error;
}

unsigned long upng_get_error_line(const upng_t* upng)
{
	return upng->error_line;
}

unsigned upng_get_width(const upng_t* upng)
{
	return upng->width;
}

unsigned upng_get_height(const upng_t* upng)
{
	return upng->height;
}

unsigned upng_get_bpp(const upng_t* upng)
{
	return upng->bpp;
}

unsigned upng_get_bitdepth(const upng_t* upng)
{
	return upng->bitdepth;
}

unsigned upng_get_components(const upng_t* upng)
{
	return upng->components;
}

unsigned upng_get_pixelsize(const upng_t* upng)
{
    return upng->pixelsize;
}

upng_format upng_get_format(const upng_t* upng)
{
	return upng->format;
}

const unsigned char* upng_get_buffer(const upng_t* upng)
{
	return upng->buffer;
}

unsigned upng_get_size(const upng_t* upng)
{
	return upng->size;
} 