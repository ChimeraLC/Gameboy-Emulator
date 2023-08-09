#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#include "main.h"
#include "gb_gpu.h"


// Timer stuff
uint8_t lcd_cycles;
uint8_t cycles;
uint16_t div_lower;
uint16_t tima_lower;
uint16_t tima_freq[] = {1024, 16, 64, 256}; 

void
init_gpu()
{
        div_lower = 0;
        tima_lower = 0;
        lcd_cycles = 0;

}

/*      MISC STUFF

void
update_lcd(uint8_t cycles)
{
        if (IOR[0x44 & 0x80]) {
                lcd_cycles += cycles;
        }
        // Update every 456 cycles
        if (lcd_cycles > 456) {
                lcd_cycles -= 456;

                // Interrupt check
                IOR[0x41] &= ~(0x4);    //STAT comparison signal
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
                if (IOR[0x44 < 144]) {
                        // LCD Stat mode
                        IOR[0x41] = (IOR[0x41] & ~(0x3));


                        if (IOR[0x41] & 0x10) { // HBLANK STAT interrupt
                                IF |= 0x2;
                        }
                        
                }
                // VBlank interrupt
                else if (IOR[0x44] = 144) {
                        // LCD Stat mode
                        IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x1;

                                                                                // Increment frame?

                        IF |= 0x1;      // Request interrupt

                        if (IOR[0x41] & 0x10) { // VBLANK STAT interrupt 
                                IF |= 0x2;
                        }
                }
                
        }
        else if (lcd_cycles > 204) {
                // LCD Stat mode
                IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x2;


                if (IOR[0x41] & 0x20) { // HBLANK STAT interrupt
                        IF |= 0x2;
                }
        }
        else if (lcd_cycles > 284)                                                                // Other states
        {
                // LCD Stat mode
                IOR[0x41] = (IOR[0x41] & ~(0x3)) | 0x3;
        
                drawline_lcd();                                                 // Check that this is in frame / is repeated unecessarily?
        } 
}

uint16_t tile_map_addr;
uint16_t tile_addr;
uint16_t tile_line;
uint8_t tile_x;
uint8_t tile_y;
uint8_t tile_1;
uint8_t tile_2;
uint8_t pixel_data;
uint8_t row[160];
void
drawline_lcd()                                                                  // Seems very incorrect
{


        // Background
        if (IOR[0x40] & 0x1) {
                
                
                // Finding mapped tile
                tile_map_addr = ((IOR[0x44] + IOR[0x42]) >> 3) * 32;        // Snapping to multiples of 8

                if (IOR[0x8] & 0x10) {
                        tile_map_addr += 0x9C00;
                }
                else {
                        tile_map_addr += 0x9800;
                }

                // Finding tile addr
                tile_addr = read_mem(tile_map_addr + (IOR[0x43] >> 3));

                // Tile data area
                if (IOR[0x40] & 0x10) {
                        tile_addr = 0x8000 + tile_addr * 16;
                }
                else {
                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                }

                // X and Y positions within the tile
                tile_y = ((IOR[0x44] + IOR[0x42]) & 0x7);
                tile_x = IOR[0x43] & 0x07;

                // Address corresponding to row
                tile_addr += tile_y * 2;

                // Getting tiles
                tile_1 = read_mem(tile_addr);
                tile_2 = read_mem(tile_addr + 1);

                // Drawing the entire row
                for (uint8_t x = 0; x < 160; x++) {

                        // Getting data for the pixel
                        pixel_data = ((tile_1 << tile_x) & 0x80) >> 7;
                        pixel_data |= ((tile_2 << tile_x) & 0x80) >> 6;

                        graphics_raw[IOR[0x44]][x] = pixel_data;

                        tile_x ++;
                        if (tile_x == 8) {      // Get next tile in row
                                tile_addr = read_mem(tile_map_addr + (IOR[0x43] >> 3) + x + 1) + 2 * tile_y;

                                // Tile data area
                                if (IOR[0x40] & 0x10) {
                                        tile_addr = 0x8000 + tile_addr * 16;
                                }
                                else {
                                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                                }
                                                
                                // Getting tiles
                                tile_1 = read_mem(tile_addr);
                                tile_2 = read_mem(tile_addr + 1);

                                tile_x = 0;
                        }
                }

        }

        // Window
        if ((IOR[0x40] & 0x20) && (IOR[0x44] >= IOR[0x4A])) {                   // Including base bit 0 enable?
                
                // Finding mapped tile
                tile_map_addr = (IOR[0x4A] >> 3) * 32;        // Snapping to multiples of 8

                if (IOR[0x8] & 0x10) {
                        tile_map_addr += 0x9C00;
                }
                else {
                        tile_map_addr += 0x9800;
                }

                // Finding tile addr
                tile_addr = read_mem(tile_map_addr + ((IOR[0x4B] - 7) >> 3));

                // Tile data area
                if (IOR[0x40] & 0x10) {
                        tile_addr = 0x8000 + tile_addr * 16;
                }
                else {
                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                }

                // X and Y positions within the tile
                tile_y = (IOR[0x4A] & 0x7);
                tile_x = (IOR[0x4B] - 7) & 0x7;

                // Address corresponding to row
                tile_addr += tile_y * 2;

                // Getting tiles
                tile_1 = read_mem(tile_addr);
                tile_2 = read_mem(tile_addr + 1);

                // Drawing the entire row
                for (uint8_t x = 0; x < 160 - (IOR[0x4B] - 7); x++) {           // Fix to properly aknowledge window placement

                        // Getting data for the pixel
                        pixel_data = ((tile_1 << tile_x) & 0x80) >> 7;
                        pixel_data |= ((tile_2 << tile_x) & 0x80) >> 6;

                        graphics_raw[IOR[0x44]][x + (IOR[0x4B] - 7)] = pixel_data;

                        tile_x ++;
                        if (tile_x == 8) {      // Get next tile in row
                                tile_addr = read_mem(tile_map_addr + ((IOR[0x4B] - 7) >> 3) + x + 1) + 2 * tile_y;

                                // Tile data area
                                if (IOR[0x40] & 0x10) {
                                        tile_addr = 0x8000 + tile_addr * 16;
                                }
                                else {
                                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                                }
                                                
                                // Getting tiles
                                tile_1 = read_mem(tile_addr);
                                tile_2 = read_mem(tile_addr + 1);

                                tile_x = 0;
                        }
                }
        }

        // Sprites
        if (IOR[0x40] & 0x2) {

        }
}

void
update_timers(uint8_t cycles) 
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

uint32_t colors[4] = {0, 1717986918, 2863311530, 4294967295};
void update_SDL()
{
        // Filling pixels corresponding to graphics array
        for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j ++) {
                        graphics[i][j] = colors[graphics_raw[i][j] & 0x3];
                }
        }

        // Applying texture to screen
        int texture_pitch = 0;
        void* texture_pixels = NULL;
        if (SDL_LockTexture(texture, NULL, &texture_pixels, &texture_pitch) != 0) {
                SDL_Log("Unable to lock texture: %s", SDL_GetError());
        }
        else {
                memcpy(texture_pixels, graphics, texture_pitch * 32);
        }
        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
}
*/