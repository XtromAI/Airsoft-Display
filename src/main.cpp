#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "hardware/watchdog.h"
#include "sh1107_driver.h"
#include "font8x8.h"
#include "font16x16.h"
#include "sh1107_demo.h"
#include "temperature.h"
#include "wave_demo.h"
#include "adc_sampler.h"
#include "dma_adc_sampler.h"
#include "voltage_filter.h"
#include "adc_config.h"

// Pre-computed constants for ADC conversion (optimization for ARM Cortex-M0+)
static constexpr float ADC_TO_VOLTAGE_SCALE = (ADCConfig::ADC_VREF * 1000.0f * ADCConfig::VDIV_RATIO) / (1 << ADCConfig::ADC_BITS);

// --- Pin assignments ---
// Display pins (SPI1)
#define PIN_SPI_SCK     14
#define PIN_SPI_MOSI    15
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21
#define PIN_SPI_RESET   20

// ADC pin for battery monitoring
#define PIN_ADC_BATTERY 26  // GP26 (ADC0)

// Status LED (onboard)
#define PIN_STATUS_LED  25  // GP25 (onboard LED)

// --- Shared data structure ---
typedef struct {
    float current_voltage_mv;
    uint32_t shot_count;
    float moving_average_mv;
    float filtered_voltage_adc;  // Filtered ADC value (after median + lowpass)
    bool data_updated;
    // Live core metrics
    uint32_t core1_uptime_ms;
    float core1_loop_hz;
    uint32_t debug_counter; // Debug: increments in Core 1 loop
    uint32_t fallback_counter; // Debug: always increments regardless of mutex
    // DMA sampling statistics
    uint32_t dma_buffer_count;   // Total buffers processed
    uint32_t dma_overflow_count; // Buffer overflow count (data loss)
    uint32_t samples_processed;  // Total samples processed through filter
    uint32_t dma_irq_count;      // Total DMA IRQs serviced
    uint32_t dma_timer_count;    // Total ADC timer triggers
} shared_data_t;

// Global shared data and mutex
volatile shared_data_t g_shared_data = {0};
mutex_t g_data_mutex;

// --- Core 0 Functions (Display & UI) ---

// Volatile flag set by timer interrupt to trigger display update
volatile bool g_display_update_flag = false;

// Timer callback to set display update flag
bool display_update_timer_callback(repeating_timer_t *rt) {
    g_display_update_flag = true;
    return true; // keep repeating
}

