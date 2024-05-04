idf_component_register(SRCS "misc.c" "sau_heap_debug.c" "wifi_connect.c" "camera.c" "analyser.c" "server_communications.c" "camau_controller.c" "registration.c" "main.c";
                    INCLUDE_DIRS ".")

# Fortify source code
component_compile_options(-D_FORTIFY_SOURCE=3) # Enable fortify source level 3