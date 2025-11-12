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
      collection_buffer(nullptr),
      samples_collected(0),
      target_samples(0),
      buffer_size(0),
      last_capture_slot(0) {
}

DataCollector::~DataCollector() {
    free_buffer();
}

// ==================================================
// Public Methods
// ==================================================

bool DataCollector::start_collection(uint32_t duration_ms) {
    if (is_collecting()) {
        printf("DataCollector: Already collecting\n");
        return false;
    }
    
    // Calculate target samples based on duration and sample rate
    target_samples = (ADCConfig::SAMPLE_RATE_HZ * duration_ms) / 1000;
    
    printf("DataCollector: Starting collection\n");
    printf("DataCollector: Duration: %lu ms\n", static_cast<unsigned long>(duration_ms));
    printf("DataCollector: Target samples: %lu\n", static_cast<unsigned long>(target_samples));
    
    state = State::PREPARING;
    
    // Allocate buffer
    if (!allocate_buffer(target_samples)) {
        printf("DataCollector: Failed to allocate buffer\n");
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

bool DataCollector::process_buffer(const uint16_t* samples, uint32_t count) {
    if (state != State::COLLECTING) {
        return false;  // Not collecting
    }
    
    if (samples == nullptr || count == 0) {
        return false;  // Invalid input
    }
    
    // Calculate how many samples to copy
    uint32_t remaining = target_samples - samples_collected;
    uint32_t to_copy = (count < remaining) ? count : remaining;
    
    // Copy samples to collection buffer
    memcpy(collection_buffer + samples_collected, samples, to_copy * sizeof(uint16_t));
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
    
    // Write to flash
    int slot = FlashStorage::write_capture(collection_buffer, samples_collected, timestamp);
    
    if (slot >= 0) {
        printf("DataCollector: Successfully wrote %lu samples to slot %d\n", 
               static_cast<unsigned long>(samples_collected), slot);
        last_capture_slot = slot;
        state = State::COMPLETE;
        
        // Free buffer to reclaim RAM
        free_buffer();
        
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
    free_buffer();
    state = State::IDLE;
    samples_collected = 0;
    target_samples = 0;
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

bool DataCollector::allocate_buffer(uint32_t num_samples) {
    // Free existing buffer if any
    free_buffer();
    
    // Calculate buffer size in bytes
    uint32_t bytes_needed = num_samples * sizeof(uint16_t);
    
    printf("DataCollector: Allocating %lu bytes for %lu samples\n", 
           static_cast<unsigned long>(bytes_needed), 
           static_cast<unsigned long>(num_samples));
    
    // Allocate buffer
    collection_buffer = new (std::nothrow) uint16_t[num_samples];
    
    if (collection_buffer == nullptr) {
        printf("DataCollector: Failed to allocate %lu bytes\n", 
               static_cast<unsigned long>(bytes_needed));
        return false;
    }
    
    buffer_size = num_samples;
    
    // Zero the buffer
    memset(collection_buffer, 0, bytes_needed);
    
    printf("DataCollector: Buffer allocated successfully\n");
    return true;
}

void DataCollector::free_buffer() {
    if (collection_buffer != nullptr) {
        delete[] collection_buffer;
        collection_buffer = nullptr;
        buffer_size = 0;
        printf("DataCollector: Buffer freed\n");
    }
}
