// lvgl
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

static void event_handler(lv_event_t * e)
{
    lv_obj_t * mbox1 = lv_msgbox_create(NULL, "Title", "Message", NULL, true);
    lv_obj_center(mbox1);
}

extern lv_indev_t * indev_keypad;
void create_test03_gui(void)
{
    int i;
    lv_group_t * group = lv_group_create();

    lv_obj_t *list1 = lv_list_create(lv_scr_act());
    lv_obj_set_size(list1, 600, 450);
	lv_obj_center(list1);
    for (i = 0; i < 50; i++) {
        lv_obj_t * btn = lv_list_add_btn(list1, LV_SYMBOL_DUMMY, "game");
        lv_obj_add_event_cb(btn, event_handler, LV_EVENT_CLICKED, NULL);
        lv_group_add_obj(group, btn);
    }

    lv_indev_set_group(indev_keypad, group);
}

int main(int argc, char *argv[])
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    create_test03_gui();

    while (1) {
        lv_timer_handler();
    }

    return 0;
}
