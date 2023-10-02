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
int start = 1;
FILE *output;

// Rom reading
uint8_t *load_rom;           // ROM from cartridge
uint8_t *load_save;             // Save file
bool has_save = false;                 // Whether to load a save file
int cartridge_type; // Cartridge banking type
char *cartridge_types[] = {"ROM ONLY", "MBC1", "MBC2", "MBC3"};
// Saving read rom/ram sizes
uint32_t rom_size;
uint32_t ram_size;
int num_banks;              // Number of rom banks

// I/O other
uint8_t joystick_flags = 0xFF; // Joystick bits for reading 0xFF00

// If emulator is active
bool active = true;

// Whether to run boot rom
bool boot_flag = true;

// Framerate syncing
long last_tick;
long current_tick;
long catchup_ticks = 0;

// Cycle Emulation
uint16_t curr_cycles;           // Cycle count of curent cpu operation
long total_cycles = 0;

int
main(int argc, char **argv)
{
        // Checking for verbose flag
        char c;
        while ((c = getopt (argc, argv, "bvdsVl")) != -1) {
                switch (c)
                {
                case 'v':       // Verbose flags
                        verbose = 1;
                        break;
                case 'V':
                        verbose = 2;
                        break;
                case 'd':       // Debug flags
                        debug = 1;
                        verbose = 2;
                        break;
                case 'b':       // Skipping boot rom
                        boot_flag = false;
                        break;
                case 's':       // Loading save file
                        has_save = true;
                        /*
                        start = atoi(optarg);
                        if (start < 0) {
                                printf("Start point must be nonnegative\n");
                                return -1;
                        }
                        */
                        break;
                case 'h':
                        usage();
                        break;
                case '?':
                        if (optopt == 's')
                        fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                        else
                        usage();
                        return -1;
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
        if (debug) {
                printf(" in debug mode");
                if (start) {
                        printf(" at start point %d", start);
                }
        }
        else if (verbose) {
                printf(" with verbose flags active");
        }
        printf("\n");


        if (read_rom(filename) == -1) {
                printf("Error reading ROM\n");
                return -1;
        }

        // Initialize Memory
        init_cpu(load_rom, load_save, num_banks, cartridge_type, boot_flag);
        init_gpu();

        // Initialize SDL
        if (init_SDL() == -1) {
                printf("Error initializing SDL\n");
                return -1;
        }
        SDL_Event event;
        
        // First frame
        last_tick = SDL_GetTicks();

        // Debug setup
        if (debug) {
        verbose = 0;
        
        for (int i = 0; i < start - 1; i ++) {
                execute_frame();
        }
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
                                        break;
                                        case SDLK_s:
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
                                        case SDLK_z:
                                        log_memory();
                                        break;
                                        case SDLK_q:    // Go to specific time
                                                long endpoint;
                                                printf("Enter desired stop point: ");
                                                scanf("%ld", &endpoint);
                                                verbose = 0;
                                                for (long i = get_opcodes(); i < endpoint; i++) {
                                                        execute_frame();
                                                }
                                                verbose = 2;
                                        break;
                                        case SDLK_w:    // Run until specific PC address
                                                int addr;
                                                printf("Enter desired PC point: ");
                                                scanf("%X", &addr);
                                                verbose = 0;
                                                while (get_PC() != addr) {
                                                        execute_frame();
                                                }
                                                verbose = 2;
                                                printf("Reached desired position\n");
                                        break;
                                }
                        }
                }
        }
        }
        
        // Normal setup
        else {
        while (active) {
                // Get SDL events
                while(SDL_PollEvent( &event ) ){
                        switch( event.type ){
                                case SDL_QUIT:  // Closing window
                                active = false;
                                break;
                                case SDL_KEYDOWN: // Key press
                                switch (event.key.keysym.sym){
                                        case SDLK_ESCAPE:
                                        active = false;
                                        break;
                                        case SDLK_RIGHT:
                                        key_press(0x1);
                                        break;
                                        case SDLK_LEFT:
                                        key_press(0x2);
                                        break;
                                        case SDLK_UP:
                                        key_press(0x4);
                                        break;
                                        case SDLK_DOWN:
                                        key_press(0x8);
                                        break;
                                        case SDLK_x:    // A Key
                                        key_press(0x10);
                                        break;
                                        case SDLK_z:    // B Key
                                        key_press(0x20);
                                        break;
                                        case SDLK_s:    // Select Key
                                        key_press(0x40);
                                        break;
                                        case SDLK_a:    // Start Key
                                        key_press(0x80);
                                        break;
                                }
                                break;
                                case SDL_KEYUP: // Key release
                                switch( event.key.keysym.sym ){
                                        case SDLK_RIGHT:
                                        key_release(0x1);
                                        break;
                                        case SDLK_LEFT:
                                        key_release(0x2);
                                        break;
                                        case SDLK_UP:
                                        key_release(0x4);
                                        break;
                                        case SDLK_DOWN:
                                        key_release(0x8);
                                        break;
                                        case SDLK_x:    // A Key
                                        key_release(0x10);
                                        break;
                                        case SDLK_z:    // B Key
                                        key_release(0x20);
                                        break;
                                        case SDLK_s:    // Select Keys  
                                        key_release(0x40);
                                        break;
                                        case SDLK_a:    // Start Key
                                        key_release(0x80);
                                        break;
                                        case SDLK_p:
                                        print_registers();
                                        break;
                                
                                }
                                break;
                        }
                }
                // Framerate alignment done via cpu.c and align_framerate

                // CPU emulation
                curr_cycles = execute();

                // Timer updates
                update_lcd(curr_cycles);

                // Rendering
                update_timers(curr_cycles);

        }
        }
        if (verbose) {
                printf("Exiting program\n");
        }
        return 1;
}

