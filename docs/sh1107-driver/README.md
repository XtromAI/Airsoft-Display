# Documentation Directory

This directory contains comprehensive documentation for the SH1107 OLED display project.

## SH1107 Custom Driver Documentation

### 📖 Main Documentation
- **[SH1107 Custom Driver](sh1107-custom-driver.md)** - Complete driver documentation with examples and troubleshooting
- **[API Reference](sh1107-api-reference.md)** - Detailed function reference and technical specifications  
- **[Quick Reference](sh1107-quick-reference.md)** - Condensed reference for common tasks

### 🔧 Development Resources
- **[Design Guide](design-guide.md)** - Project design principles and guidelines
- **[Display Driver Research](display-driver-research.md)** - Research on display technologies and drivers
- **[Display Library Comparison](display-library-comparison.md)** - Comparison of different display libraries

### 📁 Reference Materials

#### Hardware Documentation (`hardware/`)
- `pico-datasheet.pdf` - Raspberry Pi Pico specifications
- `SH1107Datasheet.pdf` - SH1107 display controller datasheet
- `Pico Pinout.jpg` - GPIO pin reference diagram
- `getting-started-with-pico.pdf` - Pico development guide
- `pico_nuke_pico_generic-1.1.uf2` - Emergency recovery firmware

#### Python Reference (`python-reference/`)
- `sh1107.py` - MicroPython SH1107 driver (reference implementation)
- `framebuf2.py` - Extended framebuffer with advanced graphics functions

## Quick Navigation

| Need to... | Read this |
|------------|-----------|
| **Get started quickly** | [Quick Reference](sh1107-quick-reference.md) |
| **Understand the full driver** | [Main Documentation](sh1107-custom-driver.md) |
| **Look up specific functions** | [API Reference](sh1107-api-reference.md) |
| **Troubleshoot issues** | [Main Documentation - Troubleshooting](sh1107-custom-driver.md#troubleshooting) |
| **See wiring diagrams** | [Main Documentation - Hardware](sh1107-custom-driver.md#hardware-requirements) |
| **Find example code** | [Main Documentation - Examples](sh1107-custom-driver.md#example-applications) |

## Driver Features Summary

✅ **Direct SPI Communication** - Optimized for Raspberry Pi Pico  
✅ **Built-in Graphics** - Text, lines, rectangles, circles, triangles  
✅ **High Performance** - 10MHz SPI, ~16ms full screen update  
✅ **Memory Efficient** - 2KB buffer, minimal overhead  
✅ **Hardware Integration** - Seamless Pico SDK integration  
✅ **Python Compatible** - Based on proven MicroPython implementation  

## Project Status

- ✅ **Driver Implementation**: Complete and tested
- ✅ **Basic Graphics**: Text, lines, rectangles working
- ✅ **Advanced Graphics**: Circles and triangles implemented  
- ✅ **Performance Optimization**: 10MHz SPI, efficient algorithms
- ✅ **Documentation**: Comprehensive guides and references
- ✅ **Examples**: Multiple demo applications included

## Getting Help

1. **Quick answers**: Check [Quick Reference](sh1107-quick-reference.md)
2. **Detailed info**: Read [Main Documentation](sh1107-custom-driver.md)
3. **Function details**: See [API Reference](sh1107-api-reference.md)
4. **Hardware issues**: Review wiring diagrams and troubleshooting sections
5. **Code examples**: Browse the example applications in the main documentation

## Version Information

- **Driver Version**: 1.3
- **Pico SDK**: 2.1.1
- **Tested Hardware**: Raspberry Pi Pico + HiLetgo SH1107 128x128 OLED
- **Last Updated**: August 2025
