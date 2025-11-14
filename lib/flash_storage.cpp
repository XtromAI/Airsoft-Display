#include "flash_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <string.h>
#include <stdio.h>

// CRC32 implementation for data verification
static uint32_t crc32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

namespace FlashStorage {

bool init() {
    // Verify flash is accessible and partition is within bounds
    // The RP2040 has 2MB flash, our partition is at 1MB-2MB
    printf("FlashStorage: Initializing flash storage\n");
    printf("FlashStorage: Partition offset: 0x%08lx\n", static_cast<unsigned long>(DATA_FLASH_OFFSET));
    printf("FlashStorage: Partition size: %lu bytes\n", static_cast<unsigned long>(DATA_FLASH_SIZE));
    printf("FlashStorage: Max captures: %lu\n", static_cast<unsigned long>(MAX_CAPTURES));
    
    // Read first few bytes to ensure flash is accessible
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + DATA_FLASH_OFFSET);
    volatile uint32_t test = *(uint32_t*)flash_ptr;
    (void)test;  // Suppress unused variable warning
    
    printf("FlashStorage: Initialized successfully\n");
    return true;
}

int write_capture(const uint16_t* samples, uint32_t count, uint32_t timestamp) {
    // Legacy function - calls write_capture_dual with no filtered data
    return write_capture_dual(samples, nullptr, count, timestamp);
}

