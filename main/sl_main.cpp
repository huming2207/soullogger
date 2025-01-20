#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void)
{
    vTaskDelay(portMAX_DELAY);
}
