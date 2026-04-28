# Register the myMqtt component with its dependencies
idf_component_register(SRCS "myMqtt.cpp"
                        INCLUDE_DIRS "include"
                        REQUIRES driver mqtt)

# Add the main directory to include path to allow access to cowAutomation_settings.h
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/main)
                        