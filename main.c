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
    int numFiles = 0; //assign a value to this based on size of /sys/class/hwmon
    FILE *data[numFiles]; //will get this as a pointer from scanner script - dynamically allocate?

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
                if (offset < (numFiles - 1)) {offset++;}
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
            refresh_sensor_window(sensors[i], data[i + offset]);
        }  
    }
}

void refresh_sensor_window(WINDOW *win, FILE *sensor) { //TODO - create API to scan files for info given a file pointer 
    //TODO - implement this function to add sensor data
    //create two columns - name and value
    box(win, 0, 0);
    mvprintw(win, 0, 1, "Test Sensor Name"); //get sensor name from file

    //refresh window when done
    wrefresh(win);
    return;
}