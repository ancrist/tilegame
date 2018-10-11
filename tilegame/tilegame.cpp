// tilegame.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <iostream>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#undef main

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>


#define SPRITE_SHEET_ROWS 8
#define SPRITE_ANIM_FRAMES 12
#define SPRITE_SIZE 64

#define WORLD_SCALE 2

#define TILE_SIZE (SPRITE_SIZE * WORLD_SCALE)

#define WORLD_SIZE_X TILE_SIZE * 64
#define WORLD_SIZE_Y TILE_SIZE * 64

typedef struct GameRenderer {
	SDL_Window* window;
	SDL_Renderer* renderer;
	Uint32 frame;
	TTF_Font* font;
	Uint32 prev_height;
}GameRenderer;

typedef struct Coord {
	int x;
	int y;
}Coord;

typedef struct Size {
	int w;
	int h;
}Size;

typedef struct Sprite {
	Coord frames[SPRITE_ANIM_FRAMES];
} Sprite;


typedef struct PlayerDirectionSprites {
	SDL_Texture* sprite_sheet_texture;
	Sprite directions[8];
}PlayerDirectionSprites;

enum PlayerState {
	Alive,
	Dead
};

typedef struct Player {
	PlayerDirectionSprites direction_sprites;
	bool directions[8];
	int prev_dir_index;
	bool moving;
	Coord location;
	PlayerState player_state;
	Uint32 anim_frame;
}Player;

typedef struct Level {
	SDL_Texture* texture;
}Level;

#define PLAYER_STANCE_DOWN			0
#define PLAYER_STANCE_DOWN_RIGHT	1
#define PLAYER_STANCE_RIGHT			2
#define PLAYER_STANCE_UP_RIGHT		3
#define PLAYER_STANCE_UP			4
#define PLAYER_STANCE_UP_LEFT		5
#define PLAYER_STANCE_LEFT			6
#define PLAYER_STANCE_DOWN_LEFT		7

struct InputState {
	bool up;
	bool down;
	bool left;
	bool right;
};

bool 
init_sdl_subsystems();

bool 
init_sdl();

bool 
init_sdl_image();

bool
init_sdl_ttf();

GameRenderer* 
init_renderer(int w, int h, bool show_window);

GameRenderer* 
init_debug_renderer(GameRenderer* renderer);

void render_debug_string(GameRenderer* renderer, const char* str);

Player* 
load_player(const char* path, GameRenderer* renderer);

SDL_Texture* 
load_texture(const char* path, SDL_Renderer* renderer);

void cleanup();

void render_level()
{
	//SDL_Rect src_rect = {
	//	sprite_sheet->sprites[SPRITE_FLOOR].loc.x,
	//	sprite_sheet->sprites[SPRITE_FLOOR].loc.y,
	//	SPRITE_SIZE,
	//	SPRITE_SIZE,
	//};

	//SDL_Rect dst_rect = src_rect;
	//dst_rect.w *= WORLD_SCALE;
	//dst_rect.h *= WORLD_SCALE;

	//for(int i = 0; i < 1280; i+= (SPRITE_SIZE*WORLD_SCALE ))
	//	for (int j = 0; j < 1280; j += (SPRITE_SIZE*WORLD_SCALE)) {
	//		dst_rect.x = i;
	//		dst_rect.y = j;
	//		SDL_RenderCopy(renderer, sprite_sheet->texture_pointer, &src_rect, &dst_rect);
	//	}
}

void render_player(GameRenderer* renderer, Player* player)
{
	static int frame_index;
	Coord c;
	int i = 0;

	for (; i < 8; i++) {
		if (player->directions[i] == true)
			break;
	}

	// when changing directions, start from first animation frame
	if (player->prev_dir_index != i) {
		player->prev_dir_index = i;
		frame_index = 0;
		player->anim_frame = 0;
	}

	player->anim_frame++;
	if (player->moving) {
		Sprite sprite = player->direction_sprites.directions[i];
				
		if (60 / player->anim_frame == 12) {
			player->anim_frame = 0;
			frame_index++;
		}

		c = sprite.frames[frame_index % SPRITE_ANIM_FRAMES];
	}
	else {
		c = player->direction_sprites.directions[i].frames[0];
	}
	

	SDL_Rect src_rect = {
		c.x,
		c.y,
		SPRITE_SIZE,
		SPRITE_SIZE,
	};

	SDL_Rect dst_rect = src_rect;

	dst_rect.x = player->location.x;
	dst_rect.y = player->location.y;
	dst_rect.w *= WORLD_SCALE;
	dst_rect.h *= WORLD_SCALE;

	SDL_RenderCopy(renderer->renderer, player->direction_sprites.sprite_sheet_texture, &src_rect, &dst_rect);
}

