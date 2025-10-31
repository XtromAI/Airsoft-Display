# Font Implementation Analysis & Recommendations

**Date:** October 26, 2025  
**Current State:** Custom bitmap fonts (8x8, 16x16)  
**Issue:** Non-standard format, difficult to manage and extend

---

## Current Font Implementation

### What You Have Now

**Files:**
- `lib/sh1107-pico/src/bitmap_font.h` - Simple font structure
- `lib/sh1107-pico/src/font8x8.cpp/h` - 8x8 font (96 ASCII characters, 1075 lines)
- `lib/sh1107-pico/src/font16x16.cpp/h` - 16x16 font (only digits 0-9, 205 lines)

**Font Structure:**
```cpp
struct BitmapFont {
    int width;
    int height;
    int glyph_count;
    int first_char;
    const unsigned char* data;
};
```

**Font Data Format:**
- **8x8 font:** Row-major, 8 bytes per glyph (1 byte per row)
- **16x16 font:** Row-major, 32 bytes per glyph (2 bytes per row, little-endian)
- **Handcrafted:** All glyphs manually created with binary comments

**Example (from font8x8.cpp):**
```cpp
// ! (0x21)
{
    0x18,  // Row 0: ...##...
    0x3C,  // Row 1: ..####..
    0x3C,  // Row 2: ..####..
    0x18,  // Row 3: ...##...
    0x18,  // Row 4: ...##...
    0x00,  // Row 5: ........
    0x18,  // Row 6: ...##...
    0x00   // Row 7: ........
}
```

### The History (from git commits)

**Timeline:**
1. **Aug 5, 2025:** "working on external fonts" - Tried to use U8g2 fonts directly
2. **Aug 5:** "working on converting font strings to byte array" - Conversion attempts
3. **Aug 6:** "font work in row major format" - Changed approach
4. **Aug 6:** "implemented 8 pixel font" - Success with custom format
5. **Aug 6-7:** Refined with font management, added 16x16 for digits

**What Happened:**
- Attempted to use U8g2's compressed font format (u8x8_font_pxplusibmcga_u)
- U8g2 fonts are complex: compressed, with metadata, multiple encoding modes
- Conversion failed, so you created your own simple row-major format
- Trade-off: Simple to understand, but labor-intensive to create

---

## Problems with Current Approach

### 1. **No Tooling - Manual Creation**
- Each character must be hand-crafted
- Binary comments like `// Row 0: ...##...` are manually typed
- Extremely error-prone and tedious
- Makes adding new glyphs painful

### 2. **Limited Character Set**
- 8x8 font: Only 96 ASCII characters (space through ~)
- 16x16 font: Only digits 0-9 (no letters!)
- No lowercase, no symbols, no extended ASCII

### 3. **Non-Standard Format**
- Can't import existing fonts easily
- Can't use font editors or converters
- Tied to your custom implementation

### 4. **Maintenance Burden**
- Want a new character? Hand-craft it pixel by pixel
- Want a different size? Start from scratch
- Want bold/italic? Create entirely new font file

### 5. **Storage Not Optimized**
- Row-major format is straightforward but not compressed
- U8g2 fonts are 30-50% smaller due to compression
- For 96 chars x 8 bytes = 768 bytes (your 8x8 font)

---

## Why U8g2 Fonts Are Hard

### U8g2 Font Format Complexity

**Example U8g2 font data:**
```cpp
const uint8_t u8g2_font_u8glib_4_tr[667] =
  "`\0\2\2\3\3\1\3\4\5\6\0\377\4\377\5\377\0\320\1\256\2~ \4@*!\5a**"
  "\42\6\323\63I\5#\12\355y\325P\325P\25\0$\12\365\271y\214\214\306\310\21%\6d\66\261\7"
  // ... hundreds more bytes of compressed data
