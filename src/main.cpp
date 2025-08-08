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
#include "sh1107_driver.h"
#include "font8x8.h"
#include "font16x16.h"
#include "sh1107_demo.h"

// --- Pin assignments ---
// Display pins (SPI1)
#define PIN_SPI_SCK     14
#define PIN_SPI_MOSI    15
#define PIN_SPI_CS      13
#define PIN_SPI_DC      21
#define PIN_SPI_RESET   20

// ADC pin for battery monitoring
#define PIN_ADC_BATTERY 26  // GP26 (ADC0)

// Button pins
#define PIN_BUTTON_OUT  9   // GP9 (drives low)
#define PIN_BUTTON_IN   10  // GP10 (input with pull-up)

// Status LED (onboard)
#define PIN_STATUS_LED  25  // GP25 (onboard LED)

// --- Shared data structure ---
typedef struct {
    float current_voltage_mv;
    uint32_t shot_count;
    float moving_average_mv;
    bool data_updated;
} shared_data_t;

// Global shared data and mutex
volatile shared_data_t g_shared_data = {0};
mutex_t g_data_mutex;

// --- Core 0 Functions (Display & UI) ---

void display_main() {
    printf("Core 0: Starting display and UI...\n");
    
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
    
    printf("Core 0: Display initialized successfully!\n");

    // Display variables
    shared_data_t local_data = {0};
    uint32_t display_update_counter = 0;
    const uint32_t demo_delay_ms = 4000;
    
    // Core 0 main loop: Display and UI
    while (1) {
        // Read shared data from Core 1 (with mutex protection)
        bool data_available = false;
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            if (g_shared_data.data_updated) {
                // Copy volatile data to local structure field by field
                local_data.current_voltage_mv = g_shared_data.current_voltage_mv;
                local_data.shot_count = g_shared_data.shot_count;
                local_data.moving_average_mv = g_shared_data.moving_average_mv;
                local_data.data_updated = g_shared_data.data_updated;
                g_shared_data.data_updated = false;
                data_available = true;
            }
            mutex_exit(&g_data_mutex);
        }
        
        // Update display with current data
        if (data_available || (display_update_counter % 100 == 0)) {
            // Clear display and show basic info
            display.clearDisplay();
            
            // Display shot count prominently
            char shot_text[32];
            snprintf(shot_text, sizeof(shot_text), "Shots: %lu", local_data.shot_count);
            display.drawString(10, 30, shot_text);
            
            // Display battery voltage
            char voltage_text[32];
            snprintf(voltage_text, sizeof(voltage_text), "%.0fmV", local_data.current_voltage_mv);
            display.drawString(10, 50, voltage_text);
            
            // Display Core status
            display.drawString(10, 70, "Core1: Data Acq");
            display.drawString(10, 90, "Core0: Display");
            
            display.display();
        }
        
        // // Periodically run the demo to show display capabilities
        // if (display_update_counter % 1000 == 0) {
        //     printf("Core 0: Running display demo...\n");
        //     sh1107_demo(display, demo_delay_ms);
        // }
        
        display_update_counter++;
        sleep_ms(50); // 20Hz display update rate
    }
}

// --- Core 1 Functions (Data Acquisition & Processing) ---

int main() {
    stdio_init_all();
    printf("Starting Airsoft Display System...\n");
    
    // Initialize mutex
    mutex_init(&g_data_mutex);
    
    // Launch display function on Core 0 (despite the confusing function name)
    // Note: multicore_launch_core1() actually launches the function on Core 0, not Core 1!
    multicore_launch_core1(display_main);
    printf("Core 1: Core 0 launched for display and UI\n");
    
    // Core 1: Initialize data acquisition hardware
    printf("Core 1: Starting data acquisition and processing...\n");
    
    // Initialize ADC for battery monitoring
    adc_init();
    adc_gpio_init(PIN_ADC_BATTERY);
    adc_select_input(0); // ADC0 (GP26)
    
    // Initialize button pins
    gpio_init(PIN_BUTTON_OUT);
    gpio_set_dir(PIN_BUTTON_OUT, GPIO_OUT);
    gpio_put(PIN_BUTTON_OUT, 0); // Drive low
    
    gpio_init(PIN_BUTTON_IN);
    gpio_set_dir(PIN_BUTTON_IN, GPIO_IN);
    gpio_pull_up(PIN_BUTTON_IN);
    
    // Initialize status LED
    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);
    
    // TODO: Initialize hardware timer and DMA for high-frequency sampling
    // TODO: Set up circular buffer for ADC samples
    // TODO: Implement moving average calculation
    // TODO: Implement shot detection logic
    
    printf("Core 1: Data acquisition hardware initialized\n");
    
    uint32_t loop_counter = 0;
    
    // Core 1 main loop: Data Acquisition & Processing
    while (true) {
        // Simple ADC reading for now (will be replaced with DMA-based sampling)
        uint16_t adc_raw = adc_read();
        float voltage_mv = (adc_raw * 3300.0f) / 4096.0f; // Convert to millivolts
        
        // Update shared data (with mutex protection)
        if (mutex_try_enter(&g_data_mutex, NULL)) {
            g_shared_data.current_voltage_mv = voltage_mv;
            g_shared_data.moving_average_mv = voltage_mv; // Simplified for now
            g_shared_data.data_updated = true;
            mutex_exit(&g_data_mutex);
        }
        
        // Blink status LED to show Core 1 is running
        gpio_put(PIN_STATUS_LED, (loop_counter / 100) & 1);
        loop_counter++;
        
        // Check button for shot counter reset (simplified logic)
        if (!gpio_get(PIN_BUTTON_IN)) {
            if (mutex_try_enter(&g_data_mutex, NULL)) {
                g_shared_data.shot_count = 0;
                mutex_exit(&g_data_mutex);
            }
            sleep_ms(500); // Simple debounce
        }
        
        sleep_ms(10); // 100Hz update rate for now
    }
    
    return 0;
}
