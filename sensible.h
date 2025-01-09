#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sensors/sensors.h>

#ifndef SENSIBLE
#define SENSIBLE

void refresh_sensor_window(WINDOW *win, int offset);
const sensors_chip_name *get_chip_name(int num);

#endif