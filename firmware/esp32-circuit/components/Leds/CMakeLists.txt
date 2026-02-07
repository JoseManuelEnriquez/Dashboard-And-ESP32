idf_component_register(SRCS "leds.c"
                    INCLUDE_DIRS "./include"
                    PRIV_REQUIRES GPIO esp_driver_gpio
                    REQUIRES Base
                    )