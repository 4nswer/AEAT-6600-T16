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

## [2.1.0] - 2025-11-04

### 🎯 Major Feature Release - Comprehensive OTP Programming System

This release adds a complete interactive menu system for programming all encoder features to OTP memory.

### Added

#### Comprehensive Programming System
- **Interactive Programming Menu**: Full-featured menu-driven programming interface
  - Step-by-step parameter configuration
  - Real-time configuration preview
  - Multiple safety confirmations
  - Configuration validation before writing

#### OTP Configuration Features
- **Resolution Configuration**: Select 10-bit, 12-bit, 14-bit, or 16-bit resolution
- **Zero Position Offset**: Three methods for setting mechanical zero
  - Set current position as zero (recommended)
  - Manual offset entry (0-4095 counts)
  - Degrees-based offset entry (0-359°)
- **Rotation Direction**: Configure clockwise or counter-clockwise counting
- **Incremental Output Configuration**:
  - Enable/disable incremental outputs
  - ABI mode (standard quadrature with A, B, Index)
  - UVW mode (three-phase commutation for BLDC motors)
- **PWM Output Configuration**:
  - Enable/disable PWM position output
  - Selectable period: 1024µs, 2048µs, 4096µs, or 8192µs
  - Trade-off selection (speed vs. resolution)

#### OTP Register Structure
- **Complete Bit Field Definitions**: All 32-bit OTP register fields documented
  - Bits 0-1: Resolution (2 bits)
  - Bits 2-13: Zero position offset (12 bits)
  - Bit 14: Direction (1 bit)
  - Bit 15: Incremental enable (1 bit)
  - Bits 16-17: Incremental mode (2 bits)
  - Bit 18: PWM enable (1 bit)
  - Bits 19-21: PWM period (3 bits)
  - Bits 22-31: Reserved
- **OTP Word Builder**: Automatic packing of configuration into 32-bit word
- **Multi-Format Display**: OTP word shown in hexadecimal, decimal, and binary

#### Safety Features
- **Multi-Level Warnings**: Critical warnings before entering programming mode
- **Confirmation System**: Must type "YES" (all capitals) to program
- **Magnetic Field Check**: Automatic verification before programming
- **Programming Status Verification**: Reads OTP_ERR and OTP_STA pins
- **Comprehensive Error Reporting**: Detailed error messages with solutions

#### Configuration Management
- **Preview Function**: Review complete configuration before writing
- **Reset to Defaults**: Quick reset of all parameters
- **Exit Without Writing**: Safe abort at any point
- **Configuration Summary**: Clear display of all current settings

#### New Utility Functions
- `waitForSerialInput()`: Blocking wait for single character
- `readSerialInt()`: Validated integer input with range checking (30s timeout)
- `buildOTPWord()`: Pack configuration structure into OTP word
- `displayOTPWord()`: Multi-format OTP word display
- `configureResolution()`: Interactive resolution configuration
- `configureZeroOffset()`: Interactive zero offset configuration
- `configureDirection()`: Interactive direction configuration
- `configureIncrementalOutput()`: Interactive incremental setup
- `configurePWMOutput()`: Interactive PWM setup
- `previewConfiguration()`: Configuration preview display
- `writeConfiguration()`: Complete OTP write procedure

#### Documentation (NEW)
- **PROGRAMMING_GUIDE.md**: Comprehensive 60+ page programming guide
  - Complete OTP register structure documentation
  - Detailed parameter descriptions
  - Configuration examples for common applications
  - Troubleshooting guide
  - Best practices and safety recommendations
  - Pre/post-programming checklists
  - Configuration management templates
  - FAQ section

#### Enhanced Namespace
- **OTPConfig namespace**: Complete OTP register definitions
  - Bit positions for all fields
  - Bit masks for field extraction
  - Value constants (resolution, direction, modes)
  - Period calculations for PWM

#### New Data Structures
- **EncoderOTPConfig struct**: Type-safe configuration storage
  - Resolution (uint8_t)
  - Zero offset (uint16_t)
  - Direction (uint8_t)
  - Incremental enable (bool)
  - Incremental mode (uint8_t)
  - PWM enable (bool)
  - PWM period (uint8_t)
  - Constructor with sensible defaults

### Changed

#### Programming Mode Enhancements
- **Complete Rewrite**: Replaced simple programming with full menu system
- **Interactive Navigation**: Menu-driven interface replaces hardcoded example
- **Real-Time Feedback**: Live configuration display with current values
- **Unit Conversions**: Automatic conversion between counts and degrees
- **Current Position Reading**: Display current position when setting zero

#### Improved SSI_Shift_Out
- **MSB First**: Changed bit order to MSB-first for correct OTP programming
- **Better Documentation**: Enhanced comments explaining bit order
- **Proper Bit Indexing**: Fixed data bit transmission order

#### Version Update
- Version bumped from 2.0.0 to 2.1.0
- Updated startup banner to show v2.1.0

### Fixed

- **Programming Data Transmission**: Corrected bit order in SSI_Shift_Out
- **Default Configuration**: Added proper default initialization
- **Menu Exit Handling**: Improved exit behavior from programming menu

### Security

- **Enhanced Safety Mechanisms**:
  - Multiple confirmation steps prevent accidental programming
  - Magnetic field verification prevents failed writes
  - Explicit "YES" confirmation (case-sensitive)
  - 30-second timeout on input operations
  - Clear warning messages at every step

### Performance

- **Maintained**: All performance characteristics from v2.0.0 preserved
- **Input Timeout**: 30-second timeout prevents indefinite blocking
- **Efficient Display**: Optimized string formatting for menu display

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
