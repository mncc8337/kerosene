#include "misc/psf.h"

// default font
#include "access/font.h"

PSF_font_t* font = (PSF_font_t*)font_h_data;

void psf_load(char* font_data) {
    font = (PSF_font_t*)font_data;
}

void psf_get_font_geometry(int* width, int* height, int* bpg) {
    *width = font->width;
    *height = font->height;
    *bpg = font->bytesperglyph;
}

char* psf_get_glyph(char chr) {
    return (char*)font + font->headersize + chr * font->bytesperglyph;
}
