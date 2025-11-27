# ADC DMA Shot Detection Research
**Date:** 2025-01-30  
**Author:** Xris (XtromAI)  
**Status:** Research Complete - Ready for Implementation

## Executive Summary

Research into implementing hardware-accelerated ADC sampling using RP2040's DMA controller for real-time shot detection in airsoft replicas. Goal: Count shots via battery voltage monitoring at 5-10 kHz sample rate with minimal CPU overhead.

**Key Findings:**
- 5 kHz sampling recommended (10× better than current 500 Hz POC)
- Double-buffered DMA achieves <1% CPU usage
- Simple adaptive threshold detection sufficient (ML/FFT overkill)
- Two-stage filtering (median + low-pass) handles motor noise
- No manual calibration needed with adaptive baseline tracking

---

## 1. Problem Statement

### Current POC Limitations (500 Hz sampling)
- **Missed shots in full-auto** - voltage doesn't recover between rounds
- **CPU-bound polling** - blocks other tasks
- **Noisy data** - motor commutation spikes corrupt readings
- **Fixed thresholds** - doesn't adapt to different guns or battery states

### Requirements
1. **Detect all shots** - semi-auto and full-auto (up to 30 RPS)
2. **Minimal CPU usage** - leave headroom for display, Bluetooth, logging
3. **Real-time response** - shot count updates within 10ms
4. **Adaptive** - work across different guns without calibration
5. **Robust** - handle motor noise, battery drain, temperature variation

---

## 2. Sample Rate Analysis

### Actual Shot Signatures (from POC data @ 500 Hz)

**Characteristics observed in log_15.csv and log_16.csv:**
```
Baseline voltage:     177-180 ADC units (~2.3V on 11.1V LiPo)
Shot voltage drop:    Down to 13-98 ADC (~85-95% drop!)
Drop duration:        30-50ms (15-25 samples @ 500 Hz)
Recovery duration:    20-40ms (10-20 samples @ 500 Hz)
Full-auto ROF:        ~20 RPS (50ms between shots)
```

**Problem:** At 500 Hz, shots blur together during full-auto bursts.

### Sample Rate Comparison

| Sample Rate | Samples/Shot | Samples/Recovery | CPU Usage | Verdict |
|-------------|--------------|------------------|-----------|---------|
| **500 Hz (current)** | 15-25 | 10-20 | ~5% | ❌ Insufficient for full-auto |
| **1 kHz** | 30-50 | 20-40 | ~2% | 🤷 Marginal improvement |
| **5 kHz** | 150-250 | 100-200 | <1% | ✅ **Recommended** |
| **10 kHz** | 300-500 | 200-400 | <1% | ✅ Overkill but enables advanced filtering |

### Recommendation: **Start with 5 kHz**

**Rationale:**
- 10× improvement over POC
- Captures full shot dynamics clearly
- Separates full-auto shots reliably
- Only 0.5% of RP2040 ADC max throughput (1 Msps)
- Leaves headroom for 10 kHz if needed

**When to use 10 kHz:**
- If 5 kHz struggles with specific gun models
- To implement digital filtering (oversample + downsample)
- To analyze motor commutation noise
- For advanced features (FFT-based diagnostics)

---

## 3. DMA Architecture

### Why DMA?

**Without DMA (current POC):**
```cpp
void loop() {
    uint16_t adc = analogRead(BATTERY_PIN);  // ~10µs per read
    delay(2);  // 500 Hz = 2ms interval
    // CPU blocked waiting
}
```
- CPU must poll ADC every 2ms
- Blocks other tasks (display updates, Bluetooth)
- Timing jitter from interrupts

**With DMA:**
```cpp
void setup() {
    configure_adc_dma();  // One-time setup
}

void adc_dma_irq() {
    // DMA automatically filled buffer
    process_samples(buffer_a, 512);  // Process in background
    swap_buffers();
}
```
- DMA transfers ADC samples to RAM automatically
- CPU only wakes when buffer full (every 102ms @ 5 kHz)
- Zero polling overhead

### Double-Buffering (Ping-Pong)

**Concept:** Two buffers alternate roles

```
        ┌──────────────┐
        │  ADC Sampler │
        └──────┬───────┘
               │
        ┌──────▼───────┐
        │ DMA Controller│
        └──────┬───────┘
               │
      ┌────────┴────────┐
      │                 │
┌─────▼─────┐     ┌─────▼─────┐
│ Buffer A  │     │ Buffer B  │
│ (512 smp) │     │ (512 smp) │
└─────┬─────┘     └─────┬─────┘
      │                 │
      └────────┬────────┘
               │
        ┌──────▼───────┐
        │ Shot Detector│
        └──────────────┘
```

