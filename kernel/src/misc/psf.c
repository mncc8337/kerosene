#include "misc/psf.h"
#include "string.h"

#include "stddef.h"

#include "access/font.h"

PSF_font_t* font;

void psf_init(char* font_data) {
    if(font_data == NULL) {
        // load default font
        font_data = font_h_data;
    }

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
