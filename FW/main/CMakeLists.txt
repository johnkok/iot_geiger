set(COMPONENT_SRCS  "main.c"
                    "display.c"
                    "pwm.c"
                    "air_quality.c"
                    "web_server.c"
                    "temp.c"
                    "mqtt_cli.c"
                    "geiger.c"
                    "wifi.c"
                    "connect.c"
                    "DHT22.c")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                     INCLUDE_DIRS "."
                     EMBED_TXTFILES "certs/cacert.pem" "certs/prvtkey.pem")

spiffs_create_partition_image(storage ../html FLASH_IN_PROJECT)
