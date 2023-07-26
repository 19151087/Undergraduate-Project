/**
 * @file lv_port_indev_templ.c
 *
 *Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"

static void keypad_init(void);
static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
static uint8_t keypad_get_key(void);
static button_t btn1, btn2, btn3;
static void on_button(button_t *btn, button_state_t state); // callback function

bool btn1_clicked = false;
bool btn2_clicked = false;
bool btn3_clicked = false;

lv_indev_t *indev_keypad;

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;
    /*Initialize your keypad or keyboard if you have*/
    keypad_init();
    /*Register a keypad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv);
    // lv_group_focus_obj()

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/
}
/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
    btn1.gpio = CONFIG_EXAMPLE_BUTTON1_GPIO;
    btn1.pressed_level = 0;
    btn1.internal_pull = true;
    btn1.callback = on_button;

    btn2.gpio = CONFIG_EXAMPLE_BUTTON2_GPIO;
    btn2.pressed_level = 0;
    btn2.internal_pull = true;
    btn2.callback = on_button;

    btn3.gpio = CONFIG_EXAMPLE_BUTTON3_GPIO;
    btn3.pressed_level = 0;
    btn3.internal_pull = true;
    btn3.callback = on_button;

    ESP_ERROR_CHECK(button_init(&btn1));
    ESP_ERROR_CHECK(button_init(&btn2));
    ESP_ERROR_CHECK(button_init(&btn3));
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint32_t last_key = 0;

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    if (act_key != 0)
    {
        data->state = LV_INDEV_STATE_PR;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch (act_key)
        {
        case 1:
            act_key = LV_KEY_LEFT;
            printf("key 25 pressed\n");
            break;
        case 2:
            act_key = LV_KEY_RIGHT;
            printf("key 26 pressed\n");
            break;
        case 3:
            act_key = LV_KEY_ENTER;
            printf("key 27 pressed\n");
            break;
        }

        last_key = act_key;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }

    data->key = last_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint8_t keypad_get_key(void)
{
    // thay vi doc tin hieu tu button, ta doc tin hieu tu callback func cua button
    if (btn1_clicked == true)
    {
        btn1_clicked = false;
        return 1;
    }
    else if (btn2_clicked == true)
    {
        btn2_clicked = false;
        return 2;
    }
    else if (btn3_clicked == true)
    {
        btn3_clicked = false;
        return 3;
    }
    else
        return 0;
}

/* Callback function for keypad*/
static void on_button(button_t *btn, button_state_t state)
{
    if (btn == &btn1 && state == BUTTON_RELEASED)
    {
        btn1_clicked = true;
    }
    else if (btn == &btn2 && state == BUTTON_RELEASED)
    {
        btn2_clicked = true;
    }
    else if (btn == &btn3 && state == BUTTON_RELEASED)
    {
        btn3_clicked = true;
    }
}

#else /*Enable this file at the top*/
#endif
