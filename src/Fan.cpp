#include "homekit.hpp"
#include "fan.hpp"

extern "C" {
    #include <freertos/queue.h>

    #include <driver/gpio.h> 

    #include <hap.h>
    #include <hap_apple_servs.h>
    #include <hap_apple_chars.h>
    #include <hap_fw_upgrade.h>

    #include <app_wifi.h>
    #include <app_hap_setup_payload.h>
}

#include <Arduino.h>
#include <RCSwitch.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <cstring>

#define RESET_GPIO  GPIO_NUM_0

char *server_cert = NULL;

int accessory_write_wrapper(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv)
{
    if (serv_priv == nullptr)
    {
        return HAP_FAIL;
    }
    auto fan = static_cast<Fan *>(serv_priv);
    fan->accessory_write(write_data, count, write_priv);
    return HAP_SUCCESS;
}

static int accessory_identify(hap_acc_t *accessory)
{
    (void)accessory;

    return HAP_SUCCESS;
}

void transmitter_process(void *pvParameters)
{
    auto *transmitter = new RCSwitch();

    auto task_params = static_cast<struct TaskParams*>(pvParameters);

    transmitter->enableTransmit(digitalPinToInterrupt(33));
    transmitter->setProtocol(11);
    transmitter->setPulseLength(228);
    transmitter->setRepeatTransmit(10);

    bool lightbulb_buffer;
    Operation fan_speed_buffer;

    while (true)
    {
        if (xQueueReceive(task_params->q1, &lightbulb_buffer, pdMS_TO_TICKS(50))) 
        {
            // dont need to do anything with the queue value
            transmitter->send(FanDataMap[Operation::F_LIGHT].value, FanDataMap[Operation::F_LIGHT].bitlength);
            vTaskDelay(50);
        }
        if (xQueueReceive(task_params->q2, &fan_speed_buffer, pdMS_TO_TICKS(50))) 
        {
            transmitter->send(FanDataMap[fan_speed_buffer].value, FanDataMap[fan_speed_buffer].bitlength);
        
            vTaskDelay(50);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));;
    }
}

Fan::Fan(hap_acc_cfg_t &hap_acc_cfg) : Accessory::Accessory()
{
    auto task_params = static_cast<struct TaskParams*>(malloc(sizeof(struct TaskParams)));
    task_params->q1 = xQueueCreate(256, sizeof(bool)); // lightbulb queue
    task_params->q2 = xQueueCreate(256, sizeof(float)); // fan queue
    this->queue_handles = task_params;
    xTaskCreate(transmitter_process, "transmit_proc", 2 * 2048, static_cast<void *>(this->queue_handles), 10, nullptr);
    
    this->fan_state = std::make_unique<struct FanState>();

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

    hap_serv_set_write_cb(fan_light_serv, &accessory_write_wrapper);
    hap_serv_set_write_cb(fan_speed_serv, &accessory_write_wrapper);

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
    for (int i = 0; i < count; i++) 
    {
        hap_write_data_t *write = &write_data[i];

        if (0 == strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ON)) 
        {
                this->fan_state->light_state = write_data->val.b;
                xQueueSend(this->queue_handles->q1, &this->fan_state->light_state, portMAX_DELAY);
        }
        else if (0 == strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_ROTATION_SPEED)) 
        {
            if (write_data->val.f > 67) this->fan_state->fan_speed_state = Operation::F_HI;
            else if (write_data->val.f > 33 and write_data->val.f <= 67) this->fan_state->fan_speed_state = Operation::F_MED;
            else if (write_data->val.f > 0 and write_data->val.f <= 33) this->fan_state->fan_speed_state = Operation::F_LOW;
            else if (fabs(write_data->val.f) == 0) this->fan_state->fan_speed_state = Operation::F_OFF;
            else {};
            
            xQueueSend(this->queue_handles->q2, &this->fan_state->fan_speed_state, portMAX_DELAY);
        }
        else 
        {
            *(write->status) = HAP_STATUS_RES_ABSENT;
            return HAP_FAIL;
        }

        hap_char_update_val(write->hc, &(write->val));
        
        *(write->status) = HAP_STATUS_SUCCESS;
        vTaskDelay(10);
    }

    return HAP_SUCCESS;
}