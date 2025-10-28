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
    bool data_updated;
    // Live core metrics
    uint32_t core1_uptime_ms;
    float core1_loop_hz;
    uint32_t debug_counter; // Debug: increments in Core 1 loop
    uint32_t fallback_counter; // Debug: always increments regardless of mutex
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
                local_data.data_updated = g_shared_data.data_updated;
                local_data.core1_uptime_ms = g_shared_data.core1_uptime_ms;
                local_data.core1_loop_hz = g_shared_data.core1_loop_hz;
                local_data.debug_counter = g_shared_data.debug_counter;
                g_shared_data.data_updated = false;
                data_available = true;
            }
            mutex_exit(&g_data_mutex);
        }
        display.clearDisplay(); // Clear display buffer
        // Draw the wave animation demo (one frame per update)
        // wave_demo_frame(display);

        // ==================================================
        // Sampler Status Message (Top Row)
        // ==================================================
        char status_msg[64];
        if (local_data.core1_loop_hz > 1.0f) {
            snprintf(status_msg, sizeof(status_msg), "%.0fHz D:%lu F:%lu", local_data.core1_loop_hz, local_data.debug_counter, local_data.fallback_counter);
        } else {
            snprintf(status_msg, sizeof(status_msg), "D:%lu F:%lu", local_data.debug_counter, local_data.fallback_counter);
        }
        display.drawString(0, 0, status_msg);

        // ==================================================
        // Temperature (Bottom Left)
        // ==================================================
        uint8_t temp_y = display.getHeight() - display.getFontHeight()/2;
        display.drawString(0, temp_y, temp_sensor.get_formatted_temperature());

        // ==================================================
        // Voltage (Center)
        // ==================================================
        char voltage_str[32];
        snprintf(voltage_str, sizeof(voltage_str), "%.2f mV", local_data.current_voltage_mv);
        display.drawString(display.centerx(), display.centery(), voltage_str);

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
    // printf("Core 1: Starting data acquisition and processing...\n");
    
    // Initialize ADC sampler with 10Hz sampling rate on ADC0 (PIN_ADC_BATTERY)
    ADCSampler adc_sampler(0); // ADC0 (GP26)
    adc_sampler.init(10); // 10Hz sampling rate (much slower for testing)
    adc_sampler.start();
    // printf("Core 1: ADC sampler initialized at 10Hz\n");
    
    // Initialize status LED
    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    
    // TODO: Initialize hardware timer and DMA for high-frequency sampling
    // TODO: Set up circular buffer for ADC samples
    // TODO: Implement moving average calculation
    // TODO: Implement shot detection logic
    
    // printf("Core 1: Data acquisition hardware initialized\n");
    
    uint32_t loop_counter = 0;
    absolute_time_t core1_start_time = get_absolute_time();
    uint32_t core1_last_metrics_time_ms = 0;
    uint32_t core1_loop_count = 0;
    float core1_loop_hz = 0.0f;
    
    // Core 1 main loop: Data Acquisition & Processing
    while (true) {
        // Get latest ADC sample from high-frequency sampler
        uint16_t adc_raw;
        float voltage_mv;
        if (adc_sampler.get_sample(&adc_raw)) {
            voltage_mv = (adc_raw * 3300.0f) / 4096.0f; // Convert to millivolts
        } else {
            voltage_mv = 0.0f; // No sample available
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
        
        // Update shared data (with mutex protection)
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            g_shared_data.current_voltage_mv = voltage_mv;
            g_shared_data.moving_average_mv = voltage_mv; // Simplified for now
            g_shared_data.core1_uptime_ms = core1_uptime_ms;
            g_shared_data.core1_loop_hz = core1_loop_hz;
            g_shared_data.debug_counter++;
            g_shared_data.data_updated = true;
            mutex_exit(&g_data_mutex);
        }
        
        // Debug: Force increment fallback counter even if mutex fails
        static uint32_t fallback_counter = 0;
        fallback_counter++;
        
        // Update fallback counter in shared data (using atomic write)
        g_shared_data.fallback_counter = fallback_counter;
        
        // Blink status LED to show Core 1 is running
        gpio_put(PIN_STATUS_LED, (loop_counter / 100) & 1);
        loop_counter++;
        
    // Kick the watchdog periodically
    watchdog_update();
    sleep_ms(10); // 100Hz update rate for now
    }
    
    return 0;
}
