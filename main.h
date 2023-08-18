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
void execute_frame();
uint8_t get_joystick();
void key_press(uint8_t key);
void key_release(uint8_t key);

#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) < (b) ? (b) : (a))