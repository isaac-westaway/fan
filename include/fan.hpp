#pragma once

#include "homekit.hpp"

extern "C" {
    #include <hap.h>
}

#include <Arduino.h>
#include <RCSwitch.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdbool>

enum class Operation {
    F_OFF = 0,
    F_LOW = 1,
    F_MED = 2,
    F_HI = 3,
    F_LIGHT = 4
};

struct FanState {
    bool light_state = 0;
    Operation fan_speed_state = Operation::F_OFF;
};

struct FanData {
    int value;
    int bitlength;
};

struct TaskParams {
    QueueHandle_t q1;
    QueueHandle_t q2;
};

static std::unordered_map<Operation, struct FanData> FanDataMap = {
    {Operation::F_OFF, {4029, 12}},
    {Operation::F_LOW, {2045, 12}},
    {Operation::F_MED, {3837, 12}},
    {Operation::F_HI, {3965, 12}},
    {Operation::F_LIGHT, {3069, 12}}
};


class Fan : public Accessory
{
public:
    Fan(hap_acc_cfg_t &hap_acc_cfg);

    int accessory_write(hap_write_data_t write_data[], int count, void *write_priv) override;

protected:
    std::unique_ptr<struct FanState> fan_state;
    struct TaskParams *queue_handles;
};