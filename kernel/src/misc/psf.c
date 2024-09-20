#include "misc/psf.h"

// default font
#include "access/font.h"

PSF_font_t* font = (PSF_font_t*)font_h_data;

bool psf_load(char* font_data) {
    PSF_font_t* temp = (PSF_font_t*)font_data;

    if(temp->magic != PSF_FONT_MAGIC)
        return true;

    font = temp;
    return false;
}

void psf_get_font_geometry(int* width, int* height, int* bpg) {
    *width = font->width;
    *height = font->height;
    *bpg = font->bytesperglyph;
}

char* psf_get_glyph(char chr) {
    return (char*)font + font->headersize + chr * font->bytesperglyph;
}
