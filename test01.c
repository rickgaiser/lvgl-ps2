// lvgl
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lvgl/demos/benchmark/lv_demo_benchmark.h"

int main(int argc, char *argv[])
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    lv_demo_benchmark();

    while (1) {
        lv_timer_handler();
    }

    return 0;
}
