idf_component_register(SRCS "main.cpp" "Fan.cpp" "Homekit.cpp"
                        INCLUDE_DIRS "." "../include"
                        REQUIRES 
                            esp_hap_core 
                            esp_hap_apple_profiles 
                            esp_hap_extras 
                            esp_hap_platform
                            hkdf-sha 
                            homekit_json_generator 
                            homekit_json_parser 
                            mu_srp 
                            homekit_qrcode 
                            button 
                            app_wifi 
                            app_hap_setup_payload

                            arduino
                            RCSwitch
                        )
