idf_component_register(SRCS "app_wifi.c"
                    INCLUDE_DIRS "."
                    REQUIRES wifi_provisioning homekit_qrcode esp_hap_core esp_hap_platform nvs_flash)
