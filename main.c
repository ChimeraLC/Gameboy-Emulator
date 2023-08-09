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
bool verbose = false;

// Rom reading
uint8_t *load_rom;           // ROM from cartridge
char *cartridge_types[] = {"ROM ONLY", "MBC1"};
uint32_t rom_size;
uint32_t ram_size;
int cartridge_type = 0;

// I/O other
uint8_t joystick_flags;

// SDL elements
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
uint32_t graphics[144][160];
uint8_t graphics_raw[144][160];

// If emulator is active
bool active = true;


int
main(int argc, char **argv)
{
        // Checking for verbose flag
        char c;
        while ((c = getopt (argc, argv, "v")) != -1) {
                switch (c)
                {
                case 'v':
                        verbose = 1;
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

                // DO CPU STUFF



                // UPDATE SDL
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

int
init_SDL()
{
        // Initialize all SDL systems
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                // Send message if fails
                printf("error initializing SDL: %s\n", SDL_GetError());
                return false;
        }
        // Create parts of window (64 by 32 pixels)
        window = SDL_CreateWindow("Gameboy", 
                                        SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED,
                                        800, 720, 0);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        SDL_RenderSetLogicalSize(renderer, 160, 144);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, 
                                        SDL_TEXTUREACCESS_STREAMING, 64, 32);

        return true; // Return true when there is no problem
}




/*
 *      Getter and Setter functions
 */
uint8_t
get_joystick()
{
        return joystick_flags;
}