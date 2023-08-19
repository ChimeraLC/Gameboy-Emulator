#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#include "main.h"
#include "gb_cpu.h"
#include "gb_gpu.h"
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

uint8_t eram_bank;      // Switchable ERAM bank if any
uint8_t wram_bank;      // Switchable WRAM bank 1-7
uint16_t bank_mask;     // Mask for smaller ROM sizes

// Basic DMG boot rom
uint8_t BIOS[0x100] = {
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
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50 };

// Number of cycles for each opcode
uint8_t OP_CYCLES[0x100] = {
	4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,   
	4,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,   
	8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,    
	8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,   
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,    
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,  
	8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4,  
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,  
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
	4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4, 
	8,12,12,12,12,16, 8,32, 8, 8,12, 8,12,12, 8,32,
	8,12,12, 0,12,16, 8,32, 8, 8,12, 0,12, 0, 8,32,
	12,12, 8, 0, 0,16, 8,32,16, 4,16, 0, 0, 0, 8,32,
	12,12, 8, 4, 0,16, 8,32,12, 8,16, 4, 0, 0, 8,32
};


/*
 *      Registers
 */

struct registers                      // 8-bit registers
{
        union {
                struct {
                        uint8_t f;
                        uint8_t a;
                };
                uint16_t af;
        };
        union {
                struct {
                        uint8_t c;
                        uint8_t b;
                };
                uint16_t bc;
        };
        union {
                struct {
                        uint8_t e;
                        uint8_t d;
                };
                uint16_t de;
        };
        union {
                struct {
                        uint8_t l;
                        uint8_t h;
                };
                uint16_t hl;
        };
};

struct registers reg;           // Registers
uint16_t PC;                    // Program counter
uint16_t SP;                    // Stack pointer
uint8_t opcode;                 // Current opcode
long opcodes_run = 0;
uint8_t cbcode;                 // Opcode for 2 byte operations
uint8_t IME = 1;                    // Interrupt Master Flag
uint8_t IE;                     // Interrupt Enable
uint8_t IF;                     // Interrupt Flag
uint8_t HALT;                   // HALT flag

// MBC1 Mapper values
uint8_t RAMG;                   // MBC1 RAM Gate Register
uint8_t RBANK1 = 1;                 // MBC1 bank register 1
uint8_t RBANK2 = 0;                 // MBC1 bank register 2
uint8_t RMODE;                  // MBC1 mode register

// Timer values
uint16_t div_lower;             // Cycle count for div timer
uint16_t tima_lower;            // Cycle count for tima timer
uint16_t lcd_cycles;            // Cycle count for lcd timer
uint16_t cpu_cycles;            // Tracks cycle count of current operation
uint16_t tima_freq[] = {1024, 16, 64, 256};     // Timer frequencies

// Variables used during instructions
uint8_t n;
uint8_t n2;
int8_t n_signed;
uint16_t nn;
uint32_t nnnn;

// Initialize the cpu values and copy rom from main
void
init_cpu(uint8_t *rom, int num_banks)
{
        // Save rom to self
        ROM = rom;
        // Initial register and pointer values
        PC = 0x0000;
        div_lower = 0;
        tima_lower = 0;
        lcd_cycles = 0;
        wram_bank = 1;
        eram_bank = 1;

        // Setting up bank mask for reading
        switch(num_banks) {
                case 2: bank_mask = 0x1; break;
                case 4: bank_mask = 0x3; break;
                case 8: bank_mask = 0x7; break;
                case 16: bank_mask = 0xF; break;
                default: bank_mask = 0x1F;
        }
        /* Values used if boot rom is skipped
        //write_mem(0xFF50, 1);
        SP = 0xFFFE;
        reg.af = 0x01B0;
        reg.bc = 0x0013;
        reg.de = 0x00D8;
        reg.hl = 0x014D;

        

        // Initial memory values
        //IOR[0x04] = 0xABCC;
        IOR[0x05] = 0x00;
        IOR[0x06] = 0x00;
        IOR[0x07] = 0x00;
        IOR[0x10] = 0x80;
        IOR[0x11] = 0xBF;
        IOR[0x12] = 0xF3;
        IOR[0x14] = 0xBF;
        IOR[0x16] = 0x3F;
        IOR[0x17] = 0x00;
        IOR[0x19] = 0xBF;
        IOR[0x1A] = 0x7F;
        IOR[0x1B] = 0xFF;
        IOR[0x1C] = 0x9F;
        IOR[0x1E] = 0xBF;
        IOR[0x20] = 0xFF;
        IOR[0x21] = 0x00;
        IOR[0x22] = 0x00;
        IOR[0x23] = 0xBF;
        IOR[0x24] = 0x77;
        IOR[0x25] = 0xF3;
        IOR[0x26] = 0xF1;
        IOR[0x40] = 0x91;
        IOR[0x42] = 0x00;
        IOR[0x43] = 0x00;
        IOR[0x45] = 0x00;
        IOR[0x47] = 0xFC;
        IOR[0x48] = 0xFF;
        IOR[0x49] = 0x0F;
        IOR[0x4A] = 0x00;
        IOR[0x4B] = 0x00;
        IE = 0x00;
        */
}

