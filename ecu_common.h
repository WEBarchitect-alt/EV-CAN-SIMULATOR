#ifndef ECU_COMMON_H
#define ECU_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// CAN ID table: Lower ID = Higher Priority
#define CAN_ID_ENGINE_RPM      0x101
#define CAN_ID_ENGINE_TEMP     0x102
#define CAN_ID_MOTOR_RPM       0x201
#define CAN_ID_GEAR            0x301
#define CAN_ID_DIAG_FAULT      0x401

// Core CAN Frame
typedef struct {
    uint32_t id;          // 11-bit Identifier
    uint8_t data[8];      // Classical CAN has an 8-byte payload limit
    uint8_t dlc;          // Data Length Code (0 to 8)
} CanFrame;

typedef enum {
    STATUS_OK = 0,
    STATUS_WARNING = 1,
    STATUS_FAULT = 2
} EcuStatus;

#define BUS_QUEUE_LIMIT 50

typedef struct {
    CanFrame frames[BUS_QUEUE_LIMIT];
    int write_index;
    int read_index;
    int count;
} CanBus;

extern CanBus g_can_bus;

static inline void init_can_bus() {
    g_can_bus.write_index = 0;
    g_can_bus.read_index = 0;
    g_can_bus.count = 0;
    memset(g_can_bus.frames, 0, sizeof(g_can_bus.frames));
}

// Push to the shared network wire
static inline int push_to_bus(CanFrame frame) {
    if (g_can_bus.count >= BUS_QUEUE_LIMIT) {
        return 0;
    }
    g_can_bus.frames[g_can_bus.write_index] = frame;
    g_can_bus.write_index = (g_can_bus.write_index + 1) % BUS_QUEUE_LIMIT;
    g_can_bus.count++;
    return 1;
}

// Pull off the shared network wire
static inline int pop_from_bus(CanFrame *out_frame) {
    if (g_can_bus.count == 0) {
        return 0;
    }
    *out_frame = g_can_bus.frames[g_can_bus.read_index];
    g_can_bus.read_index = (g_can_bus.read_index + 1) % BUS_QUEUE_LIMIT;
    g_can_bus.count--;
    return 1;
}

// ---- Byte layout (documented so encode/decode always agree) ----
// 0x101 ENGINE_RPM  : data[0]=rpm_hi, data[1]=rpm_lo, data[2]=throttle
// 0x102 ENGINE_TEMP : data[0]=temp+40 offset,          data[1]=status
// 0x201 MOTOR_RPM   : data[0]=rpm_hi, data[1]=rpm_lo
// 0x301 GEAR        : data[0]=gear,   data[1]=speed_hi, data[2]=speed_lo

// Bit-packing functions to format our sensor metrics
static inline CanFrame encode_engine_rpm(int rpm, int throttle) {
    CanFrame f;
    f.id = CAN_ID_ENGINE_RPM;
    f.dlc = 3;
    f.data[0] = (uint8_t)((rpm >> 8) & 0xFF);   // high byte
    f.data[1] = (uint8_t)(rpm & 0xFF);          // low byte
    f.data[2] = (uint8_t)(throttle & 0xFF);
    return f;
}

static inline CanFrame encode_engine_temp(float temp, EcuStatus status) {
    CanFrame f;
    f.id = CAN_ID_ENGINE_TEMP;
    f.dlc = 2;
    // Standard -40 offset is applied to support freezing temperatures in a single byte
    f.data[0] = (uint8_t)(temp + 40.0f);
    f.data[1] = (uint8_t)status;
    return f;
}

#endif