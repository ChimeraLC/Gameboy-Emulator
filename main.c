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
uint8_t rom;

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
uint8_t cbcode;                 // Opcode for 2 byte operations

// Random values
uint8_t *r1;
uint8_t *r2;
uint8_t n;
uint16_t nn;
uint32_t nnnn;


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

        read_rom(filename);

        return 1;
}



void
init()
{
        memset(memory, 0, 0x2000);
        // Initial register and pointer values
        PC = 0x0000;
        SP = 0xFFFE;
        reg.af = 0x01B0;
        reg.bc = 0x0013;
        reg.de = 0x00D8;
        reg.hl = 0x014D;

        // Initial memory values
        memory[0xFF05] = 0x00;
        memory[0xFF06] = 0x00;
        memory[0xFF07] = 0x00;
        memory[0xFF10] = 0x80;
        memory[0xFF11] = 0xBF;
        memory[0xFF12] = 0xF3;
        memory[0xFF14] = 0xBF;
        memory[0xFF16] = 0x3F;
        memory[0xFF17] = 0x00;
        memory[0xFF19] = 0xBF;
        memory[0xFF1A] = 0x7F;
        memory[0xFF1B] = 0xFF;
        memory[0xFF1C] = 0x9F;
        memory[0xFF1E] = 0xBF;
        memory[0xFF20] = 0xFF;
        memory[0xFF21] = 0x00;
        memory[0xFF22] = 0x00;
        memory[0xFF23] = 0xBF;
        memory[0xFF24] = 0x77;
        memory[0xFF25] = 0xF3;
        memory[0xFF26] = 0xF1;
        memory[0xFF40] = 0x91;
        memory[0xFF42] = 0x00;
        memory[0xFF43] = 0x00;
        memory[0xFF45] = 0x00;
        memory[0xFF47] = 0xFC;
        memory[0xFF48] = 0xFF;
        memory[0xFF49] = 0xFF;
        memory[0xFF4A] = 0x00;
        memory[0xFF4B] = 0x00;
        memory[0xFFFF] = 0x00;
        
        return;
}

void
read_rom(char *filename)
{
        FILE *rom_file = fopen(filename, "rb");

        if (rom_file == NULL) {
                printf("Error opening provided filename\n");
                return(-1);
        }

        // Copying into memory
        fseek(rom_file, 0, SEEK_END);
        long fsize = ftell(rom);
        fseek(rom_file, 0, SEEK_SET); 
        rom = (uint8_t*)malloc(fsize);
        fread(rom, fsize, 1, rom_file);
        fclose(rom_file);

        /*
        uint16_t opcode = memory[0] << 8 | memory[1];
        printf("%X", opcode);
        */
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
        switch (addr & 0xF000) {
                case 0x0000:
                break;
                case 0x1000:
                break;
                case 0x2000:
                break;
                case 0x3000:
                break;
                case 0x4000:
                break;
                case 0x5000:
                break;
                case 0x6000:
                break;
                case 0x7000:
                break;
                case 0x8000:
                break;
                case 0x9000:
                break;
                case 0xA000:
                break;
                case 0xB000:
                break;
                case 0xC000:
                break;
                case 0xD000:
                break;
                case 0xE000:
                break;
                case 0xF000:
                break;
        }
}

