idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES DHT11
                    REQUIRES Communications
                    REQUIRES Wifi
                    REQUIRES GPIO
                    REQUIRES Events
                    REQUIRES Leds
                    )
