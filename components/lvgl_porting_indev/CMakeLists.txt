set(app_src lv_port_indev.c)
set(req button lvgl esp_idf_lib_helpers)
idf_component_register(SRCS ${app_src}
                        INCLUDE_DIRS "."
                        REQUIRES ${req})