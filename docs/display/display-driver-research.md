

# **Minimal U8G2 Graphics Implementation for HiLetgo SH1107 OLED on Raspberry Pi Pico via SPI**

## **Abstract**

This report details a minimal C++ implementation for driving a 128x128 HiLetgo SH1107 OLED display using the U8G2 graphics library on a Raspberry Pi Pico. It focuses on leveraging the Pico SDK's hardware SPI capabilities and custom Hardware Abstraction Layer (HAL) callbacks for efficient communication. The report covers detailed pinout configuration, U8G2 constructor selection for full-graphics, implementation of core drawing primitives, and a comprehensive analysis of 8x8 and 16x16 font options for monochrome displays. The provided code structure ensures a robust and extensible foundation for embedded graphics projects.

## **1\. Introduction**

The integration of graphical displays into embedded systems has become increasingly prevalent, driven by the demand for intuitive user interfaces and enhanced data visualization. This report focuses on a specific implementation challenge: driving a 128x128 HiLetgo SH1107 OLED display using the U8G2 graphics library on a Raspberry Pi Pico via the Serial Peripheral Interface (SPI). The objective is to provide a functional and minimal C++ implementation that supports full-graphics capabilities, drawing primitives, and offers guidance on font selection, catering to experienced embedded systems developers and advanced hobbyists.

### **1.1. Overview of the HiLetgo SH1107 OLED Display**

The SH1107 is a robust single-chip CMOS OLED/PLED driver and controller designed for organic/polymer light-emitting diode dot-matrix graphic display systems.1 A key feature of this controller is its embedded 128x128-bit Static Random-Access Memory (SRAM), which serves as an internal frame buffer. This internal memory allows the display controller to manage the full display content directly, a crucial aspect for efficient graphics rendering. These displays are commonly available in both 128x64 and 128x128 pixel resolutions, with the present implementation specifically targeting the 128x128 variant.2

The SH1107 controller is versatile, supporting various communication interfaces, including 3-wire and 4-wire SPI, as well as I2C.1 The current implementation specifically utilizes the 4-wire SPI protocol as requested. These OLED displays are highly regarded for their high contrast ratios, wide viewing angles, and low power consumption, making them an excellent choice for compact, battery-powered applications.2 Typically, they operate with a logic supply voltage of 3.3V, which is directly compatible with the General Purpose Input/Output (GPIO) voltage levels of the Raspberry Pi Pico.2

The presence of the SH1107's embedded 128x128 SRAM is directly aligned with the requirement for full-graphics capabilities. This internal frame buffer is instrumental because it allows the U8G2 library to operate in its "full buffer mode" (often denoted by \_F in U8G2 constructors). In this mode, the entire display content is rendered into a local buffer within the microcontroller's RAM (in this case, the Raspberry Pi Pico) and then transferred to the display's internal SRAM in a single, atomic operation. This method significantly reduces visual artifacts such as tearing or flicker that can occur with "page buffer mode," where only smaller sections of the display are updated at a time. The ability to perform a complete, flicker-free update is fundamental to achieving high-quality graphics.

### **1.2. Raspberry Pi Pico (RP2040) as the Embedded Platform**

The Raspberry Pi Pico, powered by the RP2040 microcontroller, serves as a robust and cost-effective platform for a wide array of embedded projects. The RP2040 features two identical Serial Peripheral Interface (SPI) controllers, SPI0 and SPI1, offering flexible options for peripheral connectivity.12 Furthermore, the RP2040 provides 26 multi-function GPIO pins, which can be configured for diverse roles, ranging from standard digital input/output to dedicated peripheral functions like SPI.14

The Pico C/C++ Software Development Kit (SDK) provides a comprehensive set of hardware abstraction libraries. Key among these are hardware/spi.h for managing SPI communication and hardware/gpio.h for direct GPIO control.12 These SDK functions abstract low-level register manipulations into more user-friendly function calls, simplifying the development process.21 Project builds for the Pico SDK are efficiently managed using CMake, a build system generator that allows for structured inclusion of source files and external libraries, ensuring a reproducible build process.5

A critical aspect of achieving high-performance graphics on embedded systems is efficient data transfer to the display. The Pico SDK's hardware/spi.h functions, particularly spi\_write\_blocking 12, enable direct and efficient blocking SPI transfers. This is paramount for achieving the high data rates necessary for rapid display updates, as it leverages the RP2040's dedicated hardware SPI peripheral, which is significantly faster than software-emulated (bit-banged) SPI.30 By utilizing the hardware SPI, the microcontroller can dedicate its processing power to other tasks or enter low-power states more effectively, rather than being burdened with bit-level timing control.

The user's specified pinout (SCL: GP14, SDA: GP15, DC: GP21, CS: GP13, RST: GP20) is not arbitrary; it aligns optimally with the RP2040's capabilities. A review of the RP2040's GPIO function table 17 reveals that GP14 is a primary pin for

SPI1 SCK (Serial Clock) and GP15 is a primary pin for SPI1 TX (Transmit, or MOSI \- Master Out Slave In). This direct mapping dictates the use of spi1 as the hardware SPI peripheral instance for this implementation. The remaining control pins—DC (GP21), CS (GP13), and RST (GP20)—will be configured as general-purpose I/O (SIO) pins, allowing for flexible software control. This careful selection and validation of pin assignments are fundamental to ensuring robust and efficient hardware communication.

### **1.3. Introduction to the U8G2 Graphics Library**

U8G2 (Universal 8-bit Graphics Library) is an open-source, highly versatile graphics library specifically designed for embedded systems featuring monochrome displays.31 It boasts extensive support for a vast array of display controllers, including the SH1107, SSD1306, and numerous others.3

The library provides a rich set of high-level drawing functions for common graphics primitives, such as individual pixels, lines, rectangles (both outlined and filled), circles (outlined and filled), and bitmaps.33 Beyond basic shapes, U8G2 offers comprehensive font management capabilities, including a large collection of pre-rendered fonts that cater to various aesthetic and size requirements.31

U8G2 supports various page buffer modes for memory optimization, alongside a "full buffer" mode. The full buffer mode, indicated by the \_F suffix in its constructor names, allocates a complete frame buffer in the microcontroller's RAM. This approach enables flicker-free updates for complex graphics by rendering the entire display content to an off-screen buffer before transferring it to the physical display.40 This is the preferred and most effective mode for achieving the requested "full-graphics capabilities." While U8x8, a component of U8G2, is a text-only, zero-RAM library that writes directly to the display, it lacks the graphics primitives and can be slower for full-screen updates.3 For the requirements of this report, U8G2's comprehensive graphics capabilities are paramount.

The explicit request for "full-graphics capabilities and primitives" necessitates the use of U8G2's full buffer mode. This mode, while consuming more RAM (approximately 2KB for a 128x128 monochrome display, calculated as 128 pixels \* 128 pixels / 8 bits per byte \= 2048 bytes), is crucial for rendering complex scenes, animations, and overlapping primitives without visible flicker.40 The Raspberry Pi Pico's RP2040 microcontroller, with its 264KB of RAM, has ample memory to accommodate this 2KB buffer, making the full buffer mode a negligible overhead for significantly improved visual quality and performance.

### **1.4. SPI Communication Protocol Essentials**

SPI (Serial Peripheral Interface) is a synchronous serial communication protocol widely employed in embedded systems for high-speed data transfer between a master device, such as the Raspberry Pi Pico, and one or more slave devices, like the SH1107 display.15 A typical 4-wire SPI configuration involves four dedicated signal lines: Serial Clock (SCLK), Master Out Slave In (MOSI), Master In Slave Out (MISO), and Chip Select (CS).46