// Read and return memory that would be at given addr
uint8_t
read_mem(uint16_t addr)
{
        switch (addr & 0xF000) {
                case 0x0000:
                if (!IOR[0x50] && addr < 0x100) {
                        return BIOS[addr];      // BIOS memory
                }
                __attribute__ ((fallthrough));                                  // Might want to fix this
                case 0x1000:
                case 0x2000:
                case 0x3000:    // ROM bank 00
                if (RMODE & 0x1) {
                        return ROM[addr + ((RBANK2 & 0x3) << 5) * ROM_BANK_SIZE];
                }
                else {
                        return ROM[addr];
                }
                break;
                case 0x4000:
                case 0x5000:
                case 0x6000:
                case 0x7000:   // ROM bank 01~NN                                   // TODO: <16 mb roms
                return ROM[addr - ROM_ADDR + 
                    ((RBANK1 & bank_mask) + ((RBANK2 & 0x3) << 5) - 1) * 0x4000];          
                break;
                case 0x8000:
                case 0x9000:   // VRAM
                return VRAM[addr - VRAM_ADDR + (IOR[0x4F] & 0x1) * 0x2000];
                break;
                case 0xA000:
                case 0xB000:    // External Ram
                if ((RAMG & 0x0F) == 0x0A) {      // Check if RAM enabled
                        if (RMODE & 0x1) {
                                return ERAM[addr - ERAM_ADDR + (RBANK2 & 0x3) * ERAM_BANK_SIZE];
                        }
                        else {
                                return ERAM[addr - ERAM_ADDR];
                        }
                }
                return 0;
                break;
                case 0xC000:    // Working ram bank 0
                return WRAM[addr - WRAM_ADDR];
                break;
                case 0xD000:    // Working ram banks 1~7                        // Probably unecessary without CBG
                return WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000];
                break;
                case 0xE000:    // Echo rom
                return WRAM[addr - WRAM_ADDR - 0x2000];
                break;
                case 0xF000:
                if (addr < OAM_ADDR)    // Echo rom
                        return WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000 - 0x2000];
                else if (addr < 0xFEA0) // OAM
                        return OAM[addr - OAM_ADDR];
                else if (addr < IOR_ADDR)    // Unused memory
                        return 0;
                else if (addr < HRAM_ADDR)  {  // I/O registers
                        switch (addr & 0xFF) {
                                case 0x00:      // Joystick Register
                                // Looking for directional keys
                                if (!(IOR[0x00] & 0x10)) {
                                        return (IOR[0x00] | 0xF) & ((get_joystick() & 0xF) | 0xF0);
                                }
                                // Looking for action keys
                                if (!(IOR[0x00] & 0x20)) {
                                        return (IOR[0x00] | 0xF) & (((get_joystick() >> 4) & 0xF) | 0xF0);
                                }
                                return 0x00;
                                break;
                                case 0x01:      // SB
                                return IOR[0x01];
                                break;
                                case 0x02:      // SC
                                return IOR[0x02];       
                                break;
                                case 0x04:      // DIV Timer
                                return IOR[0x04];
                                case 0x05:      // TIMA
                                return IOR[0x05];
                                case 0x06:      // TMA
                                return IOR[0x06];
                                break;
                                case 0x07:       // TAC
                                return IOR[0x07];
                                break;
                                case 0x0F:      // IF register
                                return IF;
                                case 0x40:      // LCDC
                                return IOR[0x40];
                                break;
                                case 0x41:      // STAT                         // REQUIRES FIX
                                if (IOR[0x40] & 0x80) {
                                        return IOR[0x41];
                                }
                                return (IOR[0x41] & 0xF8) | 0x1;             // Check bitwise operations
                                break;
                                case 0x42:      // SCY
                                return IOR[0x42];
                                break;
                                case 0x43:      // SCX
                                return IOR[0x43];
                                break;
                                case 0x44:      // LY
                                if (IOR[0x40] & 0xF0) {
                                        return IOR[0x44];
                                }
                                return 0x00;
                                break;
                                case 0x45:      // LYC
                                return (IOR[0x45]);
                                break;
                                case 0x46:      // DMA
                                return IOR[0x46];
                                break;
                                case 0x47:      // BGP
                                return IOR[0x47];
                                break;
                                case 0x48:      // OBP0
                                return IOR[0x48];
                                break;
                                case 0x49:      // OBP1
                                return IOR[0x49];
                                break;
                                case 0x4A:      // WY
                                return (IOR[0x4A]);
                                break;
                                case 0x4B:      // WX
                                return IOR[0x4B];
                                break;
                        }
                        return IOR[addr - IOR_ADDR];
                }
                else if (addr < 0xFFFF) // HRAM
                        return HRAM[addr - HRAM_ADDR];
                else if (addr == 0xFFFF) // Interrupt enable
                        return IE;
                break;
        }

        return 1;
}

