# Repository Setup Guide

This document provides recommended settings and metadata for the GitHub repository to optimize discoverability and presentation.

## Repository Description

**Suggested Description:**
> Real-time shot counter and battery monitor for airsoft guns using Raspberry Pi Pico (RP2040). Features custom OLED driver, DMA-based ADC sampling, and dual-core signal processing.

## Repository Topics

Add the following topics to improve discoverability:

```
raspberry-pi-pico
rp2040
embedded-systems
cpp17
airsoft
oled-display
adc-sampling
signal-processing
dual-core
dma
hardware-project
maker
embedded-c
real-time
sh1107
battery-monitor
```

## Repository Settings

### General

- **Visibility:** Public
- **Features to Enable:**
  - ✅ Issues
  - ✅ Projects (optional - for roadmap tracking)
  - ✅ Wiki (optional - for extended documentation)
  - ✅ Discussions (optional - for community Q&A)

### Social Preview Image

Consider creating a social preview image (1280x640px) showing:
- Photo of the hardware setup
- OLED display showing shot counter
- Project logo or title

## About Section

**Website:** Leave blank or add portfolio/documentation link  
**Topics:** (see list above)  
**Include in the homepage:** Yes  

## Code & Automation

### Branch Protection (recommended for main branch)

- Require pull request reviews before merging
- Require status checks to pass before merging (if CI/CD added later)
- Require branches to be up to date before merging

### GitHub Actions (future consideration)

Potential workflows to add:
- Build verification on PR/push
- Documentation generation
- Release automation

## Labels

Suggested custom labels for issues:

- `hardware` - Hardware-related issues
- `display` - Display driver or UI issues
- `signal-processing` - ADC sampling or filtering
- `shot-detection` - Shot detection algorithm
- `documentation` - Documentation improvements
- `portfolio` - Portfolio presentation enhancements
- `good-first-issue` - Good for newcomers
- `help-wanted` - Looking for contributions

## Issue Templates

Consider creating issue templates:

1. **Bug Report** - For hardware or software bugs
2. **Feature Request** - For new features
3. **Documentation** - For documentation improvements
4. **Hardware Setup Help** - For build/assembly questions

## Pull Request Template

Consider creating `.github/pull_request_template.md`:

```markdown
## Description
<!-- Describe your changes in detail -->

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Hardware Testing
- [ ] Tested on Raspberry Pi Pico hardware
- [ ] Display functions correctly
- [ ] No memory issues (within RP2040 limits)
- [ ] Watchdog doesn't trigger unexpectedly

## Checklist
- [ ] My code follows the style guidelines of this project
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have tested this on actual hardware
```

## README Badges

The following badges are already included in README.md:
- License (CC BY-NC 4.0)
- Platform (Raspberry Pi Pico)
- Language (C++17)
- Status (Work in Progress)

Consider adding:
- GitHub Actions build status (when/if CI is added)
- Code size badge
- Documentation badge

## Security

### Security Policy

The project uses:
- CodeQL for security scanning
- Manual code reviews
- No external web services or API endpoints

### Dependabot (optional)

Not immediately applicable as the project has no dependency files (package.json, requirements.txt is minimal). May be useful in the future if external libraries are added.

## Releases

### Version Strategy

Current: v0.1 (development)

Suggested versioning:
- **v0.x** - Development versions
- **v1.0** - First stable release with working shot detection
- **v1.x** - Feature additions
- **v2.0** - Major architecture changes or hardware revisions

### Release Process

When ready for v1.0:
1. Tag the release: `git tag -a v1.0 -m "First stable release"`
2. Create release notes highlighting:
   - Key features implemented
   - Hardware requirements
   - Known limitations
   - Build instructions
3. Attach pre-built `.uf2` firmware files for easy installation

## Community

### Discussions Categories (if enabled)

Suggested categories:
- **General** - General discussion
- **Show and Tell** - Users sharing their builds
- **Q&A** - Hardware setup and troubleshooting
- **Ideas** - Feature suggestions and brainstorming

## Documentation Links

Ensure these are easily accessible from README:
- [Hardware Documentation](../docs/hardware/)
- [Project Status](../docs/planning/2025-10-31-PROJECT-STATUS.md)
- [Contributing Guidelines](../CONTRIBUTING.md)
- [License](../LICENSE)

## Portfolio Presentation

As this is a portfolio project, consider:

1. **README.md** should highlight:
   - ✅ Technical complexity (dual-core, DMA, DSP)
   - ✅ Custom driver development
   - ✅ Real-world application
   - ✅ Hardware integration

2. **Project demonstration:**
   - Add photos/videos to README
   - Create a demo video showing:
     - Hardware setup
     - Display in action
     - Data collection workflow
     - Signal processing visualization

3. **Code samples to highlight:**
   - Custom display driver (`lib/sh1107-pico/`)
   - DMA-based sampling (`lib/dma_adc_sampler.cpp`)
   - Digital filtering (`lib/voltage_filter.cpp`)
   - Dual-core architecture (`src/main.cpp`)

## External Links

Consider linking to:
- Personal portfolio website
- LinkedIn profile
- Technical blog posts about the project
- Demonstration videos

## Maintenance

As a portfolio project:
- Mark clearly as "work in progress" until shot detection is complete
- Update README roadmap as features are completed
- Keep documentation up to date
- Respond to issues/questions promptly (demonstrates engagement)

---

**Last Updated:** 2025-11-22  
**Status:** Ready for public release
