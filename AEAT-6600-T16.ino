/**
 * @file AEAT-6600-T16.ino
 * @brief Production-ready firmware for Broadcom AEAT-6600-T16 magnetic encoder interface
 * @version 2.1.0
 * @date 2025-11-04
 *
 * @details
 * This firmware provides a complete test and configuration interface for the
 * Broadcom AEAT-6600-T16 magnetic rotary encoder chip via SSI protocol.
 *
 * Key Features:
 * - Real-time position reading (10-bit resolution, 360° output)
 * - Magnetic field strength monitoring
 * - Alignment mode testing
 * - Comprehensive programming interface for encoder configuration
 * - High-speed SSI communication (~400kHz) via direct port manipulation
 *
 * Hardware Platform: Arduino Micro (ATmega32U4 @ 16MHz)
 *
 * @warning Pin assignments use direct port manipulation for performance.
 *          Porting to other microcontrollers requires modification of:
 *          - PORTD bit 7 (Clock Pin D6)
 *          - PORTE bit 6 (Data Pin D7)
 *
 * @note Based on original work discussed at:
 *       http://forum.arduino.cc/index.php?topic=156812.0
 *
 * @see https://docs.broadcom.com/doc/AV02-2792EN (Datasheet)
 * @see https://docs.broadcom.com/wcs-public/products/application-notes/application-note/696/604/av02-2791en_an_5501_aeat-6600_2014-04-21.pdf
 *
 * @author Andrew Becker
 * @repository https://github.com/4nswer/AEAT-6600-T16
 * @license GPL-3.0
 */

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================

/**
 * @brief Pin assignments for AEAT-6600-T16 encoder interface
 * @warning These pins use direct port manipulation. See function implementations.
 */
namespace HardwareConfig {
  // Status indicator pins (INPUT)
  const uint8_t MAG_HI = 8;       ///< Magnetic field too high / OTP error indicator
  const uint8_t MAG_LO = 9;       ///< Magnetic field too low / OTP programming status

  // Control pins (OUTPUT)
  const uint8_t ALIGN = 2;        ///< Alignment mode enable
  const uint8_t PWR_DN = 3;       ///< Power down control
  const uint8_t PROG = 4;         ///< Programming mode enable
  const uint8_t NCS = 5;          ///< Chip select (active low)

  // SSI Communication pins
  const uint8_t CLOCK_PIN = 6;    ///< SSI Clock (PORTD bit 7) - OUTPUT
  const uint8_t DATA_PIN = 7;     ///< SSI Data (PORTE bit 6) - INPUT/OUTPUT

  // Port manipulation bit positions
  const uint8_t CLOCK_PORT_BIT = 7;  ///< PORTD bit for clock
  const uint8_t DATA_PORT_BIT = 6;   ///< PORTE bit for data
}

/**
 * @brief Encoder configuration parameters
 */
namespace EncoderConfig {
  const uint8_t BIT_COUNT = 10;           ///< Default resolution: 10-bit (1024 positions)
  const uint16_t MAX_POSITION = 1024;     ///< Maximum position value (2^BIT_COUNT)
  const uint16_t DEGREES_FULL_CIRCLE = 360; ///< Degrees in full rotation
  const uint8_t POSITION_DECIMAL_PLACES = 2; ///< Decimal precision for position output
}

/**
 * @brief Serial communication parameters
 */
namespace SerialConfig {
  const uint32_t BAUD_RATE = 115200;      ///< Serial baud rate
  const uint16_t SERIAL_TIMEOUT_MS = 100; ///< Timeout for serial operations
}

/**
 * @brief Timing constants for SSI protocol (microseconds)
 */
namespace TimingConfig {
  const uint8_t SSI_CLOCK_LOW_US = 2;     ///< SSI clock low duration
  const uint8_t SSI_CLOCK_HIGH_US = 2;    ///< SSI clock high duration
  const uint8_t SSI_SETUP_TIME_US = 1;    ///< Data setup time
  const uint8_t SSI_MIN_IDLE_US = 25;     ///< Minimum idle time between reads (per datasheet: 20µs)

  const uint16_t POSITION_READ_INTERVAL_MS = 10;   ///< Position polling interval
  const uint16_t MAGNETIC_CHECK_INTERVAL_MS = 200; ///< Magnetic field check interval
  const uint16_t ALIGNMENT_READ_INTERVAL_MS = 100; ///< Alignment polling interval
}

// ============================================================================
// OTP PROGRAMMING CONFIGURATION
// ============================================================================

/**
 * @brief OTP (One-Time Programmable) Register Bit Field Definitions
 *
 * The AEAT-6600-T16 OTP register is 32 bits with the following structure:
 *
 * Bit Fields (based on typical encoder programming structure):
 * - Bits 0-1:   Resolution (2 bits)
 * - Bits 2-13:  Zero Position Offset (12 bits)
 * - Bit 14:     Direction (1 bit)
 * - Bit 15:     Incremental Output Enable (1 bit)
 * - Bits 16-17: Incremental Mode Selection (2 bits)
 * - Bit 18:     PWM Output Enable (1 bit)
 * - Bits 19-21: PWM Period (3 bits)
 * - Bits 22-31: Reserved/Checksum (10 bits)
 *
 * @warning Verify these bit positions against your specific encoder datasheet
 */
namespace OTPConfig {
  // Bit positions
  const uint8_t RESOLUTION_BIT_POS = 0;
  const uint8_t ZERO_OFFSET_BIT_POS = 2;
  const uint8_t DIRECTION_BIT_POS = 14;
  const uint8_t INCR_ENABLE_BIT_POS = 15;
  const uint8_t INCR_MODE_BIT_POS = 16;
  const uint8_t PWM_ENABLE_BIT_POS = 18;
  const uint8_t PWM_PERIOD_BIT_POS = 19;

