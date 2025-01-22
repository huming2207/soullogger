#include <sdmmc_cmd.h>
#include <cstring>
#include "ArduinoJson.hpp"
#include "sdmmc_manager.hpp"

esp_err_t sdmmc_manager::init(const char *path)
{
    ESP_LOGI(TAG, "Init start");
    host_cfg.max_freq_khz = SDMMC_FREQ_DEFAULT;
    host_cfg.slot = SDMMC_HOST_SLOT_0;
    slot_cfg.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_cfg.width = 1;
    slot_cfg.clk = PIN_CLK;
    slot_cfg.cmd = PIN_CMD;
    slot_cfg.d0 = PIN_D0;
    slot_cfg.d1 = PIN_D1;
    slot_cfg.d2 = PIN_D2;
    slot_cfg.d3 = PIN_D3;

    auto ret = esp_vfs_fat_sdmmc_mount(path, &host_cfg, &slot_cfg, &mount_cfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card, ret=0x%x", ret);
        return ret;
    }

    if (card != nullptr) {
        sdmmc_card_print_info(stdout, card);
    }

    return reload_uart_config();
}

void sdmmc_manager::get_info(sdmmc_card_t *info)
{
    if (info == nullptr) {
        return;
    }

    memcpy(info, card, sizeof(sdmmc_card_t));
}

esp_err_t sdmmc_manager::get_uart_cfg(uart_port_t port, gpio_num_t &tx, gpio_num_t &rx, gpio_num_t &rts, gpio_num_t &cts, uart_config_t *cfg_out)
{
    if (!config_doc["uart"].is<JsonArray>()) {
        ESP_LOGE(TAG, "\'uart\' object isn't array");
        return ESP_ERR_INVALID_STATE;
    }

    auto cfg_array = config_doc["uart"].as<JsonArray>();
    if (cfg_array.size() == 0) {
        ESP_LOGE(TAG, "\'uart\' array is empty");
        return ESP_ERR_INVALID_STATE;
    }

    if (cfg_array[(uint32_t)port].isNull() || !cfg_array[(uint32_t)port].is<JsonObject>()) {
        ESP_LOGE(TAG, "Invalid config for UART port %lu", (uint32_t)port);
        return ESP_ERR_INVALID_STATE;
    }

    auto cfg_obj = cfg_array[(int)port].as<JsonObject>();
    int32_t tx_pin = cfg_obj["tx_pin"];
    int32_t rx_pin = cfg_obj["rx_pin"];
    int32_t cts_pin = cfg_obj["cts_pin"];
    int32_t rts_pin = cfg_obj["rts_pin"];

    if (tx_pin > GPIO_NUM_MAX || tx_pin < 0) {
        tx = GPIO_NUM_NC;
    } else {
        tx = (gpio_num_t)tx_pin;
    }

    if (rx_pin > GPIO_NUM_MAX || rx_pin < 0) {
        rx = GPIO_NUM_NC;
    } else {
        rx = (gpio_num_t)rx_pin;
    }

    if (cts_pin > GPIO_NUM_MAX || cts_pin < 0) {
        cts = GPIO_NUM_NC;
    } else {
        cts = (gpio_num_t)cts_pin;
    }

    if (rts_pin > GPIO_NUM_MAX || rts_pin < 0) {
        rts = GPIO_NUM_NC;
    } else {
        rts = (gpio_num_t)rts_pin;
    }

    if (cfg_out == nullptr) {
        return ESP_OK;
    }

    cfg_out->flags.allow_pd = 0;
    cfg_out->source_clk = UART_SCLK_RTC;
    cfg_out->baud_rate = cfg_obj["baudRate"].as<int>();

    if (cfg_obj["stopBit"].as<double>() == 1.0f) {
        cfg_out->stop_bits = UART_STOP_BITS_1;
    } else if (cfg_obj["stopBit"].as<double>() == 1.5f) {
        cfg_out->stop_bits = UART_STOP_BITS_1_5;
    } else if (cfg_obj["stopBit"].as<double>() == 2.0f) {
        cfg_out->stop_bits = UART_STOP_BITS_2;
    } else {
        ESP_LOGE(TAG, "Invalid stop bit config: %f", cfg_obj["stopBit"].as<double>());
        return ESP_ERR_INVALID_RESPONSE;
    }

    switch (cfg_obj["dataBit"].as<uint8_t>()) {
        case 5: {
            cfg_out->data_bits = UART_DATA_5_BITS;
            break;
        }

        case 6: {
            cfg_out->data_bits = UART_DATA_6_BITS;
            break;
        }

        case 7: {
            cfg_out->data_bits = UART_DATA_7_BITS;
            break;
        }

        case 8: {
            cfg_out->data_bits = UART_DATA_8_BITS;
            break;
        }

        default: {
            ESP_LOGE(TAG, "Invalid stop bit config: %u", cfg_obj["dataBit"].as<uint8_t>());
            return ESP_ERR_INVALID_RESPONSE;
        }
    }

    const char *parity_str = cfg_obj["parity"].as<const char *>();
    if (parity_str != nullptr && strstr(parity_str, "odd") != nullptr) {
        cfg_out->parity = UART_PARITY_ODD;
    } else if (parity_str != nullptr && strstr(parity_str, "even") != nullptr) {
        cfg_out->parity = UART_PARITY_EVEN;
    } else {
        cfg_out->parity = UART_PARITY_DISABLE;
    }


    const char *flow_ctrl_str = cfg_obj["flowCtrl"].as<const char *>();
    if (flow_ctrl_str != nullptr && strstr(flow_ctrl_str, "rts") != nullptr) {
        cfg_out->flow_ctrl = UART_HW_FLOWCTRL_RTS;
    } else if (flow_ctrl_str != nullptr && strstr(flow_ctrl_str, "cts") != nullptr) {
        cfg_out->flow_ctrl = UART_HW_FLOWCTRL_CTS;
    } else if (flow_ctrl_str != nullptr && strstr(flow_ctrl_str, "rtscts") != nullptr) {
        cfg_out->flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS;
    } else {
        cfg_out->flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    }

    return ESP_OK;
}

esp_err_t sdmmc_manager::reload_uart_config(const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open UART config: %s", path);
        return ESP_FAIL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    if (file_size >= MAX_CFG_JSON_SIZE_ALLOWED) {
        fclose(file);
        ESP_LOGE(TAG, "Config file too big: %u", file_size);
        return ESP_ERR_INVALID_SIZE;
    }

    if (cfg_json != nullptr) {
        free(cfg_json);
        cfg_json = nullptr;
    }

    cfg_json = (char *) heap_caps_calloc(1, file_size + 1, MALLOC_CAP_SPIRAM);
    if (cfg_json == nullptr) {
        ESP_LOGE(TAG, "Can't allocate config file buffer");
        return ESP_ERR_NO_MEM;
    }

    size_t read_ret = fread(cfg_json, 1, file_size, file);
    if (read_ret < 1) {
        ESP_LOGE(TAG, "Read config fail: %d", read_ret);
        return ESP_ERR_INVALID_STATE;
    }

    cfg_json[file_size] = '\0';

    fclose(file);
    config_doc.clear();

    auto ret = ArduinoJson::deserializeJson(config_doc, cfg_json);
    if (ret != DeserializationError::Ok) {
        ESP_LOGE(TAG, "Failed to parse config JSON: %s", ret.c_str());
        return ESP_ERR_INVALID_RESPONSE;
    }

    return ESP_OK;
}
