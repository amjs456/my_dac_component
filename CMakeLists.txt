idf_component_register(SRCS "my_dac_component.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_i2s esp_ringbuf)