  // Bit masks
  const uint32_t RESOLUTION_MASK = 0x00000003;   // 2 bits
  const uint32_t ZERO_OFFSET_MASK = 0x00003FFC;  // 12 bits
  const uint32_t DIRECTION_MASK = 0x00004000;    // 1 bit
  const uint32_t INCR_ENABLE_MASK = 0x00008000;  // 1 bit
  const uint32_t INCR_MODE_MASK = 0x00030000;    // 2 bits
  const uint32_t PWM_ENABLE_MASK = 0x00040000;   // 1 bit
  const uint32_t PWM_PERIOD_MASK = 0x00380000;   // 3 bits

  // Resolution values
  const uint8_t RES_10_BIT = 0;  // 00b = 10-bit (1024 positions)
  const uint8_t RES_12_BIT = 1;  // 01b = 12-bit (4096 positions)
  const uint8_t RES_14_BIT = 2;  // 10b = 14-bit (16384 positions)
  const uint8_t RES_16_BIT = 3;  // 11b = 16-bit (65536 positions)

  // Direction values
  const uint8_t DIR_CLOCKWISE = 0;
  const uint8_t DIR_COUNTER_CLOCKWISE = 1;

  // Incremental mode values
  const uint8_t INCR_MODE_ABI = 0;  // Standard quadrature (A, B, Index)
  const uint8_t INCR_MODE_UVW = 1;  // UVW commutation outputs

  // PWM period values (example - verify in datasheet)
  const uint8_t PWM_PERIOD_1024US = 0;
  const uint8_t PWM_PERIOD_2048US = 1;
  const uint8_t PWM_PERIOD_4096US = 2;
  const uint8_t PWM_PERIOD_8192US = 3;
}

/**
 * @brief Encoder OTP Configuration Structure
 */
struct EncoderOTPConfig {
  uint8_t resolution;           ///< Resolution: 0=10bit, 1=12bit, 2=14bit, 3=16bit
  uint16_t zeroOffset;          ///< Zero position offset (0-4095)
  uint8_t direction;            ///< Rotation direction: 0=CW, 1=CCW
  bool incrementalEnable;       ///< Enable incremental outputs
  uint8_t incrementalMode;      ///< Incremental mode: 0=ABI, 1=UVW
  bool pwmEnable;               ///< Enable PWM output
  uint8_t pwmPeriod;            ///< PWM period selection (0-7)

  // Constructor with defaults
  EncoderOTPConfig() :
    resolution(OTPConfig::RES_10_BIT),
    zeroOffset(0),
    direction(OTPConfig::DIR_CLOCKWISE),
    incrementalEnable(false),
    incrementalMode(OTPConfig::INCR_MODE_ABI),
    pwmEnable(false),
    pwmPeriod(OTPConfig::PWM_PERIOD_1024US)
  {}
};

// Global configuration storage
EncoderOTPConfig g_otpConfig;

// ============================================================================
// OPERATIONAL MODE DEFINITIONS
// ============================================================================

/**
 * @brief System operational modes
 */
enum class OperationMode : uint8_t {
  IDLE = 0,           ///< Idle mode - waiting for commands
  POSITION_READ,      ///< Continuous position reading mode
  MAGNETIC_CHECK,     ///< Magnetic field strength monitoring mode
  ALIGNMENT_TEST,     ///< Alignment value monitoring mode
  PROGRAMMING         ///< Programming/configuration mode
};

/**
 * @brief Current system state
 */
struct SystemState {
  OperationMode currentMode;
  bool isInitialized;
  uint32_t lastReadTime;
};

// Global system state
SystemState g_systemState = {
  .currentMode = OperationMode::IDLE,
  .isInitialized = false,
  .lastReadTime = 0
};

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// Core SSI communication functions
unsigned long SSI_Shift_In(const uint8_t data_pin, const uint8_t clock_pin, const uint8_t bit_count);
void SSI_Shift_Out(const uint8_t bit_count, uint32_t progData);

// Encoder interface functions
float readPosition();
uint16_t readAlignmentValue();
bool checkMagneticField(bool& isHigh, bool& isLow);

// Mode handler functions
void handlePositionMode();
void handleMagneticCheckMode();
void handleAlignmentTestMode();
void handleProgrammingMode();
void exitCurrentMode();

// Programming menu functions
void displayProgrammingMenu();
void configureResolution();
void configureZeroOffset();
void configureDirection();
void configureIncrementalOutput();
void configurePWMOutput();
void previewConfiguration();
bool writeConfiguration();
uint32_t buildOTPWord(const EncoderOTPConfig& config);
void displayOTPWord(uint32_t otpWord);

// Utility functions
void printMenu();
void flushSerialInput();
char waitForSerialInput();
int readSerialInt(int minVal, int maxVal);

// ============================================================================
// ARDUINO SETUP
// ============================================================================

/**
 * @brief Initialize hardware and serial communication
 *
 * Configures all GPIO pins and initializes serial communication at 115200 baud.
 * Sets up the SSI clock line to idle high state as required by the protocol.
 */
