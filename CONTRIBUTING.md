# Contributing to Airsoft Display

Thank you for your interest in contributing to this project! While this is primarily a personal portfolio project, feedback, suggestions, and contributions are welcome.

## 🎯 Project Philosophy

This project demonstrates:
- Embedded systems programming with C++17
- Real-time signal processing and filtering
- Dual-core architecture on RP2040
- Custom hardware driver development
- Hardware-in-the-loop testing

## 🐛 Reporting Issues

If you find a bug or have a suggestion:

1. **Check existing issues** to avoid duplicates
2. **Provide details:**
   - Hardware setup (Pico version, display model)
   - Software version (git commit hash)
   - Steps to reproduce
   - Expected vs. actual behavior
   - Serial output or screenshots if applicable

## 💡 Suggesting Features

Feature suggestions are welcome! Please:

1. Check the [roadmap in README.md](README.md#-roadmap) first
2. Open an issue with the "enhancement" label
3. Describe the use case and expected behavior
4. Consider the scope and alignment with project goals

## 🔧 Code Contributions

### Before Starting

1. **Open an issue** to discuss your idea
2. **Read the documentation:**
   - [.github/copilot-instructions.md](.github/copilot-instructions.md) - Coding standards
   - [docs/planning/2025-10-31-RECONNECTION-GUIDE.md](docs/planning/2025-10-31-RECONNECTION-GUIDE.md) - Architecture overview
   - [docs/planning/2025-10-31-PROJECT-STATUS.md](docs/planning/2025-10-31-PROJECT-STATUS.md) - Current status

3. **Understand the constraints:**
   - RP2040 has limited RAM (264KB)
   - Real-time requirements (<10ms latency)
   - Minimal external dependencies

### Development Setup

```bash
# Clone your fork
git clone https://github.com/YOUR-USERNAME/airsoft-display.git
cd airsoft-display

# Install Pico SDK (if not already installed)
# See: https://github.com/raspberrypi/pico-sdk

# Build
mkdir build && cd build
cmake ..
make -j4

# Test on hardware
# Flash and verify behavior
```

### Code Style

Follow existing conventions (see [copilot-instructions.md](.github/copilot-instructions.md)):

- **Classes:** PascalCase (e.g., `SH1107_Display`)
- **Functions:** camelCase (e.g., `drawChar`)
- **Variables:** snake_case (e.g., `current_voltage_mv`)
- **Constants:** UPPER_SNAKE_CASE (e.g., `PIN_SPI_SCK`)
- **Header guards:** `#pragma once`

### Commit Messages

Use clear, descriptive commit messages:

```
Add shot detection threshold configuration

- Implement configurable voltage threshold
- Add serial command for runtime adjustment
- Update documentation with threshold tuning guide
```

### Pull Request Process

1. **Fork the repository**
2. **Create a feature branch** (`git checkout -b feature/your-feature`)
3. **Make your changes:**
   - Write clean, documented code
   - Test on actual hardware
   - Update documentation if needed
4. **Commit with clear messages**
5. **Push to your fork**
6. **Open a Pull Request:**
   - Reference any related issues
   - Describe what was changed and why
   - Include test results or screenshots
   - Note any breaking changes

### Testing

This project uses hardware-in-the-loop testing:

- Build and flash firmware to Pico
- Verify functionality with real hardware
- Check serial output for errors
- Monitor display for correct behavior
- Test edge cases (low battery, rapid shots, etc.)

## 📚 Documentation

Documentation contributions are valuable! Areas that need improvement:

- Hardware assembly guide
- PCB design files
- Calibration procedures
- Troubleshooting guide
- Video demos

## 🔍 Code Review

All contributions will be reviewed for:

- **Correctness:** Does it work as intended?
- **Performance:** Does it meet real-time requirements?
- **Memory:** Does it fit within RP2040 constraints?
- **Style:** Does it follow project conventions?
- **Documentation:** Is it properly documented?

## 🎓 Learning Resources

New to embedded development? Check out:

- [Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/)
- [Pico SDK Documentation](https://raspberrypi.github.io/pico-sdk-doxygen/)
- [Getting Started with Pico](docs/hardware/getting-started-with-pico.pdf)

## ❓ Questions

Have questions? Feel free to:

- Open an issue with the "question" label
- Check existing documentation in `docs/`
- Review the detailed planning documents in `docs/planning/`

## 📄 License

By contributing, you agree that your contributions will be licensed under the Creative Commons Attribution-NonCommercial 4.0 International License (CC BY-NC 4.0). This means your contributions cannot be used commercially without permission from the copyright holder.

---

**Thank you for helping make this project better!** 🎉
