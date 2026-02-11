idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES Communications Wifi GPIO Events Leds Buttons Sensors)
