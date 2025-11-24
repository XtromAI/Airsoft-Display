# 2025-11-20 Filtered Data Collection Session

**Status:** Complete  
**Participants:** Copilot + user pairing session  
**Scope:** Resolve build issues, enable longer dual-buffer captures, run end-to-end filtered data acquisition, and record verification results.

## Summary
- Fixed `std::nothrow` build failure by adding `<new>` include to `src/main.cpp`.
- Increased flash capture capacity by reducing slot count to four and deriving slot size from the 1 MB partition, enabling ~64k raw+filtered samples.
- Reworked flash write path to stream header/raw/filtered buffers directly into flash pages (no large heap allocation) to avoid panics on larger captures.
- Rebuilt, reflashed, and successfully collected a 10-second filtered capture, then downloaded and analyzed it with project tooling.

## Detailed Timeline
1. **Compilation & Build Fixes**
   - Initial `ninja` build failed: `std::nothrow` missing from `src/main.cpp`.
   - Added `#include <new>` and verified successful rebuild.
2. **Flash Storage Capacity Upgrade**
   - Encountered `FlashStorage: Data too large` when storing 50k raw+filtered samples in 128 KB slot.
   - Adjusted constants in `lib/flash_storage.h` to `MAX_CAPTURES = 4` and `CAPTURE_SLOT_SIZE = DATA_FLASH_SIZE / MAX_CAPTURES` (256 KB each).
   - Rebuilt firmware.
3. **Flash Write Stability**
   - First attempt with larger slots triggered panic due to allocating a 200 KB staging buffer.
   - Modified `FlashStorage::write_capture_dual` to stream data into 256-byte pages without heap allocation.
   - Confirmed clean build post-change.
4. **Data Collection & Verification**
   - Flashed updated firmware and performed `COLLECT 10` with filtering enabled; Pico reported save to slot 0.
   - Downloaded capture via `tools/download_data.py COM7 download 0 tools/data/capture_slot0.bin`.
   - Parsed capture using `tools/parse_capture.py tools/data/capture_slot0.bin`; generated CSV and PNG in `tools/data/`.
   - Observed filtered standard deviation drop vs. raw data (9.9 → 5.0 ADC counts).

## Artifacts & Outputs
- Firmware changes in `src/main.cpp`, `lib/flash_storage.h`, `lib/flash_storage.cpp`.
- Captured binary/CSV/plot:
  - `tools/data/capture_slot0.bin`
  - `tools/data/capture_slot0.csv`
  - `tools/data/capture_slot0.png`
- Terminal logs show successful download and parse (see session transcript).

## Testing & Validation
- Successful `ninja -C build` after each code change.
- Hardware-in-the-loop validation: Pico flashed, 10-second filtered capture stored without overflow or panic.
- Python tooling (`download_data.py`, `parse_capture.py`) executed directly from existing venv; confirmed checksums and statistics match expectations.

## Follow-Up Recommendations
1. Add guard in `DataCollector` to warn when requested duration exceeds slot capacity before collection starts.
2. Consider automated script to run compile → flash → capture → parse to streamline regression testing.
3. Update user-facing documentation (README/RECONNECTION guide) with new slot count and capture limits.