For the SH1107 display, the 4-wire SPI interface specifically utilizes the SI (D1) pin as MOSI and the SCL (D0) pin as the SPI clock.1 It is important to note that the MISO pin is generally not used for OLED displays because they are typically write-only devices, meaning data flows predominantly from the microcontroller to the display.4 This simplifies the hardware setup as the MISO pin does not require explicit configuration or management for this application.

SPI communication timing is precisely governed by two parameters: Clock Polarity (CPOL) and Clock Phase (CPHA).12 These parameters define the specific SPI mode, ensuring proper synchronization between the master and slave devices. Data is typically transmitted Most Significant Bit (MSB) first.5 The Chip Select (CS) pin is active low, meaning the slave device is enabled and ready for communication when this pin is driven to a logic low state.1 The Data/Command (DC) pin, also known as A0 or RS, is crucial for the display controller, as it differentiates between commands (DC low) and display data (DC high) being sent over the MOSI line.1 Finally, the Reset (RST) pin is also active low and is used to initialize or reinitialize the display controller to a known state.48

The Waveshare Pico-OLED-1.3 wiki explicitly states that the SH1107 on their module operates in SPI Mode 3 (CPOL=1, CPHA=1).5 This specific detail is critical for correctly configuring the Raspberry Pi Pico's

spi\_set\_format function. The documentation clarifies that "SCL is high in idle and it starts to transmit data at the second edge," which directly corresponds to Mode 3 (0x11). This precise information eliminates guesswork and ensures reliable and stable communication with the display, as the microcontroller's SPI peripheral will generate signals perfectly matched to the display controller's expectations.

## **2\. Development Environment Setup**

Establishing a proper development environment is a prerequisite for any embedded systems project. For the Raspberry Pi Pico, this involves setting up the C/C++ SDK and configuring the build system.

### **2.1. Raspberry Pi Pico C/C++ SDK Installation and Configuration**

Developing applications for the Raspberry Pi Pico in C/C++ requires a specific toolchain and environment setup. The essential components include the Arm GNU Toolchain for cross-compilation, CMake for managing the build system, Python for scripting, and Git for source code version control.49 Visual Studio Code is a widely recommended Integrated Development Environment (IDE) due to its robust extensions that streamline Pico development.5

The Pico SDK itself is typically obtained by cloning its official GitHub repository: https://github.com/raspberrypi/pico-sdk.git.49 A crucial step in the setup process is setting the

PICO\_SDK\_PATH environment variable. This variable must point to the root directory of the SDK, allowing CMake to correctly locate the necessary build files and libraries during project configuration.5

All projects built with the Pico SDK utilize a CMakeLists.txt file. This file defines the project's properties, specifies the source files to be compiled, and manages external library dependencies.15 Within this

CMakeLists.txt, the pico\_sdk\_import.cmake file is included, and the pico\_sdk\_init() function is called to integrate the SDK's build functions into the project's build process.15 The consistent use of CMake and the

pico-sdk's build system across all Pico projects establishes a robust and reproducible development workflow. This standardization simplifies the integration of external libraries like U8G2, as the method for adding source files and linking dependencies is well-defined and predictable.

### **2.2. Integrating U8G2 into a Pico SDK Project with CMake**

Integrating a third-party library such as U8G2 into a Pico SDK project requires careful configuration within the project's CMakeLists.txt file. Since U8G2 does not come with a pre-built Pico SDK integration, its source files must be included directly into the project's build system.

A recommended approach involves placing the U8G2 library's source code, specifically the csrc directory from the U8G2 GitHub repository, within a dedicated subfolder of the project (e.g., lib/u8g2). Within the project's CMakeLists.txt, one can either use add\_subdirectory() if the U8G2 library itself contains a CMake file for its build, or more commonly, define an interface library using add\_library(u8g2\_lib INTERFACE). This is then followed by target\_sources(u8g2\_lib INTERFACE...) to explicitly list U8G2's source files and target\_include\_directories(u8g2\_lib INTERFACE...) to specify its header paths.23 Once the library is defined within CMake, the main executable must link against it using

target\_link\_libraries(${PROJECT\_NAME}... u8g2\_lib).22

It is important to recognize that the U8G2 library, while designed for high portability, requires a platform-specific Hardware Abstraction Layer (HAL) when used with the Pico SDK. This means that simply including the U8G2 source files is insufficient; custom callback functions for SPI communication and GPIO control must be implemented. These callbacks serve as a bridge, translating U8G2's generic hardware requests into specific calls to the Pico SDK's hardware\_spi and hardware\_gpio functions.31 This custom HAL is a core component of the minimal implementation, defining how U8G2 interacts with the RP2040's unique hardware architecture.

These custom HAL functions, which encapsulate the Pico-specific SPI and GPIO logic, will reside in separate C/C++ source files (e.g., u8g2\_hal\_pico.c). These files, along with the necessary U8G2 source files, must be explicitly added to the project's CMakeLists.txt using target\_sources. This ensures that they are properly compiled and linked into the final firmware, maintaining modularity and clarity within the project structure.

## **3\. Hardware Interfacing: Raspberry Pi Pico to SH1107**

Accurate hardware interfacing is critical for the reliable operation of the OLED display. This section details the pinout configuration and the initialization steps for the Raspberry Pi Pico's SPI peripheral and associated GPIOs.

### **3.1. Detailed SPI Pinout Configuration for SH1107**

The user has specified the following pin assignments for connecting the HiLetgo SH1107 display to the Raspberry Pi Pico:

* SCL (Serial Clock): GP14  
* SDA (Serial Data / MOSI): GP15  
* DC (Data/Command): GP21  
* CS (Chip Select): GP13  
* RST (Reset): GP20

To validate these assignments and ensure optimal performance, a detailed review of the RP2040 GPIO Function Select Table 17 was conducted. The analysis confirms the following:

* GP14 is a primary pin for SPI1 SCK.  
* GP15 is a primary pin for SPI1 TX (MOSI).  
* GP13 is a primary pin for SPI1 CSn.

This direct correspondence between the specified SCL (GP14) and SDA (GP15) pins and the SPI1 SCK and SPI1 TX functions on the RP2040 is a critical validation. It confirms that the optimal choice for this implementation is to utilize **SPI1** on the Raspberry Pi Pico, allowing for a direct, hardware-accelerated SPI communication path. The remaining pins, DC (GP21) and RST (GP20), do not have dedicated SPI1 peripheral functions on their specified GPIOs. Therefore, they will be configured as general-purpose I/O (SIO) pins and controlled directly via the Pico SDK's GPIO functions.17 This detailed mapping is fundamental for establishing a robust hardware connection.

The following table provides a clear, actionable reference for connecting the display to the Raspberry Pi Pico, based on the specified pinout and the RP2040's capabilities:

| Raspberry Pi Pico Pin (GPIO) | SH1107 Pin Name | Function on Pico | Notes |
| :---- | :---- | :---- | :---- |
| GP14 | SCL | SPI1 SCK | Hardware SPI Clock |
| GP15 | SDA (MOSI) | SPI1 TX | Hardware SPI Data Out (Master Out) |
| GP21 | DC | SIO | Data/Command control (Software GPIO) |
| GP13 | CS | SPI1 CSn | Hardware SPI Chip Select (Active Low) |
| GP20 | RST | SIO | Reset control (Software GPIO) |

### **3.2. Initializing Raspberry Pi Pico's Hardware SPI Peripheral**

