# System Architecture

## Table of Contents
- [Overview](#overview)
- [System Components](#system-components)
- [Software Architecture](#software-architecture)
- [Communication Protocol](#communication-protocol)
- [State Machine](#state-machine)
- [Data Flow](#data-flow)
- [Performance Considerations](#performance-considerations)

## Overview

The AEAT-6600-T16 Encoder Interface is a production-grade Arduino firmware designed to interface with the Broadcom AEAT-6600-T16 magnetic rotary encoder via the SSI (Synchronous Serial Interface) protocol.

### Key Features

- **High-Speed Communication**: ~400kHz SSI clock rate via direct port manipulation
- **Multiple Operational Modes**: Position reading, magnetic field monitoring, alignment testing, and programming
- **Robust Error Handling**: Input validation and fault detection
- **Production-Ready Code**: Comprehensive documentation, type safety, and maintainability

## System Components

### Hardware Components

```mermaid
graph TB
    subgraph "Host System"
        PC[PC/Terminal]
    end

    subgraph "Arduino Micro"
        MCU[ATmega32U4<br/>16MHz]
        USB[USB Interface]
        GPIO[GPIO Pins]
        PORTD[PORTD Register]
        PORTE[PORTE Register]
    end

    subgraph "AEAT-6600-T16 Encoder"
        SENSOR[Magnetic<br/>Sensor]
        SSI_IF[SSI Interface]
        OTP[OTP Memory]
        STATUS[Status Pins]
        CTRL[Control Pins]
    end

    subgraph "Magnetic System"
        MAGNET[Permanent<br/>Magnet]
        SHAFT[Rotating<br/>Shaft]
    end

    PC <-->|USB Serial<br/>115200 baud| USB
    USB <--> MCU
    MCU <--> GPIO
    GPIO <-->|Digital I/O| PORTD
    GPIO <-->|Digital I/O| PORTE

    PORTD -->|Clock Signal| SSI_IF
    PORTE <-->|Data Signal| SSI_IF
    GPIO -->|Control Signals| CTRL
    GPIO <--|Status Signals| STATUS

    SSI_IF <--> SENSOR
    SSI_IF <--> OTP
    CTRL -.->|Mode Control| SENSOR
    STATUS -.->|Field Status| SENSOR

    SHAFT --> MAGNET
    MAGNET -->|Magnetic Field| SENSOR

    style PC fill:#e1f5ff
    style MCU fill:#ffe1e1
    style SENSOR fill:#e1ffe1
    style MAGNET fill:#f0f0f0
```

### Component Responsibilities

| Component | Responsibility | Key Signals |
|-----------|----------------|-------------|
| **Arduino Micro** | Protocol converter, command interpreter | USB Serial, GPIO |
| **AEAT-6600-T16** | Magnetic position sensing, data conversion | SSI Clock/Data, Status pins |
| **SSI Interface** | High-speed synchronous serial communication | Clock, Data |
| **OTP Memory** | Persistent encoder configuration storage | Programmed via SSI |
| **Magnetic Sensor** | Convert magnetic field to digital position | Analog sensing → Digital output |

## Software Architecture

### Module Organization

```mermaid
graph TB
    subgraph "Application Layer"
        MAIN[Main Loop<br/>Command Parser]
        MODES[Mode Handlers]
        MENU[User Interface]
    end

    subgraph "Abstraction Layer"
        ENC_IF[Encoder Interface]
        POS[Position Reader]
        MAG[Magnetic Field Check]
        ALIGN[Alignment Test]
        PROG[Programming Interface]
    end

    subgraph "Hardware Abstraction Layer"
        SSI_IN[SSI_Shift_In]
        SSI_OUT[SSI_Shift_Out]
        GPIO_IF[GPIO Interface]
    end

    subgraph "Hardware Layer"
        PORT[Direct Port Access<br/>PORTD/PORTE]
        PINS[Pin Configuration]
    end

    MAIN --> MODES
    MAIN --> MENU
    MODES --> ENC_IF

    ENC_IF --> POS
    ENC_IF --> MAG
    ENC_IF --> ALIGN
    ENC_IF --> PROG

    POS --> SSI_IN
    MAG --> GPIO_IF
    ALIGN --> SSI_IN
    PROG --> SSI_OUT

    SSI_IN --> PORT
    SSI_OUT --> PORT
    GPIO_IF --> PINS

    PORT --> HW[Hardware]
    PINS --> HW

    style MAIN fill:#ffe1e1
    style ENC_IF fill:#e1f5ff
    style SSI_IN fill:#e1ffe1
    style PORT fill:#f0f0f0
```

### Namespace Structure

The firmware is organized into logical namespaces:

```cpp
namespace HardwareConfig {
  // Pin assignments and port bit positions
  // Maps logical pins to physical MCU pins
}

namespace EncoderConfig {
  // Encoder-specific parameters
  // Resolution, scaling factors, precision
}

namespace SerialConfig {
  // Serial communication parameters
  // Baud rate, timeouts
}

namespace TimingConfig {
  // SSI protocol timing constraints
  // Clock periods, setup times, intervals
}
```

## Communication Protocol

### SSI (Synchronous Serial Interface)

The SSI protocol is a synchronous serial communication standard optimized for encoder data transfer.

#### SSI Read Operation

```mermaid
sequenceDiagram
    participant MCU as Arduino Micro
    participant CLK as Clock Line
    participant DATA as Data Line
    participant ENC as Encoder

    Note over MCU,ENC: Idle State (Clock HIGH)

    MCU->>CLK: Pull LOW
    Note over MCU: Delay 2µs
    MCU->>CLK: Pull HIGH
    Note over MCU: Delay 2µs
    Note over ENC: Initialize Transfer

    loop For each bit
        MCU->>CLK: Pull LOW
        Note over MCU: Delay 2µs
        MCU->>CLK: Pull HIGH
        Note over MCU: Delay 1µs
        ENC->>DATA: Output bit
        MCU->>DATA: Read bit
    end

    Note over MCU,ENC: Minimum 25µs idle before next read
```

#### SSI Write Operation (Programming)

```mermaid
sequenceDiagram
    participant MCU as Arduino Micro
    participant CLK as Clock Line
    participant DATA as Data Line
    participant ENC as Encoder OTP

    Note over MCU: Switch DATA to OUTPUT
    Note over MCU: Enable PROG pin

    MCU->>CLK: Pull LOW
    MCU->>DATA: Set HIGH (Start bit)
    Note over MCU: Delay 1µs
    MCU->>CLK: Pull HIGH

    loop For each data bit
        MCU->>CLK: Pull LOW
        Note over MCU: Delay 1µs
        MCU->>DATA: Output bit
        MCU->>CLK: Pull HIGH
        Note over MCU: Delay 2µs
        ENC->>DATA: Latch bit
    end

    MCU->>CLK: Pull LOW
    MCU->>DATA: Set HIGH (Stop bit)
    MCU->>CLK: Pull HIGH

    Note over MCU: Restore DATA to INPUT
    Note over MCU: Disable PROG pin
```

### Protocol Timing

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Clock Frequency** | ~400 kHz | Limited by Arduino execution speed |
| **Clock Low Time** | 2 µs | Per datasheet minimum |
| **Clock High Time** | 2 µs | Per datasheet minimum |
| **Setup Time** | 1 µs | Data valid before clock edge |
| **Minimum Idle Time** | 25 µs | Required between successive reads |

## State Machine

### Operational Modes

```mermaid
stateDiagram-v2
    [*] --> IDLE: System Boot

    IDLE --> POSITION_READ: Command 'a'
    IDLE --> MAGNETIC_CHECK: Command 'b'
    IDLE --> ALIGNMENT_TEST: Command 'c'
    IDLE --> PROGRAMMING: Command 'd'
    IDLE --> IDLE: Command 'h' (Help)

    POSITION_READ --> IDLE: Command 'e' or Exit
    MAGNETIC_CHECK --> IDLE: Command 'e' or Exit
    ALIGNMENT_TEST --> IDLE: Command 'e' or Exit
    PROGRAMMING --> IDLE: Complete or Cancel

    state POSITION_READ {
        [*] --> ReadPosition
        ReadPosition --> ReadPosition: Every 10ms
        ReadPosition --> DisplayAngle
        DisplayAngle --> ReadPosition
    }

    state MAGNETIC_CHECK {
        [*] --> CheckField
        CheckField --> CheckField: Every 200ms
        CheckField --> ReportStatus
        ReportStatus --> CheckField
    }

    state ALIGNMENT_TEST {
        [*] --> EnableAlign: Set ALIGN pin HIGH
        EnableAlign --> ReadAlign
        ReadAlign --> ReadAlign: Every 100ms
        ReadAlign --> DisplayValue
        DisplayValue --> ReadAlign
        ReadAlign --> DisableAlign: On exit
        DisableAlign --> [*]: Set ALIGN pin LOW
    }

    state PROGRAMMING {
        [*] --> RequestConfirm
        RequestConfirm --> Cancel: User cancels
        RequestConfirm --> EnableProg: User confirms
        EnableProg --> WriteData: Set PROG pin HIGH
        WriteData --> DisableProg: Set PROG pin LOW
        DisableProg --> [*]
        Cancel --> [*]
    }

    note right of IDLE
        Default state
        Waiting for commands
    end note

    note right of PROGRAMMING
        OTP write operation
        IRREVERSIBLE!
    end note
```

### State Transitions

| Current State | Event | Next State | Actions |
|---------------|-------|------------|---------|
| IDLE | User command 'a' | POSITION_READ | Start continuous position polling |
| IDLE | User command 'b' | MAGNETIC_CHECK | Start field monitoring |
| IDLE | User command 'c' | ALIGNMENT_TEST | Enable ALIGN mode, start polling |
| IDLE | User command 'd' | PROGRAMMING | Request confirmation |
| Any Mode | User command 'e' | IDLE | Disable control pins, flush buffers |
| PROGRAMMING | User confirms 'y' | Programming sequence | Enable PROG, write data |
| PROGRAMMING | User cancels | IDLE | No action taken |

## Data Flow

### Position Reading Data Flow

```mermaid
flowchart LR
    subgraph "Physical Domain"
        SHAFT[Rotating Shaft] -->|Mechanical<br/>Rotation| MAGNET[Magnet]
        MAGNET -->|Magnetic<br/>Field| HALL[Hall Effect<br/>Sensors]
    end

    subgraph "Encoder Domain"
        HALL -->|Analog<br/>Signals| ADC[ADC +<br/>Processing]
        ADC -->|10-bit<br/>Position| SSI_TX[SSI<br/>Transmitter]
    end

    subgraph "Firmware Domain"
        SSI_TX -->|Serial<br/>Data| SSI_RX[SSI_Shift_In<br/>Function]
        SSI_RX -->|Raw<br/>0-1023| SCALE[Position<br/>Scaling]
        SCALE -->|0.00-359.99°| FORMAT[Format<br/>Output]
    end

    subgraph "User Domain"
        FORMAT -->|ASCII<br/>Text| SERIAL[Serial<br/>Output]
        SERIAL -->|USB| USER[User<br/>Terminal]
    end

    style SHAFT fill:#f0f0f0
    style HALL fill:#e1ffe1
    style ADC fill:#e1ffe1
    style SSI_RX fill:#e1f5ff
    style SCALE fill:#e1f5ff
    style USER fill:#ffe1e1
```

### Magnetic Field Check Data Flow

```mermaid
flowchart TB
    START([Start Magnetic Check]) --> READ_PINS[Read MAG_HI and MAG_LO pins]

    READ_PINS --> CHECK{Evaluate<br/>Pin States}

    CHECK -->|HI=1, LO=1| FAULT[Report: FAULT<br/>Both pins active]
    CHECK -->|HI=1, LO=0| TOO_HIGH[Report: Field<br/>TOO HIGH]
    CHECK -->|HI=0, LO=1| TOO_LOW[Report: Field<br/>TOO LOW]
    CHECK -->|HI=0, LO=0| GOOD[Report: Field OK]

    GOOD --> READ_POS[Read Position]
    READ_POS --> DISPLAY_POS[Display Position]

    FAULT --> DELAY[Delay 200ms]
    TOO_HIGH --> DELAY
    TOO_LOW --> DELAY
    DISPLAY_POS --> DELAY

    DELAY --> EXIT{User pressed<br/>'e'?}
    EXIT -->|No| READ_PINS
    EXIT -->|Yes| END([Return to IDLE])

    style FAULT fill:#ffcccc
    style TOO_HIGH fill:#ffe6cc
    style TOO_LOW fill:#ffe6cc
    style GOOD fill:#ccffcc
```

## Performance Considerations

### Direct Port Manipulation

The firmware uses direct AVR port manipulation instead of Arduino's `digitalWrite()` for critical timing paths:

```mermaid
graph LR
    subgraph "Standard Arduino digitalWrite()"
        DW1[digitalWrite call] --> DW2[Pin to port mapping]
        DW2 --> DW3[Port register lookup]
        DW3 --> DW4[Bit manipulation]
        DW4 --> DW5[Register write]
    end

    subgraph "Direct Port Access"
        DP1[Direct PORTD/PORTE] --> DP2[Bit manipulation]
        DP2 --> DP3[Register write]
    end

    DW5 -.->|~4µs| RESULT[Output State Change]
    DP3 -.->|~1µs| RESULT

    style DW5 fill:#ffe6cc
    style DP3 fill:#ccffcc
```

**Performance Comparison:**

| Method | Execution Time | SSI Clock Rate | Jitter |
|--------|---------------|----------------|--------|
| `digitalWrite()` | ~4 µs per call | ~100 kHz | High |
| Direct port access | ~1 µs per call | ~400 kHz | Low |

### Memory Optimization

**Flash Memory Usage (Optimizations):**
- Use of `F()` macro for string literals → Stores strings in flash instead of RAM
- `const` qualifiers → Compiler optimizations
- Namespaces → Zero runtime overhead, organizational benefits only

**RAM Usage:**
- Minimal global state (SystemState struct: ~6 bytes)
- Stack-based variables in functions
- No dynamic memory allocation

### Timing Constraints

```mermaid
gantt
    title SSI Read Operation Timing (10-bit transfer)
    dateFormat X
    axisFormat %L

    section Initialization
    Clock LOW  :done, init1, 0, 2
    Clock HIGH :done, init2, 2, 4

    section Bit 0
    Clock LOW  :done, b0_1, 4, 6
    Clock HIGH :done, b0_2, 6, 8
    Read Data  :active, b0_3, 7, 8

    section Bit 1
    Clock LOW  :done, b1_1, 8, 10
    Clock HIGH :done, b1_2, 10, 12
    Read Data  :active, b1_3, 11, 12

    section ...
    More bits  :crit, dots, 12, 28

    section Bit 9
    Clock LOW  :done, b9_1, 28, 30
    Clock HIGH :done, b9_2, 30, 32
    Read Data  :active, b9_3, 31, 32

    section Idle
    Min Idle   :milestone, idle, 32, 57
```

**Total transfer time for 10-bit read:**
- Initialization: 4 µs
- 10 bits × 3 µs = 30 µs
- Minimum idle: 25 µs
- **Total: ~59 µs minimum between reads**
- **Maximum sample rate: ~17 kHz**

### Error Handling Strategy

```mermaid
flowchart TB
    START([Function Entry]) --> VALIDATE{Input<br/>Validation}

    VALIDATE -->|Invalid| LOG_ERROR[Log Error<br/>to Serial]
    VALIDATE -->|Valid| EXECUTE[Execute<br/>Operation]

    LOG_ERROR --> RETURN_ERROR[Return Error<br/>Code/Value]

    EXECUTE --> TIMEOUT{Timeout<br/>Check}
    TIMEOUT -->|Exceeded| LOG_TIMEOUT[Log Timeout<br/>Warning]
    TIMEOUT -->|OK| CHECK_HW{Hardware<br/>Response}

    LOG_TIMEOUT --> RETURN_ERROR

    CHECK_HW -->|Fault| LOG_HW[Log Hardware<br/>Fault]
    CHECK_HW -->|OK| RETURN_SUCCESS[Return Success]

    LOG_HW --> RETURN_ERROR

    RETURN_ERROR --> END([Function Exit])
    RETURN_SUCCESS --> END

    style LOG_ERROR fill:#ffcccc
    style LOG_TIMEOUT fill:#ffe6cc
    style LOG_HW fill:#ffcccc
    style RETURN_SUCCESS fill:#ccffcc
```

## System Reliability

### Fault Detection

| Fault Type | Detection Method | Response |
|------------|------------------|----------|
| **Serial Timeout** | `while(!Serial && millis() < 5000)` | Continue after 5s |
| **Magnetic Field Fault** | MAG_HI && MAG_LO both HIGH | Report error, continue monitoring |
| **Field Too High** | MAG_HI == HIGH only | Warning message |
| **Field Too Low** | MAG_LO == HIGH only | Warning message |
| **User Cancel** | Check for 'e' command | Exit mode cleanly |
| **Programming Abort** | Confirmation check | Cancel without write |

### Recovery Mechanisms

1. **Serial Buffer Flush**: `flushSerialInput()` prevents command overlap
2. **Mode Exit**: All control pins reset to LOW when exiting modes
3. **State Tracking**: Global `g_systemState` maintains consistent state
4. **Timeout Protection**: Serial initialization timeout prevents hang

---

**Document Version**: 1.0.0
**Last Updated**: 2025-11-04
**Author**: Claude Code
**License**: GPL-3.0
