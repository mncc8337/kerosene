#pragma once

#include "stdint.h"

#define PSF2_HAS_UNICODE_TABLE 0x1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t headersize;
    uint32_t flags;
    uint32_t numglyph;
    uint32_t bytesperglyph;
    uint32_t height;
    uint32_t width;
} PSF_font_t;

void psf_init(char* font_data);
void psf_get_font_geometry(int* width, int* height, int* bpg);
char* psf_get_glyph(char chr);