The Raspberry Pi Pico's hardware SPI peripheral must be correctly initialized to ensure reliable communication with the SH1107 display. The Pico SDK provides dedicated functions for this purpose.

The spi\_init function (spi\_init(spi\_inst\_t \*spi, uint baudrate)) is used to initialize the chosen SPI instance, in this case, spi1.12 This function puts the SPI peripheral into a known state and enables it, and it must be called before any other SPI operations. Following initialization, the

spi\_set\_format function (spi\_set\_format(spi\_inst\_t \*spi, uint data\_bits, spi\_cpol\_t cpol, spi\_cpha\_t cpha, spi\_order\_t order)) is used to configure the specific SPI communication parameters.12 For the SH1107 display, particularly the Waveshare Pico-OLED-1.3 variant, the SPI timing is explicitly documented as Mode 3 (CPOL=1, CPHA=1) with data transmitted Most Significant Bit (MSB) first.5 This precise configuration is crucial for stable communication and ensures that the Pico's SPI signals align perfectly with the display controller's expectations.

To maximize display update speed, especially when operating in full buffer mode, a higher baud rate should be selected. The RP2040 is capable of high SPI speeds 46, and a common baud rate for OLEDs is 8 MHz or higher.48 This can be set using

spi\_set\_baudrate 12 or directly within

spi\_init. For sending data to the display, the spi\_write\_blocking function (spi\_write\_blocking(spi\_inst\_t \*spi, const uint8\_t \*src, size\_t len)) is employed. This function efficiently writes a specified length of bytes from a source buffer to the SPI device in a blocking manner, leveraging the hardware peripheral for maximum throughput.12

### **3.3. GPIO Control for Display Command/Data (DC), Chip Select (CS), and Reset (RST)**

Beyond the core SPI data lines, several GPIO pins are required for controlling the SH1107 display's operation modes. These control pins—DC, CS, and RST—will be managed directly using the Pico SDK's GPIO functions.

Each control pin must first be initialized using gpio\_init(uint gpio).21 Following initialization, their direction must be set as output using

gpio\_set\_dir(uint gpio, bool out).21 For pins that are part of the SPI peripheral (like CS), their function should also be explicitly set using

gpio\_set\_function(uint gpio, enum gpio\_function fn) to GPIO\_FUNC\_SPI for the hardware SPI function, or GPIO\_FUNC\_SIO for general-purpose software control.17

The states of these control pins are manipulated using gpio\_put(uint gpio, bool value).21 The Chip Select (CS) pin is active low 1, meaning it must be driven low to enable communication with the display and high to disable it. The Data/Command (DC) pin is set high when sending display data and low when sending commands to the controller.1 The Reset (RST) pin is also active low 48 and is used to perform a hardware reset of the display. A typical reset sequence involves pulling the RST pin low, pausing for a short duration (e.g., a few milliseconds), and then pulling it high again to bring the display out of reset.2 These precise GPIO state controls are directly utilized by the U8G2 Hardware Abstraction Layer callbacks.

## **4\. U8G2 Library Porting and Custom Hardware Abstraction Layer (HAL)**

Integrating the U8G2 library with the Raspberry Pi Pico's SDK requires a custom Hardware Abstraction Layer (HAL). This layer acts as an intermediary, translating U8G2's generic hardware requests into specific Pico SDK function calls.

### **4.1. Understanding U8G2's Porting Mechanism (Callbacks)**

U8G2 achieves its high portability across various microcontroller platforms through a well-defined callback mechanism. To port the library to a new Microcontroller Unit (MCU), two primary callback functions must be provided: a "uC specific" GPIO and Delay callback, and a "u8x8 byte communication callback".45 Both functions conform to the

typedef uint8\_t (\*u8x8\_msg\_cb)(u8x8\_t \*u8x8, uint8\_t msg, uint8\_t arg\_int, void \*arg\_ptr); prototype and are passed as arguments to the U8G2 setup function during initialization.45

The GPIO and Delay callback is responsible for handling messages related to general-purpose I/O operations and timing delays. This includes U8X8\_MSG\_DELAY\_ messages, which require busy-wait delays for precise timing, and U8X8\_MSG\_GPIO\_ messages, which are used for setting or resetting the states of control pins such as SCL/SDA (if bit-banging), Reset, CS, and DC.45

The byte communication callback, on the other hand, manages messages directly related to the byte-level data transfer with the display controller. These messages include U8X8\_MSG\_BYTE\_START\_TRANSFER (to initiate a data transfer, typically by asserting the Chip Select line), U8X8\_MSG\_BYTE\_SEND (to transmit actual data bytes), and U8X8\_MSG\_BYTE\_END\_TRANSFER (to conclude a data transfer, typically by de-asserting the Chip Select line).45 This callback mechanism is fundamental to U8G2's design for platform independence. By implementing these two functions, the library effectively learns how to interact with the RP2040's specific hardware, creating a clear separation between platform-dependent code and the main graphics library logic. This modularity is a robust engineering practice that enhances code reusability and maintainability.

### **4.2. Implementing the Custom u8x8\_byte\_cb for RP2040 SPI**

The u8x8\_byte\_cb is the core of the SPI communication layer for U8G2. Its implementation for the RP2040 will leverage the Pico SDK's hardware SPI capabilities.

Within this callback function, the U8X8\_MSG\_BYTE\_SEND message is crucial. When this message is received, it indicates that U8G2 has data bytes ready to be transmitted to the display. For optimal performance, the Pico SDK's spi\_write\_blocking function is utilized here.12 This function efficiently sends multiple bytes in a single call, which is significantly more performant than sending individual bytes one at a time. This efficiency is particularly critical for full framebuffer updates, where large amounts of pixel data need to be transferred rapidly to the display.

The U8X8\_MSG\_BYTE\_START\_TRANSFER message signals the beginning of a data transfer sequence. In response, the Chip Select (CS) pin, which is active low, is asserted (pulled low) to enable communication with the display.31 Conversely, the

U8X8\_MSG\_BYTE\_END\_TRANSFER message indicates the completion of the transfer, prompting the de-assertion of the CS pin (pulled high) to disable the display.31 Finally, the

U8X8\_MSG\_BYTE\_SET\_DC message is handled by setting the Data/Command (DC) pin to the appropriate state (high for data, low for command) before the actual byte transfer, ensuring the display controller interprets the subsequent bytes correctly.31

### **4.3. Implementing the Custom u8x8\_gpio\_and\_delay\_cb for RP2040 GPIOs and Delays**

The u8x8\_gpio\_and\_delay\_cb is responsible for managing general GPIO control and precise timing delays required by the display and the U8G2 library.

When the U8X8\_MSG\_GPIO\_AND\_DELAY\_INIT message is received, it signifies the initialization phase. During this phase, the necessary GPIO pins (CS, DC, RST, and the SPI pins if using software SPI, though here we use hardware SPI) are configured. This involves calling gpio\_init to enable the GPIO, gpio\_set\_dir to set its direction as output, and gpio\_set\_function to assign its specific peripheral function (e.g., GPIO\_FUNC\_SIO for control pins, GPIO\_FUNC\_SPI for hardware SPI pins).17

For delay messages, the Pico SDK provides accurate timing functions. U8X8\_MSG\_DELAY\_MILLI is handled by calling sleep\_ms(arg\_int), which provides delays in milliseconds.21 For finer-grained delays,

U8X8\_MSG\_DELAY\_NANO and U8X8\_MSG\_DELAY\_100NANO can be implemented using sleep\_us(1) for the 100-nanosecond delay or even asm("NOP") for very short nanosecond-scale delays, although these are less critical when using hardware SPI.31 Accurate timing for these delays is crucial for the display's initialization sequence, as incorrect timing can lead to display initialization failures or erratic behavior.2

