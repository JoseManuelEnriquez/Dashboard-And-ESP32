idf_component_register(SRCS "communications.c"
                    INCLUDE_DIRS "./include"
                    REQUIRES Frozen
                    REQUIRES mqtt
                    )