#include "ecu_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

CanBus g_can_bus;

const char* status_to_str(EcuStatus status) {
    if (status == STATUS_OK) return "OK";
    if (status == STATUS_WARNING) return "WARNING";
    return "FAULT";
}

int main() {
    srand(time(NULL));
    init_can_bus();

    int eng_rpm = 1000;
    float eng_temp = 75.0f;
    int motor_rpm = 3000;
    int gear = 1;
    float vehicle_speed = 25.5f;

    printf("=====================================================\n");
    printf("     VEHICLE CAN BUS TELEMETRY SIMULATOR RUNNING     \n");
    printf("=====================================================\n");

    for (int tick = 1; tick <= 4; tick++) {
        printf("\n--- [CLOCK TICK %d] -----------------------------------\n", tick);
        int throttle = 15 * tick;

        // 1. ENGINE ECU Simulation
        eng_rpm = 800 + (throttle * 25) + (rand() % 100 - 50);
        eng_temp += 0.5f;
        CanFrame eng_rpm_frame = encode_engine_rpm(eng_rpm, throttle);
        CanFrame eng_temp_frame = encode_engine_temp(eng_temp, STATUS_OK);
        push_to_bus(eng_rpm_frame);
        push_to_bus(eng_temp_frame);

        // 2. EV MOTOR ECU Simulation
        motor_rpm = 2500 + (rand() % 400 - 200);
        CanFrame motor_frame;
        motor_frame.id = CAN_ID_MOTOR_RPM;
        motor_frame.dlc = 2;
        motor_frame.data[0] = (uint8_t)((motor_rpm >> 8) & 0xFF);  // high byte
        motor_frame.data[1] = (uint8_t)(motor_rpm & 0xFF);         // low byte
        push_to_bus(motor_frame);

        // 3. TCU (Transmission Control Unit) Simulation
        vehicle_speed = (motor_rpm * 0.012f) * gear;
        if (vehicle_speed > 31.0f && gear == 1) {
            gear = 2;
            printf(">>> [TCU ALERT] Speed > 31.0 km/h! Shifting to Gear %d\n", gear);
        }
        CanFrame tcu_frame;
        tcu_frame.id = CAN_ID_GEAR;
        tcu_frame.dlc = 3;
        tcu_frame.data[0] = (uint8_t)gear;
        uint16_t scaled_speed = (uint16_t)(vehicle_speed * 100.0f);
        tcu_frame.data[1] = (uint8_t)((scaled_speed >> 8) & 0xFF);  // high byte
        tcu_frame.data[2] = (uint8_t)(scaled_speed & 0xFF);         // low byte
        push_to_bus(tcu_frame);

        // 4. Raw Wire Dump: Print raw frames sitting on the bus wire
        printf("\n>>> [RAW WIRE DUMP] Telemetry Packed on Bus:\n");
        printf("CAN ID | DLC | Payload Hex Bytes\n");
        printf("-------+-----+---------------------------------------\n");

        CanFrame current_frame;
        CanFrame logs_to_decode[BUS_QUEUE_LIMIT];
        int frame_count = 0;

        while (pop_from_bus(&current_frame)) {
            printf("0x%03X  |  %d  | ", current_frame.id, current_frame.dlc);
            for (int i = 0; i < current_frame.dlc; i++) {
                printf("%02X ", current_frame.data[i]);
            }
            for (int i = current_frame.dlc; i < 8; i++) {
                printf(".. ");
            }
            printf("\n");
            logs_to_decode[frame_count++] = current_frame;
        }

        // 5. Decode: Receiver turns those raw hex bytes back into physical data
        printf("\n>>> [DECODED PANEL] Human-Readable Readout:\n");
        for (int i = 0; i < frame_count; i++) {
            CanFrame *f = &logs_to_decode[i];
            if (f->id == CAN_ID_ENGINE_RPM) {
                int rpm = (f->data[0] << 8) | f->data[1];
                int throt = f->data[2];
                printf("  [Engine] RPM: %d, Throttle: %d%%\n", rpm, throt);
            }
            else if (f->id == CAN_ID_ENGINE_TEMP) {
                float temp = (float)f->data[0] - 40.0f; // Decode temperature offset
                EcuStatus status = (EcuStatus)f->data[1];
                printf("  [Engine] Coolant Temp: %.1f C, Status: %s\n", temp, status_to_str(status));
            }
            else if (f->id == CAN_ID_MOTOR_RPM) {
                int m_rpm = (f->data[0] << 8) | f->data[1];
                printf("  [Motor]  RPM: %d\n", m_rpm);
            }
            else if (f->id == CAN_ID_GEAR) {
                int gear_val = f->data[0];
                uint16_t speed_raw = (f->data[1] << 8) | f->data[2];
                float speed = (float)speed_raw / 100.0f; // Decode speed scaling factor
                printf("  [TCU]    Gear: %d, Vehicle Speed: %.2f km/h\n", gear_val, speed);
            }
        }
    }
    printf("\n=====================================================\n");
    printf("     SIMULATION SUCCESSFUL: CAN FRAME ENCODING PROVEN\n");
    printf("=====================================================\n");
    return 0;
}