For direct GPIO control messages like U8X8\_MSG\_GPIO\_CS, U8X8\_MSG\_GPIO\_DC, and U8X8\_MSG\_GPIO\_RESET, the gpio\_put(pin, value) function is used. This function sets the specified GPIO pin to a high or low state based on the arg\_int parameter (typically 1 for high, 0 for low).17 This precise control over the display's control lines ensures that commands and data are properly synchronized and that the display can be reset as needed.

### **4.4. Selecting the U8G2 Constructor for SH1107 128x128 Full Buffer SPI**

The selection of the appropriate U8G2 constructor is fundamental, as it defines the display controller, resolution, buffering mode, and communication protocol. U8G2 constructor names follow a consistent pattern: U8G2\_\<Controller\>\_\<Resolution\>\_\<BufferMode\>\_\<Communication\>\_\<Interface\>.44

For the HiLetgo SH1107 display with 128x128 resolution, utilizing full-graphics capabilities and hardware SPI, the most suitable constructor is U8G2\_SH1107\_128X128\_F\_4W\_HW\_SPI.3 The

\_F suffix denotes "Full Buffer" mode, which is paramount for achieving the requested "full-graphics capabilities." This mode allocates a complete 2KB framebuffer in the Pico's RAM (128 pixels \* 128 pixels / 8 bits per pixel \= 2048 bytes). This memory allocation allows for complex drawing operations, smooth animations, and overlapping primitives without visible flicker, as the entire image is composed in memory before being sent to the display in a single, atomic transfer.40 The Raspberry Pi Pico's 264KB of RAM is more than sufficient to accommodate this buffer, making the full buffer mode a negligible overhead for significantly improved visual quality.

The constructor also takes arguments for display rotation and the specific GPIO pins connected to the CS, DC, and Reset lines.3 The rotation parameter allows the display content to be oriented as needed (e.g.,

U8G2\_R0 for no rotation, U8G2\_R1 for 90-degree rotation, etc.).

The following table summarizes the key U8G2 constructor parameters for the specified display and pinout:

| Parameter Type | Value / Description |
| :---- | :---- |
| Constructor | U8G2\_SH1107\_128X128\_F\_4W\_HW\_SPI |
| Rotation | U8G2\_R0 (No rotation, can be U8G2\_R1, etc.) |
| CS Pin | GP13 |
| DC Pin | GP21 |
| Reset Pin | GP20 |

### **4.5. SH1107-Specific Initialization Sequence Adjustments**

While U8G2 provides a generic constructor for the SH1107 controller, specific display modules, such as the HiLetgo/Waveshare variant, may benefit from fine-tuning their initialization parameters. These modules might have slightly different optimal settings for characteristics like contrast or multiplex ratio compared to U8G2's default values. Adjusting these parameters can significantly improve display quality, affecting aspects like brightness uniformity and reducing any potential buzzing sounds.41

A common approach to identify these optimal settings is to compare U8G2's internal initialization sequence with those found in other well-established drivers, such as Adafruit's SH1107 driver (Adafruit\_SH1107.cpp).15 The SH1107 datasheet also provides detailed information on various commands and their recommended parameters, including

SH110X\_DISPLAYOFF (0xAE), SH110X\_SETDISPLAYCLOCKDIV (0xD5, 0x51), SH110X\_SETCONTRAST (0x81, 0x4F), SH110X\_DCDC (0xAD, 0x8A), SH110X\_SEGREMAP (0xA0), SH110X\_COMSCANINC (0xC0), and SH110X\_SETDISPSTARTLINE (0xDC, 0x0).1

A GitHub discussion on U8G2 for SH1107 displays highlights disparities in initialization sequences, specifically regarding contrast (0x2F vs 0x4F) and multiplex ratio (0x7F vs 0x3F).41 It suggests that after calling

u8g2.begin(), additional commands can be sent using u8g2.sendF() to match Adafruit's sequence. For instance, u8g2.sendF("ca", 0x81, 0x4f); and u8g2.sendF("ca", 0xa8, 0x7f); can be used to set the contrast and multiplex ratio respectively, aligning with Adafruit's known good settings.41 This fine-tuning ensures full compatibility and optimal visual performance for the specific display module.

## **5\. Graphics Primitives and Text Rendering**

With the U8G2 library successfully ported and the display initialized, the focus shifts to leveraging its powerful graphics and text rendering capabilities.

### **5.1. Basic Drawing Operations (Pixels, Lines, Rectangles, Circles)**

U8G2 provides a comprehensive set of functions for drawing fundamental graphics primitives. These include drawPixel(x, y) for individual pixels, drawLine(x0, y0, x1, y1) for drawing lines between two points, drawRect(x, y, w, h) for outlined rectangles, and drawBox(x, y, w, h) for filled rectangles.33 For circular shapes,

drawCircle(x0, y0, rad) creates an outlined circle, while drawDisc(x0, y0, rad) draws a filled circle.33

A fundamental aspect of U8G2's operation in full buffer mode is the sequence of drawing operations. All drawing commands are performed on an off-screen buffer, which is an in-memory representation of the display content. The process typically involves calling clearBuffer() to clear this internal memory, followed by a series of drawing primitive calls to compose the desired image, and finally sendBuffer() to transfer the entire contents of this internal memory to the physical display.41 This atomic update mechanism is fundamental to achieving flicker-free full-graphics, preventing partial screen redraws from being visible and ensuring a smooth visual experience.

### **5.2. Managing the Full Graphics Buffer (clearBuffer(), sendBuffer())**

Effective management of the full graphics buffer is paramount for achieving smooth and responsive display updates. The core of this management lies in the judicious use of clearBuffer() and sendBuffer().

The clearBuffer() function is invoked to erase all pixels in the internal memory buffer, effectively preparing a blank canvas for the next frame.41 After all desired drawing operations (text, shapes, bitmaps) have been completed on this buffer, the

sendBuffer() function is called. This command transfers the entire contents of the internal memory to the display's physical memory, making the newly rendered frame visible.41

For applications requiring dynamic content or animations, this clearBuffer() \-\> drawing primitives \-\> sendBuffer() sequence is typically executed within a continuous loop. The frequency of sendBuffer() calls directly impacts the perceived smoothness of the display. To maximize the refresh rate, it is crucial to avoid unnecessary delays within the main loop.41 Optimizing the drawing logic to be as efficient as possible between

clearBuffer() and sendBuffer() calls will ensure the highest possible frame rate, leading to a more fluid and professional-looking graphical output. Any blocking delays, if absolutely necessary, should be carefully managed to minimize their impact on the display's responsiveness.

### **5.3. Font Options for Monochrome OLED Displays**

U8G2 offers extensive font support, providing a wide array of pre-rendered fonts suitable for monochrome OLED displays. The choice of font significantly influences both the visual appeal and the memory footprint of the application.

U8G2 fonts are categorized by their "Capital A Height" in pixels.37 Each font name also includes suffixes that denote specific characteristics 37:

* t: Transparent font (does not use a background color).  
* h: All glyphs have a common height.  
* m: All glyphs have common height and width (monospace).  
* 8: All glyphs fit into an 8x8 pixel box.  
* f: The font includes a full character set (up to 256 glyphs).  
* r: The font includes a reduced character set.  
* n: The font includes only numbers.

For text display, the setFont(font\_name) function is used to set the active font, and drawString(x, y, "text") or drawStr(x, y, "text") is used to render a string at specified coordinates.41 The

