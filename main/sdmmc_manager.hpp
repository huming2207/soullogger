#pragma once

#include <esp_err.h>
#include <esp_vfs_fat.h>
#include <driver/sdmmc_host.h>
#include <hal/uart_types.h>
#include <driver/uart.h>
#include "PsramAllocator.hpp"

class sdmmc_manager
{
public:
    static sdmmc_manager *instance()
    {
        static sdmmc_manager _instance;
        return &_instance;
    }

    sdmmc_manager(sdmmc_manager const &) = delete;
    void operator=(sdmmc_manager const &) = delete;

private:
    sdmmc_manager() = default;

public:
    esp_err_t init(const char *path = "/sdcard");
    void get_info(sdmmc_card_t *info);
    esp_err_t get_uart_cfg(uart_port_t port, gpio_num_t &tx, gpio_num_t &rx, gpio_num_t &rts, gpio_num_t &cts, uart_config_t *cfg_out);
    esp_err_t reload_uart_config(const char *path = "/sdcard/config.json");

private:
    sdmmc_card_t *card = nullptr;
    char *cfg_json = nullptr;
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
            .format_if_mount_failed = true,
            .max_files = 3,
            .allocation_unit_size = 128*512,
            .disk_status_check_enable = true,
            .use_one_fat = false,
    };

    sdmmc_host_t host_cfg = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_cfg = SDMMC_SLOT_CONFIG_DEFAULT();

    SpiRamAllocator json_allocator = {};
    ArduinoJson::JsonDocument config_doc = ArduinoJson::JsonDocument(&json_allocator);

private:
    static const constexpr char TAG[] = "sdmmc_mgr";
    static const constexpr gpio_num_t PIN_CMD = GPIO_NUM_35;
    static const constexpr gpio_num_t PIN_CLK = GPIO_NUM_36;
    static const constexpr gpio_num_t PIN_D0 = GPIO_NUM_37;
    static const constexpr gpio_num_t PIN_D1 = GPIO_NUM_38;
    static const constexpr gpio_num_t PIN_D2 = GPIO_NUM_39;
    static const constexpr gpio_num_t PIN_D3 = GPIO_NUM_40;
    static const constexpr uint32_t MAX_CFG_JSON_SIZE_ALLOWED = 131072;
};
