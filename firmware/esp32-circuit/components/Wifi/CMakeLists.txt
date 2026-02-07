idf_component_register(SRCS "wifi.c"
                    INCLUDE_DIRS "./include"
                    REQUIRES esp_wifi
                    REQUIRES nvs_flash
                    REQUIRES esp_system
                    )