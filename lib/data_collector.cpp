#include "data_collector.h"
#include "adc_config.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <new>

// ==================================================
// Constructor & Destructor
// ==================================================

DataCollector::DataCollector()
    : state(State::IDLE),
      raw_buffer(nullptr),
      filtered_buffer(nullptr),
      samples_collected(0),
      target_samples(0),
      buffer_size(0),
      last_capture_slot(0),
      filtering_enabled(false) {
}

DataCollector::~DataCollector() {
    free_buffers();
}

// ==================================================
// Public Methods
// ==================================================

bool DataCollector::start_collection(uint32_t duration_ms, bool enable_filtering) {
    if (is_collecting()) {
        printf("DataCollector: Already collecting\n");
        return false;
    }
    
    // Calculate target samples based on duration and sample rate
    target_samples = (ADCConfig::SAMPLE_RATE_HZ * duration_ms) / 1000;
    filtering_enabled = enable_filtering;
    
    printf("DataCollector: Starting collection\n");
    printf("DataCollector: Duration: %lu ms\n", static_cast<unsigned long>(duration_ms));
    printf("DataCollector: Target samples: %lu\n", static_cast<unsigned long>(target_samples));
    printf("DataCollector: Filtering: %s\n", filtering_enabled ? "enabled" : "disabled");
    
    state = State::PREPARING;
    
    // Allocate buffers
    if (!allocate_buffers(target_samples, filtering_enabled)) {
        printf("DataCollector: Failed to allocate buffers\n");
        state = State::ERROR;
        return false;
    }
    
    // Reset counters
    samples_collected = 0;
    
    // Ready to collect
    state = State::COLLECTING;
    printf("DataCollector: Collection started (buffer size: %lu samples)\n", 
           static_cast<unsigned long>(buffer_size));
    
    return true;
}

bool DataCollector::process_buffer(const uint16_t* raw_samples, const uint16_t* filtered_samples, uint32_t count) {
    if (state != State::COLLECTING) {
        return false;  // Not collecting
    }
    
    if (raw_samples == nullptr || count == 0) {
        return false;  // Invalid input
    }
    
    // Validate filtered samples if filtering is enabled
    if (filtering_enabled && filtered_samples == nullptr) {
        printf("DataCollector: WARNING - Filtering enabled but no filtered samples provided\n");
    }
    
    // Calculate how many samples to copy
    uint32_t remaining = target_samples - samples_collected;
    uint32_t to_copy = (count < remaining) ? count : remaining;
    
    // Copy raw samples to collection buffer
    memcpy(raw_buffer + samples_collected, raw_samples, to_copy * sizeof(uint16_t));
    
    // Copy filtered samples if available
    if (filtering_enabled && filtered_samples != nullptr) {
        memcpy(filtered_buffer + samples_collected, filtered_samples, to_copy * sizeof(uint16_t));
    }
    
    samples_collected += to_copy;
    
    // Check if collection complete
    if (samples_collected >= target_samples) {
        printf("DataCollector: Target reached (%lu samples)\n", 
               static_cast<unsigned long>(samples_collected));
        
        // Auto-finalize (write to flash)
        int slot = finalize_collection();
        if (slot >= 0) {
            printf("DataCollector: Saved to flash slot %d\n", slot);
        } else {
            printf("DataCollector: Failed to write to flash\n");
            state = State::ERROR;
        }
    }
    
    return true;
}

int DataCollector::finalize_collection() {
    if (state != State::COLLECTING) {
        printf("DataCollector: Cannot finalize - not collecting\n");
        return -1;
    }
    
    printf("DataCollector: Finalizing collection...\n");
    state = State::WRITING_FLASH;
    
    // Get current uptime as timestamp
    uint32_t timestamp = to_ms_since_boot(get_absolute_time());
    
    // Write to flash (with or without filtered data)
    int slot;
    if (filtering_enabled && filtered_buffer != nullptr) {
        slot = FlashStorage::write_capture_dual(raw_buffer, filtered_buffer, samples_collected, timestamp);
        printf("DataCollector: Wrote raw + filtered samples\n");
    } else {
        slot = FlashStorage::write_capture_dual(raw_buffer, nullptr, samples_collected, timestamp);
        printf("DataCollector: Wrote raw samples only\n");
    }
    
    if (slot >= 0) {
        printf("DataCollector: Successfully wrote %lu samples to slot %d\n", 
               static_cast<unsigned long>(samples_collected), slot);
        last_capture_slot = slot;
        state = State::COMPLETE;
        
        // Free buffers to reclaim RAM
        free_buffers();
        
        printf("DataCollector: Collection complete, state reset to IDLE\n");
    } else {
        printf("DataCollector: Flash write failed\n");
        state = State::ERROR;
    }
    
    printf("DataCollector: Returning to normal operation\n");
    return slot;
}

void DataCollector::cancel_collection() {
    if (!is_collecting()) {
        return;
    }
    
    printf("DataCollector: Collection cancelled\n");
    free_buffers();
    state = State::IDLE;
    samples_collected = 0;
    target_samples = 0;
    filtering_enabled = false;
}

uint8_t DataCollector::get_progress() const {
    if (target_samples == 0) {
        return 0;
    }
    
    uint32_t progress = (samples_collected * 100) / target_samples;
    return static_cast<uint8_t>(progress > 100 ? 100 : progress);
}

// ==================================================
// Private Methods
// ==================================================

bool DataCollector::allocate_buffers(uint32_t num_samples, bool enable_filtering) {
    // Free existing buffers if any
    free_buffers();
    
    // Calculate buffer size in bytes
    uint32_t bytes_per_buffer = num_samples * sizeof(uint16_t);
    uint32_t total_bytes = enable_filtering ? (bytes_per_buffer * 2) : bytes_per_buffer;
    
    printf("DataCollector: Allocating %lu bytes for %lu samples (%s)\n", 
           static_cast<unsigned long>(total_bytes), 
           static_cast<unsigned long>(num_samples),
           enable_filtering ? "raw + filtered" : "raw only");
    
    // Allocate raw buffer
    raw_buffer = new (std::nothrow) uint16_t[num_samples];
    if (raw_buffer == nullptr) {
        printf("DataCollector: Failed to allocate raw buffer (%lu bytes)\n", 
               static_cast<unsigned long>(bytes_per_buffer));
        return false;
    }
    memset(raw_buffer, 0, bytes_per_buffer);
    
    // Allocate filtered buffer if enabled
    if (enable_filtering) {
        filtered_buffer = new (std::nothrow) uint16_t[num_samples];
        if (filtered_buffer == nullptr) {
            printf("DataCollector: Failed to allocate filtered buffer (%lu bytes)\n", 
                   static_cast<unsigned long>(bytes_per_buffer));
            delete[] raw_buffer;
            raw_buffer = nullptr;
            return false;
        }
        memset(filtered_buffer, 0, bytes_per_buffer);
    }
    
    buffer_size = num_samples;
    
    printf("DataCollector: Buffers allocated successfully\n");
    return true;
}

void DataCollector::free_buffers() {
    if (raw_buffer != nullptr) {
        delete[] raw_buffer;
        raw_buffer = nullptr;
        printf("DataCollector: Raw buffer freed\n");
    }
    if (filtered_buffer != nullptr) {
        delete[] filtered_buffer;
        filtered_buffer = nullptr;
        printf("DataCollector: Filtered buffer freed\n");
    }
    buffer_size = 0;
}