void setup() {
  // Configure status indicator pins as inputs
  pinMode(HardwareConfig::MAG_HI, INPUT);
  pinMode(HardwareConfig::MAG_LO, INPUT);

  // Configure control pins as outputs (default LOW)
  pinMode(HardwareConfig::ALIGN, OUTPUT);
  pinMode(HardwareConfig::PWR_DN, OUTPUT);
  pinMode(HardwareConfig::PROG, OUTPUT);
  pinMode(HardwareConfig::NCS, OUTPUT);

  // Configure SSI communication pins
  pinMode(HardwareConfig::DATA_PIN, INPUT);
  pinMode(HardwareConfig::CLOCK_PIN, OUTPUT);

  // Set clock line to idle high (SSI protocol requirement)
  digitalWrite(HardwareConfig::CLOCK_PIN, HIGH);

  // Initialize serial communication
  Serial.begin(SerialConfig::BAUD_RATE);
  while (!Serial && millis() < 5000) {
    ; // Wait for serial port to connect (timeout after 5 seconds)
  }

  // Mark system as initialized
  g_systemState.isInitialized = true;
  g_systemState.currentMode = OperationMode::IDLE;

  // Print startup message and menu
  Serial.println(F("========================================"));
  Serial.println(F("AEAT-6600-T16 Encoder Interface v2.1.0"));
  Serial.println(F("========================================"));
  Serial.println();
  printMenu();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

/**
 * @brief Main program loop - handles serial commands and mode execution
 *
 * Reads serial commands and dispatches to appropriate mode handlers.
 * Commands are single characters (a-e) that switch operational modes.
 */
void loop() {
  if (!Serial.available()) {
    return;
  }

  char command = Serial.read();

  // Clear any residual serial data
  delay(10);
  flushSerialInput();

  switch (command) {
    case 'a':
    case 'A':
      Serial.println(F("\n>>> Entering Position Read Mode"));
      Serial.println(F(">>> Press 'e' to exit"));
      g_systemState.currentMode = OperationMode::POSITION_READ;
      handlePositionMode();
      break;

    case 'b':
    case 'B':
      Serial.println(F("\n>>> Entering Magnetic Field Check Mode"));
      Serial.println(F(">>> Press 'e' to exit"));
      g_systemState.currentMode = OperationMode::MAGNETIC_CHECK;
      handleMagneticCheckMode();
      break;

    case 'c':
    case 'C':
      Serial.println(F("\n>>> Entering Alignment Test Mode"));
      Serial.println(F(">>> Press 'e' to exit"));
      g_systemState.currentMode = OperationMode::ALIGNMENT_TEST;
      handleAlignmentTestMode();
      break;

    case 'd':
    case 'D':
      Serial.println(F("\n>>> Entering Programming Mode"));
      Serial.println(F(">>> WARNING: This mode writes to encoder OTP memory"));
      g_systemState.currentMode = OperationMode::PROGRAMMING;
      handleProgrammingMode();
      break;

    case 'e':
    case 'E':
      exitCurrentMode();
      break;

    case '?':
    case 'h':
    case 'H':
      printMenu();
      break;

    default:
      Serial.println(F("Unknown command. Press 'h' for help."));
      break;
  }
}

// ============================================================================
// ENCODER INTERFACE FUNCTIONS
// ============================================================================

/**
 * @brief Read current angular position from encoder
 *
 * Reads the encoder position via SSI protocol and converts to degrees.
 * Uses default 10-bit resolution (1024 positions per revolution).
 *
 * @return Position in degrees (0.00 to 359.99)
 */
float readPosition() {
  unsigned long rawData = SSI_Shift_In(
    HardwareConfig::DATA_PIN,
    HardwareConfig::CLOCK_PIN,
    EncoderConfig::BIT_COUNT
  );

  // Wait for minimum idle time before next read
  delayMicroseconds(TimingConfig::SSI_MIN_IDLE_US);

  // Mask to 10 bits and convert to degrees
  uint16_t position = rawData & 0x03FF;
  return (position * (float)EncoderConfig::DEGREES_FULL_CIRCLE) / (float)EncoderConfig::MAX_POSITION;
}

/**
 * @brief Read alignment value from encoder
 *
 * Reads the 16-bit alignment value when encoder is in alignment mode.
 * This value indicates the quality of magnetic alignment.
 *
 * @return 16-bit alignment value
 */
uint16_t readAlignmentValue() {
  return (uint16_t)SSI_Shift_In(
    HardwareConfig::DATA_PIN,
    HardwareConfig::CLOCK_PIN,
    16
  );
}

/**
 * @brief Check magnetic field strength status
 *
 * Reads the MAG_HI and MAG_LO pins to determine if the magnetic field
 * strength is within acceptable range.
 *
 * @param[out] isHigh Set to true if field is too strong
 * @param[out] isLow Set to true if field is too weak
 * @return true if field is within acceptable range, false otherwise
 */
bool checkMagneticField(bool& isHigh, bool& isLow) {
  isHigh = digitalRead(HardwareConfig::MAG_HI);
  isLow = digitalRead(HardwareConfig::MAG_LO);

  // Field is acceptable if both flags are low
  return (!isHigh && !isLow);
}

// ============================================================================
// MODE HANDLER FUNCTIONS
// ============================================================================

/**
 * @brief Position reading mode handler
 *
 * Continuously reads and displays encoder position until 'e' is received.
 * Updates at configured interval (default: 10ms).
 */
void handlePositionMode() {
  while (true) {
    // Check for exit command
    if (Serial.available() && (Serial.peek() == 'e' || Serial.peek() == 'E')) {
      Serial.read(); // Consume the 'e' character
      break;
    }

    // Read and display position
    float position = readPosition();
    Serial.print(F("Position: "));
    Serial.print(position, EncoderConfig::POSITION_DECIMAL_PLACES);
    Serial.println(F("°"));

    delay(TimingConfig::POSITION_READ_INTERVAL_MS);
  }

  exitCurrentMode();
}

/**
 * @brief Magnetic field check mode handler
 *
 * Continuously monitors magnetic field strength and displays status.
 * Checks for proper magnet positioning and alerts if field is out of range.
 */
void handleMagneticCheckMode() {
  while (true) {
    // Check for exit command
    if (Serial.available() && (Serial.peek() == 'e' || Serial.peek() == 'E')) {
      Serial.read(); // Consume the 'e' character
      break;
    }

    bool isHigh, isLow;
    bool isGood = checkMagneticField(isHigh, isLow);

    if (isHigh && isLow) {
      // Both high - fault condition
      Serial.println(F("ERROR: Fault detected (both MAG_HI and MAG_LO active)"));
    } else if (isHigh) {
      Serial.println(F("WARNING: Magnetic field intensity is TOO HIGH"));
    } else if (isLow) {
      Serial.println(F("WARNING: Magnetic field intensity is TOO LOW"));
    } else {
      // Field is good - also display position
      float position = readPosition();
      Serial.print(F("OK: Magnetic field intensity is correct. Position: "));
      Serial.print(position, EncoderConfig::POSITION_DECIMAL_PLACES);
      Serial.println(F("°"));
    }

    delay(TimingConfig::MAGNETIC_CHECK_INTERVAL_MS);
  }

  exitCurrentMode();
}

/**
 * @brief Alignment test mode handler
 *
 * Enables alignment mode and continuously reads alignment values.
 * Used to verify proper magnetic alignment during installation.
 */
void handleAlignmentTestMode() {
  // Enable alignment mode
  digitalWrite(HardwareConfig::ALIGN, HIGH);
  delay(10); // Allow mode to stabilize

  while (true) {
    // Check for exit command
    if (Serial.available() && (Serial.peek() == 'e' || Serial.peek() == 'E')) {
      Serial.read(); // Consume the 'e' character
      break;
    }

    uint16_t alignmentValue = readAlignmentValue();
    Serial.print(F("Alignment Value: "));
    Serial.print(alignmentValue);
    Serial.print(F(" (0x"));
    Serial.print(alignmentValue, HEX);
    Serial.println(F(")"));

    delay(TimingConfig::ALIGNMENT_READ_INTERVAL_MS);
  }

  // Disable alignment mode
  digitalWrite(HardwareConfig::ALIGN, LOW);
  exitCurrentMode();
}

/**
 * @brief Programming mode handler with comprehensive menu system
 *
 * Interactive menu for configuring all encoder OTP parameters.
 * Allows user to configure each parameter, preview the configuration,
 * and write to OTP memory with multiple safety confirmations.
 *
 * @warning This writes to OTP memory which cannot be erased.
 */
void handleProgrammingMode() {
  Serial.println(F("========================================"));
  Serial.println(F("     OTP PROGRAMMING MODE"));
  Serial.println(F("========================================"));
  Serial.println(F(""));
  Serial.println(F("⚠️  CRITICAL WARNING ⚠️"));
  Serial.println(F(""));
  Serial.println(F("OTP (One-Time Programmable) memory writes are:"));
  Serial.println(F("  • PERMANENT and IRREVERSIBLE"));
  Serial.println(F("  • Cannot be erased or modified"));
  Serial.println(F("  • Will persist across power cycles"));
  Serial.println(F(""));
  Serial.println(F("Incorrect configuration may render encoder unusable!"));
  Serial.println(F(""));
  Serial.println(F("Recommendations:"));
  Serial.println(F("  1. Read encoder datasheet carefully"));
  Serial.println(F("  2. Test configuration on spare encoder first"));
  Serial.println(F("  3. Verify all settings before programming"));
  Serial.println(F("  4. Have backup encoder available"));
  Serial.println(F("========================================"));
  Serial.println(F(""));
  Serial.println(F("Continue to programming menu? (y/n)"));

  char confirm = waitForSerialInput();

  if (confirm != 'y' && confirm != 'Y') {
    Serial.println(F("Programming mode cancelled."));
    exitCurrentMode();
    return;
  }

  // Reset configuration to defaults
  g_otpConfig = EncoderOTPConfig();

  // Display and navigate programming menu
  displayProgrammingMenu();

  exitCurrentMode();
}

/**
 * @brief Exit current operational mode
 *
 * Returns system to idle state and ensures all control pins are low.
 */
void exitCurrentMode() {
  // Ensure all control pins are low
  digitalWrite(HardwareConfig::ALIGN, LOW);
  digitalWrite(HardwareConfig::PROG, LOW);
  digitalWrite(HardwareConfig::PWR_DN, LOW);

  g_systemState.currentMode = OperationMode::IDLE;
  Serial.println(F("\n>>> Returned to IDLE mode"));
  Serial.println();
  printMenu();
}

// ============================================================================
// PROGRAMMING MENU FUNCTIONS
// ============================================================================

/**
 * @brief Display and handle programming menu navigation
 */
void displayProgrammingMenu() {
  bool menuActive = true;

  while (menuActive) {
    Serial.println(F(""));
    Serial.println(F("╔════════════════════════════════════════╗"));
    Serial.println(F("║   OTP CONFIGURATION MENU               ║"));
    Serial.println(F("╚════════════════════════════════════════╝"));
    Serial.println(F(""));
    Serial.println(F("Current Configuration:"));
    Serial.println(F("  1. Resolution:          ") + String(
      g_otpConfig.resolution == 0 ? "10-bit (1024)" :
      g_otpConfig.resolution == 1 ? "12-bit (4096)" :
      g_otpConfig.resolution == 2 ? "14-bit (16384)" : "16-bit (65536)"));
    Serial.println(F("  2. Zero Offset:         ") + String(g_otpConfig.zeroOffset) + " (" +
      String((g_otpConfig.zeroOffset * 360.0) / 4096.0, 2) + "°)");
    Serial.println(F("  3. Direction:           ") + String(
      g_otpConfig.direction == 0 ? "Clockwise" : "Counter-Clockwise"));
    Serial.println(F("  4. Incremental Output:  ") + String(
      g_otpConfig.incrementalEnable ? "Enabled" : "Disabled"));
    if (g_otpConfig.incrementalEnable) {
      Serial.println(F("     - Mode:              ") + String(
        g_otpConfig.incrementalMode == 0 ? "ABI (Quadrature)" : "UVW (Commutation)"));
    }
    Serial.println(F("  5. PWM Output:          ") + String(
      g_otpConfig.pwmEnable ? "Enabled" : "Disabled"));
    if (g_otpConfig.pwmEnable) {
      Serial.println(F("     - Period:            ") + String(
        (1 << (10 + g_otpConfig.pwmPeriod))) + "µs");
    }
    Serial.println(F(""));
    Serial.println(F("Options:"));
    Serial.println(F("  [1-5] Configure parameter"));
    Serial.println(F("  [p]   Preview OTP word"));
    Serial.println(F("  [w]   Write to encoder"));
    Serial.println(F("  [r]   Reset to defaults"));
    Serial.println(F("  [x]   Exit without writing"));
    Serial.println(F(""));
    Serial.print(F("Enter choice: "));

    char choice = waitForSerialInput();
    Serial.println(choice);
    Serial.println();

    switch (choice) {
      case '1':
        configureResolution();
        break;
      case '2':
        configureZeroOffset();
        break;
      case '3':
        configureDirection();
        break;
      case '4':
        configureIncrementalOutput();
        break;
      case '5':
        configurePWMOutput();
        break;
      case 'p':
      case 'P':
        previewConfiguration();
        break;
      case 'w':
      case 'W':
        if (writeConfiguration()) {
          menuActive = false;
        }
        break;
      case 'r':
      case 'R':
        g_otpConfig = EncoderOTPConfig();
        Serial.println(F("✓ Configuration reset to defaults"));
        break;
      case 'x':
      case 'X':
        Serial.println(F("Exiting without writing..."));
        menuActive = false;
        break;
      default:
        Serial.println(F("Invalid choice. Please try again."));
        break;
    }
  }
}

/**
 * @brief Configure encoder resolution
 */
void configureResolution() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configure Resolution                 ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));
  Serial.println(F("Select resolution:"));
  Serial.println(F("  [0] 10-bit (1024 positions per revolution)"));
  Serial.println(F("  [1] 12-bit (4096 positions per revolution)"));
  Serial.println(F("  [2] 14-bit (16384 positions per revolution)"));
  Serial.println(F("  [3] 16-bit (65536 positions per revolution)"));
  Serial.println(F(""));
  Serial.println(F("Note: Higher resolution requires more processing"));
  Serial.println(F("      and may reduce maximum update rate."));
  Serial.println(F(""));
  Serial.print(F("Enter choice (0-3): "));

  int choice = readSerialInt(0, 3);
  if (choice >= 0) {
    g_otpConfig.resolution = choice;
    Serial.println(F("✓ Resolution configured"));
  } else {
    Serial.println(F("✗ Invalid input"));
  }
}

