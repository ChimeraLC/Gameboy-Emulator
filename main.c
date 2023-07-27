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
uint8_t n;
uint16_t nn;


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
write_mem(uint16_t addr, uint8_t val)
{
        memory[addr] = val;
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
                        case 0x1:       // LD BC, nn                            // TODO: little endian?
                        reg.c = read_mem(PC++);
                        reg.b = read_mem(PC++);
                        break;
                        case 0x6:       // LD B, n
                        reg.b = read_mem(PC++);
                        break;
                        case 0x8:       // LD (nn),SP
                        (void) 0;
                        break;
                        case 0xE:       // LD C, n
                        reg.c = read_mem(PC++);
                        break;
                }
                break;
                case 0x10:
                switch(opcode & 0x0F) {
                        case 0x1:       // LD DE, nn                            // TODO: little endian?
                        reg.e = read_mem(PC++);
                        reg.d = read_mem(PC++);
                        break;
                        case 0x6:       // LD D, n
                        reg.d = read_mem(PC++);
                        break;
                        case 0x8:       // JR n
                        n = read_mem(PC);                                       // TODO: adding different types?
                        PC += n;
                        break;
                        case 0xE:      // LD E, n
                        reg.e = read_mem(PC++);
                        break;
                }
                break;
                case 0x20:
                switch(opcode & 0x0F) {
                        case 0x0:       // JR NZ, n
                        n = read_mem(PC++);
                        if (!(reg.f & 0x8)) {                                   // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0x1:       // LD HL, nn                            // TODO: little endian?
                        reg.l = read_mem(PC++);
                        reg.h = read_mem(PC++);
                        break;
                        case 0x2:      // LDD (HL), A
                        write_mem(reg.hl, reg.a);
                        reg.hl = reg.hl + 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x6:      // LD H, n
                        reg.h = read_mem(PC++);
                        break;
                        case 0x8:       // JR Z, n
                        n = read_mem(PC++);
                        if (reg.f & 0x8) {                                      // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0xA:      // LDD A, (HL)
                        reg.a = read_mem(reg.hl);
                        reg.hl = reg.hl + 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                        case 0xE:      // LD L, n
                        reg.l = read_mem(PC++);
                        break;
                }
                break;
                case 0x30:
                switch(opcode & 0x0F) {
                        case 0x0:       // JR NC, n
                        n = read_mem(PC++);
                        if (!(reg.f & 0x1)) {                                   // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0x1:       // LD SP, nn                            // TODO: little endian?
                        SP = read_mem(PC++);
                        SP |= read_mem(PC++) << 8;
                        break;
                        case 0x2:      // LDD (HL), A
                        write_mem(reg.hl, reg.a);
                        reg.hl = reg.hl - 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x6:      // LD HL, n
                        reg.hl = read_mem(PC++);                                // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x8:       // JR C, n
                        n = read_mem(PC++);
                        if (reg.f & 0x1) {                                      // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0xA:      // LDD A, (HL)
                        reg.a = read_mem(reg.hl);
                        reg.hl = reg.hl - 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                }
                break;
                /*
                 * Catching all register to register LDs
                 */
                case 0x40:
                case 0x50:
                case 0x60:                                                      // TODO: fix this
                case 0x70:
                switch (opcode * 0xF0) {
                        case 0x40:
                                if ((opcode * 2) & 0xF0) {      // Second half
                                        r1 = &reg.c;
                                }
                                else {                          // First half
                                        r1 = &reg.b;
                                }
                        break;
                        case 0x50:
                                if ((opcode * 2) & 0xF0) {      // Second half
                                        r1 = &reg.e;
                                }
                                else {                          // First half
                                        r1 = &reg.d;
                                }
                        break;
                        case 0x60:
                                if ((opcode * 2) & 0xF0) {      // Second half
                                        r1 = &reg.l;
                                }
                                else {                          // First half
                                        r1 = &reg.h;
                                }
                        break;
                        case 0x70:
                                if ((opcode * 2) & 0xF0) {      // Second half
                                        r1 = &reg.a;
                                }
                                else {                          // First half
                                        r1 = &reg.h;                            // TODO: Should be hl
                                }
                        break;
                }
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
                        r2 = &reg.h;                                            // TODO: SHOULD BE HL
                        break;
                        case 0xE:
                        r2 = &reg.a;
                        break;
                }
                *r1 = *r2;      // Moving contents of r2 into r1
                break;
                case 0x80:
                break;
                case 0x90:
                break;
                case 0xC0:
                switch (opcode & 0x0F) {
                        case 0x01:      // POP BC
                        (void) 0;
                        break;
                        case 0x02:      // JP NZ, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x8)) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x03:      // JP nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        PC = nn;
                        case 0x05:      // PUSH BC
                        (void) 0;
                        break;
                        case 0x0A:      // JP Z, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x8) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                }
                break;
                case 0xD0:
                switch (opcode & 0x0F) {
                        case 0x01:      // POP DE
                        (void) 0;
                        break;
                        case 0x02:      // JP NC, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x1)) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x05:      // PUSH DE
                        (void) 0;
                        break;
                        case 0x0A:      // JP C, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x1) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                }
                break;
                case 0xE0:
                switch (opcode & 0x0F) {       
                        case 0x00:      // LDH (n), A
                        write_mem(read_mem(PC++) | 0xFF00, reg.a);
                        break;
                        case 0x01:      // POP HL
                        (void) 0;
                        break;
                        case 0x02:      // LD A, (C)
                        write_mem(reg.c | 0xFF00, reg.a);
                        break;
                        case 0x05:      // PUSH HL
                        (void) 0;
                        break;
                        case 0x09:      // JP HL
                        PC = reg.hl;
                        break;
                }
                break;
                case 0xF0:
                switch (opcode & 0x0F) {
                        case 0x00:      // LDH A, (n)
                        reg.a = read_mem(read_mem(PC++) | 0xFF00);
                        break;
                        case 0x01:      // POP AF
                        (void) 0;
                        break;
                        case 0x02:      // LD (C), A
                        reg.a = read_mem(reg.c | 0xFF00);
                        break;
                        case 0x05:      // PUSH AF
                        (void) 0;
                        break;
                        case 0x08:      // LDHL SP,n
                        (void) 0;
                        case 0x09:      // LD SP, HL
                        SP = reg.hl;
                        break;
                }
                break;
        }
}