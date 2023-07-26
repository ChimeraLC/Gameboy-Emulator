#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

/*
 * CPU emulation
 */

// Memory
#define MRAM_SIZE 0x2000        // Main RAM
#define VRAM_SIZE 0x2000        // Video RAM

uint8_t memory[0xFFFF];         // Entirety of memory (may need to be larger)


// Function heads
void init();

// Registers
struct registers                      // 8-bit registers
{
        union {
                struct {
                        uint8_t a;
                        uint8_t f;
                };
                uint16_t af;
        };
        union {
                struct {
                        uint8_t b;
                        uint8_t c;
                };
                uint16_t bc;
        };
        union {
                struct {
                        uint8_t d;
                        uint8_t e;
                };
                uint16_t de;
        };
        union {
                struct {
                        uint8_t h;
                        uint8_t l;
                };
                uint16_t hl;
        };
};

struct registers reg;           // Registers
uint16_t PC;                    // Program counter
uint16_t SP;                    // Stack pointer
uint8_t opcode;                 // Current opcode

// Random values
uint8_t *r1;
uint8_t *r2;


int
main(int argc, char **argv)
{
        // Reading rom file
	if (argc < 2) {         // Checking input arguments
		printf("Missing file name");
		return -1;
	}

        // Initializing memory
        init();

        // Initialize SDL

        
        // Loading game file
        char *filename = argv[1];

        
        FILE *rom = fopen(filename, "rb");

        if (rom == NULL) {
                printf("Error");
                return(-1);
        }

        // Copying into memory
        fseek(rom, 0, SEEK_END);
        long fsize = ftell(rom);
        fseek(rom, 0, SEEK_SET); 
        
        fread(memory, fsize, 1, rom);
        fclose(rom);
        uint16_t opcode = memory[0] << 8 | memory[1];
        printf("%X", opcode);

        return 1;
}



void
init()
{

        return;
}

uint8_t
read_mem(uint16_t addr)
{
        return memory[addr];
}

void
cycle()
{
        opcode = read_mem(PC++);

        // Halting
        if (opcode == 0x76) {
                return;                 // HALT
        }
        switch (opcode & 0xF0)
        {
                case 0x00:
                switch(opcode & 0x0F) {
                        case 0x0:       // NOP
                        break;
                        case 0x6:       // LD B, n
                        reg.b = read_mem(PC++);
                        break;
                        case 0xE:       // LD C, n
                        reg.c = read_mem(PC++);
                        break;
                }
                break;
                case 0x10:
                switch(opcode & 0x0F) {
                        case 0x6:       // LD D, n
                        reg.d = read_mem(PC++);
                        break;
                        case 0xE:      // LD E, n
                        reg.e = read_mem(PC++);
                        break;
                }
                break;
                case 0x20:
                switch(opcode & 0x0F) {
                        case 0x6:      // LD H, n
                        reg.h = read_mem(PC++);
                        break;
                        case 0xE:      // LD L, n
                        reg.l = read_mem(PC++);
                        break;
                }
                break;
                case 0x30:
                switch(opcode & 0x0F) {
                        case 0x6:      // LD HL, n
                        reg.hl = read_mem(PC++);    // TODO: perhaps this needs to be stricter?
                }
                break;
                /*
                 * Catching all register to register LDs
                 */
                case 0x40:
                        //b or c
                case 0x50:
                        //d or e
                case 0x60:
                        //h or l
                case 0x70:
                        //HL or a
                switch((opcode * 2) & 0xF){ // Pairing together 0-8, 1-9, etc.
                        case 0x0:
                        r2 = &reg.b;
                        break;
                        case 0x2:
                        r2 = &reg.c;
                        break;
                        case 0x4:
                        r2 = &reg.d;
                        break;
                        case 0x6:
                        r2 = &reg.e;
                        break;
                        case 0x8:
                        r2 = &reg.h;
                        break;
                        case 0xA:
                        r2 = &reg.l;
                        break;
                        case 0xC:
                        r2 = &reg.h;    // TODO: SHOULD BE HL
                        break;
                        case 0xE:
                        r2 = &reg.a;
                        break;
                }
                break;
                /*
                // LD r1, r2
                case 0x7F:              // LD A, A
                reg.a = reg.a;
                break;
                case 0x78:              // LD A, B
                reg.a = reg.b;
                break;
                case 0x79:              // LD A, C
                reg.a = reg.c;
                break;
                case 0x7A:              // LD A, D
                reg.a = reg.d;
                break;
                case 0x7B:              // LD A, E
                reg.a = reg.e;
                break;
                case 0x7C:              // LD A, H
                reg.a = reg.h;
                break;
                case 0x7D:              // LD A, L
                reg.a = reg.l;
                break;
                */

        }
}