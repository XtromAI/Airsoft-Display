// ==================================================
// Example Integration for Data Collection
// Add to src/main.cpp
// ==================================================

// At the top of main.cpp, add includes:
#include "flash_storage.h"
#include "data_collector.h"

// Add to global variables section (after g_shared_data):
static DataCollector g_data_collector;
static bool g_serial_command_mode = false;

// ==================================================
// Serial Command Handler
// ==================================================

void handle_serial_command(const char* cmd) {
    // Trim whitespace
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    if (strncmp(cmd, "COLLECT ", 8) == 0) {
        // Parse duration
        int duration_sec = atoi(cmd + 8);
        if (duration_sec <= 0 || duration_sec > 60) {
            printf("ERROR: Invalid duration (1-60 seconds)\n");
            return;
        }
        
        printf("Starting %d second collection...\n", duration_sec);
        if (g_data_collector.start_collection(duration_sec * 1000)) {
            printf("Collection started\n");
        } else {
            printf("ERROR: Failed to start collection\n");
        }
        
    } else if (strcmp(cmd, "LIST") == 0) {
        // List stored captures
        printf("Stored captures:\n");
        int count = FlashStorage::get_capture_count();
        
        for (int i = 0; i < count; i++) {
            FlashStorage::CaptureHeader header;
            const uint16_t* samples;
            
            if (FlashStorage::read_capture(i, &header, &samples)) {
                printf("Slot %d: %lu samples, timestamp: %lu ms\n", 
                       i, 
                       static_cast<unsigned long>(header.sample_count),
                       static_cast<unsigned long>(header.timestamp));
            }
        }
        
        if (count == 0) {
            printf("  No captures stored\n");
        }
        
    } else if (strncmp(cmd, "DOWNLOAD ", 9) == 0) {
        // Download a capture
        int slot = atoi(cmd + 9);
        
        FlashStorage::CaptureHeader header;
        const uint16_t* samples;
        
        if (!FlashStorage::read_capture(slot, &header, &samples)) {
            printf("ERROR: Invalid slot %d\n", slot);
            return;
        }
        
        // Calculate total size (header + samples)
        uint32_t total_size = sizeof(FlashStorage::CaptureHeader) + 
                              (header.sample_count * sizeof(uint16_t));
        
        printf("START %lu\n", static_cast<unsigned long>(total_size));
        fflush(stdout);
        
        // Send header
        fwrite(&header, sizeof(FlashStorage::CaptureHeader), 1, stdout);
        
        // Send samples
        fwrite(samples, sizeof(uint16_t), header.sample_count, stdout);
        fflush(stdout);
        
        printf("END\n");
        
    } else if (strncmp(cmd, "DELETE ", 7) == 0) {
        // Delete a capture
        int slot = atoi(cmd + 7);
        
        if (FlashStorage::delete_capture(slot)) {
            printf("OK\n");
        } else {
            printf("ERROR: Failed to delete slot %d\n", slot);
        }
        
    } else if (strcmp(cmd, "HELP") == 0) {
        printf("Available commands:\n");
        printf("  COLLECT <seconds>  - Collect data for N seconds (1-60)\n");
        printf("  LIST               - List stored captures\n");
        printf("  DOWNLOAD <slot>    - Download a capture\n");
        printf("  DELETE <slot>      - Delete a capture\n");
        printf("  HELP               - Show this help\n");
        
    } else {
        printf("ERROR: Unknown command '%s'\n", cmd);
        printf("Type HELP for list of commands\n");
    }
}

// ==================================================
// Serial Input Reader
// ==================================================

void check_serial_input() {
    static char cmd_buffer[64];
    static int cmd_len = 0;
    
    while (true) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            break;  // No more input
        }
        
        if (c == '\n' || c == '\r') {
            if (cmd_len > 0) {
                cmd_buffer[cmd_len] = '\0';
                handle_serial_command(cmd_buffer);
                cmd_len = 0;
            }
        } else if (cmd_len < sizeof(cmd_buffer) - 1) {
            cmd_buffer[cmd_len++] = c;
        }
    }
}

// ==================================================
// Add to core1_main() function
// ==================================================

void core1_main() {
    // ... existing initialization ...
    
    // Initialize flash storage
    FlashStorage::init();
    
    while (true) {
        // ... existing DMA buffer processing ...
        
        // Check for ready DMA buffer
        if (dma_sampler.is_buffer_ready()) {
            uint32_t size;
            const uint16_t* buffer = dma_sampler.get_ready_buffer(&size);
            
            if (buffer != nullptr) {
                // Process through filters (existing code)
                // ... existing filter processing ...
                
                // If collecting data, feed to collector
                if (g_data_collector.is_collecting()) {
                    g_data_collector.process_buffer(buffer, size);
                }
                
                dma_sampler.release_buffer();
            }
        }
        
        // Check serial input
        check_serial_input();
        
        // ... rest of existing code ...
    }
}

// ==================================================
// Add to display_main() function (optional)
// ==================================================

void display_main() {
    // ... existing code ...
    
    while (true) {
        // ... existing display code ...
        
        // Show collection status on display if collecting
        if (g_data_collector.is_collecting()) {
            display.setCursor(0, 100);
            char buf[32];
            snprintf(buf, sizeof(buf), "COLLECT: %d%%", g_data_collector.get_progress());
            display.print(buf);
        }
        
        // ... rest of existing code ...
    }
}

// ==================================================
// Usage Examples
// ==================================================

/*
Via USB serial (115200 baud):

1. Start 10-second collection:
   > COLLECT 10
   
2. List captures:
   > LIST
   
3. Download capture:
   > DOWNLOAD 0
   
4. Delete capture:
   > DELETE 0

On PC side:
   python tools/download_data.py /dev/ttyACM0 list
   python tools/download_data.py /dev/ttyACM0 download 0
   python tools/parse_capture.py capture_00000.bin
*/
