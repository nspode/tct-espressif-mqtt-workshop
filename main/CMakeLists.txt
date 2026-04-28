idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES  hello esp_wifi esp_driver_rmt esp_netif esp_event myMqtt json led_strip 
                       EMBED_TXTFILES "selfsigned_techday_rootCA.pem")
                                   
                       