void display_main() {
    // Kick the watchdog as soon as possible
    watchdog_update();
    printf("Core 0: Starting display and UI...\n");
    
    // Configure SPI pins
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);


    // Create display object
    SH1107_Display display(spi1, PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET, 128, 128);

    // Create temperature object in Fahrenheit mode
    Temperature temp_sensor(TemperatureUnit::Fahrenheit);
    temp_sensor.set_calibration_offset(4.0f); // todo: make this part of user settings

    // Initialize display
    if (!display.begin()) {
        printf("Core 0: Display initialization failed!\n");
        while (1) sleep_ms(1000);
    }
    // Set display to maximum brightness
    display.setContrast(0xFF);

    printf("Core 0: Display initialized successfully!\n");

    // Optionally show a demo at startup (commented out)
    // spinning_triangle_demo(display);

    // Display variables
    shared_data_t local_data = {0};
    uint32_t display_update_counter = 0;
    const uint32_t demo_delay_ms = 4000;
    
    // Core 0 metrics
    absolute_time_t core0_start_time = get_absolute_time();
    uint32_t core0_display_count = 0;
    float core0_display_hz = 0.0f;
    absolute_time_t last_metrics_time = core0_start_time;
    
    // Set up a repeating timer for display updates (16.67ms = 60Hz)
    repeating_timer_t display_timer;
    add_repeating_timer_ms(-17, display_update_timer_callback, NULL, &display_timer); // -17ms for ~60Hz

    // Core 0 main loop: Display and UI
    while (1) {
        // Wait for timer to set update flag
        if (!g_display_update_flag) {
            tight_loop_contents();
            continue;
        }
        g_display_update_flag = false;

        // Read shared data from Core 1 (with mutex protection)
        bool data_available = false;
        
        // Always read fallback counter (no mutex needed for atomic read)
        local_data.fallback_counter = g_shared_data.fallback_counter;
        
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            if (g_shared_data.data_updated) {
                // Copy volatile data to local structure field by field
                local_data.current_voltage_mv = g_shared_data.current_voltage_mv;
                local_data.shot_count = g_shared_data.shot_count;
                local_data.moving_average_mv = g_shared_data.moving_average_mv;
                local_data.filtered_voltage_adc = g_shared_data.filtered_voltage_adc;
                local_data.data_updated = g_shared_data.data_updated;
                local_data.core1_uptime_ms = g_shared_data.core1_uptime_ms;
                local_data.core1_loop_hz = g_shared_data.core1_loop_hz;
                local_data.debug_counter = g_shared_data.debug_counter;
                local_data.dma_buffer_count = g_shared_data.dma_buffer_count;
                local_data.dma_overflow_count = g_shared_data.dma_overflow_count;
                local_data.samples_processed = g_shared_data.samples_processed;
                local_data.dma_irq_count = g_shared_data.dma_irq_count;
                local_data.dma_timer_count = g_shared_data.dma_timer_count;
                g_shared_data.data_updated = false;
                data_available = true;
            }
            mutex_exit(&g_data_mutex);
        }
        display.clearDisplay(); // Clear display buffer
        // Draw the wave animation demo (one frame per update)
        // wave_demo_frame(display);

        // ==================================================
        // Metrics Display: DMA Sampling Statistics
        // ==================================================

        uint8_t row_height = display.getFontHeight() + 4;
        uint8_t y = 4;  // Start with 4px padding at top for consistent spacing
        char metric_str[32];

        // DMA buffer statistics
        snprintf(metric_str, sizeof(metric_str), "BUF: %lu", local_data.dma_buffer_count);
        display.drawString(0, y, metric_str);
        y += row_height;

        snprintf(metric_str, sizeof(metric_str), "OVF: %lu", local_data.dma_overflow_count);
        display.drawString(0, y, metric_str);
        y += row_height;

        snprintf(metric_str, sizeof(metric_str), "SMP: %lu", local_data.samples_processed);
        display.drawString(0, y, metric_str);
        y += row_height;

    snprintf(metric_str, sizeof(metric_str), "IRQ: %lu", local_data.dma_irq_count);
    display.drawString(0, y, metric_str);
    y += row_height;

    snprintf(metric_str, sizeof(metric_str), "TMR: %lu", local_data.dma_timer_count);
    display.drawString(0, y, metric_str);
    y += row_height;

        snprintf(metric_str, sizeof(metric_str), "TMP: %s", temp_sensor.get_formatted_temperature());
        display.drawString(0, y, metric_str);
        y += row_height;

        float clamped_voltage = local_data.current_voltage_mv;
        if (clamped_voltage < 0.0f) clamped_voltage = 0.0f;
        if (clamped_voltage > 99.99f) clamped_voltage = 99.99f;
        snprintf(metric_str, sizeof(metric_str), "VOL: %05.2fV", clamped_voltage);
        display.drawString(0, y, metric_str);
        y += row_height;

        snprintf(metric_str, sizeof(metric_str), "SHT: %lu", local_data.shot_count);
        display.drawString(0, y, metric_str);

        display.display();

        core0_display_count++;

        // Update Core 0 display frequency every second
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_metrics_time, now) >= 1000000) {
            core0_display_hz = core0_display_count / 1.0f;
            core0_display_count = 0;
            last_metrics_time = now;
        }

        // Kick the watchdog periodically
        watchdog_update();
    }
}

// --- Core 1 Functions (Data Acquisition & Processing) ---

