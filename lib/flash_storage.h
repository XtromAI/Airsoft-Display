#ifndef FLASH_STORAGE_H
#define FLASH_STORAGE_H

#include <stdint.h>
#include <stdbool.h>

// ==================================================
// Flash Storage Module
// Provides simple flash read/write for data logging
// ==================================================

namespace FlashStorage {
    // Flash partition for data storage
    // Using last 1MB of flash (Pico has 2MB total)
    constexpr uint32_t DATA_FLASH_OFFSET = (1 * 1024 * 1024);  // 1MB offset from start
    constexpr uint32_t DATA_FLASH_SIZE = (1 * 1024 * 1024);    // 1MB total size
    constexpr uint32_t MAX_CAPTURES = 10;                       // Up to 10 captures
    constexpr uint32_t CAPTURE_SLOT_SIZE = (128 * 1024);       // 128KB per slot (room for header + 50k samples)
    
    // File header structure (32 bytes)
    struct CaptureHeader {
        uint32_t magic;          // 0x41444353 ("ADCS")
        uint32_t version;        // File format version (2 = raw + filtered)
        uint32_t sample_rate;    // Samples per second (5000)
        uint32_t sample_count;   // Number of samples
        uint32_t timestamp;      // Unix timestamp (or uptime ms)
        uint32_t checksum;       // CRC32 of raw sample data
        uint32_t has_filtered;   // 1 if filtered data present, 0 if not
        uint32_t checksum_filt;  // CRC32 of filtered sample data
    };
    
    // Initialize flash storage (verify partition accessible)
    bool init();
    
    // Write capture to flash (raw samples only - legacy)
    // Returns capture slot number (0-9) or -1 on error
    int write_capture(const uint16_t* samples, uint32_t count, uint32_t timestamp = 0);
    
    // Write capture to flash with both raw and filtered samples
    // filtered_samples can be nullptr if not available
    // Returns capture slot number (0-9) or -1 on error
    int write_capture_dual(const uint16_t* raw_samples, const uint16_t* filtered_samples, 
                           uint32_t count, uint32_t timestamp = 0);
    
    // Read capture from flash
    // Returns true if successful, fills header and sets samples pointer
    // For version 2 files, samples points to raw data, use read_capture_dual for filtered
    bool read_capture(int slot, CaptureHeader* header, const uint16_t** samples);
    
    // Read capture with both raw and filtered data (version 2)
    // filtered_samples will be nullptr if not present in file
    // Returns true if successful
    bool read_capture_dual(int slot, CaptureHeader* header, 
                          const uint16_t** raw_samples, const uint16_t** filtered_samples);
    
    // Get number of stored captures
    int get_capture_count();
    
    // Delete a capture (erase slot)
    bool delete_capture(int slot);
    
    // Delete all captures
    bool delete_all_captures();
    
    // Verify capture integrity (checksum)
    bool verify_capture(int slot);
    
    // Get flash usage statistics
    struct FlashStats {
        uint32_t total_size;
        uint32_t used_size;
        uint32_t free_size;
        int capture_count;
    };
    bool get_stats(FlashStats* stats);
}

#endif // FLASH_STORAGE_H
