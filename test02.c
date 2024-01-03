// lvgl
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl/demos/widgets/lv_demo_widgets.h"

int main(int argc, char *argv[])
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    lv_demo_widgets();

    while (1) {
        lv_timer_handler();
    }

    return 0;
}