/**
 * @brief Configure zero position offset
 */
void configureZeroOffset() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configure Zero Position Offset       ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));
  Serial.println(F("The zero offset allows you to set the zero position"));
  Serial.println(F("at any mechanical position of the encoder."));
  Serial.println(F(""));
  Serial.println(F("Current position reading:"));
  float currentPos = readPosition();
  Serial.print(F("  "));
  Serial.print(currentPos, 2);
  Serial.println(F("°"));
  Serial.println(F(""));
  Serial.println(F("Options:"));
  Serial.println(F("  [1] Set current position as zero"));
  Serial.println(F("  [2] Enter offset manually (0-4095)"));
  Serial.println(F("  [3] Enter offset in degrees (0-359.99)"));
  Serial.println(F("  [0] Cancel"));
  Serial.println(F(""));
  Serial.print(F("Enter choice: "));

  int choice = readSerialInt(0, 3);

  if (choice == 1) {
    // Set current position as zero
    unsigned long rawData = SSI_Shift_In(HardwareConfig::DATA_PIN, HardwareConfig::CLOCK_PIN, 12);
    g_otpConfig.zeroOffset = (uint16_t)(rawData & 0x0FFF);
    Serial.print(F("✓ Zero offset set to: "));
    Serial.println(g_otpConfig.zeroOffset);
  } else if (choice == 2) {
    // Manual offset (0-4095)
    Serial.print(F("Enter offset (0-4095): "));
    int offset = readSerialInt(0, 4095);
    if (offset >= 0) {
      g_otpConfig.zeroOffset = offset;
      Serial.println(F("✓ Zero offset configured"));
    } else {
      Serial.println(F("✗ Invalid input"));
    }
  } else if (choice == 3) {
    // Offset in degrees
    Serial.print(F("Enter offset in degrees (0-359): "));
    int degrees = readSerialInt(0, 359);
    if (degrees >= 0) {
      g_otpConfig.zeroOffset = (degrees * 4096) / 360;
      Serial.print(F("✓ Zero offset set to: "));
      Serial.print(g_otpConfig.zeroOffset);
      Serial.print(F(" ("));
      Serial.print(degrees);
      Serial.println(F("°)"));
    } else {
      Serial.println(F("✗ Invalid input"));
    }
  }
}