int write_capture_dual(const uint16_t* raw_samples, const uint16_t* filtered_samples, 
                       uint32_t count, uint32_t timestamp) {
    if (raw_samples == nullptr || count == 0) {
        printf("FlashStorage: Invalid parameters\n");
        return -1;
    }
    
    // Calculate data size
    uint32_t raw_data_size = count * sizeof(uint16_t);
    uint32_t filtered_data_size = (filtered_samples != nullptr) ? (count * sizeof(uint16_t)) : 0;
    uint32_t total_data_size = raw_data_size + filtered_data_size;
    uint32_t total_size = sizeof(CaptureHeader) + total_data_size;
    
    if (total_size > CAPTURE_SLOT_SIZE) {
        printf("FlashStorage: Data too large (%lu bytes > %lu bytes)\n", 
               static_cast<unsigned long>(total_size), 
               static_cast<unsigned long>(CAPTURE_SLOT_SIZE));
        return -1;
    }
    
    // Find first empty slot or use next sequential slot
    int slot = get_capture_count();
    if (slot >= static_cast<int>(MAX_CAPTURES)) {
        printf("FlashStorage: No free slots (max %lu captures)\n", 
               static_cast<unsigned long>(MAX_CAPTURES));
        return -1;
    }
    
    if (filtered_samples != nullptr) {
        printf("FlashStorage: Writing %lu raw + filtered samples to slot %d...\n", 
               static_cast<unsigned long>(count), slot);
    } else {
        printf("FlashStorage: Writing %lu raw samples to slot %d...\n", 
               static_cast<unsigned long>(count), slot);
    }
    
    // Calculate flash offset for this slot
    uint32_t slot_offset = DATA_FLASH_OFFSET + (slot * CAPTURE_SLOT_SIZE);
    
    // Create header
    CaptureHeader header;
    header.magic = 0x41444353;  // "ADCS"
    header.version = (filtered_samples != nullptr) ? 2 : 1;
    header.sample_rate = 5000;
    header.sample_count = count;
    header.timestamp = timestamp;
    header.checksum = crc32((const uint8_t*)raw_samples, raw_data_size);
    header.has_filtered = (filtered_samples != nullptr) ? 1 : 0;
    header.checksum_filt = (filtered_samples != nullptr) ? 
                           crc32((const uint8_t*)filtered_samples, filtered_data_size) : 0;
    
    // Prepare write buffer (header + raw data + filtered data)
    // Must be aligned to 256-byte boundary for flash programming
    uint8_t* write_buffer = new uint8_t[total_size];
    if (write_buffer == nullptr) {
        printf("FlashStorage: Failed to allocate write buffer\n");
        return -1;
    }
    
    memcpy(write_buffer, &header, sizeof(CaptureHeader));
    memcpy(write_buffer + sizeof(CaptureHeader), raw_samples, raw_data_size);
    if (filtered_samples != nullptr) {
        memcpy(write_buffer + sizeof(CaptureHeader) + raw_data_size, 
               filtered_samples, filtered_data_size);
    }
    
    // Calculate erase and write sizes
    uint32_t erase_size = ((total_size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    printf("FlashStorage: Erasing %lu bytes at offset 0x%lx...\n", 
           static_cast<unsigned long>(erase_size),
           static_cast<unsigned long>(slot_offset));
    
    // CRITICAL: Disable watchdog before flash operations
    // Flash erase can take >2 seconds which exceeds watchdog timeout
    bool watchdog_was_enabled = watchdog_hw->ctrl & WATCHDOG_CTRL_ENABLE_BITS;
    if (watchdog_was_enabled) {
        hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    }

    // Pause the other core to avoid flash contention
    multicore_lockout_start_blocking();

    // Both cores will freeze during flash operations
    // This is unavoidable - code cannot execute from flash while it's being modified
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(slot_offset, erase_size);
    restore_interrupts(ints);

    printf("FlashStorage: Erase complete\n");
    
    // Write data (256 bytes at a time)
    // Flash programming requires 256-byte alignment
    uint32_t bytes_written = 0;
    uint8_t padded[FLASH_PAGE_SIZE];  // Reusable padding buffer
    
    while (bytes_written < total_size) {
        uint32_t chunk_size = (total_size - bytes_written) > FLASH_PAGE_SIZE ? 
                               FLASH_PAGE_SIZE : (total_size - bytes_written);
        
        const uint8_t* data_to_write;
        
        // Pad to 256 bytes if needed
        if (chunk_size < FLASH_PAGE_SIZE) {
            memset(padded, 0xFF, FLASH_PAGE_SIZE);  // Flash erased state
            memcpy(padded, write_buffer + bytes_written, chunk_size);
            data_to_write = padded;
        } else {
            data_to_write = write_buffer + bytes_written;
        }
        
        // Disable interrupts and write this page
        ints = save_and_disable_interrupts();
        flash_range_program(slot_offset + bytes_written, data_to_write, FLASH_PAGE_SIZE);
        restore_interrupts(ints);
        
        bytes_written += FLASH_PAGE_SIZE;
    }
    
    delete[] write_buffer;
    
    // Release the other core now that flash operations are done
    multicore_lockout_end_blocking();

    // Re-enable watchdog if it was enabled before
    if (watchdog_was_enabled) {
        watchdog_enable(2000, 1);  // Re-enable with 2s timeout
    }
    
    printf("FlashStorage: Write complete, slot %d\n", slot);
    
    // Verify write
    if (!verify_capture(slot)) {
        printf("FlashStorage: WARNING - Verification failed for slot %d\n", slot);
        return -1;
    }
    
    return slot;
}

bool read_capture(int slot, CaptureHeader* header, const uint16_t** samples) {
    if (slot < 0 || slot >= static_cast<int>(MAX_CAPTURES)) {
        printf("FlashStorage: Invalid slot %d\n", slot);
        return false;
    }
    
    // Calculate flash address for this slot
    uint32_t slot_offset = DATA_FLASH_OFFSET + (slot * CAPTURE_SLOT_SIZE);
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + slot_offset);
    
    // Read header
    memcpy(header, flash_ptr, sizeof(CaptureHeader));
    
    // Verify magic number
    if (header->magic != 0x41444353) {
        // Empty slot
        return false;
    }
    
    // Set samples pointer (memory-mapped flash) - always points to raw data
    *samples = (const uint16_t*)(flash_ptr + sizeof(CaptureHeader));
    
    return true;
}

bool read_capture_dual(int slot, CaptureHeader* header, 
                       const uint16_t** raw_samples, const uint16_t** filtered_samples) {
    if (slot < 0 || slot >= static_cast<int>(MAX_CAPTURES)) {
        printf("FlashStorage: Invalid slot %d\n", slot);
        return false;
    }
    
    // Calculate flash address for this slot
    uint32_t slot_offset = DATA_FLASH_OFFSET + (slot * CAPTURE_SLOT_SIZE);
    const uint8_t* flash_ptr = (const uint8_t*)(XIP_BASE + slot_offset);
    
    // Read header
    memcpy(header, flash_ptr, sizeof(CaptureHeader));
    
    // Verify magic number
    if (header->magic != 0x41444353) {
        // Empty slot
        return false;
    }
    
    // Set raw samples pointer (memory-mapped flash)
    *raw_samples = (const uint16_t*)(flash_ptr + sizeof(CaptureHeader));
    
    // Set filtered samples pointer if present
    if (header->version >= 2 && header->has_filtered == 1) {
        uint32_t raw_data_size = header->sample_count * sizeof(uint16_t);
        *filtered_samples = (const uint16_t*)(flash_ptr + sizeof(CaptureHeader) + raw_data_size);
    } else {
        *filtered_samples = nullptr;
    }
    
    return true;
}

int get_capture_count() {
    int count = 0;
    
    for (int slot = 0; slot < static_cast<int>(MAX_CAPTURES); slot++) {
        uint32_t slot_offset = DATA_FLASH_OFFSET + (slot * CAPTURE_SLOT_SIZE);
        const uint32_t* magic_ptr = (const uint32_t*)(XIP_BASE + slot_offset);
        
        if (*magic_ptr == 0x41444353) {
            count++;
        } else {
            // First empty slot - assume sequential writes
            break;
        }
    }
    
    return count;
}

bool delete_capture(int slot) {
    if (slot < 0 || slot >= static_cast<int>(MAX_CAPTURES)) {
        return false;
    }
    
    uint32_t slot_offset = DATA_FLASH_OFFSET + (slot * CAPTURE_SLOT_SIZE);
    
    printf("FlashStorage: Deleting slot %d...\n", slot);
    
    bool watchdog_was_enabled = watchdog_hw->ctrl & WATCHDOG_CTRL_ENABLE_BITS;
    if (watchdog_was_enabled) {
        hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    }

    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(slot_offset, CAPTURE_SLOT_SIZE);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();

    if (watchdog_was_enabled) {
        watchdog_enable(2000, 1);
    }
    
    printf("FlashStorage: Slot %d deleted\n", slot);
    return true;
}

bool delete_all_captures() {
    printf("FlashStorage: Deleting all captures...\n");
    
    bool watchdog_was_enabled = watchdog_hw->ctrl & WATCHDOG_CTRL_ENABLE_BITS;
    if (watchdog_was_enabled) {
        hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    }

    multicore_lockout_start_blocking();
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(DATA_FLASH_OFFSET, DATA_FLASH_SIZE);
    restore_interrupts(ints);
    multicore_lockout_end_blocking();

    if (watchdog_was_enabled) {
        watchdog_enable(2000, 1);
    }
    
    printf("FlashStorage: All captures deleted\n");
    return true;
}

bool verify_capture(int slot) {
    CaptureHeader header;
    const uint16_t* samples;
    
    if (!read_capture(slot, &header, &samples)) {
        return false;
    }
    
    // Verify magic number
    if (header.magic != 0x41444353) {
        printf("FlashStorage: Invalid magic number in slot %d\n", slot);
        return false;
    }
    
    // Verify checksum
    uint32_t data_size = header.sample_count * sizeof(uint16_t);
    uint32_t calculated_crc = crc32((const uint8_t*)samples, data_size);
    
    if (calculated_crc != header.checksum) {
        printf("FlashStorage: Checksum mismatch in slot %d (expected 0x%08lx, got 0x%08lx)\n", 
               slot, static_cast<unsigned long>(header.checksum), 
               static_cast<unsigned long>(calculated_crc));
        return false;
    }
    
    return true;
}

bool get_stats(FlashStats* stats) {
    if (stats == nullptr) {
        return false;
    }
    
    stats->total_size = DATA_FLASH_SIZE;
    stats->capture_count = get_capture_count();
    stats->used_size = stats->capture_count * CAPTURE_SLOT_SIZE;
    stats->free_size = DATA_FLASH_SIZE - stats->used_size;
    
    return true;
}

} // namespace FlashStorage
