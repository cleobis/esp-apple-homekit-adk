idf_component_register(SRCS ./app_wifi.c ./app_main.c ./DB.c ./App.cpp
                       INCLUDE_DIRS ".")
add_definitions(-DHAP_LOG_LEVEL=${CONFIG_HAP_LOG_LEVEL})
