set(srcs "main.c" "mqtt_sub.c" "mqtt_pub.c" "uart.c")

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "." EMBED_TXTFILES root_cert.pem)
