#include <sdmmc_cmd.h>
#include <cstring>
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

    return ret;
}

void sdmmc_manager::get_info(sdmmc_card_t *info)
{
    if (info == nullptr) {
        return;
    }

    memcpy(info, card, sizeof(sdmmc_card_t));
}
