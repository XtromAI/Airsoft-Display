#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

#include <stdint.h>
#include <stdbool.h>
#include "flash_storage.h"

// ==================================================
// Data Collector Class
// Manages collection of ADC samples for analysis
// ==================================================

class DataCollector {
public:
    enum class State {
        IDLE,           // Not collecting
        PREPARING,      // Allocating buffer
        COLLECTING,     // Actively collecting samples
        WRITING_FLASH,  // Writing to flash
        COMPLETE,       // Ready for download
        ERROR           // Collection failed
    };
    
    DataCollector();
    ~DataCollector();
    
    // Start collection (specify duration in milliseconds)
    // enable_filtering: if true, also collect filtered samples
    // Returns true if collection started successfully
    bool start_collection(uint32_t duration_ms, bool enable_filtering = true);
    
    // Called by Core 1 when DMA buffer ready
    // raw_samples: raw ADC samples
    // filtered_samples: optional filtered samples (can be nullptr if not enabled)
    // Returns true if buffer was processed
    bool process_buffer(const uint16_t* raw_samples, const uint16_t* filtered_samples, uint32_t count);
    
    // Finalize collection (write to flash)
    // Returns flash slot number or -1 on error
    int finalize_collection();
    
    // Cancel ongoing collection
    void cancel_collection();
    
    // Get current state
    State get_state() const { return state; }
    
    // Get collection progress (0-100%)
    uint8_t get_progress() const;
    
    // Get statistics
    uint32_t get_samples_collected() const { return samples_collected; }
    uint32_t get_target_samples() const { return target_samples; }
    uint32_t get_last_capture_slot() const { return last_capture_slot; }
    
    // Check if currently collecting
    bool is_collecting() const { 
        return state == State::COLLECTING || state == State::PREPARING; 
    }
    
    // Check if collection complete
    bool is_complete() const { 
        return state == State::COMPLETE; 
    }
    
private:
    State state;
    uint16_t* raw_buffer;         // Raw sample buffer (dynamically allocated)
    uint16_t* filtered_buffer;    // Filtered sample buffer (dynamically allocated, optional)
    uint32_t samples_collected;
    uint32_t target_samples;
    uint32_t buffer_size;         // Allocated buffer size
    uint32_t last_capture_slot;   // Flash slot of last capture
    bool filtering_enabled;       // Whether to collect filtered samples
    
    // Allocate collection buffers
    bool allocate_buffers(uint32_t num_samples, bool enable_filtering);
    
    // Free collection buffers
    void free_buffers();
};

#endif // DATA_COLLECTOR_H
