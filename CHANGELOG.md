# Changelog

All notable changes to the AEAT-6600-T16 Encoder Interface project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned Features
- Multiple encoder support (read multiple encoders simultaneously)
- Data logging to SD card
- Web interface via ESP8266/ESP32 WiFi module
- CAN bus interface option
- Interrupt-driven sampling for higher performance
- Advanced filtering algorithms
- Configuration file support

---

## [2.0.0] - 2025-11-04

### 🎉 Major Release - Production-Ready Refactor

This release represents a complete overhaul of the codebase to production-level quality with comprehensive documentation.

### Added

#### Code Improvements
- **Structured Code Organization**: Introduced namespaces for configuration grouping
  - `HardwareConfig`: Pin assignments and port mappings
  - `EncoderConfig`: Encoder-specific parameters
  - `SerialConfig`: Serial communication settings
  - `TimingConfig`: SSI protocol timing constants
- **State Management**: Added `SystemState` struct for consistent state tracking
- **Function Prototypes**: Complete function declarations for better organization
- **Type Safety**: Strong typing with `enum class` for operation modes
- **Error Handling**: Comprehensive input validation and error checking
- **User Confirmation**: Safety mechanism for programming mode (requires 'y' confirmation)
- **Improved Serial Handling**:
  - Better command parsing with `Serial.peek()` instead of consuming characters
  - `flushSerialInput()` utility to prevent command overlap
  - Case-insensitive command handling (accepts both 'a' and 'A')
- **Memory Optimization**: Use of `F()` macro for string literals to save RAM

#### New Features
- **Enhanced Menu System**: Clear mode entry/exit messages
- **Help Command**: Added 'h' and '?' commands to display menu
- **Alignment Value Display**: Shows both decimal and hexadecimal values
- **Startup Banner**: Professional system identification on boot
- **Position Precision Control**: Configurable decimal places via constants

#### Documentation (NEW)
- **ARCHITECTURE.md**: Complete system architecture documentation
  - System component diagrams (Mermaid.js)
  - Software architecture layers
  - Communication protocol details
  - State machine documentation
  - Data flow diagrams
  - Performance analysis
- **HARDWARE_SETUP.md**: Comprehensive hardware setup guide
  - Component requirements
  - Detailed pin mapping tables
  - Wiring diagrams (Mermaid.js)
  - Mechanical assembly instructions
  - Power requirements and distribution
  - Step-by-step verification procedures
  - Troubleshooting guide
- **API_REFERENCE.md**: Complete API documentation
  - Serial communication specifications
  - Command interface reference
  - Function API documentation
  - Data format specifications
  - Error codes and messages
  - Usage examples
  - Integration examples (Python, C)
- **DEVELOPMENT.md**: Developer onboarding guide
  - Development environment setup
  - Code structure explanation
  - Building and uploading procedures
  - Debugging techniques
  - Testing strategies
  - Contributing guidelines
  - Porting guides for other hardware
  - Advanced topics
- **README.md**: Completely rewritten
  - Professional project overview
  - Quick start guide
  - Feature showcase
  - Command reference
  - Technical specifications
  - Roadmap
  - Extensive troubleshooting
- **CHANGELOG.md**: This file - version history tracking

#### Code Documentation
- **Comprehensive Inline Documentation**: Every function documented with Doxygen-style comments
- **Block Comments**: Major sections clearly delineated
- **Parameter Documentation**: All function parameters explained
- **Return Value Documentation**: Return values clearly specified
- **Warning Comments**: Critical notes about usage and limitations

### Changed

#### Breaking Changes
- **File Structure**: Code reorganized with namespaces (may require updates if forked)
- **Function Signatures**: Some functions now use `const` and `uint8_t` for type safety
- **Global Variables**: Moved to structured `SystemState` (breaking if accessed externally)

#### Improvements
- **Serial Command Parsing**:
  - Fixed bug where `Serial.read()` in while condition consumed exit characters
  - Now uses `Serial.peek()` for non-destructive checking
- **Magnetic Field Check**: Fixed logical operator bugs
  - Changed `&` to `&&` for proper boolean evaluation (lines 63, 69 in original)
- **Exit Mechanism**: More robust exit handling
  - Properly checks for 'e' command without consuming other data
  - Ensures all control pins are reset to LOW on exit
- **Programming Mode**: Complete implementation with safety features
  - User confirmation required
  - Clear warning messages
  - Proper PROG pin control
  - Verification reminders
- **Timing Constants**: All magic numbers replaced with named constants
  - Easier to understand and modify
  - Self-documenting code