setCursor(x, y) function can also be used to set the text drawing position.60

When designing a minimal implementation, selecting fonts with \_tr (transparent, reduced character set) or \_tn (transparent, numbers only) suffixes can significantly reduce the flash memory footprint. This is a direct optimization for embedded systems where memory resources may be constrained, as these variants include only the most commonly needed ASCII characters, avoiding the overhead of full character sets or larger fonts.

#### **5.3.1. Research on 8x8 Pixel Fonts**

For "8x8 pixel fonts," it is important to note that U8G2 categorizes fonts primarily by "Capital A Height." While some fonts may have "8x8" in their name or purpose suffix 37, it often refers to the bounding box rather than strict pixel dimensions. For true 8x8 pixel glyphs or fonts with a capital A height of 8 pixels, several options are available within the U8G2 library.37

Examples of suitable fonts for compact display and minimal resource usage include:

* u8g2\_font\_ncenB08\_tr: A commonly used 8-pixel height font, often seen in examples.41 The  
  \_tr suffix indicates a transparent, reduced character set, making it memory-efficient.  
* u8g2\_font\_micro\_tr: A very small font (5 pixels Capital A Height) that is highly compact and suitable when screen real estate is at a premium.37  
* u8g2\_font\_4x6\_tr: Another compact option (5 pixels Capital A Height) that offers good readability for its size.37

When selecting an 8x8 font, the \_tr or \_tn variants are generally preferred for a minimal implementation, as they reduce flash memory consumption by including only essential characters.

#### **5.3.2. Research on 16x16 Pixel Fonts**

For "16x16 pixel fonts," U8G2 provides a range of options with a "Capital A Height" typically ranging from 13 to 16 pixels.61 These fonts offer improved readability compared to their 8x8 counterparts but naturally consume more flash memory.

Examples of suitable 16-pixel height fonts that balance readability with resource considerations include:

* u8g2\_font\_logisoso16\_tr: This font offers a clean, readable appearance with a 16-pixel capital A height. The \_tr suffix ensures a transparent background and a reduced character set, optimizing for memory usage.  
* u8g2\_font\_inr16\_tr / u8g2\_font\_inb16\_tr: These are 16-pixel height fonts (regular and bold variants) that can be good choices for general text display. The \_tr suffix again signifies transparent and reduced character sets.38  
* u8g2\_font\_10x20\_tf: While this font has a 20-pixel total height, its 10-pixel width makes it a common choice for larger text. The \_tf suffix indicates a full character set, which will consume more memory.61

U8G2's support for 16x16 pixel fonts is demonstrated in examples such as 16x16Font.ino within the U8G2 Arduino examples.62 When selecting a 16x16 font for a minimal implementation, prioritizing

\_tr or \_tn variants is advisable to minimize flash memory consumption, especially if the full ASCII character set is not strictly required.

The following table provides a comparative overview of selected 8x8 and 16x16 U8G2 fonts:

| Font Name | Capital A Height (px) | Approx. Total Pixel Height (px) | Type | Background | Character Set | Notes |
| :---- | :---- | :---- | :---- | :---- | :---- | :---- |
| u8g2\_font\_ncenB08\_tr | 8 | 10-12 | Proportional | Transparent | Reduced | Good balance of size and memory. |
| u8g2\_font\_micro\_tr | 5 | 6-8 | Proportional | Transparent | Reduced | Very compact, minimal memory. |
| u8g2\_font\_4x6\_tr | 5 | 6-8 | Monospace | Transparent | Reduced | Small, fixed-width, good for data. |
| u8g2\_font\_logisoso16\_tr | 16 | 20-24 | Proportional | Transparent | Reduced | Clear, readable, good for headlines. |
| u8g2\_font\_inr16\_tr | 16 | 20-24 | Monospace | Transparent | Reduced | Fixed-width, suitable for tabular data. |
| u8g2\_font\_10x20\_tf | 16 | 20 | Monospace | Transparent | Full | Larger, more memory, full ASCII support. |

### **5.3.3. Implementing Text Display with Chosen Fonts**

Once a font has been selected, implementing text display with U8G2 is straightforward. The u8g2.setFont(font\_name) function is used to set the currently active font for all subsequent text drawing operations.41 After setting the font, text strings can be drawn to the display buffer using

u8g2.drawStr(x, y, "text"), where x and y specify the starting coordinates of the text.41 The

u8g2.setCursor(x, y) function can also be used to explicitly position the text cursor.60 The drawing color for text, as with other primitives, is set using

u8g2.setDrawColor(color).33

For a 128x128 display, there is ample screen real estate to accommodate multiple lines of text, different font sizes, and even mixed text and graphics. To create more professional-looking layouts, U8G2 provides functions like getDisplayWidth() and getDisplayHeight() to query the display dimensions.36 These, combined with font-specific metrics such as

getFontAscent() and getFontDescent(), enable dynamic text wrapping, centering, and precise vertical alignment. This capability allows for sophisticated text layouts that enhance the overall "full-graphics capabilities" of the display.

## **6\. Minimal Implementation Example Code**

This section outlines the structure of the minimal C++ implementation, detailing the main application logic, the custom Hardware Abstraction Layer (HAL), and the CMake build system configuration.

### **6.1. main.cpp (Main Application Logic)**

The main.cpp file encapsulates the high-level application logic and interaction with the U8G2 library. Its design prioritizes a clear separation of concerns, ensuring that hardware-specific implementations are abstracted away into the custom HAL file. This approach promotes clean, readable code and simplifies future modifications or extensions.

The main.cpp includes necessary Pico SDK headers such as pico/stdlib.h, hardware/spi.h, and hardware/gpio.h. It also includes the primary U8G2 header, U8g2lib.h. The specific pin assignments for the display (GP14, GP15, GP21, GP13, GP20) are defined, typically as constants. A global u8g2 object is declared, instantiated with the chosen constructor (U8G2\_SH1107\_128X128\_F\_4W\_HW\_SPI) and the appropriate pin arguments.

Within the main() function, stdio\_init\_all() is called to initialize standard I/O, enabling serial output for debugging or monitoring. The display initialization is performed by calling u8g2.begin(). Optionally, any SH1107-specific initialization sequence adjustments (e.g., contrast or multiplex ratio fine-tuning) can be applied using u8g2.sendF() commands immediately after u8g2.begin(). The core display loop then follows the pattern of u8g2.clearBuffer() to prepare the drawing canvas, followed by U8G2 API calls to set fonts (u8g2.setFont()), draw text (u8g2.drawStr()), and render various graphics primitives (u8g2.drawPixel(), u8g2.drawLine(), u8g2.drawBox(), u8g2.drawCircle()). Finally, u8g2.sendBuffer() is called to push the completed frame to the display. A sleep\_ms() call can be included to introduce a delay for demonstration purposes, controlling the refresh rate.

### **6.2. u8g2\_hal\_pico.c (Custom HAL Implementation)**

The u8g2\_hal\_pico.c file is the dedicated custom Hardware Abstraction Layer that bridges U8G2's generic hardware requests to the Raspberry Pi Pico's specific SDK functions. This file includes Pico SDK headers (pico/stdlib.h, hardware/spi.h, hardware/gpio.h) and the U8G2 internal header (u8x8.h). Global variables are used to store the SPI instance (spi1) and the GPIO pin numbers for CS, DC, and RST, making them accessible to the callback functions.

The file implements two critical callback functions:

