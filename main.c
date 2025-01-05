/*
Sensible: A TUI hardware monitoring program using ncurses.
Authors: @Spiritfader, @NerdyKyogre
*/
#include "sensible.h"
#define SENSOR_WIDTH 1 //placeholder value for now

int main(int argc, char **argv) {
    //standard ncurses setup
    //initialize screen, get windows in place
    initscr();

    //get screen dimensions
    int scrHeight, scrWidth;
    getmaxyx(stdscr, scrHeight, scrWidth);

    int numSensors = scrWidth % SENSOR_WIDTH;

    //initialize sensor windows
    WINDOW *sensors[numSensors];
    for (int i = 0; i < numSensors; i++) {
        sensors[i] = newWin((scrHeight - 3), (scrWidth / numSensors), (i * (scrWidth / numSensors)), 0);
    }
    //set up commands - this window is static
    WINDOW *commands = newWin(3, scrWidth, 0, (scrHeight - 3));
    mvprintw(commands, 0, 1, "Use the left and right arrow keys to view more sensors, or press Q to quit Sensible.");
    wrefresh(commands);

    //get list of sensors to check
    //TODO: implement interface to scan /sys/class/hwmon for this information
    //Should return list of file pointers which we can pass to refresh and scan
    //use parallel list: pressing arrow keys changes the offset


    int offset = 0; //start at top of list
    //int numChips; //assign a value to this based on size of /sys/class/hwmon
    //FILE *data[numChips]; //will get this as a pointer from scanner script - dynamically allocate?

    //refresh each window
    //need keypad mode to use arrows
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    while (1) {
        int input = getch();
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
        }  
    }
}

void refresh_sensor_window(WINDOW *win, int offset) { //TODO - create API to scan files for info given a file pointer 
    //TODO - implement this function to add sensor data
    //create two columns - name and value
    box(win, 0, 0);

    const sensors_chip_name *chip = get_chip_name(offset);

    if (chip == NULL) { //display empty box if no sensor available
        wrefresh(win);
        return;
    }

    mvprintw(win, 0, 1, chip.prefix); //TODO: get chip label from api
    mvprintw(win, 1, 1, "SENSOR");
    mvprintw(win, 1, 16, "VALUE"); //placeholder width

    int line = 3;

    //copied from libsensors stackoverflow post
    sensors_feature const *feat;
    int f = 0;
    while ((feat = sensors_get_features(chip, &f)) != 0) {
        char *label = sensors_get_label(chip, feat); //according to libsensors documentation this dynamically allocates a string, which we can free as soon as it's in the buffer
        mvprintw(win, line++, 1, label);
        free(label);

        sensors_subfeature const *subf;
        int s = 0;
        while ((subf = sensors_get_all_subfeatures(chip, feat, &s)) != 0) {
            double val;
            int rc = sensors_get_value(chip, subf.number, &val);
            if (rc > 0) {
                mvprintw(win, line, 1, subf.name);
                mvprintw(win, line++, 16, "%d", val);
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
    return = sensors_get_detected_chips(0, &num);
}