void update_player_location(Player* player)
{
	if (player->directions[PLAYER_STANCE_DOWN]) {
		player->location.y+=10;
	}
	if (player->directions[PLAYER_STANCE_DOWN_LEFT]) {
		player->location.y+= 10;
		player->location.x-= 10;
	}
	if (player->directions[PLAYER_STANCE_DOWN_RIGHT]) {
		player->location.y+= 10;
		player->location.x+= 10;
	}
	if (player->directions[PLAYER_STANCE_UP]) {
		player->location.y-= 10;
	}
	if (player->directions[PLAYER_STANCE_UP_LEFT]) {
		player->location.y-= 10;
		player->location.x-= 10;
	}
	if (player->directions[PLAYER_STANCE_UP_RIGHT]) {
		player->location.y-= 10;
		player->location.x+= 10;
	}
	if (player->directions[PLAYER_STANCE_LEFT]) {
		player->location.x-= 10;
	}
	if (player->directions[PLAYER_STANCE_RIGHT]) {
		player->location.x+= 10;
	}
}

bool update_player_state(SDL_Event e, InputState* input_state, Player* player)
{
	if (e.type == SDL_KEYDOWN) {
		const Uint8* kb_state = SDL_GetKeyboardState(NULL);
		
		if (kb_state[SDL_SCANCODE_UP] || kb_state[SDL_SCANCODE_DOWN] || kb_state[SDL_SCANCODE_LEFT] || kb_state[SDL_SCANCODE_RIGHT]) {
			player->directions[PLAYER_STANCE_UP] = kb_state[SDL_SCANCODE_UP];
			player->directions[PLAYER_STANCE_DOWN] = kb_state[SDL_SCANCODE_DOWN];
			player->directions[PLAYER_STANCE_LEFT] = kb_state[SDL_SCANCODE_LEFT];
			player->directions[PLAYER_STANCE_RIGHT] = kb_state[SDL_SCANCODE_RIGHT];

			player->directions[PLAYER_STANCE_UP_RIGHT] = kb_state[SDL_SCANCODE_UP] && kb_state[SDL_SCANCODE_RIGHT];
			if (player->directions[PLAYER_STANCE_UP_RIGHT]) {
				player->directions[PLAYER_STANCE_UP] = 0;
				player->directions[PLAYER_STANCE_RIGHT] = 0;
			}
			player->directions[PLAYER_STANCE_UP_LEFT] = kb_state[SDL_SCANCODE_UP] && kb_state[SDL_SCANCODE_LEFT];
			if (player->directions[PLAYER_STANCE_UP_LEFT]) {
				player->directions[PLAYER_STANCE_UP] = 0;
				player->directions[PLAYER_STANCE_LEFT] = 0;
			}
			player->directions[PLAYER_STANCE_DOWN_RIGHT] = kb_state[SDL_SCANCODE_DOWN] && kb_state[SDL_SCANCODE_RIGHT];
			if (player->directions[PLAYER_STANCE_DOWN_RIGHT]) {
				player->directions[PLAYER_STANCE_DOWN] = 0;
				player->directions[PLAYER_STANCE_RIGHT] = 0;
			}
			player->directions[PLAYER_STANCE_DOWN_LEFT] = kb_state[SDL_SCANCODE_DOWN] && kb_state[SDL_SCANCODE_LEFT];
			if (player->directions[PLAYER_STANCE_DOWN_LEFT]) {
				player->directions[PLAYER_STANCE_DOWN] = 0;
				player->directions[PLAYER_STANCE_LEFT] = 0;
			}

			player->moving = true;

			update_player_location(player);

			return true;
		}
	}
	else if (e.type == SDL_KEYUP) {
		player->moving = false;
		return false;
	}
	return false;
}

