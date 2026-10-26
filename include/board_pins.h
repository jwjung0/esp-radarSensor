// Classic ESP32 DevKit — LD1125H on UART2 (default RX2/TX2 GPIOs).
#pragma once

#ifndef BOARD_LED_PIN
#define BOARD_LED_PIN 2 // D2 — onboard LED on many DevKits
#endif

// HLK-LD1125H — UART2 (ESP TX2 <-> sensor RX, ESP RX2 <-> sensor TX).
#define PIN_RADAR_RX 16 // RX2
#define PIN_RADAR_TX 17 // TX2

#define RADAR_UART_BAUD 115200
