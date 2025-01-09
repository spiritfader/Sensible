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
    //global ncurses timeout - we use this as the keyboard polling interval
    //results in getch() taking a maximum of 50 ms to check before returning ERR
    timeout(50);

    //get screen dimensions and store in scrHeight + scrWidth
    int scrHeight, scrWidth;
    getmaxyx(stdscr, scrHeight, scrWidth);

    //each sensor window has a specified width, check how many we can fit
    int numSensors = scrWidth / SENSOR_WIDTH;

    //initialize sensor windows
    WINDOW *sensors[numSensors]; //most ncurses functions take a window pointer so that's how we'll store them
    for (int i = 0; i < numSensors; i++) {
        //height, width, y position, x position
        sensors[i] = newwin((scrHeight - 3), (scrWidth / numSensors), 0, (i * (scrWidth / numSensors)));
    }
    //set up commands - this window is static but needs to be refreshed each loop
    WINDOW *commands = newwin(3, 64, (scrHeight - 3), 0);
    //set up interval indicator at x position 64 beside commands
    //this will need to be refreshed every loop as well
    WINDOW *intervalVal = newwin(3, scrWidth - 64, (scrHeight - 3), 64);
    
    //offset is the number of places we've scrolled down in the list of sensors
    //i.e. the index of the first sensor we show in the UI
    //starts at top of list of sensors
    int offset = 0;

    //refresh each window
    //need keypad mode to use arrows
    //noecho and cbreak modes prevent ui spam on keypresses
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int loop = 20, input, interval = 20; //we use loop to poll faster than the sensors refresh interval

    //main ncurses logic loop
    while (1) {
        //take input, or continue with ERR once we pass the timeout interval
        input = getch(); //this will always take 50 ms, or whatever value we set
        
        //only refresh on keypress or every interval-th 50ms loop (1 second)
        if ((loop < interval) && (input == ERR)) {
            loop++;
            continue;
        }
        //if we want to fully refresh, we do the rest of the loop
        loop = 0;
        //the specific keypress we get is stored in input - keypad mode provides macro definitions for special keys like arrows
        switch (input) {
            case KEY_LEFT: //KEY_LEFT is one such macro
                if (offset > 0) {offset--;} //scroll up the list by one, unless we're already at the top
                break;
            case KEY_RIGHT:
                if (get_chip_name(offset + numSensors) != NULL) {offset++;} //this returns a pointer if there is a sensor after our current list to scroll down to, otherwise returns NULL
                sensors_cleanup(); //get_chip_name() initializes sensors but doesn't clean them as we repurpose it in refresh_sensor_window()
                break;
            //increase or decrease refresh interval based on arrow keys
            case KEY_UP:
                if (interval > 5) {interval /= 2;} //half the number of loops per refresh, doubling the speed, to a max speed of 4x per second (every 5 50ms loops)
                break;
            case KEY_DOWN:
                if (interval < 320) {interval *= 2;} //double the number of loops per refresh, halving the speed, to a min speed of once per 16 seconds (every 320 50ms loops)
                break;
            case 'q':
                //end ncurses session before exiting
                endwin();
                exit(0);
            default: //if no keypress, i.e. once a second, we don't need to scroll and just continue to refresh the windows with the latest data
                break;
        }

        //refresh each window's data using the latest offset position
        for (int i = 0; i < numSensors; i++) {
            refresh_sensor_window(sensors[i], (i + offset));
        }
        //refresh the text at the bottom of the screen (commands and interval)
        //if this doesn't work, move into the above for loop
        mvwprintw(commands, 0, 1, "Use the left and right arrow keys to view more sensors, \nor press Q to quit Sensible.");
        wrefresh(commands);
        //calculate current refresh time in seconds (timeout ms * loop count) and print
        float rate = (50.0 * interval) / 1000.0;
        mvwprintw(intervalVal, 0, 1, "Refreshing sensor data every:");
        mvwprintw(intervalVal, 1, 1, "%.2f seconds", rate);
        wrefresh(intervalVal);
    }
}

void refresh_sensor_window(WINDOW *win, int offset) { 
    /* Refreshes sensor data for each sensor window
    Inputs: 
        - WINDOW *win - pointer to the window to be refreshed
        - int offset - the index of sensor data used to populate the window
    */
    //clear window first to avoid overlapping text on scrolling
    wclear(win);
    //make window sexy
    box(win, 0, 0);

    //grab our chip name pointer - this is the heading where all our data comes from
    const sensors_chip_name *chip = get_chip_name(offset);

    //in the edge case that we have more horizontal screen space than sensors, we can end up having to display a NULL sensor
    //should this occur, we simply push an empty box to the screen, otherwise we'd throw an exception
    //it should be impossible to scroll into this state thanks to the check in main()
    if (chip == NULL) { 
        wrefresh(win);
        return;
    }

    //set up title and headings
    mvwprintw(win, 0, 1, (*chip).prefix);
    mvwprintw(win, 1, 1, "SENSOR");
    mvwprintw(win, 1, 16, "           VALUE");

    //line indicates the next line to print info on
    int line = 3;

    //we loop over major features - sensors_get_features increments f as it runs, simulating a stack
    sensors_feature const *feat;
    int f = 0;
    while ((feat = sensors_get_features(chip, &f)) != 0) { //loop through features until one is NULL
        char *label = sensors_get_label(chip, feat); //according to libsensors documentation this dynamically allocates a human-readable string for the sensor label, which we can free as soon as it's in the buffer
        //print the label for the major feature - subfeatures of it will be below
        mvwprintw(win, line++, 1, label); //note for autumn: c postfix uses the value and *then* increments it, so this has the same effect as passing "line" as a param and doing "line++;" on the next line. ++line is opposite - would increment, then use it, passing line + 1
        free(label); //can't escape the malloc shit even if someone else writes it

        //subfeature loop works the exact same way as the feature loop
        sensors_subfeature const *subf;
        int s = 0;
        while ((subf = sensors_get_all_subfeatures(chip, feat, &s)) != 0) {
            double val; //value at the subfeature
            int rc = sensors_get_value(chip, (*subf).number, &val);
            //rc will store the relevant value in val and return its exit status; if it fails somehow, we skip
            //this should ALWAYS execute unless a field somehow has a label and a NULL value simultaneously
            if (rc >= 0) {
                //left justify the name, then right justify the value, with a max of all the decimal places any normal person needs
                mvwprintw(win, line, 1, (*subf).name);
                mvwprintw(win, line++, 16, "%16.2lf", val);
            }
        }
        line++; //add an extra line between features
    }
    //need to cleanup so we can re-init sensor values for the next window
    sensors_cleanup();
    //refresh window when done
    wrefresh(win);
    return;
}

const sensors_chip_name *get_chip_name(int num) {
    /* returns chip name at the given offset, or NULL if does not exist
    inputs: int num, position in list of sensors
    */
    //literally just a wrapper
    sensors_init(NULL);
    return sensors_get_detected_chips(0, &num);
}