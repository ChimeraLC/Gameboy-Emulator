#include <unistd.h>

/*
 * Shared variables
 */
extern int verbose;

/*
 * Function headers
 */

int read_rom(char *filename);
int init_SDL();
uint8_t get_joystick();


#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) < (b) ? (b) : (a))