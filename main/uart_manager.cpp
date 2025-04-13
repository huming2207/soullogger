#include <esp_log.h>
#include "uart_manager.hpp"
#include "config_loader.hpp"

esp_err_t uart_manager::init()
{
    auto *cfg = config_loader::instance();
    esp_err_t ret = cfg->get_uart_cfg(uart_port, pin_tx, pin_rx, pin_rts, pin_cts, &uart_cfg);
    if (ret != ESP_OK && uart_is_driver_installed(uart_port) && uart_port != UART_NUM_0) {
        ret = uart_driver_delete(uart_port);
        ESP_LOGW(TAG, "UART%d not configured", uart_port);
        return ret;
    } else {
        ret = uart_driver_install(uart_port, RX_BUF_SIZE, TX_BUF_SIZE, 20, &uart_queue, 0);
        ret = ret ?: uart_param_config(uart_port, &uart_cfg);
        ret = ret ?: uart_set_pin(uart_port, pin_tx, pin_rx, pin_rts, pin_cts);
        ret = ret ?: uart_enable_pattern_det_baud_intr(uart_port, '\n', 1, 9, 0, 0);
        ret = ret ?: uart_pattern_queue_reset(uart_port, 20);
        ESP_LOGI(TAG, "UART%d configured ret=0x%x, Tx=%d, Rx=%d, RTS=%d, CTS=%d", uart_port, ret, pin_tx, pin_rx, pin_rts, pin_cts);
    }

    rx_ringbuf = xRingbufferCreateWithCaps(RX_RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    if (rx_ringbuf == nullptr) {
        ESP_LOGE(TAG, "Rx ringbuffer alloc failed");
        if (uart_is_driver_installed(uart_port)) {
            uart_driver_delete(uart_port);
        }
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Creating event task: %s", task_name);
    xTaskCreateWithCaps(uart_event_task, task_name, 32768, this, tskIDLE_PRIORITY + 3, &evt_task_handle, MALLOC_CAP_SPIRAM);
    return ret;
}

void uart_manager::uart_event_task(void *_ctx)
{
    uart_event_t evt = {};
    auto *ctx = (uart_manager *)_ctx;
    if (_ctx == nullptr || ctx->uart_queue == nullptr) {
        ESP_LOGE(TAG, "UART queue not configured");
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        if (!xQueueReceive(ctx->uart_queue, (void *)&evt, (TickType_t)portMAX_DELAY)) {
            continue;
        }

        switch (evt.type) {
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;

            case UART_FIFO_OVF:
            case UART_BUFFER_FULL: {
                ESP_LOGW(TAG, "UART%d buffer full", ctx->uart_port);
                uart_flush(ctx->uart_port);
                break;
            }
            case UART_FRAME_ERR: {
                ESP_LOGW(TAG, "UART%d frame error", ctx->uart_port);
                break;
            }
            case UART_PARITY_ERR: {
                ESP_LOGW(TAG, "UART%d parity error", ctx->uart_port);
                break;
            }
            case UART_PATTERN_DET: {
                size_t buf_size = 0;
                uart_get_buffered_data_len(ctx->uart_port, &buf_size);
                int pos = uart_pattern_pop_pos(ctx->uart_port);
                if (pos < 0) {
                    // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                    // record the position. We should set a larger queue size.
                    // As an example, we directly flush the rx buffer here.
                    uart_flush_input(ctx->uart_port);
                } else if (pos != 0) {
                    uint8_t *buf = nullptr;
                    size_t buf_offset = 0;
                    if (ctx->enable_timestamp) {
                        char ts_str[128] = { 0 };
                        struct timeval val = {};
                        gettimeofday(&val, nullptr);
                        snprintf(ts_str, sizeof(ts_str), "[%lld%06ld] ", val.tv_sec, val.tv_usec);
                        ts_str[sizeof(ts_str) - 1] = '\0';
                        buf_offset = strnlen(ts_str, sizeof(ts_str));
                    }

                    auto rb_ret = xRingbufferSendAcquire(ctx->rx_ringbuf, (void **)&buf, pos + buf_offset, pdMS_TO_TICKS(300));
                    if (rb_ret != pdTRUE || buf == nullptr) {
                        ESP_LOGW(TAG, "UART%d failed to allocate ringbuffer, giving up len=%d", ctx->uart_port, pos);
                        uart_flush_input(ctx->uart_port);
                        break;
                    }

                    int read_ret = uart_read_bytes(ctx->uart_port, buf + buf_offset, pos, pdMS_TO_TICKS(300));
                    if (read_ret < 0) {
                        ESP_LOGW(TAG, "UART%d failed to read: %d", ctx->uart_port, read_ret);
                        uart_flush_input(ctx->uart_port); // Try to reset...
                    }

                    xRingbufferSendComplete(ctx->rx_ringbuf, buf);
                } else {
                    ESP_LOGE(TAG, "UART%d pattern detection error (nothing for Rx?)", ctx->uart_port);
                }

                break;
            }
            case UART_WAKEUP: {
                ESP_LOGI(TAG, "Waking up from UART %u", ctx->uart_port);
                break;
            }
            default: {
                break;
            }
        }
    }
}

esp_err_t uart_manager::wait_for_newline(uint8_t **buf, size_t *len_out, uint32_t wait_ticks)
{
    if (buf == nullptr || len_out == nullptr) {
        return ESP_ERR_INVALID_ARG;
    }

    void *out = xRingbufferReceive(rx_ringbuf, len_out, wait_ticks);
    if (out == nullptr) {
        return ESP_ERR_TIMEOUT;
    }

    *buf = (uint8_t *)out;
    return ESP_OK;
}

void uart_manager::finish_newline(uint8_t *buf)
{
    vRingbufferReturnItem(rx_ringbuf, buf);
}

void uart_manager::toggle_timestamp_prepend(bool enable)
{
    enable_timestamp = enable;
}
