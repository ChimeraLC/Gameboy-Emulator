#include <stdio.h>
#include <SDL.h>
#include <SDL_audio.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

#include "main.h"
#include "gb_cpu.h"
#include "gb_gpu.h"



// Tileman Variables
uint16_t tile_map_addr;
uint16_t tile_addr;
uint16_t tile_line;
uint8_t tile_x;
uint8_t tile_y;
uint8_t tile_1;
uint8_t tile_2;
uint8_t pixel_data;
uint8_t row[160];
uint8_t sprite_x;
uint8_t sprite_y;
uint8_t sprite_tile;
uint8_t sprite_attr;


// SDL elements
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
uint32_t graphics[144][160];
uint8_t graphics_raw[144][160];

void
init_gpu()
{
}

void
drawline_lcd()                                                                  // Seems very incorrect
{
        if (verbose == 3) {
                printf("Drawing line %d\n", get_IOR(0x44));
        }
        // Background
        if (get_IOR(0x40) & 0x1) {
                
                
                // Finding mapped tile
                tile_map_addr = ((get_IOR(0x44) + get_IOR(0x42)) >> 3) * 32;        // Snapping to multiples of 8

                if (get_IOR(0x8) & 0x10) {
                        tile_map_addr += 0x9C00;
                }
                else {
                        tile_map_addr += 0x9800;
                }

                // Finding tile addr
                tile_addr = read_mem(tile_map_addr + (get_IOR(0x43) >> 3));

                // Tile data area
                if (get_IOR(0x40) & 0x10) {
                        tile_addr = 0x8000 + tile_addr * 16;
                }
                else {
                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                }

                // X and Y positions within the tile
                tile_y = ((get_IOR(0x44) + get_IOR(0x42)) & 0x7);
                tile_x = get_IOR(0x43) & 0x07;

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

                        graphics_raw[get_IOR(0x44)][x] = pixel_data;

                        tile_x ++;
                        if (tile_x == 8) {      // Get next tile in row
                                tile_addr = read_mem(tile_map_addr + (get_IOR(0x43) >> 3) + x + 1) + 2 * tile_y;

                                // Tile data area
                                if (get_IOR(0x40) & 0x10) {
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
        if ((get_IOR(0x40) & 0x20) && (get_IOR(0x44) >= get_IOR(0x4A))) {                   // Including base bit 0 enable?
                
                // Finding mapped tile
                tile_map_addr = (get_IOR(0x4A) >> 3) * 32;        // Snapping to multiples of 8

                if (get_IOR(0x8) & 0x10) {
                        tile_map_addr += 0x9C00;
                }
                else {
                        tile_map_addr += 0x9800;
                }

                // Finding tile addr
                tile_addr = read_mem(tile_map_addr + ((get_IOR(0x4B) - 7) >> 3));

                // Tile data area
                if (get_IOR(0x40) & 0x10) {
                        tile_addr = 0x8000 + tile_addr * 16;
                }
                else {
                        tile_addr = 0x8800 + ((tile_addr + 128) % 0x100) * 16;
                }

                // X and Y positions within the tile
                tile_y = (get_IOR(0x4A) & 0x7);
                tile_x = (get_IOR(0x4B) - 7) & 0x7;

                // Address corresponding to row
                tile_addr += tile_y * 2;

                // Getting tiles
                tile_1 = read_mem(tile_addr);
                tile_2 = read_mem(tile_addr + 1);

                // Drawing the entire row
                for (uint8_t x = 0; x < 160 - (get_IOR(0x4B) - 7); x++) {           // Fix to properly aknowledge window placement

                        // Getting data for the pixel
                        pixel_data = ((tile_1 << tile_x) & 0x80) >> 7;
                        pixel_data |= ((tile_2 << tile_x) & 0x80) >> 6;

                        graphics_raw[get_IOR(0x44)][x + (get_IOR(0x4B) - 7)] = pixel_data;

                        tile_x ++;
                        if (tile_x == 8) {      // Get next tile in row
                                tile_addr = read_mem(tile_map_addr + ((get_IOR(0x4B) - 7) >> 3) + x + 1) + 2 * tile_y;

                                // Tile data area
                                if (get_IOR(0x40) & 0x10) {
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
        if (get_IOR(0x40) & 0x2) {
                for (int i = 39; i > 0; i--) {  // Draw lower values on top

                        sprite_y = get_OAM(i * 4);
                        sprite_x = get_OAM(i * 4 + 1);
                        sprite_tile = get_OAM(i * 4 + 2);
                        sprite_attr = get_OAM(i * 4 + 3);

                        if (get_IOR(0x44) + 8 - 2 * (get_IOR(0x40) & 0x04) < sprite_y
                         && get_IOR(0x44) >= sprite_y - 16) {
                                                                                // TODO: limit sprites per row?
                                // Skipping invisible sprites
                                if (sprite_x == 0 || sprite_x >= 168) {
                                        continue;
                                }

                                bool flip_x = sprite_attr & 0x40;
                                bool flip_y = sprite_attr & 0x20;
                                
                                // Calculating tile address
                                // Horizontal pixel line
                                tile_addr = get_IOR(0x44) - (sprite_y - 16);
                                if (flip_y) {
                                        tile_addr = 7 + 2 * (get_IOR(0x40) & 0x04) -
                                                tile_addr;
                                }

                                // Getting tiles
                                tile_1 = read_mem(tile_addr);
                                tile_2 = read_mem(tile_addr + 1);

                                if (flip_x) {
                                        tile_1 >>= sprite_x - MIN(sprite_x, 160);        // Maybe clean these up?
                                        tile_2 >>= sprite_x - MIN(sprite_x, 160);
                                        for (int j = MIN(sprite_x, 160) - 1;
                                            j >= MAX(sprite_x - 8, 0); j--) {
                                                // Getting data for the pixel
                                                pixel_data = ((tile_1 << tile_x) & 0x80) >> 7;
                                                pixel_data |= ((tile_2 << tile_x) & 0x80) >> 6;

                                                if (pixel_data != 0){           // Ignoring transparent
                                                        graphics_raw[get_IOR(0x44)][j] = pixel_data;

                                                        // TODO: palettes!!!!----------------------------------------------------------
                                                }

                                                tile_1 >>= 1;
                                                tile_2 >>= 1;
                                            }
                                }
                                else {
                                        tile_1 >>= MAX(sprite_x - 8, 0) - (sprite_x - 8);        // Maybe clean these up?
                                        tile_2 >>= MAX(sprite_x - 8, 0) - (sprite_x - 8);
                                        for (int j = MAX(sprite_x - 8, 0); 
                                            j < MIN(sprite_x, 160); j++) {
                                                // Getting data for the pixel
                                                pixel_data = ((tile_1 << tile_x) & 0x80) >> 7;
                                                pixel_data |= ((tile_2 << tile_x) & 0x80) >> 6;

                                                if (pixel_data != 0){           // Ignoring transparent
                                                        graphics_raw[get_IOR(0x44)][j] = pixel_data;

                                                        // TODO: palettes!!!!----------------------------------------------------------
                                                }

                                                tile_1 >>= 1;
                                                tile_2 >>= 1;
                                        }
                                }
                        }

                }
        }
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
                                        SDL_TEXTUREACCESS_STREAMING, 160, 144);

        return true; // Return true when there is no problem
}

uint32_t colors[4] = {0, 1717986918, 2863311530, 4294967295};
void update_SDL()
{
        // Filling pixels corresponding to graphics array
        for (int i = 0; i < 144; i++) {
                for (int j = 0; j < 160; j ++) {
                        graphics[i][j] = colors[graphics_raw[i][j]];
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
