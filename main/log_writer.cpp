#include <esp_log.h>
#include "log_writer.hpp"
#include "sdmmc_manager.hpp"

esp_err_t log_writer::init()
{
    esp_err_t ret = sdmmc_manager::instance()->init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Can't set up SD card 0x%x", ret);
        return ret;
    }

    ret = uart1.init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "UART1 not configured, 0x%x", ret);
    }

    ret = uart2.init();
    if (ret != ESP_OK){
        ESP_LOGW(TAG, "UART2 not configured, 0x%x", ret);
    }

    return ret;
}
