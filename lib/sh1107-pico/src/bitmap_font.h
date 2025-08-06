#pragma once

struct BitmapFont {
    int width;
    int height;
    int glyph_count;
    int first_char;
    const unsigned char* data;
};
