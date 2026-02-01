/**
 * @file AEAT-6600-T16.ino
 * @brief Production-ready firmware for Broadcom AEAT-6600-T16 magnetic encoder interface
 * @version 2.0.0
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
 * - Programming interface for encoder configuration
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

// Utility functions
void printMenu();
void flushSerialInput();

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
  Serial.println(F("AEAT-6600-T16 Encoder Interface v2.0.0"));
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
 * @brief Programming mode handler
 *
 * Writes configuration data to encoder's OTP (One-Time Programmable) memory.
 *
 * @warning This writes to OTP memory which cannot be erased.
 *          Use with extreme caution.
 *
 * @note Currently configured with example data. Modify as needed.
 */
void handleProgrammingMode() {
  Serial.println(F("========================================"));
  Serial.println(F("WARNING: PROGRAMMING MODE"));
  Serial.println(F("========================================"));
  Serial.println(F("This mode writes to OTP memory!"));
  Serial.println(F("Send 'y' to confirm, any other key to cancel"));

  // Wait for confirmation
  while (!Serial.available()) {
    ; // Wait for input
  }

  char confirm = Serial.read();
  flushSerialInput();

  if (confirm != 'y' && confirm != 'Y') {
    Serial.println(F("Programming cancelled."));
    exitCurrentMode();
    return;
  }

  // Example programming data (32-bit)
  // Modify this value based on your requirements
  uint32_t programmingData = 0b00000000000000000100101100000111;

  Serial.print(F("Programming data: 0x"));
  Serial.println(programmingData, HEX);
  Serial.println(F("Writing to encoder..."));

  // Enable programming mode
  digitalWrite(HardwareConfig::PROG, HIGH);
  delay(10);

  // Write data via SSI
  SSI_Shift_Out(32, programmingData);

  // Disable programming mode
  digitalWrite(HardwareConfig::PROG, LOW);

  Serial.println(F("Programming complete."));
  Serial.println(F("Verify encoder configuration before using."));

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
      // Data bits
      bitWrite(PORTE, HardwareConfig::DATA_PORT_BIT, bitRead(progData, i - 1));
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
  Serial.println(F("  d - Programming Mode (write encoder configuration)"));
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
