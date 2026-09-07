idf_component_register(SRCS "potentioMeter.c" "throttleControl_app.c"
                    INCLUDE_DIRS "."
                    REQUIRES bluetooth
                    PRIV_REQUIRES esp_driver_gpio esp_adc)