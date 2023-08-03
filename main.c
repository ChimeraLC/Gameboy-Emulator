#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

/*
 *      Constants definitions
 */
#define ROM_ADDR 0x0000
#define VRAM_ADDR 0x8000
#define ERAM_ADDR 0xA000
#define WRAM_ADDR 0xC000
#define OAM_ADDR 0xFE00
#define IOR_ADDR 0xFF00
#define HRAM_ADDR 0xFF80

/*
 *      Memory
 */

uint8_t *ROM;           // ROM from cartridge
uint8_t VRAM[0x4000];   // Virtual RAM
uint8_t ERAM[0x2000];   // External RAM
uint8_t WRAM[0x8000];   // Working RAM
uint8_t OAM[0xA0];      // Object Attribute Memory
uint8_t IOR[0x80];      // I/O Registers
uint8_t HRAM[0x7F];     // High RAM

uint8_t rom_bank;       // Switchable ROM bank via mapper
uint8_t vram_bank;      // Switchable VRAM bank 0/1 (CBG only)
uint8_t eram_bank;      // Switchable ERAM bank if any
uint8_t wram_bank;      // Switchable WRAM bank 1-7

/*
 *      Specific Stuff
 */
static uint8_t BIOS[0x100] = {
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x4C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50 };

/*
 *      Registers
 */

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
uint8_t IME;                    // Interrupt Master Flag
uint8_t IE;                     // Interrupt Enable
uint8_t IF;                     // Interrupt Flag
uint8_t HALT;                   // HALT flag

// Random values
uint8_t *r1;
uint8_t *r2;
uint8_t n;
uint16_t nn;
uint32_t nnnn;
bool active = true;

/*
 *      Function definitions
 */

void init_mem();
uint8_t read_mem(uint16_t addr);
void write_mem(uint16_t addr, uint8_t val);
void read_rom(char *filename);
void execute();

int
main(int argc, char **argv)
{
        // Reading rom file
	if (argc < 2) {         // Checking input arguments
		printf("Missing file name");
		return -1;
	}

        // Initializing memory
        init_mem();

        // Initialize SDL

        
        // Loading game file
        char *filename = argv[1];

        read_rom(filename);

        // Startup Sequence
        (void) BIOS;

        // Emulating cycles
        while (active)
        {
                // Handle interrupts
                if (IME && (IE & IF)) {         // Check for correspondings flags
                        // Put PC on stack
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0xff);

                        // Vblank
                        if (IE & IF & 0x1) {
                                PC = 0x40;
                                IF &= ~(0x1);
                        }

                        // LCD STAT
                        if (IE & IF & 0x2) {
                                PC = 0x48;
                                IF &= ~(0x2);
                        }

                        // Timer
                        if (IE & IF & 0x4) {
                                PC = 0x50;
                                IF &= ~(0x4);
                        }

                        // Serial
                        if (IE & IF & 0x8) {
                                PC = 0x58;
                                IF &= ~(0x8);
                        }

                        // Joypad
                        if (IE & IF & 0x10) {
                                PC = 0x60;
                                IF &= ~(0x10);
                        }
                }

                opcode = read_mem(PC++);        // Read opcode

                execute();                      // Execute current opcode

                // TODO: delay to sync hardware cycles
        }

        return 1;
}

void
init_mem()
{
        /*

        memset(VRAM, 0, 0x2000);
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
        */
}

uint8_t
read_mem(uint16_t addr)
{
        switch (addr & 0xF000) {
                case 0x0000:
                case 0x1000:
                case 0x2000:
                case 0x3000:
                return ROM[addr - ROM_ADDR];
                break;
                case 0x4000:
                case 0x5000:
                case 0x6000:
                case 0x7000:
                return ROM[addr - ROM_ADDR + (rom_bank - 1) * 0x4000];
                break;
                case 0x8000:
                case 0x9000:
                return VRAM[addr - VRAM_ADDR + (vram_bank) * 0x2000];
                break;
                case 0xA000:
                case 0xB000:
                return ERAM[addr - ERAM_ADDR + (eram_bank) * 0x2000];
                break;
                case 0xC000:
                return WRAM[addr - WRAM_ADDR];
                break;
                case 0xD000:
                return WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000];
                break;
                case 0xE000:
                return WRAM[addr - WRAM_ADDR - 0x2000];
                break;
                case 0xF000:
                if (addr < OAM_ADDR)
                        return WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000 - 0x2000];
                if (addr < 0xFEA0)
                        return OAM[addr - OAM_ADDR];
                if (addr < IOR_ADDR)
                        return 1;
                if (addr < HRAM_ADDR)
                        return IOR[addr - IOR_ADDR];
                if (addr < 0xFFFF)
                        return HRAM[addr - HRAM_ADDR];
                return IE;
                break;
        }

        return 1;
}

void
write_mem(uint16_t addr, uint8_t val)
{
        switch (addr & 0xF000) {
                case 0x0000:
                case 0x1000:
                case 0x2000:
                case 0x3000:
                return;
                break;
                case 0x4000:
                case 0x5000:
                case 0x6000:
                case 0x7000:
                return;
                break;
                case 0x8000:
                case 0x9000:
                VRAM[addr - VRAM_ADDR + (vram_bank) * 0x2000] = val;
                break;
                case 0xA000:
                case 0xB000:
                ERAM[addr - ERAM_ADDR + (eram_bank) * 0x2000] = val;
                break;
                case 0xC000:
                WRAM[addr - WRAM_ADDR] = val;
                break;
                case 0xD000:
                WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000] = val;
                break;
                case 0xE000:
                WRAM[addr - WRAM_ADDR - 0x2000] = val;
                break;
                case 0xF000:
                if (addr < OAM_ADDR)
                        WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000 - 0x2000] = val;
                if (addr < 0xFEA0)
                        OAM[addr - OAM_ADDR] = val;
                if (addr < IOR_ADDR)
                        return;
                if (addr < HRAM_ADDR)
                        IOR[addr - IOR_ADDR] = val;
                if (addr < 0xFFFF)
                        HRAM[addr - HRAM_ADDR] = val;
                IE = val;
                break;
        }
}

void
read_rom(char *filename)
{
        FILE *rom_file = fopen(filename, "rb");

        if (rom_file == NULL) {
                printf("Error opening provided filename\n");
                return;
        }

        // Copying into memory
        fseek(rom_file, 0, SEEK_END);
        long fsize = ftell(rom_file);
        fseek(rom_file, 0, SEEK_SET); 
        ROM = (uint8_t*)malloc(fsize);
        fread(ROM, fsize, 1, rom_file);
        fclose(rom_file);

}



void
execute()                                                                         // TODO: fix references to (HL) to be accurate
{                                                                               // TODO: still missing misc, rotates, bit opcodes (3.3.5, 3.3.6, 3.3.7)
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