```

**Why it's complex:**
1. **Compressed encoding** - Not just raw pixel data
2. **Variable-width glyphs** - Each char can be different width
3. **Metadata per glyph** - Width, height, X/Y offset encoded
4. **RLE compression** - Run-length encoding for efficiency
5. **Requires decoder** - Can't just read bytes directly

**To use U8g2 fonts, you'd need:**
- Implement their decompression algorithm
- Handle variable-width rendering
- Deal with glyph metrics (ascent, descent, kerning)
- ~500+ lines of font rendering code

**Why you didn't:** Too much complexity for uncertain benefit when custom fonts worked.

---

## Recommended Solutions

### Option 1: Keep Custom Format, Add Tooling ⭐ **EASIEST**

**Idea:** Create a Python script to convert standard bitmap fonts to your format.

**Workflow:**
1. Find/create fonts in BDF, PCF, or bitmap image format
2. Run conversion script: `python tools/font_converter.py myfont.bdf > font_output.cpp`
3. Drop generated .cpp file into your project

**Benefits:**
- Keep your simple format that works
- Eliminates manual pixel-pushing
- Can use existing font libraries (GNU FreeFont, etc.)
- Quick to implement

**Implementation:**
```python
# tools/font_converter.py
# Reads BDF/TTF/PNG fonts, outputs your C++ array format
# ~100-200 lines of Python
```

**Effort:** 2-4 hours to write converter script

---

### Option 2: Use Adafruit GFX Font Format 🔄 **BALANCED**

**What is it:**
- Simpler than U8g2, more standard than yours
- Used by Adafruit GFX library (which you already researched)
- Variable-width characters
- Well-documented format

**Adafruit GFX Font Structure:**
```cpp
typedef struct {
  uint16_t bitmapOffset;  // Pointer into GFXfont->bitmap
  uint8_t  width;         // Bitmap dimensions in pixels
  uint8_t  height;
  uint8_t  xAdvance;      // Distance to advance cursor (x axis)
  int8_t   xOffset;       // X dist from cursor pos to UL corner
  int8_t   yOffset;       // Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
  uint8_t  *bitmap;      // Glyph bitmaps, concatenated
  GFXglyph *glyph;       // Glyph array
  uint8_t   first;       // ASCII extents (first char)
  uint8_t   last;        // ASCII extents (last char)
  uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;
```

**Benefits:**
- Hundreds of pre-converted fonts available
- Tools exist (fontconvert utility from Adafruit)
- Better proportional spacing
- Industry standard for embedded systems

**Drawbacks:**
- More complex than your current format
- Requires more rendering code
- Variable-width adds complexity

**Effort:** 1-2 days to implement renderer + convert fonts

---

### Option 3: Simplified U8g2 Integration 🔧 **MOST POWERFUL**

**Idea:** Use U8g2 fonts BUT only implement a subset of features.

**What you'd do:**
1. Take U8g2 font format (you have the library already!)
2. Implement ONLY fixed-width font support (simpler decoder)
3. Skip compression, use uncompressed U8g2 fonts only
4. Use their font data, your rendering code

**Benefits:**
- Access to 100+ ready-made fonts
- Proven font quality
- Future-proof (U8g2 actively maintained)
- Can upgrade to full U8g2 later if needed

**Drawbacks:**
- Most complex to implement
- May run into same issues you had before
- Overkill if you only need a few fonts

**Effort:** 3-5 days for minimal U8g2 font support

---

### Option 4: Web-Based Font Generator 🌐 **QUICK & DIRTY**

**Idea:** Use online tools to generate font arrays manually when needed.

**Tools:**
- https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
- http://oleddisplay.squix.ch/
- https://rop.nl/truetype2gfx/

**Workflow:**
1. Upload TTF font to web tool
2. Select characters you want
3. Copy generated C code
4. Adapt format to match yours

**Benefits:**
- Zero coding required
- Visual preview of output
- Works right now

**Drawbacks:**
- Manual process each time
- Limited customization
- Still need to adapt format

**Effort:** 30 minutes per font

---

## My Recommendation: **Option 1 + Option 4**

### Why This Combo?

**Short term (today):** Use web generators when you need a new font
- Quick results
- No development time
- Good enough for current needs

**Medium term (next week):** Write font converter script
- Automate the tedious part
- Keep your simple format
- Future-proof your workflow

### Implementation Plan

**Phase 1: Immediate (30 mins)**
1. Use https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
2. Generate any missing fonts you need (16x16 full alphabet?)
3. Quick-adapt output to your format

**Phase 2: Better Solution (2-4 hours)**
```python
# tools/convert_font.py
import sys
from PIL import Image, ImageDraw, ImageFont

def convert_bdf_to_custom_format(bdf_file, output_file):
    """
    Reads BDF font, outputs your row-major C++ array format
    """
    # Parse BDF or use PIL to render
    # Generate C++ code matching your format
    # Include binary comments automatically
    pass

# Usage: python tools/convert_font.py input.bdf output.cpp --width 8 --height 8
```

**Phase 3: Document It**
- Add `docs/FONT-WORKFLOW.md` explaining how to add new fonts
- Never hand-craft a glyph again!

---

## What About the 16x16 Font?

### Current Problem
You only have digits 0-9 in 16x16. If you want to display text (like "SHOTS" or "BATTERY"), you need the full alphabet.

### Quick Fix Options:

**A) Generate full 16x16 font via web tool**
- Use font customiser with a nice chunky font
- Generate A-Z, a-z, 0-9, symbols
- ~10 minutes of work

**B) Only use 8x8 for text, 16x16 for numbers**
- Good enough for most displays
- Numbers big, text small
- Already works!

**C) Mix fonts**
- 16x16 numbers for shot count
- 8x8 for labels
- Common pattern in embedded displays

---

## Bottom Line

**Your current fonts work fine** - they do the job. The problem isn't quality, it's **maintainability and extensibility**.

### Do This Now:
1. ✅ Keep current fonts (they work!)
2. 📝 Document in `docs/archive/FONT-DECISION.md` why you chose this format
3. 🛠️ When you need a new font, use web generator first
4. 🔮 Eventually: Write converter script to automate

### Don't Do This:
- ❌ Rewrite everything to use U8g2 (too much work for uncertain benefit)
- ❌ Hand-craft more glyphs (life's too short)
- ❌ Feel bad about custom format (it's simpler and works!)

---

## Files to Create

I can help you create these if you want:

1. **`tools/convert_font.py`** - BDF/TTF to your format converter
2. **`docs/FONT-WORKFLOW.md`** - How to add/modify fonts
3. **`docs/archive/FONT-DECISION.md`** - Why you chose this format (like the driver decision)

Want me to start with any of these?