// Write val to memory that would be at addr
void
write_mem(uint16_t addr, uint8_t val)
{
        switch (addr & 0xF000) {
                case 0x0000:
                case 0x1000:    // RAM enable
                RAMG = val;
                break;
                case 0x2000:
                case 0x3000:    // ROM bank number
                RBANK1 = val;
                if (!(RBANK1 & 0x1F)) {
                        RBANK1 |= 0x1;
                }
                break;
                case 0x4000:
                case 0x5000:    // ROM bank number upper bits
                RBANK2 = val;
                break;
                case 0x6000:
                case 0x7000:    // Banking mode
                RMODE = val;
                break;
                case 0x8000:
                case 0x9000:    // VRAM
                VRAM[addr - VRAM_ADDR + (IOR[0x4F] & 0x1) * 0x2000] = val;
                break;
                case 0xA000:
                case 0xB000:    // External RAM
                ERAM[addr - ERAM_ADDR + (eram_bank) * 0x2000] = val;
                break;
                case 0xC000:    // WRAM bank 0
                WRAM[addr - WRAM_ADDR] = val;
                break;
                case 0xD000:    // WRAM banks 1-7
                WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000] = val;
                break;
                case 0xE000:    // ECHO RAM
                WRAM[addr - WRAM_ADDR - 0x2000] = val;
                break;
                case 0xF000:
                if (addr < OAM_ADDR)    // ECHO RAM
                        WRAM[addr - WRAM_ADDR + (wram_bank - 1) * 0x1000 - 0x2000] = val;
                else if (addr < 0xFEA0)
                        OAM[addr - OAM_ADDR] = val;
                else if (addr < IOR_ADDR)
                        return;
                else if (addr < HRAM_ADDR)   // I/O Registers
                        switch (addr & 0xFF) {
                                case 0x00:      // Joystick registers
                                IOR[0x00] = val & 0x30;
                                break;
                                case 0x01:      // SB
                                IOR[0x01] = val;
                                break;
                                case 0x02:      // SC
                                IOR[0x02] = val;
                                //if (verbose && val == 0x81) {printf("%c", IOR[0x01]);}
                                break;
                                case 0x04:      // DIV timer
                                IOR[0x04] = 0;
                                div_lower = 0;
                                break;
                                case 0x05:      // TIMA
                                IOR[0x05] = val;
                                break;
                                case 0x06:      // TMA
                                IOR[0x06] = val;
                                break;
                                case 0x07:      // TAC
                                IOR[0x07] = val;
                                break;
                                case 0x0F:      // IF register
                                IF = val;
                                break;
                                case 0x40:      // LCDC
                                IOR[0x40] = val;
                                break;
                                case 0x41:      // STAT                         // FIX
                                IOR[0x41] &= ~(0x78);
                                IOR[0x41] |= val & (0x78);
                                break;
                                case 0x42:      // SCY
                                IOR[0x42] = val;
                                break;
                                case 0x43:      // SCX
                                IOR[0x43] = val;
                                break;
                                case 0x44:      // LY
                                IOR[0x44] = 0;                                  // Is this supposed to be a reset?
                                break;
                                case 0x45:      // LYC
                                IOR[0x45] = val;
                                break;
                                case 0x46:      // OAM DMA Transfer
                                IOR[0x46] = val;                                // Does this require some bitwies opperations  
                                for (uint8_t i = 0; i < 0xA0; i++) {            // TODO: check it doesn't exceed 0xDF?
                                        OAM[i] = read_mem((val << 8) + i);
                                }
                                cpu_cycles += 160;  // DMA cycles
                                break;
                                case 0x47:      // BGP
                                IOR[0x47] = val;
                                break;
                                case 0x48:      // OBP0
                                IOR[0x48] = val;
                                break;
                                case 0x49:      // OBP1
                                IOR[0x49] = val;
                                break;
                                case 0x4A:      // WY
                                IOR[0x4A] = val;
                                break;
                                case 0x4B:      // WX
                                IOR[0x4B] = val;
                                break;
                                case 0x50:      // BOOT Rom
                                IOR[0x50] = 1;
                                if (verbose) {
                                        printf("Exiting boot rom\n");
                                }
                                break;
                                default:
                                IOR[addr - IOR_ADDR] = val;                     // Might have issues
                                break;
                        }
                else if (addr < 0xFFFF) // HRAM
                        HRAM[addr - HRAM_ADDR] = val;
                else if (addr == 0xFFFF)        // Interrupt Enable
                        IE = val;
                break;
        }
}

