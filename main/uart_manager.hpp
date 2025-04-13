#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/ringbuf.h>

class uart_manager
{
public:
    explicit uart_manager(const char *_name = "uart0", uart_port_t _port = UART_NUM_0) : task_name(_name), uart_port(_port) {}
    esp_err_t init();
    static void uart_event_task(void *_ctx);
    esp_err_t wait_for_newline(uint8_t **buf, size_t *len_out, uint32_t wait_ticks = portMAX_DELAY);
    void finish_newline(uint8_t *buf);
    void toggle_timestamp_prepend(bool enable);

private:
    const char *task_name;
    bool enable_timestamp = true;
    uart_port_t uart_port;
    gpio_num_t pin_tx = GPIO_NUM_NC;
    gpio_num_t pin_rx = GPIO_NUM_NC;
    gpio_num_t pin_rts = GPIO_NUM_NC;
    gpio_num_t pin_cts = GPIO_NUM_NC;
    QueueHandle_t uart_queue = nullptr;
    RingbufHandle_t rx_ringbuf = nullptr;
    TaskHandle_t evt_task_handle = nullptr;
    uart_config_t uart_cfg = {};

private:
    static const constexpr size_t TX_BUF_SIZE = 256;
    static const constexpr size_t RX_BUF_SIZE = 8192;
    static const constexpr size_t RX_RINGBUF_SIZE = 2097152; // 2MB
    static const constexpr char TAG[] = "uart_wrapper";
};