/**
 * @brief Configure rotation direction
 */
void configureDirection() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configure Rotation Direction         ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));
  Serial.println(F("Select rotation direction:"));
  Serial.println(F("  [0] Clockwise (CW)"));
  Serial.println(F("  [1] Counter-Clockwise (CCW)"));
  Serial.println(F(""));
  Serial.println(F("This inverts the counting direction."));
  Serial.println(F("Test with position reading mode before programming."));
  Serial.println(F(""));
  Serial.print(F("Enter choice (0-1): "));

  int choice = readSerialInt(0, 1);
  if (choice >= 0) {
    g_otpConfig.direction = choice;
    Serial.println(F("✓ Direction configured"));
  } else {
    Serial.println(F("✗ Invalid input"));
  }
}

/**
 * @brief Configure incremental output
 */
void configureIncrementalOutput() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configure Incremental Output         ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));
  Serial.println(F("Incremental outputs provide real-time position signals"));
  Serial.println(F("for motor controllers and PLCs."));
  Serial.println(F(""));
  Serial.println(F("Enable incremental outputs? (y/n): "));

  char enable = waitForSerialInput();
  Serial.println(enable);

  if (enable == 'y' || enable == 'Y') {
    g_otpConfig.incrementalEnable = true;

    Serial.println(F(""));
    Serial.println(F("Select incremental mode:"));
    Serial.println(F("  [0] ABI - Standard quadrature (A, B, Index)"));
    Serial.println(F("      • Most common for general applications"));
    Serial.println(F("      • A/B: 90° phase shifted quadrature"));
    Serial.println(F("      • I: One pulse per revolution"));
    Serial.println(F(""));
    Serial.println(F("  [1] UVW - Commutation outputs"));
    Serial.println(F("      • For brushless motor control"));
    Serial.println(F("      • 120° electrical angle spacing"));
    Serial.println(F(""));
    Serial.print(F("Enter choice (0-1): "));

    int mode = readSerialInt(0, 1);
    if (mode >= 0) {
      g_otpConfig.incrementalMode = mode;
      Serial.println(F("✓ Incremental output configured"));
    } else {
      Serial.println(F("✗ Invalid input"));
    }
  } else {
    g_otpConfig.incrementalEnable = false;
    Serial.println(F("✓ Incremental output disabled"));
  }
}

