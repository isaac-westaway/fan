#include "fan.hpp"
#include "homekit.hpp"

extern "C" {
    #include <freertos/task.h>
}

#include <Arduino.h>

#define FAN_TASK_PRIORITY   1
#define FAN_TASK_STACKSIZE  4 * 1024
#define FAN_TASK_NAME       "hap_fan"

void instantiation_of_homekit(void *pvParameter)
{
    hap_acc_cfg_t cfg = {
        .name = "fan",
        .model = "yeah",
        .manufacturer = "isaacw",
        .serial_num = "001122334455",
        .fw_rev = "0.9.0",
        .hw_rev = nullptr,
        .pv = "1.1.0",
        .cid = HAP_CID_FAN,
    };

    auto *fan = new Fan(cfg);
}

extern "C" void app_main(void)
{
    initArduino();

    Serial.begin(115200);
    xTaskCreate(instantiation_of_homekit, FAN_TASK_NAME, FAN_TASK_STACKSIZE, nullptr, FAN_TASK_PRIORITY, nullptr);
}
