idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_driver_gpio
                    REQUIRES DHT11
                    REQUIRES Frozen
                    PRIV_REQUIRES mqtt
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash)
