#ifndef DMA_ADC_SAMPLER_H
#define DMA_ADC_SAMPLER_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "adc_config.h"
// ==================================================
// DMA ADC Sampler Class
// Implements 5 kHz sampling with double-buffering
// ==================================================

class DMAADCSampler {
public:
    DMAADCSampler();
    ~DMAADCSampler();
    
    // Initialize DMA, ADC, and timer for 5 kHz sampling
    bool init();
    
    // Start DMA sampling
    void start();
    
    // Stop DMA sampling
    void stop();
    
    // Check if a buffer is ready for processing
    bool is_buffer_ready();
    
    // Get pointer to ready buffer and its size
    // Returns nullptr if no buffer ready
    const uint16_t* get_ready_buffer(uint32_t* size);
    
    // Mark current buffer as processed (enables next swap)
    void release_buffer();
    
    // Get statistics
    uint32_t get_buffer_count() const { return buffer_count; }
    uint32_t get_overflow_count() const { return overflow_count; }
    
private:
    // DMA interrupt handler (static for C callback)
    static void dma_irq_handler();
    
    // Instance pointer for interrupt handler
    static DMAADCSampler* instance;
    
    // Timer callback for ADC triggering
    static bool timer_callback(repeating_timer_t *rt);
    
    // Double buffers (ping-pong)
    static constexpr uint32_t BUFFER_SIZE = ADCConfig::BUFFER_SIZE;
    uint16_t buffer_a[BUFFER_SIZE];
    uint16_t buffer_b[BUFFER_SIZE];
    
    // DMA configuration
    int dma_channel;
    dma_channel_config dma_config;
    
    // Buffer management
    volatile bool buffer_a_ready;  // true when buffer A is full and ready to process
    volatile bool buffer_b_ready;  // true when buffer B is full and ready to process
    volatile bool using_buffer_a;  // true when DMA is writing to buffer A
    volatile uint32_t buffer_count;  // Number of buffers filled
    volatile uint32_t overflow_count;  // Number of buffer overflows (data loss)
    
    // Processing state
    volatile bool buffer_locked;  // true when application is processing a buffer
    
    // Timer for ADC triggering
    repeating_timer_t adc_timer;
    bool timer_running;
    
    // Running state
    bool initialized;
    bool running;
};

#endif // DMA_ADC_SAMPLER_H