void render_clear(GameRenderer* renderer)
{
	SDL_SetRenderDrawColor(renderer->renderer, 0xff, 0xff, 0xff, 0xff);
	SDL_RenderClear(renderer->renderer);
}

void log_player_state(Player* player, GameRenderer* renderer)
{
	char buf[128] = {};
	//sprintf_s(buf, "U: %d, D: %d, L: %d, R: %d, UR: %d, UL: %d, DR: %d, DL: %d",
	//	player->directions[PLAYER_STANCE_UP],
	//	player->directions[PLAYER_STANCE_DOWN],
	//	player->directions[PLAYER_STANCE_LEFT],
	//	player->directions[PLAYER_STANCE_RIGHT],
	//	player->directions[PLAYER_STANCE_UP_RIGHT],
	//	player->directions[PLAYER_STANCE_UP_LEFT],
	//	player->directions[PLAYER_STANCE_DOWN_RIGHT],
	//	player->directions[PLAYER_STANCE_DOWN_LEFT]);
	sprintf_s(buf, "x: %d, y: %d", player->location.x, player->location.y);
	SDL_SetWindowTitle(renderer->window, buf);
}

void log_fps(GameRenderer* renderer)
{
	static Uint32 last_time = 0;
	Uint32 current_time = SDL_GetTicks();
	
	if (current_time - last_time >= 1000) {
		char buf[128];
		sprintf_s(buf, "FPS: %d", renderer->frame);
		SDL_SetWindowTitle(renderer->window, buf);
		renderer->frame = 0;
		last_time = current_time;
	}
}


GameRenderer* init_debug_renderer(GameRenderer* renderer)
{
	int x = 0, y = 0;
	int width, height;
	GameRenderer* r = init_renderer(640, 480, false);
	SDL_GetWindowPosition(renderer->window, &x, &y);
	SDL_GetWindowSize(renderer->window, &width, &height);
	int right_border = 0;
	SDL_GetWindowBordersSize(renderer->window, NULL, NULL, NULL, &right_border);
	SDL_SetWindowPosition(r->window, x + width + right_border, y);


	r->font = TTF_OpenFont("c:/windows/Fonts/segoeui.ttf", 12);
	r->prev_height = 0;
	return r;
}


void render_debug_string(GameRenderer* renderer, const char* str)
{
	int w, h;
	int ret = TTF_SizeText(renderer->font, str, &w, &h);
	SDL_Color color = { 0xff, 0xff, 0xff, 0xff };
	SDL_Surface* text_surface= TTF_RenderText_Blended(renderer->font, str, color);
	SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer->renderer, text_surface);
	
	SDL_Rect dst = {
		0,
		renderer->prev_height,
		w,
		h,
	};

	renderer->prev_height += h;

	SDL_RenderCopy(renderer->renderer, tex, NULL, &dst);
	SDL_RenderPresent(renderer->renderer);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	bool show_debug_window = false;
	int direction = 1;
	int window_w = 1280;
	int window_h = window_w / 2;
	Uint32 last_time = SDL_GetTicks();
	InputState input_state = {};
	SDL_Event e;
	bool quit = false;
	
	if (!init_sdl_subsystems()) {
		SDL_Quit();
		exit(1);
	}

	GameRenderer* renderer = init_renderer(window_w, window_h, true);

	GameRenderer* debug_renderer = init_debug_renderer(renderer);
	
	if (!renderer) {
		cleanup();
		exit(1);
	}
	
	Player* player = load_player("Resources/textures/player_sprites.png", renderer);
	if (!player) {
		printf("Unable to load player\n");
		exit(1);
	}
	
	while(!quit){
		while (SDL_PollEvent(&e) == 1) {
			if (e.type == SDL_WINDOWEVENT) {
				if(e.window.event == SDL_WINDOWEVENT_CLOSE)
					quit = true;
			} else {
				bool updated = update_player_state(e, &input_state, player);

				char buf[128];
				if (updated) {
					sprintf_s(buf, "Frame: %d, x: %d, y: %d", renderer->frame, player->location.x, player->location.y);
					render_debug_string(debug_renderer, buf);
				}
				else {
					sprintf_s(buf, "Moving: %d", player->moving);
					render_debug_string(debug_renderer, buf);
				}
			}
		}
		

		renderer->frame++;
		//#TODO: render constant frame rate

		
		render_clear(renderer);

		//render_level();
		render_player(renderer, player);


		SDL_RenderPresent(renderer->renderer);
	}

	cleanup();
	SDL_Quit();
	return 0;
}

