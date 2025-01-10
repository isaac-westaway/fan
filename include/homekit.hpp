#pragma once

extern "C" {
    #include <driver/gpio.h>
    
    #include <hap.h>

}

class Accessory
{
public:
    Accessory();

    static void accessory_reset_network_handler(void *arg);
    static void accessory_reset_to_factory_handler(void *arg);
    static void accessory_reset_key_init(gpio_num_t gpio_pin);

    static void accessory_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

    virtual int accessory_write(hap_write_data_t write_data[], int count, void *write_priv) = 0;

    virtual ~Accessory() = default;
};