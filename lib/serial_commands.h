#pragma once

#include "data_collector.h"

/**
 * @brief Serial command handler for data collection system
 * 
 * Provides a simple command-line interface over USB serial for:
 * - Starting data collection (COLLECT)
 * - Listing stored captures (LIST)
 * - Downloading captures (DOWNLOAD)
 * - Deleting captures (DELETE)
 * - Help text (HELP)
 */
class SerialCommands {
public:
    /**
     * @brief Initialize serial command handler
     * @param collector Reference to data collector instance
     */
    static void init(DataCollector* collector);
    
    /**
     * @brief Check for and process any pending serial input
     * 
     * Call this frequently from main loop (non-blocking).
     * Accumulates characters until newline, then processes command.
     */
    static void check_input();
    
private:
    static DataCollector* s_collector;
    static char s_cmd_buffer[64];
    static int s_cmd_len;
    
    /**
     * @brief Process a complete command string
     * @param cmd Null-terminated command string
     */
    static void handle_command(const char* cmd);
};