int main() {
    stdio_init_all();
    sleep_ms(2000); // Wait for USB serial to initialize
    
    // Check if we are recovering from a watchdog reset
    if (watchdog_caused_reboot()) {
        printf("[Watchdog] System recovered from watchdog reset!\n");
    }
    // Enable the watchdog: timeout 2 seconds, pause on debug
    watchdog_enable(2000, 1);
    printf("Starting Airsoft Display System...\n");
    
    // Initialize mutex
    mutex_init(&g_data_mutex);
    
    // Launch display function on Core 0 (despite the confusing function name)
    // Note: multicore_launch_core1() actually launches the function on Core 0, not Core 1!
    multicore_launch_core1(display_main);
    // printf("Core 1: Core 0 launched for display and UI\n");
    
    // Core 1: Initialize data acquisition and processing...
    printf("Core 1: Starting data acquisition and processing...\n");
    
    // Initialize DMA ADC sampler with 5 kHz sampling
    DMAADCSampler dma_sampler;
    if (!dma_sampler.init()) {
        printf("Core 1: Failed to initialize DMA sampler!\n");
        while (1) sleep_ms(1000);
    }
    dma_sampler.start();
    printf("Core 1: DMA sampler started at 5 kHz\n");
    
    // Initialize voltage filter (median + low-pass)
    VoltageFilter voltage_filter;
    
    // Initialize status LED
    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    
    printf("Core 1: Data acquisition hardware initialized\n");
    
    uint32_t loop_counter = 0;
    absolute_time_t core1_start_time = get_absolute_time();
    uint32_t core1_last_metrics_time_ms = 0;
    uint32_t core1_loop_count = 0;
    float core1_loop_hz = 0.0f;
    uint32_t core1_last_debug_log_ms = 0;
    
    // Sample processing variables
    uint32_t total_samples_processed = 0;
    float last_filtered_value = 0.0f;
    float accumulated_voltage_mv = 0.0f;
    uint32_t voltage_sample_count = 0;
    
    // Core 1 main loop: Data Acquisition & Processing
    while (true) {
        // Check if a DMA buffer is ready for processing
        bool buffer_ready = dma_sampler.is_buffer_ready();
        if (buffer_ready) {
            uint32_t buffer_size = 0;
            const uint16_t* buffer = dma_sampler.get_ready_buffer(&buffer_size);
            
            if (buffer != nullptr && buffer_size > 0) {
                // Process all samples in the buffer through the filter chain
                for (uint32_t i = 0; i < buffer_size; ++i) {
                    // Filter the raw ADC sample
                    float filtered_adc = voltage_filter.process(buffer[i]);
                    last_filtered_value = filtered_adc;
                    
                    // Convert to voltage (millivolts) using pre-computed combined scale factor
                    float voltage_mv = filtered_adc * ADC_TO_VOLTAGE_SCALE;
                    
                    // Accumulate for moving average
                    accumulated_voltage_mv += voltage_mv;
                    voltage_sample_count++;
                    
                    total_samples_processed++;
                }
                
                // Release the buffer back to DMA
                dma_sampler.release_buffer();
            }
        }
        
        // Calculate uptime and loop frequency every second
        uint32_t core1_uptime_ms = absolute_time_diff_us(core1_start_time, get_absolute_time()) / 1000;
        core1_loop_count++;
        if (core1_last_metrics_time_ms == 0) core1_last_metrics_time_ms = core1_uptime_ms;
        if (core1_uptime_ms - core1_last_metrics_time_ms >= 1000) {
            core1_loop_hz = core1_loop_count / ((core1_uptime_ms - core1_last_metrics_time_ms) / 1000.0f);
            core1_loop_count = 0;
            core1_last_metrics_time_ms = core1_uptime_ms;
        }

        if (core1_uptime_ms - core1_last_debug_log_ms >= 1000) {
            core1_last_debug_log_ms = core1_uptime_ms;
         uint fifo_level = adc_fifo_get_level();
         bool dma_busy = dma_sampler.is_dma_busy();
         uint32_t dma_remaining = dma_sampler.get_dma_transfer_remaining();
         uint32_t adc_fcs = adc_hw->fcs;
         uint32_t adc_cs = adc_hw->cs;
            printf("[DMA] t=%lums ready=%d buf=%lu ovf=%lu irq=%lu tmr=%lu samp=%lu loop_hz=%.2f\n",
                   core1_uptime_ms,
                   buffer_ready,
                   dma_sampler.get_buffer_count(),
                   dma_sampler.get_overflow_count(),
                   dma_sampler.get_irq_count(),
                   dma_sampler.get_timer_trigger_count(),
             total_samples_processed,
             core1_loop_hz);
         printf("      fifo=%u dma_busy=%d dma_rem=%lu adc_fcs=0x%08lx adc_cs=0x%08lx\n",
             fifo_level,
             dma_busy,
             static_cast<unsigned long>(dma_remaining),
             static_cast<unsigned long>(adc_fcs),
             static_cast<unsigned long>(adc_cs));
        }
        
        // Update shared data (with mutex protection)
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            // Calculate moving average voltage from accumulated samples
            float avg_voltage_mv = 0.0f;
            if (voltage_sample_count > 0) {
                avg_voltage_mv = accumulated_voltage_mv / voltage_sample_count;
            }
            
            g_shared_data.current_voltage_mv = avg_voltage_mv;
            g_shared_data.moving_average_mv = avg_voltage_mv;
            g_shared_data.filtered_voltage_adc = last_filtered_value;
            g_shared_data.core1_uptime_ms = core1_uptime_ms;
            g_shared_data.core1_loop_hz = core1_loop_hz;
            g_shared_data.debug_counter++;
            g_shared_data.dma_buffer_count = dma_sampler.get_buffer_count();
            g_shared_data.dma_overflow_count = dma_sampler.get_overflow_count();
            g_shared_data.samples_processed = total_samples_processed;
            g_shared_data.dma_irq_count = dma_sampler.get_irq_count();
            g_shared_data.dma_timer_count = dma_sampler.get_timer_trigger_count();
            g_shared_data.data_updated = true;
            
            // Reset accumulators after updating shared data
            accumulated_voltage_mv = 0.0f;
            voltage_sample_count = 0;
            
            mutex_exit(&g_data_mutex);
        }
        
        // Debug: Force increment fallback counter even if mutex fails
        static uint32_t fallback_counter = 0;
        fallback_counter++;
        
        // Update fallback counter in shared data (using atomic write)
        g_shared_data.fallback_counter = fallback_counter;
        
        // Blink status LED to show Core 1 is running
        gpio_put(PIN_STATUS_LED, (loop_counter / 1000) & 1);
        loop_counter++;
        
        // Kick the watchdog periodically
        watchdog_update();
        
        // Small yield to prevent tight looping when no buffers ready
        // DMA will interrupt when buffer is ready
        tight_loop_contents();
    }
    
    return 0;
}