**State Machine:**
1. **DMA writes to Buffer A** → CPU processes Buffer B
2. **Buffer A full** → Interrupt → Swap
3. **DMA writes to Buffer B** → CPU processes Buffer A
4. **Buffer B full** → Interrupt → Swap
5. Repeat

**Benefits:**
- **Zero data loss** - always have a buffer being filled
- **Real-time processing** - CPU works while DMA fills next buffer
- **Simple logic** - just swap pointers on interrupt

### Memory Layout

```cpp
// RP2040 has 264 KB SRAM total
// DMA buffers allocation:

uint16_t dma_buffer_a[512];  // 1 KB
uint16_t dma_buffer_b[512];  // 1 KB
// Total: 2 KB (0.76% of RAM)

// Remaining 262 KB for:
// - Display framebuffer (~8 KB for 128x64 OLED)
// - Bluetooth stack (~20 KB)
// - Application logic (~10 KB)
// - Plenty of headroom
```

**Buffer Size Rationale:**
- 512 samples @ 5 kHz = **102.4ms of data**
- Enough to capture 2-3 full-auto shots
- Power of 2 for efficient indexing
- Small enough for low latency

### RP2040 DMA Specifications

**From RP2040 Datasheet:**
- **12 independent DMA channels** (we need 1)
- **Paced transfers** - can trigger from ADC FIFO
- **Chaining support** - auto-reconfigure for ping-pong
- **IRQ on complete** - notify CPU when buffer full
- **Max transfer rate** - 500 MB/s (way more than we need)

**Our Usage:**
- Sample rate: 5 kHz = 10 KB/s (0.002% of max throughput)
- Latency: 102ms (buffer fill time)
- CPU overhead: <1% (just process samples when buffer ready)

---

## 4. Shot Detection Algorithms

### 4.1 Time-Domain vs. Frequency-Domain

#### Time-Domain Detection (Recommended)

**Concept:** Monitor voltage over time, detect sudden drops

```
Voltage
  │
  │  ┌─────────────────────┐  Baseline
  │  │                     │
  │  │                     │
  │  │                     └─────  Recovery
  │  │                    ╱
  │  │                   ╱
  │  │                  ╱
  │  │                 ╱
  │  │                ╱
  │  │               ╱
  │  └──────────────┘  ← Shot detected!
  │
  └────────────────────────────────▶ Time
```

**Algorithm:**
```cpp
if (baseline - adc_value > THRESHOLD) {
    shot_detected = true;
}
```

**Pros:**
- Simple, fast, robust
- Easy to tune
- Low CPU usage
- Works perfectly for battery voltage monitoring

**Cons:**
- Needs threshold tuning per gun (solved with adaptive thresholds)

#### Frequency-Domain Detection (FFT) - NOT Recommended

**Concept:** Transform to frequency space, detect motor RPM peaks

**Why NOT to use it:**
1. **Massive overkill** - your voltage drop is 85%, not subtle
2. **High CPU cost** - FFT requires 128-512 sample blocks
3. **Added latency** - must accumulate samples before transform
4. **Complex tuning** - frequency bins, windowing, etc.
5. **No benefit** - time-domain already works perfectly

**When FFT *might* be useful (future research):**
- Motor health diagnostics (vibration analysis)
- Gun identification by motor frequency fingerprint
- Advanced anomaly detection

**Verdict:** Don't use FFT for shot counting. Save it for advanced features.

---

### 4.2 Noise Filtering Strategies

#### Problem: Motor Commutation Noise

**Observed in POC data (full-auto burst):**
```
Sample values during shot:
35, 239, 200, 9, 198, 212, 205, 240, 14, 230, 22, 40...
       ^^^       ^^^       ^^^       ^^^
      Spikes!   Spikes!   Spikes!   Spikes!
```

**Cause:** Brushed motor arcing creates electrical noise
**Effect:** ADC reads saturate (255) or drop to zero randomly

#### Solution 1: Moving Average (Basic)

```cpp
float moving_average(float new_sample) {
    static float buffer[WINDOW_SIZE];
    static int index = 0;
    
    buffer[index] = new_sample;
    index = (index + 1) % WINDOW_SIZE;
    
    float sum = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / WINDOW_SIZE;
}
```

**Pros:** Simple, smooths noise
**Cons:** 
- Requires buffer (25 floats = 100 bytes RAM)
- All samples weighted equally
- Introduces lag proportional to window size

