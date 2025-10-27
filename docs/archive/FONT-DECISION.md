# Font Implementation Decision

**Date:** August 5-6, 2025  
**Decision:** Custom row-major bitmap font format  
**Alternative Considered:** U8g2 compressed fonts

---

## What Happened

### Timeline
1. **Aug 5:** Attempted to use U8g2 font format (u8x8_font_pxplusibmcga_u)
2. **Aug 5:** Tried converting U8g2 font strings to byte arrays
3. **Aug 6:** Switched to custom row-major format
4. **Aug 6:** Successfully implemented 8x8 font (96 ASCII chars)
5. **Aug 6-7:** Added 16x16 font for digits, font management system

### Why Not U8g2 Fonts?

**U8g2 Font Complexity:**
- Compressed encoding (not raw pixel data)
- Variable-width glyphs with metadata
- RLE compression requiring decoder
- ~500+ lines of font rendering code needed
- Unclear benefit for this specific use case

**Decision:**
Instead of fighting with U8g2's complex format, created a simple row-major bitmap format:
- 8 bytes per glyph for 8x8 fonts
- 32 bytes per glyph for 16x16 fonts (2 bytes per row, little-endian)
- Easy to understand and debug
- Works perfectly for fixed-width display needs

## Trade-offs

### Benefits of Custom Format ✅
- Simple and straightforward
- Easy to debug visually (binary comments)
- No external dependencies
- Exactly what's needed, nothing more
- Fast rendering (no decompression)

### Drawbacks ⚠️
- Manual glyph creation is tedious
- No font tooling/converters
- Limited to hand-crafted characters
- Non-standard format
- Takes time to add new glyphs

## Current State (October 2025)

**Fonts Available:**
- `font8x8`: 96 ASCII characters (space through ~)
- `font16x16`: Digits 0-9 only

**Limitation:** 
The 16x16 font only has numbers. To display text like "SHOTS" or "BATTERY" in large font, would need to hand-craft 26+ more glyphs.

## Future Improvements

See `docs/FONT-ANALYSIS.md` for detailed recommendations on:
1. Creating a font converter tool (BDF → custom format)
2. Using web-based font generators
3. Alternative font formats (Adafruit GFX)

**Bottom Line:** Custom format works well but needs tooling to be maintainable long-term.

---

**Related Files:**
- Current implementation: `lib/sh1107-pico/src/font*.cpp/h`
- Analysis: `docs/FONT-ANALYSIS.md`
- U8g2 library (unused): `lib/u8g2/`
