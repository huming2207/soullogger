idf_component_register(
        SRCS
            "sl_main.cpp"
            "sdmmc_manager.cpp" "sdmmc_manager.hpp"
            "config_loader.cpp" "config_loader.hpp"
            "uart_manager.cpp" "uart_manager.hpp"
        PRIV_REQUIRES fatfs driver
        INCLUDE_DIRS "."
)