1. **u8x8\_byte\_pico\_hw\_spi(u8x8\_t \*u8x8, uint8\_t msg, uint8\_t arg\_int, void \*arg\_ptr):** This function handles byte-level SPI communication. It processes messages such as U8X8\_MSG\_BYTE\_INIT (for initial SPI setup), U8X8\_MSG\_BYTE\_SET\_DC (to control the Data/Command pin using gpio\_put), U8X8\_MSG\_BYTE\_SEND (to transmit data using spi\_write\_blocking for efficient bulk transfers), U8X8\_MSG\_BYTE\_START\_TRANSFER (to assert CS low using gpio\_put), and U8X8\_MSG\_BYTE\_END\_TRANSFER (to de-assert CS high using gpio\_put).12 While  
   spi\_write\_blocking is a blocking function, in a production environment, checking its return value for errors 63 would be a robust practice, though it might be omitted for brevity in a minimal demonstration.  
2. **u8x8\_gpio\_and\_delay\_pico(u8x8\_t \*u8x8, uint8\_t msg, uint8\_t arg\_int, void \*arg\_ptr):** This function manages GPIO control and timing delays. It handles U8X8\_MSG\_GPIO\_AND\_DELAY\_INIT (for initial setup of GPIOs using gpio\_init, gpio\_set\_dir, gpio\_set\_function), U8X8\_MSG\_DELAY\_MILLI (using sleep\_ms), and U8X8\_MSG\_DELAY\_NANO/U8X8\_MSG\_DELAY\_100NANO (using sleep\_us or asm("NOP") for very short delays).21 It also processes GPIO control messages like  
   U8X8\_MSG\_GPIO\_CS, U8X8\_MSG\_GPIO\_DC, and U8X8\_MSG\_GPIO\_RESET, directing them to gpio\_put for setting the respective pin states. Accurate timing for delays is particularly important during display initialization sequences.

### **6.3. CMakeLists.txt (Build System Configuration)**

The CMakeLists.txt file is central to the project's build system, ensuring a reproducible and consistent compilation process. It defines how the Pico SDK, U8G2 library, and custom HAL are integrated.

The file begins by specifying the minimum required CMake version (cmake\_minimum\_required(VERSION 3.12)). It then includes the Pico SDK's build functions via include($ENV{PICO\_SDK\_PATH}/external/pico\_sdk\_import.cmake). The project name is defined (project(pico\_sh1107\_u8g2 C CXX ASM)), and pico\_sdk\_init() is called to initialize the SDK's build system.

To integrate the U8G2 library and the custom HAL, their source files are added to the executable target using target\_sources(${PROJECT\_NAME} main.cpp u8g2\_hal\_pico.c...).15 Any necessary include directories for U8G2 headers are specified with

target\_include\_directories. Finally, the executable is linked against the required libraries: target\_link\_libraries(${PROJECT\_NAME} pico\_stdlib hardware\_spi).15

pico\_add\_extra\_outputs(${PROJECT\_NAME}) is included to generate various output formats (e.g.,.uf2 file for flashing), and pico\_enable\_stdio\_usb(${PROJECT\_NAME} 1\) enables USB serial output for debugging. This well-structured CMakeLists.txt ensures that the project can be built consistently across different development environments, which is a hallmark of robust embedded software development.

## **7\. Conclusion and Future Considerations**

This report has detailed a minimal C++ implementation for driving a 128x128 HiLetgo SH1107 OLED display on a Raspberry Pi Pico using the U8G2 graphics library via SPI. The implementation leverages the Pico SDK's hardware SPI capabilities and a custom Hardware Abstraction Layer (HAL) to ensure efficient and reliable communication. The selection of U8G2's "full buffer" mode, combined with hardware SPI, is crucial for achieving flicker-free, high-fidelity graphics, maximizing the display's potential. The detailed research into 8x8 and 16x16 font options provides a foundation for text rendering that balances readability with memory efficiency.

The chosen architecture, separating application logic from hardware-specific callbacks, offers a robust and extensible foundation. This modularity simplifies debugging and allows for future enhancements without extensive refactoring. The use of CMake ensures a reproducible build process, a key characteristic of professional embedded software development.

While this implementation provides a solid starting point, several avenues for future consideration and enhancement exist:

* **Error Handling:** For a production-grade system, implementing robust error checking within the SPI and GPIO callbacks is advisable. Functions like spi\_write\_blocking return status codes that can be used to detect and handle communication failures.63  
* **Advanced Graphics:** The U8G2 library supports a wide range of advanced graphics features, including bitmaps, custom shapes, and animations. Exploring these capabilities can further enrich the display's output.  
* **User Input Integration:** If the display module includes user buttons (as some Waveshare Pico-OLED modules do 6), integrating these into the application logic can create interactive user interfaces.  
* **Power Management:** Implementing power-saving modes for the OLED display, such as turning it off or dimming it when not in use, can significantly reduce overall power consumption, especially for battery-powered devices.  
* **Memory Optimization:** Although the Pico's RAM is ample for a 128x128 full buffer, for extremely constrained environments or larger future projects, exploring U8G2's "page buffer" modes could be considered. This would reduce RAM usage at the cost of more complex drawing loops and potential flicker.34  
* **Font Customization:** For highly specific aesthetic requirements, U8G2 offers tools and methods for converting custom fonts or generating specialized character sets, providing ultimate control over text appearance.

This minimal implementation serves as a comprehensive guide, providing the necessary technical details and code structure for developers to confidently integrate the U8G2 library with the Raspberry Pi Pico and SH1107 OLED display, laying the groundwork for more complex and feature-rich embedded graphical applications.

#### **Works cited**

