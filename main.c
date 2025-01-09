/*
Sensible: A TUI hardware monitoring program using ncurses.
Authors: @Spiritfader, @NerdyKyogre
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <sensors/sensors.h>
#include "sensible.h"
#define SENSOR_WIDTH 32

int main(int argc, char **argv) {
    //standard ncurses setup
    //initialize screen, get windows in place
    initscr();
    timeout(50);

    //get screen dimensions
    int scrHeight, scrWidth;
    getmaxyx(stdscr, scrHeight, scrWidth);

    int numSensors = scrWidth / SENSOR_WIDTH;

    //initialize sensor windows
    WINDOW *sensors[numSensors];
    for (int i = 0; i < numSensors; i++) {
        sensors[i] = newwin((scrHeight - 3), (scrWidth / numSensors), 0, (i * (scrWidth / numSensors)));
    }
    //set up commands - this window is static
    WINDOW *commands = newwin(3, scrWidth, (scrHeight - 3), 0);
    mvwprintw(commands, 0, 1, "Use the left and right arrow keys to view more sensors, or press Q to quit Sensible.");
    wrefresh(commands);
    
    int offset = 0; //start at top of list

    //refresh each window
    //need keypad mode to use arrows
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int loop = 0, input; //we use loop to poll faster than the refresh interval

    while (1) {
        input = getch();
        
        //only refresh on keypress or every 20th 50ms loop (1 second)
        if ((loop <= 20) && (input == ERR)) {
            loop++;
            continue;
        }

        //if we want to fully refresh, we do the rest of the loop
        loop = 0;
        //this equals ERR on no input
        switch (input) {
            case KEY_LEFT:
                if (offset > 0) {offset--;}
                break;
            case KEY_RIGHT:
                if (get_chip_name(offset + numSensors) != NULL) {offset++;}
                sensors_cleanup();
                break;
            case 'q':
                //end before exiting
                endwin();
                exit(0);
            default:
                break;
        }

        //refresh all windows
        for (int i = 0; i < numSensors; i++) {
            refresh_sensor_window(sensors[i], (i + offset));
            mvwprintw(commands, 0, 1, "Use the left and right arrow keys to view more sensors, or press Q to quit Sensible.");
            wrefresh(commands);

        }
    }
}

void refresh_sensor_window(WINDOW *win, int offset) { 
    wclear(win);
    //create two columns - name and value
    box(win, 0, 0);

    const sensors_chip_name *chip = get_chip_name(offset);

    if (chip == NULL) { //display empty box if no sensor available
        wrefresh(win);
        return;
    }

    mvwprintw(win, 0, 1, (*chip).prefix);
    mvwprintw(win, 1, 1, "SENSOR");
    mvwprintw(win, 1, 16, "           VALUE");

    int line = 3;

    sensors_feature const *feat;
    int f = 0;
    while ((feat = sensors_get_features(chip, &f)) != 0) {
        char *label = sensors_get_label(chip, feat); //according to libsensors documentation this dynamically allocates a string, which we can free as soon as it's in the buffer
        mvwprintw(win, line++, 1, label);
        free(label);

        sensors_subfeature const *subf;
        int s = 0;
        while ((subf = sensors_get_all_subfeatures(chip, feat, &s)) != 0) {
            double val;
            int rc = sensors_get_value(chip, (*subf).number, &val);
            //sensors_get_value(chip, (*subf).number, &val);
            if (rc >= 0) {
                mvwprintw(win, line, 1, (*subf).name);
                mvwprintw(win, line++, 16, "%16.2lf", val);
            }
        }
        line++;
    }

    sensors_cleanup();
    //refresh window when done
    wrefresh(win);
    return;
}

const sensors_chip_name *get_chip_name(int num) {
    /* returns chip name at the given offset, or NULL if does not exist
    inputs: int num, position in list of sensors
    */
    sensors_init(NULL);
    return sensors_get_detected_chips(0, &num);
}