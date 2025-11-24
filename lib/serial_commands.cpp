#include "serial_commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "flash_storage.h"

// Static member initialization
DataCollector* SerialCommands::s_collector = nullptr;
char SerialCommands::s_cmd_buffer[64] = {0};
int SerialCommands::s_cmd_len = 0;

void SerialCommands::init(DataCollector* collector) {
    s_collector = collector;
    s_cmd_len = 0;
}

void SerialCommands::check_input() {
    while (true) {
        int c = getchar_timeout_us(0);
        if (c == PICO_ERROR_TIMEOUT) {
            break;  // No more input
        }
        
        if (c == '\n' || c == '\r') {
            if (s_cmd_len > 0) {
                s_cmd_buffer[s_cmd_len] = '\0';
                handle_command(s_cmd_buffer);
                s_cmd_len = 0;
            }
        } else if (s_cmd_len < sizeof(s_cmd_buffer) - 1) {
            s_cmd_buffer[s_cmd_len++] = c;
        }
    }
}

void SerialCommands::handle_command(const char* cmd) {
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
        if (s_collector && s_collector->start_collection(duration_sec * 1000)) {
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
            const uint16_t* raw_samples;
            const uint16_t* filtered_samples;
            
            if (FlashStorage::read_capture_dual(i, &header, &raw_samples, &filtered_samples)) {
                printf("Slot %d: %lu samples, v%lu, %s, timestamp: %lu ms\n", 
                       i, 
                       static_cast<unsigned long>(header.sample_count),
                       static_cast<unsigned long>(header.version),
                       (header.has_filtered ? "raw+filtered" : "raw only"),
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
        const uint16_t* raw_samples;
        const uint16_t* filtered_samples;
        
        if (!FlashStorage::read_capture_dual(slot, &header, &raw_samples, &filtered_samples)) {
            printf("ERROR: Invalid slot %d\n", slot);
            return;
        }
        
        // Calculate total size (header + raw samples + filtered samples if present)
        uint32_t raw_data_size = header.sample_count * sizeof(uint16_t);
        uint32_t filtered_data_size = (header.version >= 2 && header.has_filtered) ? raw_data_size : 0;
        uint32_t total_size = sizeof(FlashStorage::CaptureHeader) + raw_data_size + filtered_data_size;
        
        printf("START %lu\n", static_cast<unsigned long>(total_size));
        fflush(stdout);
        
        // Send header
        fwrite(&header, sizeof(FlashStorage::CaptureHeader), 1, stdout);
        
        // Send raw samples
        fwrite(raw_samples, sizeof(uint16_t), header.sample_count, stdout);
        
        // Send filtered samples if present
        if (filtered_samples != nullptr) {
            fwrite(filtered_samples, sizeof(uint16_t), header.sample_count, stdout);
        }
        
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
