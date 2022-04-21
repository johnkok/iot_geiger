set(COMPONENT_SRCS  "main.c"
                    "display.c"
                    "pwm.c"
                    "air_quality.c"
                    "web_server.c"
                    "temp.c"
                    "mqtt_cli.c"
                    "geiger.c"
                    "wifi.c"
                    "connect.c")

idf_component_register(SRCS "${COMPONENT_SRCS}"
                     INCLUDE_DIRS "."
                     EMBED_TXTFILES "certs/cacert.pem" "certs/prvtkey.pem"
                     EMBED_FILES "html/favicon.ico" "html/index.html")
