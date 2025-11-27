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
#include "wave_demo.h"
#include "dma_adc_sampler.h"
#include "voltage_filter.h"
#include "adc_config.h"
#include "flash_storage.h"
#include "data_collector.h"
#include "serial_commands.h"
#include <new>

// Pre-computed constants for ADC conversion (optimization for ARM Cortex-M0+)
static constexpr float ADC_TO_VOLTAGE_SCALE = (ADCConfig::ADC_VREF * 1000.0f * ADCConfig::VDIV_RATIO * ADCConfig::ADC_CALIBRATION) / (1 << ADCConfig::ADC_BITS);
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
    float raw_avg_adc;
    float raw_adc_voltage_mv;
    uint16_t raw_min_adc;
    uint16_t raw_max_adc;
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

// Data collection globals
static DataCollector g_data_collector;

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

    // Allow this core to be safely paused during flash operations
    multicore_lockout_victim_init();
    
    // Configure SPI pins
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_MOSI, GPIO_FUNC_SPI);


    // Create display object
    SH1107_Display display(spi1, PIN_SPI_CS, PIN_SPI_DC, PIN_SPI_RESET, 128, 128);


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
    absolute_time_t last_display_update = get_absolute_time();
    while (1) {
        // Wait for timer to set update flag, but also force update after 100ms timeout
        // This prevents display from freezing if timer gets stuck
        absolute_time_t current_time = get_absolute_time();
        int64_t us_since_update = absolute_time_diff_us(last_display_update, current_time);
        
        if (!g_display_update_flag && us_since_update < 100000) {
            tight_loop_contents();
            continue;
        }
        g_display_update_flag = false;
        last_display_update = current_time;

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
                local_data.raw_avg_adc = g_shared_data.raw_avg_adc;
                local_data.raw_adc_voltage_mv = g_shared_data.raw_adc_voltage_mv;
                local_data.raw_min_adc = g_shared_data.raw_min_adc;
                local_data.raw_max_adc = g_shared_data.raw_max_adc;
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

    float voltage_v = local_data.current_voltage_mv * 0.001f;
    if (voltage_v < 0.0f) voltage_v = 0.0f;
    if (voltage_v > 99.99f) voltage_v = 99.99f;
    snprintf(metric_str, sizeof(metric_str), "VOL: %05.2fV", voltage_v);
        display.drawString(0, y, metric_str);
        y += row_height;

        float adc_voltage_v = local_data.raw_adc_voltage_mv * 0.001f;
        if (adc_voltage_v < 0.0f) adc_voltage_v = 0.0f;
        if (adc_voltage_v > ADCConfig::ADC_VREF) adc_voltage_v = ADCConfig::ADC_VREF;
        snprintf(metric_str, sizeof(metric_str), "ADC: %05.2fV", adc_voltage_v);
        display.drawString(0, y, metric_str);
        y += row_height;

        snprintf(metric_str, sizeof(metric_str), "RAW: %05.0f", local_data.raw_avg_adc);
        display.drawString(0, y, metric_str);
        y += row_height;

        snprintf(metric_str, sizeof(metric_str), "MN:%4u MX:%4u", local_data.raw_min_adc, local_data.raw_max_adc);
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

    // Prepare current core to participate in lockouts when necessary
    multicore_lockout_victim_init();

    // Launch display function on the second core
    multicore_launch_core1(display_main);
    // printf("Core 1: Core 0 launched for display and UI\n");
    
    // Core 1: Initialize data acquisition and processing...
    printf("Core 1: Starting data acquisition and processing...\n");
    
    // Initialize flash storage for data collection
    FlashStorage::init();
    printf("Core 1: Flash storage initialized\n");
    
    // Initialize serial command handler
    SerialCommands::init(&g_data_collector);
    printf("Core 1: Serial commands initialized (type HELP for commands)\n");
    
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
    float last_avg_voltage_mv = 0.0f;
    float last_raw_avg = 0.0f;
    float last_raw_adc_mv = 0.0f;
    uint16_t last_raw_min = 0;
    uint16_t last_raw_max = 0;
    
    // Core 1 main loop: Data Acquisition & Processing
    while (true) {
        // Check if a DMA buffer is ready for processing
        bool buffer_ready = dma_sampler.is_buffer_ready();
        if (buffer_ready) {
            uint32_t buffer_size = 0;
            const uint16_t* buffer = dma_sampler.get_ready_buffer(&buffer_size);
            
            if (buffer != nullptr && buffer_size > 0) {
                uint32_t raw_sum = 0;
                uint16_t raw_min = 0xFFFF;
                uint16_t raw_max = 0;
                
                // Temporary buffer for filtered samples (only allocated if collecting)
                uint16_t* filtered_buffer = nullptr;
                if (g_data_collector.is_collecting()) {
                    filtered_buffer = new (std::nothrow) uint16_t[buffer_size];
                }

                // Process all samples in the buffer through the filter chain
                for (uint32_t i = 0; i < buffer_size; ++i) {
                    uint16_t sample = buffer[i];
                    raw_sum += sample;
                    if (sample < raw_min) raw_min = sample;
                    if (sample > raw_max) raw_max = sample;
                    
                    // Filter the raw ADC sample
                    float filtered_adc = voltage_filter.process(sample);
                    last_filtered_value = filtered_adc;
                    
                    // Store filtered sample for data collection (scaled back to uint16_t)
                    if (filtered_buffer != nullptr) {
                        // Clamp to 12-bit range and round
                        float clamped = (filtered_adc < 0.0f) ? 0.0f : 
                                       (filtered_adc > ADCConfig::ADC_MAX) ? static_cast<float>(ADCConfig::ADC_MAX) : 
                                       filtered_adc;
                        filtered_buffer[i] = static_cast<uint16_t>(clamped + 0.5f);
                    }
                    
                    // Convert to voltage (millivolts) using pre-computed combined scale factor
                    float voltage_mv = filtered_adc * ADC_TO_VOLTAGE_SCALE;
                    
                    // Accumulate for moving average
                    accumulated_voltage_mv += voltage_mv;
                    voltage_sample_count++;
                    
                    total_samples_processed++;
                }

                float buffer_avg = static_cast<float>(raw_sum) / static_cast<float>(buffer_size);
                last_raw_avg = buffer_avg;
                last_raw_min = raw_min;
                last_raw_max = raw_max;
                last_raw_adc_mv = (buffer_avg / static_cast<float>(ADCConfig::ADC_MAX)) * ADCConfig::ADC_VREF * ADCConfig::ADC_CALIBRATION * 1000.0f;
                
                // If collecting data, feed buffers to collector
                if (g_data_collector.is_collecting()) {
                    g_data_collector.process_buffer(buffer, filtered_buffer, buffer_size);
                }
                
                // Clean up temporary filtered buffer
                if (filtered_buffer != nullptr) {
                    delete[] filtered_buffer;
                }
                
                // Release the buffer back to DMA
                dma_sampler.release_buffer();
            }
        }
        
        // Debug: Print message after collection completes
        static bool was_collecting = false;
        bool is_collecting_now = g_data_collector.is_collecting();
        // Removed debug message for consistency
        was_collecting = is_collecting_now;
        
        // Check for serial input commands
        SerialCommands::check_input();
        
        // Calculate uptime and loop frequency every second
        uint32_t core1_uptime_ms = absolute_time_diff_us(core1_start_time, get_absolute_time()) / 1000;
        core1_loop_count++;
        if (core1_last_metrics_time_ms == 0) core1_last_metrics_time_ms = core1_uptime_ms;
        if (core1_uptime_ms - core1_last_metrics_time_ms >= 1000) {
            core1_loop_hz = core1_loop_count / ((core1_uptime_ms - core1_last_metrics_time_ms) / 1000.0f);
            core1_loop_count = 0;
            core1_last_metrics_time_ms = core1_uptime_ms;
        }

        // Debug logging disabled to reduce serial clutter
        // Uncomment below to re-enable periodic debug output
        /*
        if (core1_uptime_ms - core1_last_debug_log_ms >= 1000) {
            core1_last_debug_log_ms = core1_uptime_ms;
            uint fifo_level = adc_fifo_get_level();
            bool dma_busy = dma_sampler.is_dma_busy();
            uint32_t dma_remaining = dma_sampler.get_dma_transfer_remaining();
            uint32_t adc_fcs = adc_hw->fcs;
            uint32_t adc_cs = adc_hw->cs;
            printf("[DMA] t=%lums ready=%d buf=%lu ovf=%lu irq=%lu tmr=%lu samp=%lu loop_hz=%.2f avg=%.1fmV\n",
                   core1_uptime_ms,
                   buffer_ready,
                   dma_sampler.get_buffer_count(),
                   dma_sampler.get_overflow_count(),
                   dma_sampler.get_irq_count(),
                   dma_sampler.get_timer_trigger_count(),
                   total_samples_processed,
                   core1_loop_hz,
                   last_avg_voltage_mv);
            printf("      fifo=%u dma_busy=%d dma_rem=%lu adc_fcs=0x%08lx adc_cs=0x%08lx\n",
                   fifo_level,
                   dma_busy,
                   static_cast<unsigned long>(dma_remaining),
                   static_cast<unsigned long>(adc_fcs),
                   static_cast<unsigned long>(adc_cs));
        }
        */
        
        // Update shared data (with mutex protection)
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            // Calculate moving average voltage from accumulated samples
            float avg_voltage_mv = last_avg_voltage_mv;
            if (voltage_sample_count > 0) {
                avg_voltage_mv = accumulated_voltage_mv / voltage_sample_count;
                last_avg_voltage_mv = avg_voltage_mv;
            }

            // Add diode drop to show true battery voltage (pre-diode)
            g_shared_data.current_voltage_mv = avg_voltage_mv + ADCConfig::DIODE_DROP_MV;
            g_shared_data.moving_average_mv = avg_voltage_mv + ADCConfig::DIODE_DROP_MV;
            g_shared_data.filtered_voltage_adc = last_filtered_value;
            g_shared_data.core1_uptime_ms = core1_uptime_ms;
            g_shared_data.core1_loop_hz = core1_loop_hz;
            g_shared_data.debug_counter++;
            g_shared_data.dma_buffer_count = dma_sampler.get_buffer_count();
            g_shared_data.dma_overflow_count = dma_sampler.get_overflow_count();
            g_shared_data.samples_processed = total_samples_processed;
            g_shared_data.dma_irq_count = dma_sampler.get_irq_count();
            g_shared_data.dma_timer_count = dma_sampler.get_timer_trigger_count();
            g_shared_data.raw_avg_adc = last_raw_avg;
            g_shared_data.raw_adc_voltage_mv = last_raw_adc_mv;
            g_shared_data.raw_min_adc = last_raw_min;
            g_shared_data.raw_max_adc = last_raw_max;
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
