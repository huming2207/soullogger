#pragma once

#include <esp_err.h>
#include "uart_manager.hpp"
#include "config_loader.hpp"

class log_writer
{
public:
    static log_writer *instance()
    {
        static log_writer _instance;
        return &_instance;
    }

    log_writer(log_writer const &) = delete;
    void operator=(log_writer const &) = delete;

private:
    log_writer() = default;

public:
    esp_err_t init();

private:
    uart_manager uart1 = uart_manager("uart1_mgr", UART_NUM_1);
    uart_manager uart2 = uart_manager("uart2_mgr", UART_NUM_2);

private:
    static const constexpr char TAG[] = "logger";
};
