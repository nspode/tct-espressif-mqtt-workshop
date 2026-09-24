# No ESP-IDF 6.x o componente "json" foi substituído pelo gerenciado "espressif/cjson"
if(IDF_VERSION_MAJOR GREATER_EQUAL 6)
    set(JSON_COMPONENT cjson)
else()
    set(JSON_COMPONENT json)
endif()

idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES  hello esp_wifi esp_driver_rmt esp_netif esp_event myMqtt ${JSON_COMPONENT} led_strip )