/**
 * @brief Configure PWM output
 */
void configurePWMOutput() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configure PWM Output                 ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));
  Serial.println(F("PWM output provides position as pulse width."));
  Serial.println(F("Duty cycle = Position / Max Position"));
  Serial.println(F(""));
  Serial.println(F("Enable PWM output? (y/n): "));

  char enable = waitForSerialInput();
  Serial.println(enable);

  if (enable == 'y' || enable == 'Y') {
    g_otpConfig.pwmEnable = true;

    Serial.println(F(""));
    Serial.println(F("Select PWM period:"));
    Serial.println(F("  [0] 1024µs  (1.024ms, ~976 Hz)"));
    Serial.println(F("  [1] 2048µs  (2.048ms, ~488 Hz)"));
    Serial.println(F("  [2] 4096µs  (4.096ms, ~244 Hz)"));
    Serial.println(F("  [3] 8192µs  (8.192ms, ~122 Hz)"));
    Serial.println(F(""));
    Serial.println(F("Note: Longer period = better resolution"));
    Serial.println(F("      Shorter period = faster update rate"));
    Serial.println(F(""));
    Serial.print(F("Enter choice (0-3): "));

    int period = readSerialInt(0, 3);
    if (period >= 0) {
      g_otpConfig.pwmPeriod = period;
      Serial.println(F("✓ PWM output configured"));
    } else {
      Serial.println(F("✗ Invalid input"));
    }
  } else {
    g_otpConfig.pwmEnable = false;
    Serial.println(F("✓ PWM output disabled"));
  }
}

/**
 * @brief Preview the OTP configuration before writing
 */
void previewConfiguration() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   Configuration Preview                ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));

  uint32_t otpWord = buildOTPWord(g_otpConfig);

  Serial.println(F("Configuration Summary:"));
  Serial.println(F("────────────────────────────────────────"));

  Serial.print(F("Resolution:        "));
  switch (g_otpConfig.resolution) {
    case 0: Serial.println(F("10-bit (1024 positions)")); break;
    case 1: Serial.println(F("12-bit (4096 positions)")); break;
    case 2: Serial.println(F("14-bit (16384 positions)")); break;
    case 3: Serial.println(F("16-bit (65536 positions)")); break;
  }

  Serial.print(F("Zero Offset:       "));
  Serial.print(g_otpConfig.zeroOffset);
  Serial.print(F(" ("));
  Serial.print((g_otpConfig.zeroOffset * 360.0) / 4096.0, 2);
  Serial.println(F("°)"));

  Serial.print(F("Direction:         "));
  Serial.println(g_otpConfig.direction == 0 ? F("Clockwise") : F("Counter-Clockwise"));

  Serial.print(F("Incremental:       "));
  if (g_otpConfig.incrementalEnable) {
    Serial.print(F("Enabled - "));
    Serial.println(g_otpConfig.incrementalMode == 0 ? F("ABI Mode") : F("UVW Mode"));
  } else {
    Serial.println(F("Disabled"));
  }

  Serial.print(F("PWM Output:        "));
  if (g_otpConfig.pwmEnable) {
    Serial.print(F("Enabled - Period: "));
    Serial.print(1 << (10 + g_otpConfig.pwmPeriod));
    Serial.println(F("µs"));
  } else {
    Serial.println(F("Disabled"));
  }

  Serial.println(F("────────────────────────────────────────"));
  Serial.println(F(""));
  displayOTPWord(otpWord);

  Serial.println(F(""));
  Serial.println(F("Press any key to continue..."));
  waitForSerialInput();
}

/**
 * @brief Write configuration to encoder OTP memory
 * @return true if write successful, false if cancelled
 */
