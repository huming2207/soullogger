idf_component_register(
        SRCS
            "sl_main.cpp"
            "sdmmc_manager.cpp" "sdmmc_manager.hpp"
            "config_loader.cpp" "config_loader.hpp"
            "uart_manager.cpp" "uart_manager.hpp"
            "log_writer.cpp" "log_writer.hpp"
        PRIV_REQUIRES fatfs driver
        INCLUDE_DIRS "."
)
