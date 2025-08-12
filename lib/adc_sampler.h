
#ifndef ADC_SAMPLER_H
#define ADC_SAMPLER_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"

// ==================================================
// ADCSampler Class
// ==================================================

class ADCSampler {
public:
    ADCSampler(uint input_channel = 0);
    ~ADCSampler();

    void init(uint32_t sample_rate_hz);
    void set_channel(uint input_channel);
    void start();
    void stop();
    bool get_sample(uint16_t* out_sample);

private:
    static int64_t timer_callback(alarm_id_t id, void *user_data);
    void handle_sample();

    static constexpr uint32_t BUFFER_SIZE = 256;
    volatile uint16_t buffer[BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t sample_interval_us;
    alarm_id_t alarm_id;
    volatile bool sampling;
    uint input_channel;
};

#endif // ADC_SAMPLER_H
