#include "homekit.hpp"
#include "fan.hpp"

extern "C" {
    #include <esp_log.h>

    #include <freertos/queue.h>

    #include <hap.h>
    #include <hap_apple_servs.h>
    #include <hap_apple_chars.h>
    #include <hap_fw_upgrade.h>

    #include <iot_button.h>
    #include <app_wifi.h>
    #include <app_hap_setup_payload.h>

    #include <string.h>
}

#include <Arduino.h>
#include <RCSwitch.h>

#include <iostream>
#include <memory>

#define RESET_GPIO  GPIO_NUM_0

char *server_cert = NULL;

int accessory_write_wrapper(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv) {
    if (serv_priv == nullptr) {
        return HAP_FAIL;
    }
    Fan *fan = reinterpret_cast<Fan *>(serv_priv);
    fan->accessory_write(write_data, count, write_priv);
    return HAP_SUCCESS;
}

static int accessory_identify(hap_acc_t *accessory) {
    (void)accessory;

    return HAP_SUCCESS;
}

// pure to simply send a state update to the fan
void transmitter_process(void *pvParameters)
{
    auto *transmitter = new RCSwitch();

    auto task_params = reinterpret_cast<struct TaskParams*>(pvParameters);

    transmitter->enableTransmit(digitalPinToInterrupt(33));
    transmitter->setProtocol(11);
    transmitter->setPulseLength(228);
    transmitter->setRepeatTransmit(20); // read more into this as it is the cause for some extraneous transmissions

    bool lightbulb_buffer;
    float fan_speed_buffer;

    while (true)
    {
        if (xQueueReceive(task_params->q1, &lightbulb_buffer, pdMS_TO_TICKS(50))) 
        {
            transmitter->send(FanDataMap[Operation::F_LIGHT].value, FanDataMap[Operation::F_LIGHT].bitlength);
        }
        if (xQueueReceive(task_params->q2, &fan_speed_buffer, pdMS_TO_TICKS(50))) 
        {
            if (fan_speed_buffer > 67)
            {
                transmitter->send(FanDataMap[Operation::F_HI].value, FanDataMap[Operation::F_HI].bitlength);
            }
            else if (fan_speed_buffer > 33 and fan_speed_buffer <= 67)
            {
                transmitter->send(FanDataMap[Operation::F_MED].value, FanDataMap[Operation::F_MED].bitlength);
            }
            else if (fan_speed_buffer > 0 and fan_speed_buffer <= 33)
            {
                transmitter->send(FanDataMap[Operation::F_LOW].value, FanDataMap[Operation::F_LOW].bitlength);
            }
            else if (fabs(fan_speed_buffer) == 0)
            {
                transmitter->send(FanDataMap[Operation::F_OFF].value, FanDataMap[Operation::F_OFF].bitlength);
            }
            else {};
        }

        vTaskDelay(pdMS_TO_TICKS(228 + 50));
    }
}

Fan::Fan(hap_acc_cfg_t &hap_acc_cfg) : Accessory::Accessory()
{
    fan_state = std::unique_ptr<struct FanState>(new FanState);

    struct TaskParams *task_params = static_cast<struct TaskParams*>(malloc(sizeof(struct TaskParams)));
    task_params->q1 = xQueueCreate(256, sizeof(bool)); // lightbulb queue
    task_params->q2 = xQueueCreate(256, sizeof(float)); // fan queue

    queue_handles = task_params;

    xTaskCreate(transmitter_process, "transmit_proc", 2 * 2048, static_cast<void *>(task_params), 10, nullptr);

    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;
    hap_set_config(&hap_cfg);

    hap_init(HAP_TRANSPORT_WIFI);

    hap_acc_cfg.identify_routine = &accessory_identify;

    auto fan_acc = hap_acc_create(&hap_acc_cfg);

    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(fan_acc, product_data, sizeof(product_data));

    hap_acc_add_wifi_transport_service(fan_acc, 0);

    auto fan_light_serv = hap_serv_create(HAP_SERV_UUID_LIGHTBULB);
    auto fan_speed_serv = hap_serv_create(HAP_SERV_UUID_FAN);

    hap_serv_add_char(fan_light_serv, hap_char_name_create("Esp Light"));
    hap_serv_add_char(fan_light_serv, hap_char_on_create(false));
    fan_state->light_state = false;

    hap_serv_add_char(fan_speed_serv, hap_char_name_create("Esp Fan"));
    hap_serv_add_char(fan_speed_serv, hap_char_rotation_speed_create(0.0));
    fan_state->light_state = 0.0;

    hap_serv_set_priv(fan_light_serv, this);
    hap_serv_set_priv(fan_speed_serv, this);

    hap_serv_set_write_cb(fan_light_serv, accessory_write_wrapper);
    hap_serv_set_write_cb(fan_speed_serv, accessory_write_wrapper);

    hap_acc_add_serv(fan_acc, fan_light_serv);
    hap_acc_add_serv(fan_acc, fan_speed_serv);
    
    hap_fw_upgrade_config_t ota_config = {
        .server_cert_pem = server_cert,
    };
    fan_light_serv = hap_serv_fw_upgrade_create(&ota_config);
    fan_speed_serv = hap_serv_fw_upgrade_create(&ota_config);

    hap_acc_add_serv(fan_acc, fan_light_serv);
    hap_acc_add_serv(fan_acc, fan_speed_serv);
    
    hap_add_accessory(fan_acc);

    accessory_reset_key_init(RESET_GPIO);

    hap_set_setup_code("111-22-333");
    hap_set_setup_id("ES32");
    app_hap_setup_payload("111-22-333", "ES32", true, hap_acc_cfg.cid);

    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);

    app_wifi_init();

    esp_event_handler_register(HAP_EVENT, ESP_EVENT_ANY_ID, accessory_event_handler, NULL);

    hap_start();

    app_wifi_start(portMAX_DELAY);

    vTaskDelete(NULL);
}

int Fan::accessory_write(hap_write_data_t write_data[], int count, void *write_priv)
{
    for (int i = 0; i < count; i++) {
        hap_write_data_t *write = &write_data[i];
        if (0 == strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) {
                fan_state->light_state = write_data->val.b;
                xQueueSend(queue_handles->q1, &fan_state->light_state, portMAX_DELAY);
        } else if (0 == strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ROTATION_SPEED)) {
            xQueueSend(queue_handles->q2, &write_data->val.f, portMAX_DELAY);
        }
        else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
            return HAP_FAIL;
        }

        hap_char_update_val(write->hc, &(write->val));
        
        *(write->status) = HAP_STATUS_SUCCESS;
        vTaskDelay(10);
    }

    return HAP_SUCCESS;
}