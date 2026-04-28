idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES  hello esp_wifi esp_driver_rmt esp_netif esp_event myMqtt json led_strip 
                       EMBED_TXTFILES   "aws_iot_rootCA.pem"
                                        "device_cert.pem"
                                        "device_private_key.pem")
                                   
                       