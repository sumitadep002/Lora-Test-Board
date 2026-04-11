# STM32G070CBTX FreeRTOS Exploration

This project is dedicated to exploring **FreeRTOS** implementations on the **STM32G070** microcontroller. It focuses on building a robust, asynchronous architecture for managing peripherals like I2C LCDs and user inputs via GPIO interrupts.

## Project Objectives
- Study and implement FreeRTOS primitives (Tasks, Queues, Thread Flags).
- Develop asynchronous drivers for hardware peripherals.
- Explore power management and interrupt-driven logic in an RTOS environment.

## Completed Milestones & Issues

The following milestones have been successfully implemented and closed:

| Issue ID | Description |
| :--- | :--- |
| **#13** | Implement Asynchronous FreeRTOS-based LCD Management |
| **#12** | Handle User inputs via push button |
| **#9** | Bring up 16x2 I2C LCD |
| **#8** | Bring up I2C |
| **#6** | Process User input |
| **#5** | Add GPIO ISR |
| **#3** | Bring up FreeRTOS |
| **#1** | Implement RTOS safe logging port |

## Project Structure
- `project/`: Main STM32CubeIDE project directory.
  - `Core/`: Standard initialization and main application logic.
  - `lcd/`: Asynchronous I2C LCD driver and management task.
  - `cfg_btn/`: Interrupt-driven button handler with duration measurement.
  - `Drivers/`: STM32 HAL and CMSIS drivers.
  - `Middlewares/`: FreeRTOS source.

## Hardware Requirements
- **Microcontroller**: STM32G070CBTX
- **Display**: 16x2 I2C LCD (Address `0x3E`)
- **Input**: Push button connected to Config Switch Pin (`CFG_SW_Pin`)
- **Debug**: UART1 @ 115200 baud for logging.
