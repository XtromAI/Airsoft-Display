# Hardware Timers, Interrupts, and Display Updates on Embedded Systems

## Should We Use Interrupts for Display Updates?

**Short answer:** No, you should not perform display updates directly inside an interrupt handler.

### Why Not?
- **Display updates are slow:** Drawing to a display and sending data over SPI can take several milliseconds. This is a long time for an interrupt context and can block other, more critical interrupts (like ADC sampling or DMA completion).
- **Interrupts should be short:** Best practice is to keep interrupt handlers as short and fast as possible—set a flag, increment a counter, or trigger a lightweight event. Heavy operations (like display updates, memory copies, or complex logic) should be done in the main loop or a dedicated thread/task.
- **Potential for deadlocks or race conditions:** If your display update code uses mutexes or interacts with shared data, running it in an interrupt context can lead to deadlocks or data corruption, since interrupts can preempt code holding a lock.
- **Debugging is harder:** Long or complex interrupt handlers make debugging and timing analysis much more difficult.

### Best Practice
Use the timer interrupt (or callback) only to set a flag or post an event. The main loop then checks this flag and performs the display update in normal (non-interrupt) context. This approach combines precise timing with safe, non-blocking code.

---

## What About Having the Hardware Timer Call the Display Loop Directly?

**Short answer:** Also not recommended.

### Why?
- **Same reasons as above:** Display updates are slow and can block other interrupts.
- **Interrupt context is not safe for complex operations:** Mutexes, shared data, and other system resources are not always safe to use in an interrupt context.
- **System stability:** Keeping heavy work out of interrupts makes your firmware more robust and easier to maintain.

### Summary
The standard, robust, and safe method for embedded systems is:
- Use a hardware timer interrupt to set a flag at the desired update rate.
- In your main loop, check the flag and perform the display update when set.

This gives you precise timing without the risks and downsides of running heavy code in an interrupt context.

If you need even more precise timing, consider using a real-time operating system (RTOS) with task scheduling, but for bare-metal or simple firmware, the flag method is best practice.
