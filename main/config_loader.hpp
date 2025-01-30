#pragma once

#include <ArduinoJson.hpp>
#include <driver/gpio.h>
#include <driver/uart.h>
#include "PsramAllocator.hpp"

class config_loader
{
public:
    static config_loader *instance()
    {
        static config_loader _instance;
        return &_instance;
    }

    config_loader(config_loader const &) = delete;
    void operator=(config_loader const &) = delete;

private:
    config_loader() = default;

public:
    esp_err_t reload_config(const char *path = "/sdcard/config.json");
    esp_err_t get_uart_cfg(uart_port_t port, gpio_num_t &tx, gpio_num_t &rx, gpio_num_t &rts, gpio_num_t &cts, uart_config_t *cfg_out);


private:
    char *cfg_json = nullptr;
    SpiRamAllocator json_allocator = {};
    ArduinoJson::JsonDocument config_doc = ArduinoJson::JsonDocument(&json_allocator);

private:
    static const constexpr char TAG[] = "cfg_loader";
    static const constexpr uint32_t MAX_CFG_JSON_SIZE_ALLOWED = 131072;
};

