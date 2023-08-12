#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#include "main.h"
#include "gb_cpu.h"
#include "gb_gpu.h"

// Verbosity
int verbose = 0;
int debug = 0;
int start = 28627;

// Rom reading
uint8_t *load_rom;           // ROM from cartridge
char *cartridge_types[] = {"ROM ONLY", "MBC1"};
uint32_t rom_size;
uint32_t ram_size;
int cartridge_type = 0;

// I/O other
uint8_t joystick_flags;

// If emulator is active
bool active = true;

// Cycle Emulation
uint16_t curr_cycles;
long total_cycles = 0;

int
main(int argc, char **argv)
{
        // Checking for verbose flag
        char c;
        while ((c = getopt (argc, argv, "vdV")) != -1) {
                switch (c)
                {
                case 'v':
                        verbose = 1;
                        break;
                case 'V':
                        verbose = 2;
                        break;
                case 'd':
                        debug = 1;
                        break;
                default:
                        abort ();
                }
        }
        
        // Reading rom file
	if (optind == argc) {         // Checking input arguments
		printf("Missing file name");
		return -1;
	}
        // Loading game file
        char *filename = argv[optind];

        printf("Loading rom %s", filename);
        if (verbose) {
                printf(" with verbose flags active");
        }
        printf("\n");


        if (read_rom(filename) == -1) {
                printf("Error reading ROM\n");
                return -1;
        }

        // Initialize Memory
        init_cpu(load_rom);
        init_gpu();

        // Initialize SDL
        if (init_SDL() == -1) {
                printf("Error initializing SDL\n");
                return -1;
        }
        SDL_Event event;
        // Debug setup
        if (debug) {
        verbose = 0;
        for (int i = 0; i < start - 1; i ++) {
                execute_frame();
        }
        update_SDL();
        verbose = 2;
        while (active) {
                // Get SDL events
                while(SDL_PollEvent( &event ) ){
                        switch( event.type ){
                                case SDL_KEYDOWN:
                                switch (event.key.keysym.sym){
                                        case SDLK_ESCAPE:
                                        active = false;
                                        break;
                                        case SDLK_a:
                                        execute_frame();
                                        update_SDL();
                                        break;
                                        case SDLK_b:
                                        verbose = 0;
                                        for (int i = 0; i < 9; i++) {
                                                execute_frame();
                                        }
                                        verbose = 2;
                                        execute_frame();
                                        break;
                                        case SDLK_d:
                                        verbose = 0;
                                        for (int i = 0; i < 99; i++) {
                                                execute_frame();
                                        }
                                        verbose = 2;
                                        execute_frame();
                                        break;
                                        case SDLK_f:
                                        verbose = 0;
                                        for (int i = 0; i < 999; i++) {
                                                execute_frame();
                                        }
                                        verbose = 2;
                                        execute_frame();
                                        break;
                                        case SDLK_g:
                                        verbose = 0;
                                        for (int i = 0; i < 9999; i++) {
                                                execute_frame();
                                        }
                                        verbose = 2;
                                        execute_frame();
                                        break;
                                        case SDLK_c:
                                        print_registers();
                                        break;
                                        case SDLK_x:
                                        print_lcd();
                                        break;
                                }
                        }
                }
        }
        }
        else {
        while (active) {
                // Get SDL events
                while(SDL_PollEvent( &event ) ){
                        switch( event.type ){
                                case SDL_KEYDOWN:
                                switch (event.key.keysym.sym){
                                        case SDLK_ESCAPE:
                                        active = false;
                                        break;
                                        case SDLK_RIGHT:
                                        joystick_flags &= ~(0x1);
                                        break;
                                        case SDLK_LEFT:
                                        joystick_flags &= ~(0x2);
                                        break;
                                        case SDLK_UP:
                                        joystick_flags &= ~(0x4);
                                        break;
                                        case SDLK_DOWN:
                                        joystick_flags &= ~(0x8);
                                        break;
                                        case SDLK_x:    // A Key
                                        joystick_flags &= ~(0x10);
                                        break;
                                        case SDLK_z:    // B Key
                                        joystick_flags &= ~(0x20);
                                        break;
                                        case SDLK_s:    // Select Key
                                        joystick_flags &= ~(0x40);
                                        break;
                                        case SDLK_a:    // Start Key
                                        joystick_flags &= ~(0x80);
                                        break;
                                }
                                break;
                                case SDL_KEYUP:
                                switch( event.key.keysym.sym ){
                                        case SDLK_RIGHT:
                                        joystick_flags |= (0x1);
                                        break;
                                        case SDLK_LEFT:
                                        joystick_flags |= (0x2);
                                        break;
                                        case SDLK_DOWN:
                                        joystick_flags |= (0x4);
                                        break;
                                        case SDLK_UP:
                                        joystick_flags |= (0x8);
                                        break;
                                        case SDLK_x:    // A Key
                                        joystick_flags |= (0x10);
                                        break;
                                        case SDLK_z:    // B Key
                                        joystick_flags |= (0x20);
                                        break;
                                        case SDLK_s:    // Select Key
                                        joystick_flags |= (0x40);
                                        break;
                                        case SDLK_a:    // Start Key
                                        joystick_flags |= (0x80);
                                        break;
                                
                                }
                                break;
                        }
                }

                // Align framerate
                for (int i = 0; i < 20; i++) usleep(900);

                // DO CPU STUFF
                curr_cycles = execute();
                update_timers(curr_cycles);

                // Rendering
                update_lcd(curr_cycles);

                // UPDATE SDL
                //update_SDL();

                // TIming?
                total_cycles += curr_cycles;
                //4194304
                if (total_cycles > 4000) {
                        update_SDL();
                        total_cycles = 0;
                }
        }
        }
        return 1;
}


