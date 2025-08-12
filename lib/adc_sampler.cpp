
#include "adc_sampler.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/timer.h"

// ==================================================
// Constructor & Destructor
// ==================================================

ADCSampler::ADCSampler(uint input_channel)
    : head(0), tail(0), sample_interval_us(100), alarm_id(0), sampling(false), input_channel(input_channel) {
    // Zero buffer
    for (uint32_t i = 0; i < BUFFER_SIZE; ++i) buffer[i] = 0;
}

ADCSampler::~ADCSampler() {
    stop();
}

// ==================================================
// Internal: Timer Callback
// ==================================================
int64_t ADCSampler::timer_callback(alarm_id_t id, void *user_data) {
    ADCSampler* self = static_cast<ADCSampler*>(user_data);
    if (!self->sampling) return 0;
    self->handle_sample();
    // Schedule next sample
    return -((int64_t)self->sample_interval_us);
}

void ADCSampler::handle_sample() {
    uint16_t sample = adc_hw->result;
    buffer[head] = sample;
    head = (head + 1) % BUFFER_SIZE;
}

// ==================================================
// ADCSampler API
// ==================================================

void ADCSampler::init(uint32_t sample_rate_hz) {
    adc_init();
    adc_gpio_init(26 + input_channel); // Configure GPIO pin for ADC (GP26=ADC0, GP27=ADC1, etc.)
    adc_select_input(input_channel); // Configurable ADC input
    adc_fifo_setup(false, false, 0, false, false);
    adc_run(true);
    sample_interval_us = 1000000 / sample_rate_hz;
}

void ADCSampler::set_channel(uint input_channel) {
    this->input_channel = input_channel;
    adc_select_input(input_channel);
}

void ADCSampler::start() {
    sampling = true;
    alarm_id = add_alarm_in_us(sample_interval_us, timer_callback, this, true);
}

void ADCSampler::stop() {
    sampling = false;
    cancel_alarm(alarm_id);
}

bool ADCSampler::get_sample(uint16_t* out_sample) {
    if (tail == head) return false;
    *out_sample = buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    return true;
}