1. SH1107 \- Display Future, accessed August 4, 2025, [https://www.displayfuture.com/Display/datasheet/controller/SH1107.pdf](https://www.displayfuture.com/Display/datasheet/controller/SH1107.pdf)  
2. Interfacing Arduino Board with SH1107 OLED Display in I2C Mode \- Simple Circuit, accessed August 4, 2025, [https://simple-circuit.com/interfacing-arduino-sh1107-oled-display-i2c-mode/](https://simple-circuit.com/interfacing-arduino-sh1107-oled-display-i2c-mode/)  
3. Grove \- OLED Display 1.12 (SH1107) V3.0 \- SPI/IIC \-3.3 V/5V \- Seeed Wiki, accessed August 4, 2025, [https://wiki.seeedstudio.com/Grove-OLED-Display-1.12-SH1107\_V3.0/](https://wiki.seeedstudio.com/Grove-OLED-Display-1.12-SH1107_V3.0/)  
4. MicroPython SH1107 display driver for 128x128 and 128x64 pixel displays \- GitHub, accessed August 4, 2025, [https://github.com/peter-l5/SH1107](https://github.com/peter-l5/SH1107)  
5. Pico OLED 1.3 \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-OLED-1.3](https://www.waveshare.com/wiki/Pico-OLED-1.3)  
6. Raspberry Pi Pico-OLED-1.3 User Guide \- Spotpear, accessed August 4, 2025, [https://spotpear.com/index/study/detail/id/566.html](https://spotpear.com/index/study/detail/id/566.html)  
7. 1.3" OLED Display Module for Raspberry Pi Pico (64×128) \- Model rocket shop, accessed August 4, 2025, [https://modelrockets.co.uk/electronics-and-payloads/displays/1-3-oled-display-module-for-raspberry-pi-pico-64128/](https://modelrockets.co.uk/electronics-and-payloads/displays/1-3-oled-display-module-for-raspberry-pi-pico-64128/)  
8. 1.3inch OLED Display Module for Raspberry Pi Pico, 64×128, SPI/I2C \- PiShop.us, accessed August 4, 2025, [https://www.pishop.us/product/1-3inch-oled-display-module-for-raspberry-pi-pico-64-128-spi-i2c/](https://www.pishop.us/product/1-3inch-oled-display-module-for-raspberry-pi-pico-64-128-spi-i2c/)  
9. 1.3 inch OLED Display Module for Raspberry Pi Pico, 64×128, SPI/I2C \- Waveshare, accessed August 4, 2025, [https://www.waveshare.com/pico-oled-1.3.htm](https://www.waveshare.com/pico-oled-1.3.htm)  
10. Waveshare 1.3inch OLED Display Module for Raspberry Pi Pico, 64×128 Pixels, 4-Wire SPI and I2C Interface Embedded SH1107 Driver with Two User Buttons for Easy Interacting in Oman | Whizz Single Board Computers, accessed August 4, 2025, [https://oman.whizzcart.com/product/20040064/waveshare-1-3inch-oled-display-module-for-raspberry-pi-pico-64-128-pixels-4-wire-spi-and-i2c-interface-embedded-sh1107-driver-with-two-user-buttons-for-easy-interacting/](https://oman.whizzcart.com/product/20040064/waveshare-1-3inch-oled-display-module-for-raspberry-pi-pico-64-128-pixels-4-wire-spi-and-i2c-interface-embedded-sh1107-driver-with-two-user-buttons-for-easy-interacting/)  
11. Waveshare 19376 Pico-OLED-1.3, 9,90 € \- Welectron, accessed August 4, 2025, [https://www.welectron.com/Waveshare-19376-Pico-OLED-13\_1](https://www.welectron.com/Waveshare-19376-Pico-OLED-13_1)  
12. Raspberry Pi Pico SDK: hardware\_spi \- Lorenz Ruprecht, accessed August 4, 2025, [https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/group\_\_hardware\_\_spi.html](https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/group__hardware__spi.html)  
13. Raspberry Pi Pico SDK: hardware\_spi/include/hardware/spi.h File ..., accessed August 4, 2025, [https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/rp2\_\_common\_2hardware\_\_spi\_2include\_2hardware\_2spi\_8h.html](https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/rp2__common_2hardware__spi_2include_2hardware_2spi_8h.html)  
14. Raspberry Pi Pico W \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Raspberry\_Pi\_Pico\_W](https://www.waveshare.com/wiki/Raspberry_Pi_Pico_W)  
15. Raspberry Pi Pico (RP2040) SPI Example with MicroPython and C/C++ \- DigiKey, accessed August 4, 2025, [https://www.digikey.com/en/maker/projects/raspberry-pi-pico-rp2040-spi-example-with-micropython-and-cc/9706ea0cf3784ee98e35ff49188ee045](https://www.digikey.com/en/maker/projects/raspberry-pi-pico-rp2040-spi-example-with-micropython-and-cc/9706ea0cf3784ee98e35ff49188ee045)  
16. Raspberry Pi Pico SDK: hardware\_spi/include/hardware/spi.h File Reference, accessed August 4, 2025, [https://cec-code-lab.aps.edu/robotics/resources/pico-c-api/rp2\_\_common\_2hardware\_\_spi\_2include\_2hardware\_2spi\_8h.html](https://cec-code-lab.aps.edu/robotics/resources/pico-c-api/rp2__common_2hardware__spi_2include_2hardware_2spi_8h.html)  
17. Raspberry Pi Pico SDK: hardware\_gpio \- Lorenz Ruprecht, accessed August 4, 2025, [https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/group\_\_hardware\_\_gpio.html](https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/group__hardware__gpio.html)  
18. Raspberry Pi Pico SDK: hardware\_gpio/include/hardware/gpio.h File Reference \- Lorenz Ruprecht, accessed August 4, 2025, [https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/gpio\_8h.html](https://lorenz-ruprecht.at/docu/pico-sdk/1.4.0/html/gpio_8h.html)  
19. Raspberry Pi Pico SDK: hardware\_spi/include/hardware/spi.h Source File \- CEC-Code-Lab, accessed August 4, 2025, [https://cec-code-lab.aps.edu/robotics/resources/pico-c-api/rp2\_\_common\_2hardware\_\_spi\_2include\_2hardware\_2spi\_8h\_source.html](https://cec-code-lab.aps.edu/robotics/resources/pico-c-api/rp2__common_2hardware__spi_2include_2hardware_2spi_8h_source.html)  
20. C/C++ drivers for WaveShare 3.5 inch LCD display for the Pico \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=382292](https://forums.raspberrypi.com/viewtopic.php?t=382292)  
21. Using the Raspberry Pi Pico C/C++ SDK \- V. Hunter Adams, accessed August 4, 2025, [https://vanhunteradams.com/Pico/Setup/UsingPicoSDK.html](https://vanhunteradams.com/Pico/Setup/UsingPicoSDK.html)  
22. Pi pico : Cmake and new project creation \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=349107](https://forums.raspberrypi.com/viewtopic.php?t=349107)  
23. CMake: Adding u8g2 library \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=311492](https://forums.raspberrypi.com/viewtopic.php?t=311492)  
24. Pico LCD 1.8 \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-LCD-1.8](https://www.waveshare.com/wiki/Pico-LCD-1.8)  
25. Pico-8SEG-LED \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-8SEG-LED](https://www.waveshare.com/wiki/Pico-8SEG-LED)  
26. Pico-CAN-B \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-CAN-B](https://www.waveshare.com/wiki/Pico-CAN-B)  
27. Raspberry Pi Pico \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Raspberry\_Pi\_Pico](https://www.waveshare.com/wiki/Raspberry_Pi_Pico)  
28. Pico RGB LED \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-RGB-LED](https://www.waveshare.com/wiki/Pico-RGB-LED)  
29. Pico-Get-Start-Windows \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-Get-Start-Windows](https://www.waveshare.com/wiki/Pico-Get-Start-Windows)  
30. OLED Display using U8x8 library \- hardware vs software SPI \- Arduino Forum, accessed August 4, 2025, [https://forum.arduino.cc/t/oled-display-using-u8x8-library-hardware-vs-software-spi/1053666](https://forum.arduino.cc/t/oled-display-using-u8x8-library-hardware-vs-software-spi/1053666)  
31. Port U8G2 Graphics Library to STM32 – Step‑by‑Step Guide, accessed August 4, 2025, [https://controllerstech.com/how-to-port-u8g2-graphic-lib-to-stm32/](https://controllerstech.com/how-to-port-u8g2-graphic-lib-to-stm32/)  
32. U8g2 \- Arduino Library List, accessed August 4, 2025, [https://www.arduinolibraries.info/libraries/u8g2](https://www.arduinolibraries.info/libraries/u8g2)  
33. u8g2reference · olikraus/u8g2 Wiki \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/u8g2reference](https://github.com/olikraus/u8g2/wiki/u8g2reference)  
34. Where can I find detailed instructions on using the u8g2 library? : r/arduino \- Reddit, accessed August 4, 2025, [https://www.reddit.com/r/arduino/comments/1lvb3dd/where\_can\_i\_find\_detailed\_instructions\_on\_using/](https://www.reddit.com/r/arduino/comments/1lvb3dd/where_can_i_find_detailed_instructions_on_using/)  
35. 6 ways to communicate with STM32 part 4\. Graphics, graphics, and I2C., accessed August 4, 2025, [https://nerd.mmccoo.com/index.php/2018/01/08/6-ways-to-communicate-with-stm32-part-4-graphics-graphics-and-i2c/](https://nerd.mmccoo.com/index.php/2018/01/08/6-ways-to-communicate-with-stm32-part-4-graphics-graphics-and-i2c/)  
36. u8g2 graphics processing library \- LuatOS documentation, accessed August 4, 2025, [https://wiki.luatos.org/api/u8g2.html](https://wiki.luatos.org/api/u8g2.html)  
37. fntlist8 · olikraus/u8g2 Wiki \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/fntlist8](https://github.com/olikraus/u8g2/wiki/fntlist8)  
38. fntlistallplain · olikraus/u8g2 Wiki · GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/fntlistallplain](https://github.com/olikraus/u8g2/wiki/fntlistallplain)  
39. Font sizes Adafruit GFX library : r/arduino \- Reddit, accessed August 4, 2025, [https://www.reddit.com/r/arduino/comments/1c7wki1/font\_sizes\_adafruit\_gfx\_library/](https://www.reddit.com/r/arduino/comments/1c7wki1/font_sizes_adafruit_gfx_library/)  
40. Why does it cut off the second half? More info in comments. Please help. : r/arduino \- Reddit, accessed August 4, 2025, [https://www.reddit.com/r/arduino/comments/e970e6/why\_does\_it\_cut\_off\_the\_second\_half\_more\_info\_in/](https://www.reddit.com/r/arduino/comments/e970e6/why_does_it_cut_off_the_second_half_more_info_in/)  
41. SH1107 i2c display low frame rate? · Issue \#2158 · olikraus/u8g2 ..., accessed August 4, 2025, [https://github.com/olikraus/u8g2/issues/2158](https://github.com/olikraus/u8g2/issues/2158)  
42. U8g2 infinite side move \- General Guidance \- Arduino Forum, accessed August 4, 2025, [https://forum.arduino.cc/t/u8g2-infinite-side-move/1239843](https://forum.arduino.cc/t/u8g2-infinite-side-move/1239843)  
43. Question regarding ssd1306 and sensor details (using u8g2) \- Displays \- Arduino Forum, accessed August 4, 2025, [https://forum.arduino.cc/t/question-regarding-ssd1306-and-sensor-details-using-u8g2/544574](https://forum.arduino.cc/t/question-regarding-ssd1306-and-sensor-details-using-u8g2/544574)  
44. u8x8setupcpp · olikraus/u8g2 Wiki · GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/u8x8setupcpp](https://github.com/olikraus/u8g2/wiki/u8x8setupcpp)  
45. Programming the Raspberry Pi Pico in C \- IOPress, accessed August 4, 2025, [https://www.iopress.info/index.php/books/9-programs/52-picocprograms](https://www.iopress.info/index.php/books/9-programs/52-picocprograms)  
46. Raspberry Pi Pico (RP2040) SPI Example with MicroPython and C/C++ | Digi-Key Electronics \- YouTube, accessed August 4, 2025, [https://www.youtube.com/watch?v=jdCnqiov6es](https://www.youtube.com/watch?v=jdCnqiov6es)  
47. SPI screen with Arduino IDE \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=345467](https://forums.raspberrypi.com/viewtopic.php?t=345467)  
48. Interfacing Arduino with SH1107 OLED Display in SPI Mode \- Simple Circuit, accessed August 4, 2025, [https://simple-circuit.com/arduino-sh1107-oled-spi-mode-interfacing/](https://simple-circuit.com/arduino-sh1107-oled-spi-mode-interfacing/)  
49. Pico LCD 1.3 \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Pico-LCD-1.3](https://www.waveshare.com/wiki/Pico-LCD-1.3)  
50. Sound Sensor \- Waveshare Wiki, accessed August 4, 2025, [https://www.waveshare.com/wiki/Sound\_Sensor](https://www.waveshare.com/wiki/Sound_Sensor)  
51. Raspberry Pi Pico and RP2040 \- C/C++ Part 3: How to Use PIO \- Digikey.nl, accessed August 4, 2025, [https://www.digikey.nl/en/maker/projects/raspberry-pi-pico-and-rp2040-cc-part-3-how-to-use-pio/123ff7700bc547c79a504858c1bd8110](https://www.digikey.nl/en/maker/projects/raspberry-pi-pico-and-rp2040-cc-part-3-how-to-use-pio/123ff7700bc547c79a504858c1bd8110)  
52. Porting to new MCU platform · olikraus/u8g2 Wiki \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/Porting-to-new-MCU-platform](https://github.com/olikraus/u8g2/wiki/Porting-to-new-MCU-platform)  
53. Using the Raspberry Pi Pico GPIO with the C/C++ SDK | Computer/Electronics Workbench, accessed August 4, 2025, [https://ceworkbench.wordpress.com/2023/01/04/using-the-raspberry-pi-pico-gpio-with-the-c-c-sdk/](https://ceworkbench.wordpress.com/2023/01/04/using-the-raspberry-pi-pico-gpio-with-the-c-c-sdk/)  
54. pico-examples/gpio/hello\_7segment/hello\_7segment.c at master · raspberrypi/pico-examples \- GitHub, accessed August 4, 2025, [https://github.com/raspberrypi/pico-examples/blob/master/gpio/hello\_7segment/hello\_7segment.c](https://github.com/raspberrypi/pico-examples/blob/master/gpio/hello_7segment/hello_7segment.c)  
55. gpio\_put\_mask() \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=326719](https://forums.raspberrypi.com/viewtopic.php?t=326719)  
56. RP2040 read GPIO with memory mapped access \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=369240](https://forums.raspberrypi.com/viewtopic.php?t=369240)  
57. Read value from SPI on Raspberry Pi Pico using Rust \- Stack Overflow, accessed August 4, 2025, [https://stackoverflow.com/questions/76382019/read-value-from-spi-on-raspberry-pi-pico-using-rust](https://stackoverflow.com/questions/76382019/read-value-from-spi-on-raspberry-pi-pico-using-rust)  
58. Adafruit\_SH110x/Adafruit\_SH1107.cpp at master · adafruit ... \- GitHub, accessed August 4, 2025, [https://github.com/adafruit/Adafruit\_SH110x/blob/master/Adafruit\_SH1107.cpp](https://github.com/adafruit/Adafruit_SH110x/blob/master/Adafruit_SH1107.cpp)  
59. Is this SPI only? : r/arduino \- Reddit, accessed August 4, 2025, [https://www.reddit.com/r/arduino/comments/1adwrqi/is\_this\_spi\_only/](https://www.reddit.com/r/arduino/comments/1adwrqi/is_this_spi_only/)  
60. u8x8reference · olikraus/u8g2 Wiki \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/u8x8reference](https://github.com/olikraus/u8g2/wiki/u8x8reference)  
61. fntlist16 · olikraus/u8g2 Wiki \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/u8g2/wiki/fntlist16](https://github.com/olikraus/u8g2/wiki/fntlist16)  
62. U8g2\_Arduino/examples/u8x8/16x16Font/16x16Font.ino at master · olikraus/U8g2\_Arduino \- GitHub, accessed August 4, 2025, [https://github.com/olikraus/U8g2\_Arduino/blob/master/examples/u8x8/16x16Font/16x16Font.ino](https://github.com/olikraus/U8g2_Arduino/blob/master/examples/u8x8/16x16Font/16x16Font.ino)  
63. i2c\_read\_blocking() causes pico to hang \- Raspberry Pi Forums, accessed August 4, 2025, [https://forums.raspberrypi.com/viewtopic.php?t=366890](https://forums.raspberrypi.com/viewtopic.php?t=366890)