int
read_rom(char *filename)
{
        FILE *rom_file = fopen(filename, "rb");

        if (rom_file == NULL) {
                printf("Error opening provided filename\n");
                return -1;
        }

        // Copying into memory
        fseek(rom_file, 0, SEEK_END);
        long fsize = ftell(rom_file);
        fseek(rom_file, 0, SEEK_SET); 
        load_rom = (uint8_t*)malloc(fsize);
        fread(load_rom, fsize, 1, rom_file);
        fclose(rom_file);

        // ROM Cartridge Type
        switch (load_rom[0x147]) {
                case 0:
                cartridge_type = 0;
                break;
                case 1:
                case 2:
                case 3:
                cartridge_type = 1;
                break;
                default:
                if (verbose) {
                        printf("Unsupported ROM type\n");
                }
                return -1;
        }
        if (verbose) {
                printf("The ROM has a %s cartridge type\n", cartridge_types[cartridge_type]);
        }

        // ROM Size
        switch (load_rom[0x148]) {
                case 0x0: rom_size = 0x8000; break;
                case 0x1: rom_size = 0x10000; break;
                case 0x2: rom_size = 0x20000; break;
                case 0x3: rom_size = 0x40000; break;
                case 0x4: rom_size = 0x80000; break;
                case 0x5: rom_size = 0x100000; break;
                case 0x6: rom_size = 0x200000; break;
                case 0x7: rom_size = 0x400000; break;
                case 0x8: rom_size = 0x800000; break;
                case 0x52: rom_size = 0x120000; break;
                case 0x53: rom_size = 0x140000; break;
                case 0x54: rom_size = 0x160000; break;
                default: if (verbose) printf("Error reading ROM size");
        }

        if (verbose) printf("The ROM has size %X\n", rom_size);

        // RAM Size
        switch (load_rom[0x149]) {
                case 0x0: ram_size = 0; break;
                case 0x1: ram_size = 0x800; break;
                case 0x2: ram_size = 0x2000; break;
                case 0x3: ram_size = 0x8000; break;
                case 0x4: ram_size = 0x20000; break;
                case 0x5: ram_size = 0x10000; break;
                default: if (verbose) printf("Error reading RAM size");
        }

        if (verbose) printf("The RAM has size %X\n", ram_size);

        // Returning without errors
        return 0;

}

/*
 *      Getter and Setter functions
 */
uint8_t
get_joystick()
{
        return joystick_flags;
}

void
execute_frame()
{
        curr_cycles = execute();
        update_timers(curr_cycles);

        // Rendering
        update_lcd(curr_cycles);

}