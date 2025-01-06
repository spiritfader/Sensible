#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sensors/sensors.h>
//#include <linux/hwmon.h>
//#include <linux/hwmon-sysfs.h>

#ifndef SENSIBLE
#define SENSIBLE

typedef struct {
    char label[16];
    int value;
} sensor;

void refresh_sensor_window(WINDOW *win, int offset);
const sensors_chip_name *get_chip_name(int num);

#endif