void
cycle()                                                                         // TODO: fix references to (HL) to be accurate
{                                                                               // TODO: still missing misc, rotates, bit opcodes (3.3.5, 3.3.6, 3.3.7)
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
                        case 0x03:      // INC BC
                        reg.bc += 1;
                        break;
                        case 0x04:      // INC B
                        reg.b += 1;
                        reg.f &= ~(0xE0);
                        if (reg.b == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.b & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x05:      // DEC B
                        reg.b -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.b == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.b & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:       // LD B, n
                        reg.b = read_mem(PC++);
                        break;
                        case 0x8:       // LD (nn),SP
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        write_mem(nn, SP & 0xFF);
                        write_mem(++nn, SP >> 4);                               // Check if the endianess is correct
                        break;
                        case 0x9:       // ADD HL, BC
                        nnnn = reg.hl + reg.bc;
                        reg.f &= ~(0x70);
                        if ((reg.hl ^ reg.bc ^ nnnn) & 0x1000) {
                                reg.f |= 0x20;
                        }
                        if (nnnn & 0xFFFF0000) {
                                reg.f |= 0x10;
                        }
                        reg.hl = nnnn & 0xFFFF;
                        break;
                        case 0x0B:      // DEC BC
                        reg.bc -= 1;
                        break;
                        case 0x0C:      // INC C
                        reg.c += 1;
                        reg.f &= ~(0xE0);
                        if (reg.c == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.c & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x0D:      // DEC C
                        reg.c -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.c == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.c & 0x0F)) {
                                reg.f |= 0x20;
                        }
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
                        case 0x03:      // INC DE
                        reg.de += 1;
                        break;
                        case 0x04:      // INC D
                        reg.d += 1;
                        reg.f &= ~(0xE0);
                        if (reg.d == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.d & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x05:      // DEC D
                        reg.d -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.d == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.d & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:       // LD D, n
                        reg.d = read_mem(PC++);
                        break;
                        case 0x8:       // JR n
                        n = read_mem(PC);                                       // TODO: adding different types?
                        PC += n;
                        break;
                        case 0x9:       // ADD HL, DE
                        nnnn = reg.hl + reg.de;
                        reg.f &= ~(0x70);
                        if ((reg.hl ^ reg.de ^ nnnn) & 0x1000) {
                                reg.f |= 0x20;
                        }
                        if (nnnn & 0xFFFF0000) {
                                reg.f |= 0x10;
                        }
                        reg.hl = nnnn & 0xFFFF;
                        break;
                        case 0x0B:      // DEC DE
                        reg.de -= 1;
                        break;
                        case 0x0C:      // INC E
                        reg.e += 1;
                        reg.f &= ~(0xE0);
                        if (reg.e == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.e & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x0D:      // DEC E
                        reg.e -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.e == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.e & 0x0F)) {
                                reg.f |= 0x20;
                        }
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
                        if (!(reg.f & 0x80)) {                                   // Check if flag math is right
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
                        case 0x03:      // INC HL
                        reg.hl += 1;
                        break;
                        case 0x04:      // INC H
                        reg.h += 1;
                        reg.f &= ~(0xE0);
                        if (reg.h == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.h & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x05:      // DEC H
                        reg.h -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.h == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.h & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:      // LD H, n
                        reg.h = read_mem(PC++);
                        break;
                        case 0x8:       // JR Z, n
                        n = read_mem(PC++);
                        if (reg.f & 0x80) {                                      // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0x9:       // ADD HL, HL
                        nnnn = reg.hl + reg.hl;
                        reg.f &= ~(0x70);
                        if ((reg.hl ^ reg.hl ^ nnnn) & 0x1000) {
                                reg.f |= 0x20;
                        }
                        if (nnnn & 0xFFFF0000) {
                                reg.f |= 0x10;
                        }
                        reg.hl = nnnn & 0xFFFF;
                        break;
                        case 0xA:      // LDD A, (HL)
                        reg.a = read_mem(reg.hl);
                        reg.hl = reg.hl + 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x0B:      // DEC HL
                        reg.hl -= 1;
                        break;
                        case 0x0C:      // INC L
                        reg.l += 1;
                        reg.f &= ~(0xE0);
                        if (reg.l == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.l & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x0D:      // DEC L
                        reg.l -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.l == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.l & 0x0F)) {
                                reg.f |= 0x20;
                        }
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
                        if (!(reg.f & 0x10)) {                                   // Check if flag math is right
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
                        case 0x03:      // INC SP
                        SP += 1;
                        break;
                        case 0x04:      // INC (HL)
                        n = read_mem(reg.hl) + 1;
                        reg.f &= ~(0xE0);
                        if (n == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(n & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        write_mem(reg.hl, n);
                        break;
                        case 0x05:      // DEC (HL)
                        n = read_mem(reg.hl) - 1;
                        reg.f &= ~(0xE0);
                        if (n == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(n & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        write_mem(reg.hl, n);
                        break;
                        case 0x6:      // LD HL, n
                        reg.hl = read_mem(PC++);                                // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x8:       // JR C, n
                        n = read_mem(PC++);
                        if (reg.f & 0x10) {                                      // Check if flag math is right
                                PC += n;                                        // Adding different types?
                        }
                        break;
                        case 0x9:       // ADD HL, SP
                        nnnn = reg.hl + SP;
                        reg.f &= ~(0x70);
                        if ((reg.hl ^ SP ^ nnnn) & 0x1000) {
                                reg.f |= 0x20;
                        }
                        if (nnnn & 0xFFFF0000) {
                                reg.f |= 0x10;
                        }
                        reg.hl = nnnn & 0xFFFF;
                        break;
                        case 0xA:      // LDD A, (HL)
                        reg.a = read_mem(reg.hl);
                        reg.hl = reg.hl - 1;                                    // TODO: perhaps this needs to be stricter?
                        break;
                        case 0x0B:      // DEC SP
                        SP -= 1;
                        break;
                        case 0x0C:      // INC A
                        reg.a += 1;
                        reg.f &= ~(0xE0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.a & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x0D:      // DEC A
                        reg.a -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if (!(reg.a & 0x0F)) {
                                reg.f |= 0x20;
                        }
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
                switch (opcode & 0x0F) {
                        case 0x00:      // ADD A, B
                        nn = reg.a + reg.b;
                        reg.f &= ~(0xF0);                                       // Could probably just set to 0
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.b ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x01:      // ADD A, C
                        nn = reg.a + reg.c;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.c ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x02:      // ADD A, D
                        nn = reg.a + reg.d;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.d ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x03:      // ADD A, E
                        nn = reg.a + reg.e;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.e ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x04:      // ADD A, H
                        nn = reg.a + reg.h;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.h ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x05:      // ADD A, L
                        nn = reg.a + reg.l;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.l ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x06:      // ADD A, (HL)
                        n = read_mem(reg.hl);
                        nn = reg.a + n;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x07:      // ADD A, A
                        nn = reg.a + reg.a;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.a ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x08:      // ADC A, B
                        nn = reg.a + reg.b + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.b ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x09:      // ADC A, C
                        nn = reg.a + reg.c + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.c ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0A:      // ADC A, D
                        nn = reg.a + reg.d + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.d ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0B:      // ADC A, E
                        nn = reg.a + reg.e + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.e ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0C:      // ADC A, H
                        nn = reg.a + reg.h + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.h ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0D:      // ADC A, L
                        nn = reg.a + reg.l + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.l ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0E:      // ADC A, (HL)
                        n = read_mem(reg.hl);
                        nn = reg.a + n + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0F:      // ADC A, A
                        nn = reg.a + reg.a + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ reg.a ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                }
                break;
                case 0x90:
                switch (opcode & 0x0F) {
                        case 0x00:      // SUB B
                        nn = reg.a - reg.b;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.b ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x01:      // SUB C
                        nn = reg.a - reg.c;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.c ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x02:      // SUB D
                        nn = reg.a - reg.d;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.d ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x03:      // SUB E
                        nn = reg.a - reg.e;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.e ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x04:      // SUB H
                        nn = reg.a - reg.h;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.h ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x05:      // SUB L
                        nn = reg.a - reg.l;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.l ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x06:      // SUB (HL)
                        n = read_mem(reg.hl);
                        nn = reg.a - n;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x07:      // SUB A                                // TODO: could probably be simplified
                        nn = reg.a - reg.a;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.a ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x08:      // SBC A, B
                        nn = reg.a - reg.b - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.b ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x09:      // SBC A, C
                        nn = reg.a - reg.c - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.c ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0A:      // SBC A, D
                        nn = reg.a - reg.d - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.d ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0B:      // SBC A, E
                        nn = reg.a - reg.e - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.e ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0C:      // SBC A, H
                        nn = reg.a - reg.h - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.h ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0D:      // SBC A, L
                        nn = reg.a - reg.l - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.l ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0E:      // SBC A, (HL)
                        n = read_mem(reg.hl);
                        nn = reg.a - n - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x0F:      // SBC A, A
                        nn = reg.a - reg.a - ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.a ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                }
                break;
                case 0xA0:
                switch (opcode & 0x0F) {
                        case 0x00:      // AND B
                        reg.a &= reg.b;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x01:      // AND C
                        reg.a &= reg.c;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x02:      // AND D
                        reg.a &= reg.d;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x03:      // AND E
                        reg.a &= reg.b;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x04:      // AND H
                        reg.a &= reg.h;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x05:      // AND L
                        reg.a &= reg.l;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x06:      // AND (HL)
                        n = read_mem(reg.hl);
                        reg.a &= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x07:      // AND A
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x08:      // XOR B
                        reg.a ^= reg.b;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x09:      // XOR C
                        reg.a ^= reg.c;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0A:      // XOR D
                        reg.a ^= reg.d;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0B:      // XOR E
                        reg.a ^= reg.e;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0C:      // XOR H
                        reg.a ^= reg.h;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0D:      // XOR L
                        reg.a ^= reg.l;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0E:      // XOR (HL)
                        n = read_mem(reg.hl);
                        reg.a ^= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0F:      // XOR A
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                }
                break;
                case 0xB0:
                switch (opcode & 0x0F) {
                        case 0x00:      // OR B
                        reg.a |= reg.b;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x01:      // OR C
                        reg.a |= reg.c;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x02:      // OR D
                        reg.a |= reg.d;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x03:      // OR E
                        reg.a |= reg.b;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x04:      // OR H
                        reg.a |= reg.h;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x05:      // OR L
                        reg.a |= reg.l;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x06:      // OR (HL)
                        n = read_mem(reg.hl);
                        reg.a |= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x07:      // OR A
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x08:      // CP B
                        nn = reg.a - reg.b;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.b ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;                                  // TODO: Sub and SRC and CP? set if no carry, vs set if carry?
                        }
                        break;
                        case 0x09:      // CP C
                        nn = reg.a - reg.c;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.c ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0A:      // CP D
                        nn = reg.a - reg.d;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.d ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0B:      // CP E
                        nn = reg.a - reg.e;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.e ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0C:      // CP H
                        nn = reg.a - reg.h;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.h ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0D:      // CP L
                        nn = reg.a - reg.l;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.l ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0E:      // CP (HL)
                        n = read_mem(reg.hl);
                        nn = reg.a - n;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0F:      // CP A                                // TODO: could probably be simplified
                        nn = reg.a - reg.a;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ reg.a ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                }     
                break;
                case 0xC0:
                switch (opcode & 0x0F) {
                        case 0x00:      // RET NZ
                        if (!(reg.f & 0x80)) {
                                nn = read_mem(SP++);
                                nn |= read_mem(SP++) << 8;
                                PC = nn;
                        }
                        break;
                        case 0x01:      // POP BC
                        nn = read_mem(SP++);
                        nn |= read_mem(SP++) << 8;
                        reg.bc = nn;
                        break;
                        case 0x02:      // JP NZ, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x80)) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x03:      // JP nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        PC = nn;
                        break;
                        case 0x04:      // Call NZ, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x80)) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                        }
                        break;
                        case 0x05:      // PUSH BC
                        write_mem(--SP, reg.b);
                        write_mem(--SP, reg.c);
                        break;
                        case 0x06:      // ADD A, #
                        n = read_mem(PC++);
                        nn = reg.a + n;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x07:      // RST 00
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0000;
                        break;
                        case 0x08:      // RET Z
                        if (reg.f & 0x80) {
                                nn = read_mem(SP++);
                                nn |= read_mem(SP++) << 8;
                                PC = nn;
                        }
                        break;
                        case 0x09:      // RET
                        nn = read_mem(SP++);
                        nn |= read_mem(SP++) << 8;
                        PC = nn;
                        break;
                        case 0x0A:      // JP Z, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x80) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x0B:      // Two byte instructions
                        cbcode = read_mem(PC++);
                        switch(cbcode & 0x07) { //Relevant register
                                case 0x00:
                                n = reg.b;
                                break;
                                case 0x01:
                                n = reg.c;
                                break;
                                case 0x02:
                                n = reg.d;
                                break;
                                case 0x03:
                                n = reg.e;
                                break;
                                case 0x04:
                                n = reg.h;
                                break;
                                case 0x05:
                                n = reg.l;
                                break;
                                case 0x06:
                                n = read_mem(reg.hl);
                                break;
                                case 0x07:
                                n = reg.a;
                                break;
                        }
                        switch(cbcode & 0xF0) {
                                case 0x30:      //SWAP n
                                (void) 0;
                                break;
                        }
                        break;
                        case 0x0C:      // Call Z, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x80) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                        }
                        break;
                        case 0x0D:      // CALL nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = nn;
                        break;
                        case 0x0F:      // RST 08
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0008;
                        break;
                        case 0x0E:      // ADC A, #
                        n = read_mem(PC++);
                        nn = reg.a + n + ((reg.f >> 4) & 0x1);
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                }
                break;
                case 0xD0:
                switch (opcode & 0x0F) {
                        case 0x00:      // RET NC
                        if (!(reg.f & 0x10)) {
                                nn = read_mem(SP++);
                                nn |= read_mem(SP++) << 8;
                                PC = nn;
                        }
                        break;
                        case 0x01:      // POP DE
                        nn = read_mem(SP++);
                        nn |= read_mem(SP++) << 8;
                        reg.de = nn;
                        break;
                        case 0x02:      // JP NC, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x10)) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x04:      // Call NC, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x10)) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                        }
                        break;
                        case 0x05:      // PUSH DE
                        write_mem(--SP, reg.d);
                        write_mem(--SP, reg.e);
                        break;
                        case 0x06:      // SUB #
                        n = read_mem(PC++);
                        nn = reg.a - n;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        reg.a = nn & 0xFF;
                        break;
                        case 0x07:      // RST 10
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0010;
                        break;
                        case 0x08:      // RET C
                        if (reg.f & 0x10) {
                                nn = read_mem(SP++);
                                nn |= read_mem(SP++) << 8;
                                PC = nn;
                        }
                        case 0x09:      // RETI
                        (void) 0;
                        break;
                        case 0x0A:      // JP C, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x10) {                                    // TODO: check that this flag check is right
                                PC = nn;
                        }
                        break;
                        case 0x0C:      // Call C, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x10) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                        }
                        break;
                        case 0x0F:      // RST 18
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0018;
                        break;
                }
                break;
                case 0xE0:
                switch (opcode & 0x0F) {       
                        case 0x00:      // LDH (n), A
                        write_mem(read_mem(PC++) | 0xFF00, reg.a);
                        break;
                        case 0x01:      // POP HL
                        nn = read_mem(SP++);
                        nn |= read_mem(SP++) << 8;
                        reg.hl = nn;
                        break;
                        case 0x02:      // LD A, (C)
                        write_mem(reg.c | 0xFF00, reg.a);
                        break;
                        case 0x05:      // PUSH HL
                        write_mem(--SP, reg.h);
                        write_mem(--SP, reg.l);
                        break;
                        case 0x06:      // AND #
                        n = read_mem(PC++);
                        reg.a &= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x20;
                        break;
                        case 0x07:      // RST 20
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0020;
                        break;
                        case 0x08:      // ADD SP, #
                        (void) 0;
                        case 0x09:      // JP HL
                        PC = reg.hl;
                        break;
                        case 0x0E:      // XOR #
                        n = read_mem(PC++);
                        reg.a ^= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x0F:      // RST 28
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0028;
                        break;
                }
                break;
                case 0xF0:
                switch (opcode & 0x0F) {
                        case 0x00:      // LDH A, (n)
                        reg.a = read_mem(read_mem(PC++) | 0xFF00);
                        break;
                        case 0x01:      // POP AF
                        nn = read_mem(SP++);
                        nn |= read_mem(SP++) << 8;
                        reg.af = nn;                                            // Is this legal?
                        break;
                        case 0x02:      // LD (C), A
                        reg.a = read_mem(reg.c | 0xFF00);
                        break;
                        case 0x05:      // PUSH AF
                        write_mem(--SP, reg.a);
                        write_mem(--SP, reg.f);                                 // Is this legal? (force reg.f & 0xF to be 0?)
                        break;
                        case 0x06:      // OR #
                        n = read_mem(PC++);
                        reg.a |= n;
                        reg.f &= ~(0xF0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x07:      // RST 30
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0030;
                        break;
                        case 0x08:      // LDHL SP,n
                        (void) 0;
                        case 0x09:      // LD SP, HL
                        SP = reg.hl;
                        break;
                        case 0x0E:      // CP #
                        nn = reg.a - reg.a;
                        reg.f &= ~(0xF0);
                        if ((nn & 0xFF) == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a ^ n ^ nn) & 0x10) {
                                reg.f |= 0x20;
                        }
                        if (nn & 0xFF00) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x0F:      // RST 38
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0038;
                        break;
                }
                break;
        }
}