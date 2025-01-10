idf_component_register(SRCS "main.cpp" "Fan.cpp" "Homekit.cpp"
                        INCLUDE_DIRS "." "../include"
                        REQUIRES
                            freertos
                            esp_driver_gpio
                            esp_hap_core 
                            esp_hap_apple_profiles 
                            esp_hap_extras 
                            esp_hap_platform
                            esp_hap_hkdf-sha 
                            esp_hap_json_generator 
                            esp_hap_json_parser 
                            esp_hap_mu_srp 
                            homekit_qrcode 
                            button 
                            app_wifi 
                            app_hap_setup_payload

                            arduino
                            RCSwitch
                        )
