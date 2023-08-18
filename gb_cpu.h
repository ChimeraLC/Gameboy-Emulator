#include <unistd.h>


/*
 *	Function headers
 */
void init_cpu(uint8_t *rom, int num_banks);
uint8_t read_mem(uint16_t addr);
void write_mem(uint16_t addr, uint8_t val);
void update_timers(uint16_t cycles);
void update_lcd(uint16_t cycles);
uint8_t execute();
uint8_t get_IOR(uint16_t addr);
uint8_t get_OAM(uint16_t addr);
uint16_t get_PC();
long get_opcodes();
void log_memory();
void print_registers();
void update_joystick();
void print_lcd();
void test();

/*
 *      Constants definitions
 */
#define ROM_ADDR 0x0000
#define ROM_BANK_SIZE 0x4000
#define VRAM_ADDR 0x8000
#define VRAM_BANK_SIZE 0x2000
#define ERAM_ADDR 0xA000
#define ERAM_BANK_SIZE 0x2000
#define WRAM_ADDR 0xC000
#define WRAM_BANK_SIZE 0x1000
#define OAM_ADDR 0xFE00
#define IOR_ADDR 0xFF00
#define HRAM_ADDR 0xFF80