- **Output Formatting**: Clearer, more professional messages
  - Consistent formatting
  - Status indicators (>>>, OK:, WARNING:, ERROR:)
  - Units clearly displayed (° for degrees)

### Fixed

- **Bug Fix**: Boolean operator in magnetic field check (used `&` instead of `&&`)
- **Bug Fix**: Serial command consumption in mode loops
- **Bug Fix**: Programming mode was incomplete (now fully implemented)
- **Bug Fix**: Missing mode exit handling for alignment test
- **Improvement**: ALIGN pin now properly disabled on alignment test exit
- **Improvement**: Better serial buffer management prevents command overlap

### Security

- **OTP Programming Safety**: Added confirmation prompt to prevent accidental programming
- **Input Validation**: Commands validated before execution
- **State Protection**: System state properly managed and reset on errors

### Performance

- **Maintained**: SSI communication speed (~400kHz) preserved
- **Optimized**: Memory usage via `F()` macro and constant optimization
- **Improved**: Reduced overhead in mode handlers with better flow control

---

## [1.0.0] - Previous Releases

### Original Implementation

The original implementation provided basic functionality:

- Position reading via SSI protocol
- Magnetic field strength monitoring
- Alignment mode testing
- Basic programming mode (incomplete)
- Arduino Micro compatibility
- Direct port manipulation for speed

**Known Issues (Fixed in 2.0.0)**:
- Logical operator bugs in magnetic field check
- Serial command parsing issues
- Incomplete programming mode
- Limited error handling
- Minimal documentation
- Magic numbers throughout code
- Poor code organization

---

## Version History Summary

```
[2.0.0] - 2025-11-04 - Production-Ready Release
  - Complete code refactor
  - Comprehensive documentation suite
  - Bug fixes and improvements
  - Enhanced features and safety

[1.0.0] - Previous - Original Implementation
  - Basic functionality
  - SSI communication
  - Mode operations
```

---

## Migration Guide

### Upgrading from 1.0.0 to 2.0.0

#### For End Users

**Hardware**: No changes required - pin assignments remain identical

**Usage**: Minor command interface improvements
- All original commands work the same way
- New features: Help command ('h'), case-insensitive commands
- Programming mode now requires confirmation ('y')

**Arduino IDE**: Simply replace the .ino file and upload

#### For Developers

**Code Changes**:
```cpp
// OLD (v1.0.0)
const int MAG_HI = 8;
const int BIT_COUNT = 10;

// NEW (v2.0.0)
namespace HardwareConfig {
  const uint8_t MAG_HI = 8;
}
namespace EncoderConfig {
  const uint8_t BIT_COUNT = 10;
}
```

**Function Calls**: Most functions unchanged, but now use namespace-scoped constants

**Global State**:
```cpp
// OLD (v1.0.0)
// No formal state management

// NEW (v2.0.0)
extern SystemState g_systemState;
if (g_systemState.currentMode == OperationMode::IDLE) {
    // ...
}
```

#### For Fork Maintainers

**Namespace Resolution**: Update references to constants to include namespace
**State Management**: Consider adopting `SystemState` pattern
**Documentation**: Review new documentation structure for your fork
**Testing**: Verify all modes work correctly after merge

---

## Versioning Policy

This project follows [Semantic Versioning](https://semver.org/):

- **MAJOR** version (X.0.0): Incompatible API changes
- **MINOR** version (0.X.0): Added functionality (backwards compatible)
- **PATCH** version (0.0.X): Bug fixes (backwards compatible)

### Version Number Format

```
MAJOR.MINOR.PATCH

Example: 2.1.3
- 2: Major version (breaking changes)
- 1: Minor version (new features)
- 3: Patch version (bug fixes)
```

---

## Contributing

See [DEVELOPMENT.md](DEVELOPMENT.md) for contribution guidelines.

### Changelog Entry Format

When contributing, please add entries in this format:

```markdown
### Added
- New feature description [#issue-number]

### Changed
- Changed behavior description [#issue-number]

### Deprecated
- Deprecated feature warning [#issue-number]

### Removed
- Removed feature description [#issue-number]

### Fixed
- Bug fix description [#issue-number]

### Security
- Security fix description [#issue-number]
```

---

## Links

- **Repository**: https://github.com/4nswer/AEAT-6600-T16
- **Issues**: https://github.com/4nswer/AEAT-6600-T16/issues
- **Discussions**: https://github.com/4nswer/AEAT-6600-T16/discussions

---

**Document Version**: 1.0.0
**Last Updated**: 2025-11-04
**Maintainer**: Andrew Becker
