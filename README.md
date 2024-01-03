## Build
To compile run `make lvgl` first. This will clone the lvgl repository.
Then run `make all`.

There are 3 tests:
- test01: LVGL Benchmark
- test02: LVGL Widgets demo
- test03: Simple self made

## Controller input
A very basic controller input is implemented:
- up key is mapped to previous
- down key is mapped to next
- x key is mapped to enter
Try test03 to see how that works.

For your own application you probably need to create your own key mapping.
A 'mouse' using one of the joysticks would also be possible.
See lv_port_indev.c for customization.

## Display output
The video is software rendered, 640x480, 32bit, with tranparency. Currently this output is put in a texture and shown full-screen.
Many options are possible here.
See lv_port_disp.c for customization.

## Your own application using LVGL
Please do with this code whatever you like. Have fun.