bool init_sdl_subsystems()
{
	return init_sdl() && init_sdl_image() && init_sdl_ttf();
}


bool init_sdl_ttf()
{
	if (TTF_Init() == -1) {
		printf("Could not initialize SDL TTF\n");
		return false;
	}
	return true;
}

bool init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Could not initialize SDL\n");
		return false;
	}
	return true;
}

bool init_sdl_image()
{
	int img_flags = IMG_INIT_PNG;

	if (!IMG_Init(img_flags) & img_flags) {
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}				
	return true;
}

GameRenderer* init_renderer(int w, int h, bool show_window)
{
	GameRenderer* renderer = (GameRenderer*)malloc(sizeof(GameRenderer));
	
	renderer->window = SDL_CreateWindow("The best game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, (show_window)? SDL_WINDOW_SHOWN : SDL_WINDOW_HIDDEN);

	if (renderer->window == NULL) {
		printf("Could not create window\n");
		free(renderer);
		renderer = NULL;
	} else {
		renderer->renderer = SDL_CreateRenderer(renderer->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		if (renderer->renderer == NULL) {
			printf("Could not initialize renderer: %s\n", SDL_GetError());
			free(renderer);
			renderer = NULL;
		}
	}

	renderer->frame = 0;
	return renderer;
}


Player* load_player(const char* path, GameRenderer* renderer)
{
	Player* p = (Player*)malloc(sizeof(Player));
	ZeroMemory(p, sizeof(Player));
	
	p->direction_sprites.sprite_sheet_texture = load_texture(path, renderer->renderer);
			
	if (p->direction_sprites.sprite_sheet_texture == NULL)
	{
		printf("Unable to load player sprite sheet %s! SDL_image Error: %s\n", path, IMG_GetError());
		free(p);
		p = NULL;
	}
	else
	{
		SDL_SetTextureBlendMode(p->direction_sprites.sprite_sheet_texture, SDL_BLENDMODE_BLEND);
		Uint32 format;
		int access, w, h;
		
		SDL_QueryTexture(p->direction_sprites.sprite_sheet_texture, &format, &access, &w, &h);
		const char* format_name = SDL_GetPixelFormatName(format);

		for (int i = 0; i < SPRITE_SHEET_ROWS; i++) {
			for (int j = 0; j < SPRITE_ANIM_FRAMES; j++) {
				p->direction_sprites.directions[i].frames[j] = { j * SPRITE_SIZE, i * SPRITE_SIZE };
			}
		}

		p->moving = false;
		p->location = { 0,0 };
		p->anim_frame = 0;
	}

	return p;
}


SDL_Texture* load_texture(const char* path, SDL_Renderer* renderer)
{
	//The final texture
	SDL_Texture* new_texture = NULL;

	//Load image at specified path
	SDL_Surface* loaded_surface = IMG_Load(path); 
	
	if (loaded_surface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
	}
	else
	{
		int ret = SDL_SetSurfaceRLE(loaded_surface, 1);
		SDL_SetColorKey(loaded_surface, SDL_TRUE, SDL_MapRGB(loaded_surface->format, 0xff, 0xff, 0xff));

		//Create texture from surface pixels
		new_texture = SDL_CreateTextureFromSurface(renderer, loaded_surface);
		if (new_texture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loaded_surface);
	}

	return new_texture;
}

void cleanup()
{
//	SDL_DestroyTexture(level_texture);

	//Deallocate surface
//	SDL_FreeSurface(main_surface);
//	main_surface = NULL;

	//Destroy window
//	SDL_DestroyWindow(window);
//	window = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}