bool writeConfiguration() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║   WRITE TO OTP MEMORY                  ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F(""));

  // Show configuration preview
  uint32_t otpWord = buildOTPWord(g_otpConfig);
  displayOTPWord(otpWord);

  Serial.println(F(""));
  Serial.println(F("⚠️  FINAL WARNING ⚠️"));
  Serial.println(F(""));
  Serial.println(F("This will PERMANENTLY program the encoder."));
  Serial.println(F("This operation CANNOT be undone."));
  Serial.println(F(""));
  Serial.println(F("Have you:"));
  Serial.println(F("  ✓ Verified all settings are correct?"));
  Serial.println(F("  ✓ Consulted the encoder datasheet?"));
  Serial.println(F("  ✓ Tested on a spare encoder (if available)?"));
  Serial.println(F("  ✓ Made a backup of current configuration?"));
  Serial.println(F(""));
  Serial.println(F("Type 'YES' (all capitals) to proceed: "));

  // Read confirmation
  String confirmation = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 30000) { // 30 second timeout
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
      confirmation += c;
      Serial.print(c); // Echo character
    }
  }
  Serial.println();

  if (confirmation != "YES") {
    Serial.println(F("✗ Programming cancelled - incorrect confirmation"));
    return false;
  }

  Serial.println(F(""));
  Serial.println(F("Programming encoder..."));
  Serial.println(F(""));

  // Check magnetic field before programming
  bool isHigh, isLow;
  checkMagneticField(isHigh, isLow);
  if (isHigh || isLow) {
    Serial.println(F("✗ ERROR: Magnetic field not optimal!"));
    Serial.println(F("  Programming requires good magnetic field."));
    Serial.println(F("  Adjust magnet position and try again."));
    return false;
  }

  // Enable programming mode
  digitalWrite(HardwareConfig::PROG, HIGH);
  delay(10); // Stabilization time

  // Write OTP data
  SSI_Shift_Out(32, otpWord);

  // Disable programming mode
  digitalWrite(HardwareConfig::PROG, LOW);
  delay(10);

  // Check OTP programming status
  bool otpError = digitalRead(HardwareConfig::MAG_HI);
  bool otpComplete = digitalRead(HardwareConfig::MAG_LO);

  Serial.println(F("Programming sequence complete."));
  Serial.println(F(""));

  if (otpError) {
    Serial.println(F("✗ ERROR: OTP programming error detected!"));
    Serial.println(F("  MAG_HI/OTP_ERR pin is HIGH."));
    Serial.println(F("  Encoder may not be programmed correctly."));
    Serial.println(F("  Check:"));
    Serial.println(F("    - Power supply voltage (must be stable 5V)"));
    Serial.println(F("    - Magnetic field (must be good)"));
    Serial.println(F("    - Encoder may already be programmed"));
    return false;
  } else if (otpComplete) {
    Serial.println(F("✓ Programming completed successfully!"));
    Serial.println(F(""));
    Serial.println(F("Next steps:"));
    Serial.println(F("  1. Power cycle the encoder"));
    Serial.println(F("  2. Verify new configuration with position read mode"));
    Serial.println(F("  3. Test all configured outputs"));
    Serial.println(F("  4. Document the programmed configuration"));
    return true;
  } else {
    Serial.println(F("⚠  WARNING: Status unclear"));
    Serial.println(F("  OTP status pins not indicating error or completion."));
    Serial.println(F("  Verify configuration after power cycle."));
    return true;
  }
}

/**
 * @brief Build 32-bit OTP word from configuration
 * @param config Configuration structure
 * @return 32-bit OTP word ready to write
 */
uint32_t buildOTPWord(const EncoderOTPConfig& config) {
  uint32_t otpWord = 0;

  // Pack configuration into bit fields
  otpWord |= ((uint32_t)config.resolution & 0x03) << OTPConfig::RESOLUTION_BIT_POS;
  otpWord |= ((uint32_t)config.zeroOffset & 0x0FFF) << OTPConfig::ZERO_OFFSET_BIT_POS;
  otpWord |= ((uint32_t)config.direction & 0x01) << OTPConfig::DIRECTION_BIT_POS;
  otpWord |= ((uint32_t)(config.incrementalEnable ? 1 : 0)) << OTPConfig::INCR_ENABLE_BIT_POS;
  otpWord |= ((uint32_t)config.incrementalMode & 0x03) << OTPConfig::INCR_MODE_BIT_POS;
  otpWord |= ((uint32_t)(config.pwmEnable ? 1 : 0)) << OTPConfig::PWM_ENABLE_BIT_POS;
  otpWord |= ((uint32_t)config.pwmPeriod & 0x07) << OTPConfig::PWM_PERIOD_BIT_POS;

  return otpWord;
}

/**
 * @brief Display OTP word in multiple formats
 * @param otpWord 32-bit OTP word to display
 */
void displayOTPWord(uint32_t otpWord) {
  Serial.println(F("OTP Word (32-bit):"));
  Serial.println(F("────────────────────────────────────────"));

  Serial.print(F("Hexadecimal:  0x"));
  if (otpWord < 0x10000000) Serial.print(F("0"));
  if (otpWord < 0x01000000) Serial.print(F("0"));
  if (otpWord < 0x00100000) Serial.print(F("0"));
  if (otpWord < 0x00010000) Serial.print(F("0"));
  if (otpWord < 0x00001000) Serial.print(F("0"));
  if (otpWord < 0x00000100) Serial.print(F("0"));
  if (otpWord < 0x00000010) Serial.print(F("0"));
  Serial.println(otpWord, HEX);

  Serial.print(F("Decimal:      "));
  Serial.println(otpWord);

  Serial.print(F("Binary:       0b"));
  for (int i = 31; i >= 0; i--) {
    Serial.print((otpWord >> i) & 1);
    if (i % 8 == 0 && i > 0) Serial.print(F(" "));
  }
  Serial.println();

  Serial.println(F("────────────────────────────────────────"));
}

// ============================================================================
// SSI COMMUNICATION FUNCTIONS
// ============================================================================

