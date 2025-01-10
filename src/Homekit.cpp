#include "homekit.hpp"

extern "C" {
    #include <esp_log.h>

    #include <driver/gpio.h>

    #include <hap.h>

    #include <iot_button.h>
}

#define RESET_NETWORK_BUTTON_TIMEOUT    3
#define RESET_TO_FACTORY_BUTTON_TIMEOUT 10

// this is pretty useless and could just be moved entirely  to the fan constructor
Accessory::Accessory() {}

void Accessory::accessory_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    static const char *TAG = "Hap Fan";

    switch(event_id)
    {
        case HAP_EVENT_PAIRING_STARTED:
            ESP_LOGI(TAG, "\nPairing started\n");
            break;
        case HAP_EVENT_PAIRING_ABORTED:
            ESP_LOGI(TAG, "\nPairing aborted\n");
            break;
        case HAP_EVENT_CTRL_PAIRED:
            ESP_LOGI(TAG, "\nController %s Paired. Controller count: %d\n", (char *)event_data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_UNPAIRED:
            ESP_LOGI(TAG, "\nController %s REmoved. Controller count: %d\n", (char *)event_data, hap_get_paired_controller_count());
            break;
        case HAP_EVENT_CTRL_CONNECTED:
            ESP_LOGI(TAG, "\nController %s Connected\n", (char *)event_data);
            break;
        case HAP_EVENT_CTRL_DISCONNECTED:
            ESP_LOGI(TAG, "\nController %s Disconnected\n", (char *)event_data);
            break;
        case HAP_EVENT_ACC_REBOOTING: {
            char *reason = (char *)event_data;
            ESP_LOGI(TAG, "\nAccessory Rebooting. Reason: %s\n", reason ? reason : "null");
            break;
        }
        case HAP_EVENT_PAIRING_MODE_TIMED_OUT:
            ESP_LOGI(TAG, "\nPairing mode timed out\n");
            break;
        case HAP_EVENT_GET_ACC_COMPLETED:
            ESP_LOGI(TAG, "\nGet accessory completed\n");
            break;
        case HAP_EVENT_GET_CHAR_COMPLETED:
            ESP_LOGI(TAG, "\nGet char completed\n");
            break;
        case HAP_EVENT_SET_CHAR_COMPLETED:
            ESP_LOGI(TAG, "\nSet char completed\n");
            break;
        default:
            ESP_LOGI(TAG, "\nUnknown event: %ld\n", event_id);
            break;
    }
}


void Accessory::accessory_reset_network_handler(void *arg)
{
    hap_reset_network();
}

void Accessory::accessory_reset_to_factory_handler(void *arg)
{
    hap_reset_to_factory();
}

void Accessory::accessory_reset_key_init(gpio_num_t gpio_pin) 
{
    auto reset_network_button_handle = iot_button_create(gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(reset_network_button_handle, RESET_NETWORK_BUTTON_TIMEOUT, accessory_reset_network_handler, nullptr);
    iot_button_add_on_press_cb(reset_network_button_handle, RESET_TO_FACTORY_BUTTON_TIMEOUT, accessory_reset_to_factory_handler, nullptr);
}