/*
 *   Read the provided ROM and parse out the cartridge header data.
 */
int
read_rom(char *filename)
{
        FILE *rom_file = fopen(filename, "rb");

        // Output error if file missing
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
                case 15:
                case 16:
                case 17:
                case 18:
                case 19:
                cartridge_type = 3;
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
                case 0x0: rom_size = 0x8000; num_banks = 2; break;
                case 0x1: rom_size = 0x10000; num_banks = 4; break;
                case 0x2: rom_size = 0x20000; num_banks = 8; break;
                case 0x3: rom_size = 0x40000; num_banks = 16; break;
                case 0x4: rom_size = 0x80000; num_banks = 32; break;
                case 0x5: rom_size = 0x100000; num_banks = 64; break;
                case 0x6: rom_size = 0x200000; num_banks = 128; break;
                case 0x7: rom_size = 0x400000; num_banks = 256; break;
                case 0x8: rom_size = 0x800000; num_banks = 512; break;
                case 0x52: rom_size = 0x120000; num_banks = 72; break;
                case 0x53: rom_size = 0x140000; num_banks = 80; break;
                case 0x54: rom_size = 0x160000; num_banks = 96; break;
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

        // TODO: save files
        if (has_save) {
                // Get save name
                char save_name[100];
                sprintf(save_name, filename);
                // Replace .gb with .sav
                char *suffix = strstr(save_name, ".gb");
                if (suffix == NULL) {
                        printf("Error with filename");
                        return -1;
                }
                sprintf(suffix, ".sav");
                if (verbose) printf("Loading save at %s\n", save_name);
                // Attempt to open file
                FILE *save_file = fopen(save_name, "rb");
                if (!save_file) {
                        printf("Given save file does not exist\n");
                        return -1;
                }
                // Copying into memory
                load_save = (uint8_t*)malloc(ram_size);
                fread(load_save, ram_size, 1, save_file);
                fclose(save_file);
        }

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

/*
 * Updates joystick flags (set corresponding key to 0 in joystick flags)
 */
void
key_press(uint8_t key)
{
        if (joystick_flags & key) {
                update_joystick();
                joystick_flags &= ~(key);
        }
        update_joystick();
}


/*
 * Updates joystick flags (set corresponding key to 1 in joystick flags)
 */
void
key_release(uint8_t key)
{
        joystick_flags |= key;
        update_joystick();
}
/*
 *      Wait between visual frames so that the timing is right
 *      Each frame should be 1.8 milliseconds
 */
void
align_framerate()
{
        // Milliseconds
        current_tick = SDL_GetTicks();
        long frame_diff = current_tick - last_tick;
        last_tick = current_tick;
        // ~17 milliseconds per frame
        if (frame_diff + catchup_ticks < 17) {
                SDL_Delay(17 - frame_diff);                                     // TODO: get this to 16.666 rather than 17
                catchup_ticks = 0;
        }
        // Catching up framerate when behind
        else {
                catchup_ticks += frame_diff - 17;
        }
        last_tick = current_tick;
}

/*
 * Execute a single opcode worth of actions (for debugging)
 */
void
execute_frame()
{
        curr_cycles = execute();
        // Rendering
        update_lcd(curr_cycles);
        // Update timers
        update_timers(curr_cycles);

}

void
usage()
{
    fprintf(stderr, "Usage: main [-bhdvV] <filename>\n");
    fprintf(stderr, "Options\n");\
    fprintf(stderr, "\t-b         Skip boot rom.\n");
    fprintf(stderr, "\t-s         Load save from this file.\n");
    fprintf(stderr, "\t-h         Print this message.\n");
    fprintf(stderr, "\t-d         Initiate in debug mode.\n");
    fprintf(stderr, "\t-v         Print basic debug messages.\n");
    fprintf(stderr, "\t-V         Print additional debug info.\n");
    fprintf(stderr, "\t-l         Log to the file Log.txt\n");
}