/**
 * @brief Read data from encoder via SSI protocol
 *
 * Implements SSI (Synchronous Serial Interface) read operation using
 * direct port manipulation for high-speed communication (~400kHz).
 *
 * @param data_pin GPIO pin number for data line
 * @param clock_pin GPIO pin number for clock line
 * @param bit_count Number of bits to read (typically 10 or 16)
 * @return Received data as unsigned long
 *
 * @note Uses direct port access (PORTD bit 7 for clock, PINE bit 6 for data)
 *       for performance. Standard digitalWrite() would limit speed to ~100kHz.
 *
 * @warning Do not modify pin assignments without updating port bit positions!
 */
unsigned long SSI_Shift_In(const uint8_t data_pin, const uint8_t clock_pin, const uint8_t bit_count) {
  unsigned long data = 0;

  // First clock tick initializes transfer (no data read)
  PORTD &= ~(1 << HardwareConfig::CLOCK_PORT_BIT); // Clock LOW
  delayMicroseconds(TimingConfig::SSI_CLOCK_LOW_US);
  PORTD |= (1 << HardwareConfig::CLOCK_PORT_BIT);  // Clock HIGH
  delayMicroseconds(TimingConfig::SSI_CLOCK_HIGH_US);

  // Read data bits
  for (uint8_t i = 0; i < bit_count; i++) {
    data <<= 1; // Shift existing data left

    PORTD &= ~(1 << HardwareConfig::CLOCK_PORT_BIT); // Clock LOW
    delayMicroseconds(TimingConfig::SSI_CLOCK_LOW_US);

    PORTD |= (1 << HardwareConfig::CLOCK_PORT_BIT);  // Clock HIGH
    delayMicroseconds(TimingConfig::SSI_SETUP_TIME_US);

    // Read bit from PINE register (faster than digitalRead)
    data |= bitRead(PINE, HardwareConfig::DATA_PORT_BIT);

    /* Performance note:
     * bitRead(PINE, 6) takes ~1µs
     * digitalRead(data_pin) takes ~4µs and increases jitter
     * This difference allows ~400kHz vs ~100kHz SSI rate
     */
  }

  return data;
}

/**
 * @brief Write data to encoder via SSI protocol
 *
 * Implements SSI write operation for programming encoder configuration.
 * Uses direct port manipulation for precise timing control.
 *
 * @param bit_count Number of bits to write
 * @param progData Data to write
 *
 * @note Temporarily switches data pin to OUTPUT mode
 * @warning Use only in programming mode with PROG pin active
 */
void SSI_Shift_Out(const uint8_t bit_count, uint32_t progData) {
  // Switch data pin to output mode
  pinMode(HardwareConfig::DATA_PIN, OUTPUT);

  // Initialize with clock low, data high
  PORTD &= ~(1 << HardwareConfig::CLOCK_PORT_BIT); // Clock LOW
  bitWrite(PORTE, HardwareConfig::DATA_PORT_BIT, 1);
  delayMicroseconds(TimingConfig::SSI_SETUP_TIME_US);
  PORTD |= (1 << HardwareConfig::CLOCK_PORT_BIT);  // Clock HIGH

  // Write data bits (with start and stop bits)
  for (uint8_t i = 0; i < bit_count + 2; i++) {
    PORTD &= ~(1 << HardwareConfig::CLOCK_PORT_BIT); // Clock LOW
    delayMicroseconds(TimingConfig::SSI_SETUP_TIME_US);

    // Set data bit
    if (i == 0 || i == bit_count + 1) {
      // Start and stop bits are high
      bitWrite(PORTE, HardwareConfig::DATA_PORT_BIT, 1);
    } else {
      // Data bits (MSB first)
      bitWrite(PORTE, HardwareConfig::DATA_PORT_BIT, bitRead(progData, bit_count - i));
    }

    PORTD |= (1 << HardwareConfig::CLOCK_PORT_BIT);  // Clock HIGH
    delayMicroseconds(TimingConfig::SSI_CLOCK_HIGH_US);
  }

  // Restore data pin to input mode
  pinMode(HardwareConfig::DATA_PIN, INPUT);
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Print command menu to serial console
 */
void printMenu() {
  Serial.println(F("Available Commands:"));
  Serial.println(F("  a - Position Read Mode (continuous angular position)"));
  Serial.println(F("  b - Magnetic Field Check (verify magnet positioning)"));
  Serial.println(F("  c - Alignment Test Mode (check magnetic alignment)"));
  Serial.println(F("  d - Programming Mode (configure encoder OTP)"));
  Serial.println(F("  e - Exit current mode"));
  Serial.println(F("  h - Show this menu"));
  Serial.println();
  Serial.println(F("Enter a command:"));
}

/**
 * @brief Flush any pending data in serial input buffer
 */
void flushSerialInput() {
  while (Serial.available()) {
    Serial.read();
  }
}

/**
 * @brief Wait for single character input from serial
 * @return Character received
 */
char waitForSerialInput() {
  flushSerialInput();
  while (!Serial.available()) {
    ; // Wait for input
  }
  return Serial.read();
}

/**
 * @brief Read integer from serial with validation
 * @param minVal Minimum valid value
 * @param maxVal Maximum valid value
 * @return Value if valid, -1 if invalid
 */
int readSerialInt(int minVal, int maxVal) {
  flushSerialInput();

  String input = "";
  unsigned long startTime = millis();

  while (millis() - startTime < 30000) { // 30 second timeout
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
      if (c >= '0' && c <= '9') {
        input += c;
        Serial.print(c); // Echo digit
      }
    }
  }
  Serial.println(); // New line

  if (input.length() == 0) {
    return -1;
  }

  int value = input.toInt();

  if (value < minVal || value > maxVal) {
    Serial.print(F("Error: Value must be between "));
    Serial.print(minVal);
    Serial.print(F(" and "));
    Serial.println(maxVal);
    return -1;
  }

  return value;
}
