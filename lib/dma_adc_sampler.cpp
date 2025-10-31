#include "dma_adc_sampler.h"
#include "hardware/irq.h"
#include <stdio.h>
#include <string.h>

// ==================================================
// Static member initialization
// ==================================================

DMAADCSampler* DMAADCSampler::instance = nullptr;

// ==================================================
// Constructor & Destructor
// ==================================================

DMAADCSampler::DMAADCSampler()
    : dma_channel(-1),
      buffer_a_ready(false),
      buffer_b_ready(false),
      using_buffer_a(true),
      buffer_count(0),
      overflow_count(0),
      buffer_locked(false),
      locked_buffer_is_a(false),
      timer_running(false),
      initialized(false),
      running(false) {
    
    // Zero buffers
    memset(buffer_a, 0, sizeof(buffer_a));
    memset(buffer_b, 0, sizeof(buffer_b));
    
    // Set singleton instance for interrupt handler
    instance = this;
}

DMAADCSampler::~DMAADCSampler() {
    stop();
    
    // Free DMA channel if allocated
    if (dma_channel >= 0) {
        dma_channel_unclaim(dma_channel);
    }
    
    instance = nullptr;
}

// ==================================================
// Initialization
// ==================================================

bool DMAADCSampler::init() {
    if (initialized) {
        return true;  // Already initialized
    }
    
    // Initialize ADC hardware
    // Note: This class assumes exclusive ownership of the ADC peripheral.
    // If ADC is shared with other components, coordinate initialization externally.
    adc_init();
    adc_gpio_init(ADCConfig::ADC_GPIO);
    adc_select_input(ADCConfig::ADC_CHANNEL);
    
    // Configure ADC FIFO for DMA pacing
    // Enable FIFO, enable DMA requests, set threshold to 1
    adc_fifo_setup(
        true,   // Enable FIFO
        true,   // Enable DMA data request (DREQ)
        1,      // DREQ threshold (assert when >= 1 sample)
        false,  // Don't generate error IRQ
        false   // Don't shift samples to 8-bit
    );
    
    // Claim a DMA channel
    dma_channel = dma_claim_unused_channel(true);
    if (dma_channel < 0) {
        printf("DMAADCSampler: Failed to claim DMA channel\n");
        return false;
    }
    
    // Configure DMA channel
    dma_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);  // 16-bit transfers
    channel_config_set_read_increment(&dma_config, false);  // Always read from ADC FIFO
    channel_config_set_write_increment(&dma_config, true);  // Increment write address
    channel_config_set_dreq(&dma_config, DREQ_ADC);  // Pace transfers using ADC DREQ
    
    // Start with buffer A
    dma_channel_configure(
        dma_channel,
        &dma_config,
        buffer_a,           // Write to buffer A
        &adc_hw->fifo,      // Read from ADC FIFO
        BUFFER_SIZE,        // Transfer count
        false               // Don't start yet
    );
    
    // Enable DMA interrupt on completion
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    initialized = true;
    printf("DMAADCSampler: Initialized (DMA channel %d)\n", dma_channel);
    
    return true;
}

// ==================================================
// Start/Stop
// ==================================================

void DMAADCSampler::start() {
    if (!initialized) {
        printf("DMAADCSampler: Cannot start - not initialized\n");
        return;
    }
    
    if (running) {
        return;  // Already running
    }
    
    // Reset state
    buffer_a_ready = false;
    buffer_b_ready = false;
    using_buffer_a = true;
    buffer_count = 0;
    overflow_count = 0;
    buffer_locked = false;
    
    // Start DMA transfer first (ready to receive ADC data)
    dma_channel_start(dma_channel);
    
    // Start timer to trigger ADC conversions at 5 kHz
    add_repeating_timer_us(
        -ADCConfig::SAMPLE_PERIOD_US,  // Negative for accurate timing
        timer_callback,
        this,
        &adc_timer
    );
    timer_running = true;
    
    running = true;
    printf("DMAADCSampler: Started (5 kHz sampling)\n");
}

void DMAADCSampler::stop() {
    if (!running) {
        return;
    }
    
    // Stop timer (stops triggering new conversions)
    if (timer_running) {
        cancel_repeating_timer(&adc_timer);
        timer_running = false;
    }
    
    // Disable DMA channel
    dma_channel_abort(dma_channel);
    
    running = false;
    printf("DMAADCSampler: Stopped\n");
}

// ==================================================
// Timer Callback (ADC Triggering)
// ==================================================

bool DMAADCSampler::timer_callback(repeating_timer_t *rt) {
    // Trigger single ADC conversion (timer-paced sampling)
    // The result will go to FIFO, which triggers DMA
    // Using ADC_CS_START_ONCE bit (bit 3) to trigger one conversion
    hw_set_bits(&adc_hw->cs, 1u << 3);  // ADC_CS_START_ONCE_BITS
    
    return true;  // Keep repeating
}

// ==================================================
// DMA Interrupt Handler
// ==================================================

void DMAADCSampler::dma_irq_handler() {
    if (instance == nullptr) {
        return;
    }
    
    // Check if our channel triggered the interrupt
    if (!dma_channel_get_irq0_status(instance->dma_channel)) {
        return;
    }
    
    // Clear interrupt
    dma_channel_acknowledge_irq0(instance->dma_channel);
    
    // Buffer just completed
    instance->buffer_count++;
    
    // Mark the buffer that just filled as ready
    if (instance->using_buffer_a) {
        // Buffer A is now full
        if (instance->buffer_a_ready) {
            // Buffer A wasn't processed before next fill - overflow!
            instance->overflow_count++;
        }
        instance->buffer_a_ready = true;
        
        // Switch to buffer B for next transfer
        instance->using_buffer_a = false;
        dma_channel_set_write_addr(instance->dma_channel, instance->buffer_b, true);
    } else {
        // Buffer B is now full
        if (instance->buffer_b_ready) {
            // Buffer B wasn't processed before next fill - overflow!
            instance->overflow_count++;
        }
        instance->buffer_b_ready = true;
        
        // Switch to buffer A for next transfer
        instance->using_buffer_a = true;
        dma_channel_set_write_addr(instance->dma_channel, instance->buffer_a, true);
    }
}

// ==================================================
// Buffer Access (for application)
// ==================================================

bool DMAADCSampler::is_buffer_ready() {
    return buffer_a_ready || buffer_b_ready;
}

const uint16_t* DMAADCSampler::get_ready_buffer(uint32_t* size) {
    // Return buffer A if ready and not locked
    if (buffer_a_ready && !buffer_locked) {
        buffer_locked = true;
        locked_buffer_is_a = true;
        if (size) *size = BUFFER_SIZE;
        return buffer_a;
    }
    
    // Return buffer B if ready and not locked
    if (buffer_b_ready && !buffer_locked) {
        buffer_locked = true;
        locked_buffer_is_a = false;
        if (size) *size = BUFFER_SIZE;
        return buffer_b;
    }
    
    // No buffer ready
    if (size) *size = 0;
    return nullptr;
}

void DMAADCSampler::release_buffer() {
    if (!buffer_locked) {
        return;  // No buffer was locked
    }
    
    // Disable interrupts during critical section to prevent race with DMA ISR
    uint32_t irq_status = save_and_disable_interrupts();
    
    // Clear the ready flag for the specific buffer that was locked
    if (locked_buffer_is_a) {
        buffer_a_ready = false;
    } else {
        buffer_b_ready = false;
    }
    
    buffer_locked = false;
    
    // Re-enable interrupts
    restore_interrupts(irq_status);
}
