#pragma once

#include <esp_err.h>

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
};
