#include <unistd.h>

/*
 * Shared variables
 */
extern int verbose;             // Verbosity

/*
 * Function headers
 */

int read_rom(char *filename);
int init_SDL();
void execute_frame();
uint8_t get_joystick();
void align_framerate();
void key_press(uint8_t key);
void key_release(uint8_t key);
void usage();

#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) < (b) ? (b) : (a))