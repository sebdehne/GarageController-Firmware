#define FIRMWARE_VERSION 7

// reserve flash (in bytes) for firmware and userspace
#define FIRMWARE_SIZE 40000 // should be large enough to hold the firmware
#define USERDATA_SIZE 1

// note: when using DEBUG - power is not removed and the ChipCap2 does not make new measurements
// #define DEBUG

#define LORA_RETRY_DELAY random(0, 500)
#define LORA_RETRY_FIRMWARE_COUNT 30