#### Solution 2: Exponential Moving Average (Better)

```cpp
float ema(float new_sample) {
    static float filtered = 0;
    const float ALPHA = 0.1f;  // Smoothing factor
    
    filtered = ALPHA * new_sample + (1 - ALPHA) * filtered;
    return filtered;
}
```

**Pros:** 
- **No buffer needed!** (zero RAM overhead)
- Recent samples weighted more heavily
- Faster response than moving average

**Cons:** 
- Still has some lag
- Doesn't reject sudden spikes well

#### Solution 3: Median Filter (Best for Spikes)

```cpp
float median_filter(float new_sample) {
    static float buffer[5];
    static int index = 0;
    
    buffer[index] = new_sample;
    index = (index + 1) % 5;
    
    float sorted[5];
    memcpy(sorted, buffer, 5 * sizeof(float));
    insertion_sort(sorted, 5);
    
    return sorted[2];  // Middle value
}
```

**Pros:**
- **Perfect for killing single spikes**
- Preserves sharp edges (shot transitions)
- Non-linear (can't be replicated with averaging)

**Cons:**
- Requires small buffer (5 floats = 20 bytes)
- Needs sorting (but only 5 elements, so fast)

#### Solution 4: Digital Low-Pass Filter (Most Efficient)

**IIR (Infinite Impulse Response) Butterworth Filter:**

```cpp
class LowPassFilter {
private:
    float x_prev = 0;  // Previous input
    float y_prev = 0;  // Previous output
    
    // Coefficients for 100 Hz cutoff @ 5 kHz sample rate
    static constexpr float A0 = 0.06745527;
    static constexpr float A1 = 0.06745527;
    static constexpr float B1 = -0.86508946;
    
public:
    float process(float x) {
        float y = A0 * x + A1 * x_prev - B1 * y_prev;
        x_prev = x;
        y_prev = y;
        return y;
    }
};
```

**Pros:**
- **Mathematically optimal** frequency response
- Minimal CPU (3 multiplies, 2 additions)
- Minimal RAM (2 floats = 8 bytes)
- Smooth, predictable behavior

**Cons:**
- Requires coefficient calculation (use online tools)
- Harder to understand conceptually

#### Recommended Hybrid Approach

**Two-stage filtering:**

```
Raw ADC → Median Filter → Low-Pass Filter → Shot Detector
          (kill spikes)   (smooth noise)
```

**Implementation:**
```cpp
class VoltageFilter {
private:
    MedianFilter median{5};      // 5-sample median
    LowPassFilter lpf{100};      // 100 Hz cutoff
    
public:
    float process(uint16_t raw_adc) {
        float despiked = median.process(raw_adc);
        float smoothed = lpf.process(despiked);
        return smoothed;
    }
};
```

**Workflow:**
1. **Median (5 samples = 1ms @ 5kHz):** Removes 255 ADC spikes
2. **Low-pass (100 Hz cutoff):** Smooths remaining jitter
3. **Result:** Clean signal preserving shot edges

**CPU cost:** ~10 arithmetic operations per sample = negligible @ 5 kHz

---

### 4.3 Adaptive Threshold Detection

#### Problem: Different Guns Have Different Signatures

**Gun A (high-torque motor):**
- Baseline: 180 ADC
- Shot drop: 50% (90 ADC)

**Gun B (high-speed motor):**
- Baseline: 177 ADC
- Shot drop: 85% (26 ADC)

**Battery state affects baseline:**
- Full charge: 185 ADC baseline
- Half charge: 165 ADC baseline
- Low charge: 145 ADC baseline

#### Solution: Self-Tuning Adaptive Thresholds

```cpp
class AdaptiveShotDetector {
private:
    float baseline = 0;
    float shot_threshold = 0;
    
    enum State { IDLE, DROPPING, RECOVERING };
    State state = IDLE;
    uint32_t samples_since_event = 0;
    
    // Tuning parameters (based on POC data)
    static constexpr float THRESHOLD_RATIO = 0.45f;  // 45% drop
    static constexpr float RECOVERY_RATIO = 0.90f;   // 90% recovered
    static constexpr uint32_t MIN_SHOT_GAP = 50;     // 10ms @ 5kHz
    static constexpr uint32_t MAX_SHOT_TIME = 250;   // 50ms timeout
    
public:
    void update_baseline(float adc_value) {
        // Only update baseline when idle (not shooting)
        if (state == IDLE) {
            // Slow exponential tracking
            baseline = baseline * 0.99f + adc_value * 0.01f;
        }
        
        // Adaptive threshold: 45% of current baseline
        shot_threshold = baseline * THRESHOLD_RATIO;
    }
    
    bool process_sample(float adc_value) {
        update_baseline(adc_value);
        
        bool shot_detected = false;
        float drop = baseline - adc_value;
        
        switch (state) {
            case IDLE:
                // Detect sudden drop below threshold
                if (drop > shot_threshold) {
                    state = DROPPING;
                    shot_detected = true;  // Count shot NOW
                    samples_since_event = 0;
                }
                break;
                
            case DROPPING:
                samples_since_event++;
                
                // Wait for voltage to start rising
                if (adc_value > (baseline - shot_threshold * RECOVERY_RATIO)) {
                    state = RECOVERING;
                }
                
                // Timeout protection (stuck in drop)
                if (samples_since_event > MAX_SHOT_TIME) {
                    state = IDLE;
                }
                break;
                
            case RECOVERING:
                samples_since_event++;
                
                // Back to baseline?
                if (adc_value >= baseline * RECOVERY_RATIO) {
                    // Enforce minimum gap before next shot
                    if (samples_since_event >= MIN_SHOT_GAP) {
                        state = IDLE;
                    }
                }
                
                // Timeout protection
                if (samples_since_event > MAX_SHOT_TIME * 2) {
                    state = IDLE;
                }
                break;
        }
        
        return shot_detected;
    }
};
```

**Benefits:**
- **No manual calibration needed**
- Adapts to battery drain over time
- Works with different gun models automatically
- Handles temperature variations
- Self-corrects if thresholds drift

**Tuning parameters from POC data:**
- `THRESHOLD_RATIO = 0.45`: Shots drop 45-85%, so 45% catches all
- `RECOVERY_RATIO = 0.90`: Wait for 90% recovery before next shot
- `MIN_SHOT_GAP = 50`: 10ms minimum (faster than any airsoft gun)
- `MAX_SHOT_TIME = 250`: 50ms timeout (longer than any shot duration)

---

### 4.4 Machine Learning - NOT Recommended

#### What It Would Look Like

**Approach:**
1. Collect thousands of labeled shots (semi, auto, different guns)
2. Train neural network or SVM classifier
3. Deploy model using TensorFlow Lite Micro
4. Feed 50-sample windows to model for classification

**Pros:**
- Could handle wildly different guns without manual tuning
- Might detect anomalies (jams, double-feeds)
- Could classify shot types
- Cool factor

**Cons:**
- **Massive overkill** for your data (85% voltage drop is obvious)
- Requires hundreds of labeled training examples
- Slower inference than threshold detection
- RAM hungry (model weights + inference buffers)
- Harder to debug ("why did it miss that shot?" → "🤷 neural net")
- Complex toolchain (training, quantization, deployment)

#### Verdict: Don't Use ML for Shot Counting

**Why?**

Your shot signature is **textbook** threshold detection:
```
Baseline: 177 ADC
Shot:     13 ADC
Drop:     164 ADC units (85%!)
```

This is like:
- Using ChatGPT to calculate 2 + 2
- Using cruise control to drive 10 feet
- Using nuclear bomb to kill mosquito

**When ML *might* make sense (future):**
- Anomaly detection (motor wear, jams)
- Predictive maintenance (detect subtle degradation)
- Multi-gun classification (auto-detect which gun is connected)
- After 1000+ hours of data collection

For MVP shot counting: State machine + adaptive thresholds is the right tool.

---

## 5. Data Representation - uint16_t vs uint8_t

### Question: Should we compress ADC data to save RAM?

**Option 1: Keep as uint16_t (RP2040 native)**
```cpp
uint16_t dma_buffer[512];  // 1024 bytes
float voltage = buffer[i] * (3.3f / 4095.0f);
```

**Option 2: Scale to uint8_t**
```cpp
uint8_t dma_buffer[512];   // 512 bytes
uint16_t raw = adc_read();
buffer[i] = raw >> 4;      // 12-bit → 8-bit
float voltage = buffer[i] * (3.3f / 255.0f);
```

### Memory Savings Analysis

**RAM saved:** 512 bytes (0.19% of 264 KB total)

**Precision lost:**
- 12-bit resolution: 3.3V / 4096 = **0.8 mV per step**
- 8-bit resolution: 3.3V / 256 = **12.9 mV per step**
- **Loss: 16× worse precision**

### Does Precision Matter?

**For shot detection:** Probably not

Your shots drop from 177 → 13 ADC (8-bit scale):
- Drop magnitude: 164 ADC units
- Even with 16× worse precision, still detecting **10 ADC unit** change
- Plenty of signal-to-noise ratio

**For other features:** Maybe yes

- **Battery percentage estimation:** Needs high precision
- **Motor health diagnostics:** Subtle voltage changes matter
- **Future FFT analysis:** Needs precision for frequency resolution

### Recommendation: **Use uint16_t**

**Rationale:**
1. **RAM is cheap:** 512 bytes = 0.19% of total RAM (insignificant)
2. **Precision is free:** ADC already gives 12-bit, why throw away?
3. **Future-proofing:** Advanced features may need precision
4. **Simplicity:** One less conversion step = fewer bugs
5. **Performance:** Avoid extra bit-shift operation per sample

**Exception:** When logging to flash for long-term storage, *then* compress to 8-bit.

---

## 6. Gun Calibration Strategy

### Do We Need Per-Gun Calibration?

**What varies between guns:**

| Parameter | Likely Variance | Impact on Detection |
|-----------|-----------------|---------------------|
| Baseline voltage | ±5% (wire resistance) | Low (adaptive baseline handles it) |
| Shot drop depth | ±20% (motor type) | Medium (45% threshold catches all) |
| Recovery time | ±30% (spring strength) | Low (state machine adapts) |
| Full-auto ROF | ±50% (gear ratio) | Medium (affects shot overlap) |

### Approach 1: No Calibration (Adaptive Only)

**Pros:**
- Zero user friction
- Works out of box
- Self-adjusts to battery drain

**Cons:**
- May need conservative thresholds
- Might miss edge cases

### Approach 2: Auto-Calibration on First Use

**Process:**
```
1. User turns on device
2. Display shows: "Fire 5 semi-auto shots to calibrate"
3. System measures:
   - Average baseline voltage
   - Average shot drop depth
   - Average recovery time
4. Saves profile to flash
5. Uses adaptive thresholds around calibrated values
```

**Pros:**
- Optimized for specific gun
- One-time 30-second process
- Still adapts during use

**Cons:**
- User must remember to calibrate
- Needs UI flow

### Approach 3: Continuous Learning

**Concept:**
```cpp
class LearningDetector {
private:
    struct ShotProfile {
        float avg_drop;
        float avg_recovery_time;
        uint32_t sample_count;
    };
    
    ShotProfile profile;
    
public:
    void update_profile(float drop, float recovery) {
        // Exponential moving average of observed shots
        profile.avg_drop = profile.avg_drop * 0.95f + drop * 0.05f;
        profile.avg_recovery_time = profile.avg_recovery_time * 0.95f + recovery * 0.05f;
        profile.sample_count++;
        
        // After 100 shots, save to flash
        if (profile.sample_count % 100 == 0) {
            save_to_flash(profile);
        }
    }
};
```

**Pros:**
- No user action needed
- Continuously refines over time
- Adapts to gun aging/wear

**Cons:**
- More complex
- Potential for drift if bad data

### Recommendation: **Start with Approach 1, Add 2 if Needed**

**Phase 1 (MVP):**
- Pure adaptive thresholds
- No calibration UI
- Test with multiple guns to validate

**Phase 2 (If needed):**
- Add optional calibration menu item
- Log shot statistics to identify problem guns
- Implement auto-calibration if >5% missed shots detected

**Rationale:**
- YAGNI principle (You Aren't Gonna Need It)
- 85% voltage drop is so obvious, calibration may be unnecessary
- Can always add later based on real-world testing

---

## 7. Implementation Checklist

### Hardware Configuration

- [ ] **ADC Setup**
  - [ ] Configure ADC GPIO (GP26-GP29)
  - [ ] Set 12-bit resolution (default for RP2040)
  - [ ] Enable ADC clock
  - [ ] Configure FIFO (8-sample deep)

- [ ] **DMA Setup**
  - [ ] Allocate 2× 512-sample buffers (ping-pong)
  - [ ] Configure DMA channel 0
  - [ ] Set transfer size: 512 × uint16_t
  - [ ] Enable pacing from ADC FIFO
  - [ ] Configure chain to alternate buffers
  - [ ] Enable IRQ on transfer complete

- [ ] **Timer Setup**
  - [ ] Configure timer for 5 kHz (200µs period)
  - [ ] Trigger ADC conversion on timer overflow
  - [ ] Verify timing with oscilloscope/logic analyzer

### Software Components

- [ ] **Filter Chain**
  - [ ] Implement `MedianFilter` class (5-sample window)
  - [ ] Implement `LowPassFilter` class (100 Hz IIR)
  - [ ] Implement `VoltageFilter` wrapper (median → lowpass)
  - [ ] Unit test with synthetic noisy data

- [ ] **Shot Detector**
  - [ ] Implement `AdaptiveShotDetector` state machine
  - [ ] Tune threshold parameters from POC data
  - [ ] Add timeout protections
  - [ ] Unit test with real shot waveforms

- [ ] **DMA Handler**
  - [ ] Implement ping-pong buffer swap logic
  - [ ] Process samples in batches (512 at a time)
  - [ ] Track buffer overflow (data loss detection)
  - [ ] Add performance metrics (CPU %, latency)

- [ ] **Integration**
  - [ ] Wire shot count to display system
  - [ ] Add Bluetooth notification on shot
  - [ ] Implement shot logging to flash
  - [ ] Add diagnostic mode (raw ADC graph on display)

### Testing Plan

- [ ] **Bench Testing**
  - [ ] Verify 5 kHz sample rate (oscilloscope)
  - [ ] Confirm DMA buffer swaps correctly
  - [ ] Measure CPU usage (<1% target)
  - [ ] Test filter performance with synthetic noise

- [ ] **Semi-Auto Testing**
  - [ ] Single shot detection (should be 100%)
  - [ ] Rapid semi (5 shots/second)
  - [ ] Slow semi (1 shot/second)
  - [ ] Verify no false positives (idle for 1 minute)

- [ ] **Full-Auto Testing**
  - [ ] Short burst (5 rounds)
  - [ ] Long burst (30 rounds)
  - [ ] Maximum ROF (30+ RPS if possible)
  - [ ] Compare counted shots to manually counted shots

- [ ] **Multi-Gun Testing**
  - [ ] Test with high-torque motor gun
  - [ ] Test with high-speed motor gun
  - [ ] Test with stock motor gun
  - [ ] Verify adaptive thresholds work without calibration

- [ ] **Edge Cases**
  - [ ] Battery drain test (full to empty)
  - [ ] Temperature test (cold to warm)
  - [ ] Low battery voltage (<7V)
  - [ ] Motor stall (jam simulation)

### Performance Targets

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Sample rate accuracy | 5000 Hz ±1% | Oscilloscope on trigger pin |
| CPU usage | <1% | FreeRTOS task stats |
| Shot detection latency | <20ms | Timestamp shot vs. audio recording |
| Semi-auto accuracy | >99% | Manual count vs. device count |
| Full-auto accuracy | >95% | Manual count vs. device count |
| False positive rate | <1 per hour | Idle device monitoring |
| RAM usage | <10 KB | Linker map analysis |

---

## 8. Configuration Constants

### DMA Buffer Configuration

```cpp
namespace ADCConfig {
    // Sampling
    constexpr uint32_t SAMPLE_RATE_HZ = 5000;
    constexpr uint32_t SAMPLE_PERIOD_US = 1'000'000 / SAMPLE_RATE_HZ;  // 200µs
    
    // Buffers (ping-pong)
    constexpr uint32_t BUFFER_SIZE = 512;  // Must be power of 2
    constexpr uint32_t BUFFER_TIME_MS = (BUFFER_SIZE * 1000) / SAMPLE_RATE_HZ;  // 102ms
    
    // ADC hardware
    constexpr uint32_t ADC_GPIO = 26;  // GP26 = ADC0
    constexpr uint32_t ADC_BITS = 12;
    constexpr uint32_t ADC_MAX = (1 << ADC_BITS) - 1;  // 4095
    constexpr float ADC_VREF = 3.3f;
    
    // Voltage divider (11.1V LiPo → 3.3V max on ADC)
    constexpr float VDIV_R1 = 10000.0f;  // 10kΩ to battery
    constexpr float VDIV_R2 = 3900.0f;   // 3.9kΩ to ground
    constexpr float VDIV_RATIO = (VDIV_R1 + VDIV_R2) / VDIV_R2;  // 3.56
}
```

### Filter Configuration

```cpp
namespace FilterConfig {
    // Median filter (spike rejection)
    constexpr uint32_t MEDIAN_WINDOW = 5;  // 1ms @ 5kHz
    
    // Low-pass filter (noise smoothing)
    constexpr float LPF_CUTOFF_HZ = 100.0f;
    constexpr float LPF_SAMPLE_RATE = ADCConfig::SAMPLE_RATE_HZ;
    
    // IIR coefficients (100 Hz @ 5 kHz)
    // Calculated using: http://www.micromodeler.com/dsp/
    constexpr float LPF_A0 = 0.06745527f;
    constexpr float LPF_A1 = 0.06745527f;
    constexpr float LPF_B1 = -0.86508946f;
}
```

### Shot Detection Configuration

```cpp
namespace ShotConfig {
    // Thresholds (from POC data analysis)
    constexpr float BASELINE_NOMINAL_ADC = 2844.0f;  // 177 @ 8-bit = 2844 @ 12-bit
    constexpr float THRESHOLD_RATIO = 0.45f;  // Detect drops >45%
    constexpr float RECOVERY_RATIO = 0.90f;   // Consider recovered at 90%
    
    // Timing constraints
    constexpr uint32_t MIN_SHOT_GAP_SAMPLES = 50;   // 10ms @ 5kHz
    constexpr uint32_t MAX_DROP_SAMPLES = 250;      // 50ms timeout
    constexpr uint32_t MAX_RECOVERY_SAMPLES = 500;  // 100ms timeout
    
    // Baseline tracking
    constexpr float BASELINE_ALPHA = 0.01f;  // 1% weight to new samples (slow tracking)
    constexpr uint32_t BASELINE_INIT_SAMPLES = 1000;  // 200ms initialization
}
```

---

## 9. Known Limitations and Future Work

### Current Limitations

1. **Single ADC channel** - only monitors battery voltage
   - Future: Add current sensor for power calculation
   
2. **No shot energy calculation** - only counts shots
   - Future: Integrate voltage × current × time = energy per shot

3. **Fixed sample rate** - 5 kHz not adjustable at runtime
   - Future: Dynamic sample rate based on detected ROF

4. **No gun identification** - can't tell which gun is connected
   - Future: ML-based gun fingerprinting from voltage signature

5. **No jam detection** - doesn't detect stuck motors
   - Future: Timeout detection if voltage drops >100ms

### Future Research Areas

#### Advanced Diagnostics
- **Motor health monitoring** - detect bearing wear from vibration
- **Gear mesh analysis** - identify stripped gears from irregular cycling
- **Battery aging detection** - track internal resistance increase

#### Multi-Sensor Fusion
- **Accelerometer** - detect gun movement/recoil
- **Microphone** - audio-based shot confirmation
- **Current sensor** - power consumption analysis

#### Machine Learning (When Justified)
- Collect 1000+ hours of multi-gun data
- Train classification model for:
  - Gun type auto-detection
  - Failure mode prediction
  - User shooting pattern analysis

#### Connectivity
- **Real-time streaming** - live ADC data to phone for analysis
- **Cloud analytics** - aggregate statistics across users
- **Firmware updates** - tune detection parameters remotely

---

## 10. Reference Data

### POC Shot Signature Examples

**Semi-auto shot (log_15.csv, line 253-293):**
```
Line | Sample | Elapsed | Event
-----|--------|---------|------
252  | 179    | 2       | Baseline
253  | 98     | 2       | ← Trigger pull
254  | 236    | 2       | Motor noise
255  | 201    | 2       | 
256  | 192    | 2       | 
257  | 200    | 2       | 
258  | 250    | 2       | Motor noise spike
259  | 13     | 2       | ← Bottom of drop
260  | 13     | 2       | Motor running
...  | ...    | ...     | ...
276  | 87     | 2       | Starting recovery
277  | 160    | 2       | 
278  | 167    | 2       | 
...  | ...    | ...     | ...
285  | 177    | 2       | ← Back to baseline
286  | 177    | 2       | 
```

**Key metrics:**
- Drop duration: ~40ms (lines 253-276)
- Bottom voltage: 13 ADC (7% of baseline)
- Recovery duration: ~18ms (lines 276-285)
- Total shot time: ~58ms

**Full-auto burst (log_15.csv, lines 345-423):**
```
Line | Sample | Event
-----|--------|------
344  | 177    | Baseline
345  | 35     | ← Shot 1 start
346  | 239    | Noise spike
347  | 200    | 
348  | 9      | ← Bottom
...  | ...    | ...
368  | 153    | Partial recovery
369  | 168    | 
370  | 170    | 
371  | 172    | Still recovering...
372  | 168    | ← Shot 2 starts before full recovery!
373  | 171    | 
...  | ...    | ...
```

**Challenge:** Shots overlap, voltage never returns to baseline during burst.

**Solution:** State machine with RECOVERING state that can detect new drops.

---

## 11. Tools and Resources

### Development Tools
- **RP2040 SDK** - [pico-sdk](https://github.com/raspberrypi/pico-sdk)
- **DMA Examples** - [pico-examples/dma](https://github.com/raspberrypi/pico-examples/tree/master/dma)
- **ADC Examples** - [pico-examples/adc](https://github.com/raspberrypi/pico-examples/tree/master/adc)

### Analysis Tools
- **Oscilloscope** - Verify 5 kHz timing
- **Logic Analyzer** - Decode DMA state machine
- **Excel/Python** - Analyze CSV shot data
- **MATLAB/Octave** - Filter design (if needed)

### Filter Design Tools
- **MicroModeler DSP** - [micromodeler.com/dsp](http://www.micromodeler.com/dsp/)
- **SciPy Signal** - `scipy.signal.butter()` for IIR coefficients
- **Online Calculator** - [t-filter.engineerjs.com](https://www.t-filter.engineerjs.com/)

### Documentation
- **RP2040 Datasheet** - Chapter 4 (DMA), Chapter 5 (ADC)
- **ARM CMSIS-DSP** - Optimized filter functions (if needed)

---

## 12. Decision Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2025-01-30 | Use 5 kHz sample rate (not 1 kHz or 10 kHz) | 10× better than POC, low overhead, upgradeable to 10 kHz |
| 2025-01-30 | Use DMA double-buffering (not polling) | <1% CPU usage, zero data loss, real-time capable |
| 2025-01-30 | No FFT-based detection | Time-domain threshold detection sufficient |
| 2025-01-30 | No machine learning for MVP | 85% voltage drop is obvious, ML is overkill |
| 2025-01-30 | Two-stage filtering (median + IIR) | Kills motor spikes while preserving shot edges |
| 2025-01-30 | Adaptive thresholds (no manual calibration) | Self-tunes to gun, battery, temperature |
| 2025-01-30 | Keep uint16_t ADC data (not uint8_t) | 512 bytes RAM savings insignificant, precision useful |
| 2025-01-30 | Start without calibration UI | Test if adaptive detection works for all guns first |

---

## 13. Next Steps

### Immediate (Week 1)
1. Implement DMA double-buffer infrastructure
2. Implement median + low-pass filter chain
3. Implement adaptive shot detector state machine
4. Bench test with POC CSV data playback

### Short-term (Week 2-3)
5. Integrate with display system
6. Add Bluetooth shot notifications
7. Field test with semi-auto
8. Field test with full-auto

### Medium-term (Week 4+)
9. Test with multiple gun models
10. Tune thresholds based on real data
11. Add diagnostic mode (live ADC graph)
12. Implement shot logging to flash

### Long-term (Future)
13. Explore 10 kHz sampling if 5 kHz insufficient
14. Add current sensor for power measurement
15. Research ML-based gun identification
16. Implement predictive maintenance features

---

## Appendices

### Appendix A: RP2040 ADC Specifications

- **Resolution:** 12-bit (0-4095)
- **Sample rate:** Up to 500 ksps
- **Channels:** 5 (4 external GPIOs + 1 internal temp sensor)
- **FIFO depth:** 8 samples
- **DMA support:** Yes, paced transfers
- **Input range:** 0-3.3V (no negative voltages!)
- **Input impedance:** ~100 kΩ
- **Conversion time:** ~2µs @ 48 MHz ADC clock

### Appendix B: Voltage Divider Calculation

**Given:**
- Battery: 11.1V nominal (3S LiPo, 9.0V-12.6V range)
- ADC max: 3.3V
- Required ratio: 11.1V / 3.3V = 3.36×

**Resistor selection:**
```
R1 = 10 kΩ (to battery)
R2 = 3.9 kΩ (to ground)
Ratio = (R1 + R2) / R2 = 13.9 / 3.9 = 3.56×

Max voltage on ADC = 12.6V / 3.56 = 3.54V ⚠️ (slightly over!)
Better: R1 = 10kΩ, R2 = 3.3kΩ → ratio = 4.03× → 3.13V max ✅
```

**Recommendation:** Use 10kΩ + 3.3kΩ for safety margin.

### Appendix C: CPU Usage Estimation

**DMA overhead:**
- Buffer swap IRQ: ~10µs every 102ms = 0.01% CPU
- Sample processing: 512 samples @ 20 ops each = 10,240 ops
- At 133 MHz clock: 10,240 / 133,000,000 = 0.008% CPU per buffer
- Total: **0.018% CPU** (negligible!)

**Comparison to polling:**
- 500 Hz polling: `analogRead()` every 2ms = 500µs/s = 0.05% CPU
- But blocks other tasks during read!
- DMA is 3× more efficient AND non-blocking

---

**End of Document**