/*
 * Handle interrupts and execute a single opcode
 */
uint8_t
execute()                                                                         // TODO: fix references to (HL) to be accurate
{                                                                               // TODO: still missing misc, rotates, bit opcodes (3.3.5, 3.3.6, 3.3.7)
        

        // Handle interrupts
        if ((HALT || IME) && (IE & IF)) {         // Check for correspondings flags
                // Exit halt
                HALT = 0;
                
                // If interrupts are enables
                if (IME) {
                        IME = 0;

                        // Put PC on stack
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0xFF);

                        // Vblank
                        if (IE & IF & 0x1) {
                                PC = 0x40;
                                IF &= ~(0x1);
                        }

                        // LCD STAT
                        else if (IE & IF & 0x2) {
                                //if (verbose) printf("STAT interrupt\n");
                                PC = 0x48;
                                IF &= ~(0x2);
                        }

                        // Timer
                        else if (IE & IF & 0x4) {
                                //if (verbose) printf("Timer interrupt\n");
                                PC = 0x50;
                                IF &= ~(0x4);
                        }

                        // Serial
                        else if (IE & IF & 0x8) {
                                //if (verbose) printf("Serial interrupt\n");
                                PC = 0x58;
                                IF &= ~(0x8);
                        }

                        // Joypad
                        else if (IE & IF & 0x10) {
                                if (verbose) printf("Joypad interrupt\n");
                                PC = 0x60;
                                IF &= ~(0x10);
                        }
                }
        }                                                                       // Should I wait a cycle?


        // Doing nothing if halted (4 cycles)
        if (HALT) {
                return 4;
        }

        // Read next opcode
        opcode = read_mem(PC++);
        opcodes_run += 1;

        // Default cycle length of operation
        cpu_cycles = OP_CYCLES[opcode];

        // Debug outputs
        if (verbose == 2) {
                printf("Opcode: %X, PC: %X\n", opcode, PC - 1);
        }

        // Halting
        if (opcode == 0x76) {
                HALT = 1;
                return cpu_cycles;                 // HALT
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
                        case 0x2:       // LD (BC), A
                        write_mem(reg.bc, reg.a);
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
                        if ((reg.b & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:       // LD B, n
                        reg.b = read_mem(PC++);
                        break;
                        case 0x7:       // RLCA
                        reg.a = (reg.a >> 7) | (reg.a << 1);
                        reg.f &= ~(0xF0);
                        /*      Blargg tests dont want this?
                        if (reg.a == 0) {
                                reg.f |= 0x0;
                        }
                        */
                        if (reg.a & 0x01) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x8:       // LD (nn),SP
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        write_mem(nn, SP & 0xFF);
                        write_mem(++nn, SP >> 8); 
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
                        case 0xA:      // LD A, (BC)
                        reg.a = read_mem(reg.bc);
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
                        if ((reg.c & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0xE:       // LD C, n
                        reg.c = read_mem(PC++);
                        break;
                        case 0xF:       // RRCA
                        reg.a = ((reg.a << 7) | (reg.a >> 1));
                        reg.f &= ~(0xF0);
                        /*
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        */
                        if (reg.a & 0x80) {
                                reg.f |= 0x10;
                        }
                        break;
                }
                break;
                case 0x10:
                switch(opcode & 0x0F) {
                        case 0x0:       // STOP
                        (void) 0;
                        HALT = 1;
                        break;
                        case 0x1:       // LD DE, nn 
                        reg.e = read_mem(PC++);
                        reg.d = read_mem(PC++);
                        break;
                        case 0x2:       // LD (DE), A
                        write_mem(reg.de, reg.a);
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
                        if ((reg.d & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:       // LD D, n
                        reg.d = read_mem(PC++);
                        break;
                        case 0x7:       // RLA
                        n = reg.a;
                        reg.a = (reg.a << 1) | ((reg.f & 0x10) >> 4);
                        reg.f &= ~(0xF0);
                        /*
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        */
                        if (n & 0x80) {
                                reg.f |= 0x10;
                        }
                        break;
                        case 0x8:       // JR n                                                                                 
                        n_signed = (int8_t) read_mem(PC++);
                        PC += n_signed;
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
                        case 0xA:       // LD A, (DE)
                        reg.a = read_mem(reg.de);
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
                        if ((reg.e & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0xE:      // LD E, n
                        reg.e = read_mem(PC++);
                        break;
                        case 0xF:       // RRA
                        n = reg.a;
                        reg.a = reg.a >> 1 | ((reg.f & 0x10) << 3);
                        reg.f &= ~(0xF0);
                        /*
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        */
                        if (n & 0x01) {
                                reg.f |= 0x10;
                        }
                        break;
                }
                break;
                case 0x20:
                switch(opcode & 0x0F) {
                        case 0x0:       // JR NZ, n
                        n_signed = (int8_t) read_mem(PC++);
                        if (!(reg.f & 0x80)) {                         
                                PC += n_signed;                       
                        }
                        break;
                        case 0x1:       // LD HL, nn                 
                        reg.l = read_mem(PC++);
                        reg.h = read_mem(PC++);
                        break;
                        case 0x2:      // LDD (HL), A
                        write_mem(reg.hl, reg.a);
                        reg.hl = reg.hl + 1;                           
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
                        if ((reg.h & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0x6:      // LD H, n
                        reg.h = read_mem(PC++);
                        break;
                        case 0x7:       // DAA           
                        n2 = 0;
                        if ((reg.f & 0x20) || (!(reg.f & 0x40) && (reg.a & 0xf) > 9)) {
                                n2 = 6;
                        }
                        if ((reg.f & 0x10) || (!(reg.f & 0x40) && reg.a > 0x99)) {
                                n2 |= 0x60;
                                reg.f |= 0x10;
                        }
                        reg.a += (reg.f & 0x40) ? -n2 : n2;
                        reg.f &= ~(0xA0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        break;
                        case 0x8:       // JR Z, n
                        n_signed = (int8_t) read_mem(PC++);
                        if (reg.f & 0x80) {                                     
                                PC += n_signed;                            
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
                        reg.hl = reg.hl + 1;                               
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
                        if ((reg.l & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0xE:      // LD L, n
                        reg.l = read_mem(PC++);
                        break;
                        case 0xF:       // CPL
                        reg.a ^= 0xFF;
                        reg.f |= 0x60;
                        break;
                }
                break;
                case 0x30:
                switch(opcode & 0x0F) {
                        case 0x0:       // JR NC, n
                        n_signed = (int8_t) read_mem(PC++);
                        if (!(reg.f & 0x10)) {                                 
                                PC += n_signed;                                    
                        }
                        break;
                        case 0x1:       // LD SP, nn                          
                        SP = read_mem(PC++);
                        SP |= read_mem(PC++) << 8;
                        break;
                        case 0x2:      // LDD (HL), A
                        write_mem(reg.hl, reg.a);
                        reg.hl = reg.hl - 1;                                  
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
                        if ((n & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        write_mem(reg.hl, n);
                        break;
                        case 0x6:      // LD (HL), n
                        write_mem(reg.hl, read_mem(PC++));                              
                        break;
                        case 0x7:       // SCF
                        reg.f &= ~(0x60);
                        reg.f |= 0x10;
                        break;
                        case 0x8:       // JR C, n
                        n_signed = (int8_t) read_mem(PC++);
                        if (reg.f & 0x10) {                                     
                                PC += n_signed;                 
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
                        reg.hl = reg.hl - 1;                         
                        break;
                        case 0xB:      // DEC SP
                        SP -= 1;
                        break;
                        case 0xC:      // INC A
                        reg.a += 1;
                        reg.f &= ~(0xE0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        if (!(reg.a & 0x0F)) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0xD:      // DEC A
                        reg.a -= 1;
                        reg.f &= ~(0xE0);
                        if (reg.a == 0) {
                                reg.f |= 0x80;
                        }
                        reg.f |= 0x40;
                        if ((reg.a & 0x0F) == 0xF) {
                                reg.f |= 0x20;
                        }
                        break;
                        case 0xE:       // LD A, imm
                        reg.a = read_mem(PC++);
                        break;
                        case 0xF:       // CCF
                        reg.f &= ~(0x60);
                        reg.f ^= 0x10;
                        break;
                }
                break;
                /*
                 * Catching all register to register LDs
                 */
                case 0x40:
                case 0x50:
                case 0x60:                                       
                case 0x70:
                switch(opcode & 0x7){ // Pairing together 0-8, 1-9, etc.
                        case 0x0:
                        n = reg.b;
                        break;
                        case 0x1:
                        n = reg.c;
                        break;
                        case 0x2:
                        n = reg.d;
                        break;
                        case 0x3:
                        n = reg.e;
                        break;
                        case 0x4:
                        n = reg.h;
                        break;
                        case 0x5:
                        n = reg.l;
                        break;
                        case 0x6:
                        n = read_mem(reg.hl);
                        break;
                        case 0x7:
                        n = reg.a;
                        break;
                }
                switch (opcode & 0xF0) {
                        case 0x40:
                                if (opcode & 0x8) {      // Second half
                                        reg.c = n;
                                }
                                else {                          // First half
                                        reg.b = n;
                                }
                        break;
                        case 0x50:
                                if (opcode & 0x8) {      // Second half
                                        reg.e = n;
                                }
                                else {                          // First half
                                        reg.d = n;
                                }
                        break;
                        case 0x60:
                                if (opcode & 0x8) {      // Second half
                                        reg.l = n;
                                }
                                else {                          // First half
                                        reg.h = n;
                                }
                        break;
                        case 0x70:
                                if (opcode & 0x8) {      // Second half
                                        reg.a = n;
                                }
                                else {                          // First half
                                        write_mem(reg.hl, n);
                                }
                        break;
                }
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
                        reg.a &= reg.e;
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
                        reg.a ^= reg.a;
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
                        reg.a |= reg.e;
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
                                reg.f |= 0x10;                                  
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
                        if (!(reg.f & 0x80)) {                    
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
                                PC = nn;
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
                        if (reg.f & 0x80) {                       
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
                                case 0x00:      
                                switch (cbcode & 0x8) {
                                        case 0x0:       // RLC n
                                        n = (n << 1) | (n >> 7);
                                        reg.f &= ~(0xF0);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        if (n & 0x01) {
                                                reg.f |= 0x10;
                                        }
                                        break;
                                        case 0x8:       // RRC n
                                        n = (n >> 1) | (n << 7);
                                        reg.f &= ~(0xF0);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        if (n & 0x80) {
                                                reg.f |= 0x10;
                                        }
                                        break;
                                }
                                break;
                                case 0x10:
                                switch (cbcode & 0x8) {
                                        case 0x0:       // RL n
                                        n2 = n;
                                        n = (n << 1) | ((reg.f & 0x10) >> 4);
                                        reg.f &= ~(0xF0);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        if (n2 & 0x80) {
                                                reg.f |= 0x10;
                                        }
                                        break;
                                        case 0x8:       // RR n
                                        n2 = n;
                                        n = (n >> 1) | ((reg.f & 0x10) << 3);
                                        reg.f &= ~(0xF0);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        if (n2 & 0x01) {
                                                reg.f |= 0x10;
                                        }
                                        break;
                                }
                                break;
                                case 0x20:
                                switch (cbcode & 0x8) {                      
                                        case 0x0:       // SLA n
                                        reg.f &= ~(0xF0);
                                        if (n & 0x80) {
                                                reg.f |= 0x10;
                                        }
                                        n = (n << 1);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        break;
                                        case 0x8:       // SRA n
                                        reg.f &= ~(0xF0);
                                        if (n & 0x01) {
                                                reg.f |= 0x10;
                                        }
                                        n = (n >> 1);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        if (n & 0x40) {
                                                n |= 0x80;
                                        }
                                        break;
                                }
                                break;
                                case 0x30:
                                switch (cbcode & 0x8) {
                                        case 0x0:      // SWAP n
                                        n = ((n & 0xF0) >> 4) | ((n & 0x0F) << 4);
                                        reg.f &= ~(0xF0);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        break;
                                        case 0x8:       // SRL n
                                        reg.f &= ~(0xF0);
                                        if (n & 0x01) {
                                                reg.f |= 0x10;
                                        }
                                        n = (n >> 1);
                                        if (n == 0) {
                                                reg.f |= 0x80;
                                        }
                                        break;
                                }
                                break;
                        }
                        // Bit opcodes
                        if ((cbcode & 0xC0) == 0x40) {    // BIT b, r     
                                n2 = (cbcode >> 3) & 0x7;
                                reg.f &= ~(0xE0);
                                reg.f |= 0x20;
                                if ((n & (0x1 << n2)) == 0) {
                                        reg.f |= 0x80;
                                }
                                
                        }
                        if ((cbcode & 0xC0) == 0x80) {      // RES b, r
                                n2 = (cbcode >> 3) & 0x7;
                                n &= ~(0x1 << n2);
                        }
                        if ((cbcode & 0xC0) == 0xC0) {      // SET b, r
                                n2 = (cbcode >> 3) & 0x7;
                                n |= (0x1 << n2);
                        }
                        switch(cbcode & 0x07) { // Return to register           // TODO: might not always be needed
                                case 0x00:
                                reg.b = n;
                                break;
                                case 0x01:
                                reg.c = n;
                                break;
                                case 0x02:
                                reg.d = n;
                                break;
                                case 0x03:
                                reg.e = n;
                                break;
                                case 0x04:
                                reg.h = n;
                                break;
                                case 0x05:
                                reg.l = n;
                                break;
                                case 0x06:
                                write_mem(reg.hl, n);
                                break;
                                case 0x07:
                                reg.a = n;
                                break;
                        }
                        break;
                        case 0x0C:      // Call Z, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x80) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                                PC = nn;
                        }
                        break;
                        case 0x0D:      // CALL nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = nn;
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
                        case 0x0F:      // RST 08
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0008;
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
                        if (!(reg.f & 0x10)) {                         
                                PC = nn;
                        }
                        break;
                        case 0x04:      // Call NC, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (!(reg.f & 0x10)) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                                PC = nn;
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
                        break;
                        case 0x09:      // RETI
			nn = read_mem(SP++);
			nn |= read_mem(SP++) << 8;
			PC = nn;
			IME = 1;
                        break;
                        case 0x0A:      // JP C, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x10) {                         
                                PC = nn;
                        }
                        break;
                        case 0x0C:      // Call C, nn
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        if (reg.f & 0x10) {
                                write_mem(--SP, PC >> 8);
                                write_mem(--SP, PC & 0x00FF);
                                PC = nn;
                        }
                        break;
                        case 0xE:       // SBC A, imm
                        n = read_mem(PC++);
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
                        case 0x02:      // LD (C), A
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
                        n_signed = (int8_t) read_mem(PC++);
                        nn = SP + n_signed;
                        reg.f &= ~(0xF0);
                        if (n_signed >= 0) {                                
                                if ((SP & 0xFF) + n_signed > 0xFF) {
                                        reg.f |= 0x10;
                                }
                                if ((SP & 0xF) + (n_signed & 0xF) > 0xF) {
                                        reg.f |= 0x20;
                                }
                        }
                        else {
                                if ((nn & 0xFF) <= (SP & 0xFF)) {
                                        reg.f |= 0x10;
                                }
                                if ((nn & 0xF) <= (SP & 0xF)) {
                                        reg.f |= 0x20;
                                }
                        }
                        SP = nn & 0xFFFF;
                        break;
                        case 0x09:      // JP HL
                        PC = reg.hl;
                        break;
                        case 0xA:       // LD (imm), A
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        write_mem(nn, reg.a);
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
                        reg.af = nn & 0xFFF0;                            
                        break;
                        case 0x02:      // LD A, (C)
                        reg.a = read_mem(reg.c | 0xFF00);
                        break;
                        case 0x03:      // DI                                   // TODO: should be delayed?
                        IME = 0;
                        break;
                        case 0x05:      // PUSH AF
                        write_mem(--SP, reg.a);
                        write_mem(--SP, reg.f);
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
                        n_signed = (int8_t) read_mem(PC++);
                        nn = SP + n_signed;
                        reg.f &= ~(0xF0);
                        if (n_signed >= 0) {                                 
                                if ((SP & 0xFF) + n_signed > 0xFF) {
                                        reg.f |= 0x10;
                                }
                                if ((SP & 0xF) + (n_signed & 0xF) > 0xF) {
                                        reg.f |= 0x20;
                                }
                        }
                        else {
                                if ((nn & 0xFF) <= (SP & 0xFF)) {
                                        reg.f |= 0x10;
                                }
                                if ((nn & 0xF) <= (SP & 0xF)) {
                                        reg.f |= 0x20;
                                }
                        }                                             
                        reg.hl = nn;                    
                        break;
                        case 0x09:      // LD SP, HL
                        SP = reg.hl;
                        break;
                        case 0xA:       // LD A, (imm)
                        nn = read_mem(PC++);
                        nn |= read_mem(PC++) << 8;
                        reg.a = read_mem(nn);
                        break;
                        case 0x0B:      // EI                                   // TODO: should be delayed?
                        IME = 1;
                        break;
                        case 0x0E:      // CP #
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
                        break;
                        case 0x0F:      // RST 38
                        write_mem(--SP, PC >> 8);
                        write_mem(--SP, PC & 0x00FF);
                        PC = 0x0038;
                        break;
                }
                break;
        }
        return cpu_cycles;
}


/*
 *  Update cpu timers
 */
void
update_timers(uint16_t cycles) 
{

        // DIV timer
        div_lower += cycles;
        if (div_lower > 256) {
                div_lower -= 256;
                IOR[0x04] += 1;
        }

        // TIMA timer                                                           // TODO: emulate timer issues? + delays
        if (IOR[0x07] & 0x4) {
                tima_lower += cycles;
                if (tima_lower > tima_freq[IOR[0x07] & 0x3]) {
                        tima_lower -= tima_freq[IOR[0x07] & 0x3];
                        IOR[0x05] ++;
                        if (IOR[0x05] == 0) {   // Overflow
                                IOR[0x05] = IOR[0x06];
                                IF |= 0x4;
                        }
                }
        }
}

/*
 * Update visual timers
 */
void
update_lcd(uint16_t cycles)
{
        if (IOR[0x40] & 0x80)
                lcd_cycles += cycles;
        // Update every 456 cycles
        if (lcd_cycles > 456) {
                
                lcd_cycles -= 456;

                // Interrupt check
                if (IOR[0x44] == IOR[0x45]) {
                        if (IOR[0x41] & 0x40) {
                                IF |= 0x2;
                        }
                        IOR[0x41] |= 0x4;
                }
                else {
                        IOR[0x41] &= ~(0x4);
                }

                // Increment 0x44
                IOR[0x44] += 1;
                IOR[0x44] %= 154;

                // normal line process
                if (IOR[0x44] < 144) {
                        // LCD Stat mode
                        IOR[0x41] = (IOR[0x41] & ~(0x3));


                        if (IOR[0x41] & 0x10) { // HBLANK STAT interrupt
                                IF |= 0x2;
                        }
                        
                }
                // VBlank interrupt
                else if (IOR[0x44] == 144) {
                        // LCD Stat mode
                        IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x1;

                                                                                // Increment frame?

                        IF |= 0x1;      // Request interrupt

                        if (IOR[0x41] & 0x10) { // VBLANK LCDC interrupt 
                                IF |= 0x2;
                        }

                        update_SDL();
                        align_framerate();
                }
                
        }
        else if (lcd_cycles > 284 && (IOR[0x41] & 0x3) == 2)                     // Other states
        {
                // LCD Stat mode
                IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x3;
        
                drawline_lcd(); 
        } 
        else if (lcd_cycles > 204 && (IOR[0x41] & 0x3) == 0) {
                // LCD Stat mode
                IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x2;


                if (IOR[0x41] & 0x20) { // HBLANK STAT interrupt
                        IF |= 0x2;
                }
        }
}

/*
 * Getter and setter methods, alongside debug functions
*/
uint8_t
get_IOR(uint16_t addr)
{
        return IOR[addr];
}

uint8_t
get_OAM(uint16_t addr)
{
        return OAM[addr];
}

uint16_t
get_PC()
{
        return PC;
}

long
get_opcodes()
{
        return opcodes_run;
}

void
print_registers() 
{
        printf("A: %X F: ", reg.a);
        for (int i = 0; i < 4; i ++) {
                if ((reg.f << i) & 0x80) printf("1");
                else printf("0");
        }
        printf("\n");
        printf("BC: %X\n", reg.bc);
        printf("DE: %X\n", reg.de);
        printf("HL: %X\n", reg.hl);
        printf("PC: %X, SP: %X\n", PC, SP);
        printf("IE: %X, IF: %X\n", IE, IF);
        printf("IME: %X, HALT: %X\n", IME, HALT);
        printf("Opcodes run: %ld\n", opcodes_run);
}

void
print_lcd()
{
        printf("LCD Control: ");
        for (int i = 0; i < 8; i ++) {
                if ((IOR[0x40] << i) & 0x80) printf("1");
                else printf("0");
        }
        printf("\n");
}

/* 
 *      Outputs memory 0x0000 - 0xFFFF to file Log.txt
 */
void
log_memory()
{
        FILE *output;
        output = fopen("Log.txt", "w+");
        for (uint32_t i = 0x0; i <= 0xFFFF; i++) {
                fprintf(output, "%4X: %2X\n", i, read_mem(i));
        }
        fclose(output);
}

void
test()
{
        nn = PC;
        printf("addr: %X, val: %X\n", nn, read_mem(PC++));
}
/*
 * Triggered when joystick changes
 */
void
update_joystick()
{
        IF |= 0x10;
}