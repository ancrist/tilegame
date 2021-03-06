// tilegame.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vector>
#include <iostream>

#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "load_shaders.h"

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
	SDL_GLContext gl_context;
	GLuint shader_program;
	ImVec4 clear_color;
	GLuint tex;
	bool show_overlay;
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
	GLuint vertex_count;
	GLuint player_vbo_id;
	GLuint player_vao_id;
	GLuint player_tex_id;
	GLuint player_ebo_id;
}Player;


typedef struct GameState {
	Player* player;
}GameState;

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

unsigned int VBO, VAO, EBO;
unsigned int texture;

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

bool 
init_opengl_loader();

void 
render(GameRenderer* renderer, GameState* state);

GameRenderer* 
init_renderer(int w, int h, int vsync);

GameRenderer* 
init_debug_renderer(GameRenderer* renderer);

void render_debug_string(GameRenderer* renderer, const char* str);

//Player* 
//load_player(const char* path, GameRenderer* renderer);

Player*
load_player_gl(const char* path, GameRenderer* renderer);

void 
load_gl_shaders(GameRenderer* renderer);

void 
render_player(GameRenderer* renderer, Player* player);

SDL_Texture*
load_texture(const char* path, SDL_Renderer* renderer);

void cleanup();

void show_overlay_window(GameRenderer* renderer);


void clear_window(GameRenderer* renderer)
{
	int w, h;
	SDL_GetWindowSize(renderer->window, &w, &h);

	glViewport(0, 0, w, h);
	glClearColor(renderer->clear_color.x, renderer->clear_color.y, renderer->clear_color.z, renderer->clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

void render_world(GameRenderer* renderer, GameState* state)
{
	glBindTexture(GL_TEXTURE_2D, state->player->player_tex_id);
	glBindVertexArray(state->player->player_vao_id);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	//glBindTexture(GL_TEXTURE_2D, texture);
	//glBindVertexArray(VAO);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void render_gui(GameRenderer* renderer)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(renderer->window);
	ImGui::NewFrame();

	if (renderer->show_overlay)
		show_overlay_window(renderer);
	//ImGui::ShowDemoWindow(&state->show_overlay);

	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void render(GameRenderer* renderer, GameState* state)
{
	clear_window(renderer);

	render_world(renderer, state);

	render_gui(renderer);

	SDL_GL_SwapWindow(renderer->window);
}


bool init_opengl_loader()
{
	if (!gladLoadGL()) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Tilegame", "Unable to load GL extensions", NULL);
		return false;
	}
	return true;
}

#define BUFFER_OFFSET(offset) ((void *)(offset))

Player* load_player_gl(const char* path, GameRenderer* renderer)
{
	Player* p = new Player();

	float vertices[] = {
		// positions          // colors           // texture coords
		1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,    0.0f, 0.0,  // top right     
		1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,    0.0f, 1.0f, // bottom right  
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  1.0f, 1.0f,// bottom left  
		-1.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,  1.0f, 0.0f // top left		
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	glGenVertexArrays(1, &p->player_vao_id);
	glGenBuffers(1, &p->player_vbo_id);
	glGenBuffers(1, &p->player_ebo_id);

	glBindVertexArray(p->player_vao_id);

	glBindBuffer(GL_ARRAY_BUFFER, p->player_vbo_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->player_ebo_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glGenTextures(1, &p->player_tex_id);
	renderer->tex = p->player_tex_id;
	glBindTexture(GL_TEXTURE_2D, p->player_tex_id); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
										   // set the texture wrapping parameters

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	// The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
	
	GLuint px_format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
	glTexImage2D(GL_TEXTURE_2D, 0, px_format, width, height, 0, px_format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	
	stbi_image_free(data);

	load_gl_shaders(renderer);
	
	for (int i = 0; i < SPRITE_SHEET_ROWS; i++) {
		for (int j = 0; j < SPRITE_ANIM_FRAMES; j++) {
			p->direction_sprites.directions[i].frames[j] = { j * SPRITE_SIZE, i * SPRITE_SIZE };
		}
	}

	p->moving = false;
	p->location = { 0,0 };
	p->anim_frame = 0;


	return p;
}



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
	const int vsync_enabled = 0;
	GameRenderer* r = init_renderer(640, 480, vsync_enabled);
	SDL_GetWindowPosition(renderer->window, &x, &y);
	SDL_GetWindowSize(renderer->window, &width, &height);
	int right_border = 0;
	SDL_GetWindowBordersSize(renderer->window, NULL, NULL, NULL, &right_border);
	SDL_SetWindowPosition(r->window, x + width + right_border, y);

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

void show_overlay_window(GameRenderer* renderer)
{
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::Begin("Game stats", &renderer->show_overlay, ImGuiWindowFlags_NoTitleBar);

	//ImGui::PopStyleColor();

	//ImGui::Text("Game average frame time %.3f ms (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	//ImGui::Separator();

	{
		ImGuiIO& io = ImGui::GetIO();
		//				ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");

						// Here we are grabbing the font texture because that's the only one we have access to inside the demo code.
						// Remember that ImTextureID is just storage for whatever you want it to be, it is essentially a value that will be passed to the render function inside the ImDrawCmd structure.
						// If you use one of the default imgui_impl_XXXX.cpp renderer, they all have comments at the top of their file to specify what they expect to be stored in ImTextureID.
						// (for example, the imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer. The imgui_impl_glfw_gl3.cpp renderer expect a GLuint OpenGL texture identifier etc.)
						// If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers to ImGui::Image(), and gather width/height through your own functions, etc.
						// Using ShowMetricsWindow() as a "debugger" to inspect the draw data that are being passed to your render will help you debug issues if you are confused about this.
						// Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
		//ImTextureID my_tex_id = io.Fonts->TexID;
		//float my_tex_w = (float)io.Fonts->TexWidth;
		//float my_tex_h = (float)io.Fonts->TexHeight;

		//ImGui::Text("");
		//ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::Image((ImTextureID)renderer->tex, ImVec2(256, 256), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 0));
		//if (ImGui::IsItemHovered())
		//{
		//	ImGui::BeginTooltip();
		//	float region_sz = 32.0f;
		//	float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > my_tex_w - region_sz) region_x = my_tex_w - region_sz;
		//	float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > my_tex_h - region_sz) region_y = my_tex_h - region_sz;
		//	float zoom = 4.0f;
		//	ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
		//	ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
		//	ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
		//	ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
		//	ImGui::Image((ImTextureID)renderer->tex, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
		//	ImGui::EndTooltip();
		//}
	}



	ImGui::End();
}


void load_gl_shaders(GameRenderer* renderer)
{
	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "Resources\\shaders\\tilegame.vert" },
		{ GL_FRAGMENT_SHADER, "Resources\\shaders\\tilegame.frag" },
		{ GL_NONE, NULL }
	};

	renderer->shader_program = load_shaders(shaders);
	glUseProgram(renderer->shader_program);
	GLenum err = glGetError();
	int a = 0;
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	bool show_debug_window = false;
	int direction = 1;
	int window_w = 1280;
	int window_h = window_w / 2;
	Uint32 last_time = SDL_GetTicks();
	SDL_Event event;
	bool quit = false;
	GameRenderer* renderer = nullptr;
	GameState* state = nullptr;
	const int vsync_enabled = 1;

	if (!init_sdl_subsystems()) {
		SDL_Quit();
		exit(1);
	}

	renderer = init_renderer(window_w, window_h, vsync_enabled);

	if (!renderer) {
		cleanup();
		exit(1);
	}

	state = new GameState();
	renderer->show_overlay = false;

	if (!init_opengl_loader()) {
		exit(0);
	}
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplSDL2_InitForOpenGL(renderer->window, renderer->gl_context);
	ImGui_ImplOpenGL3_Init("#version 430");

	ImGui::StyleColorsDark();

	renderer->clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	state->player = load_player_gl("Resources/textures/black-brick-wall-texture.bmp", renderer);
	if (!state->player) {
		printf("Unable to load player\n");
		exit(1);
	}
	
	int x = 0, y = 0;
	
	while(!quit){
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			
			if (!ImGui::GetIO().WantCaptureKeyboard) {
				if (event.type == SDL_KEYDOWN) {

					if (event.key.keysym.scancode == SDL_SCANCODE_O)
						renderer->show_overlay = !renderer->show_overlay;
				}
			}

			if (event.type == SDL_WINDOWEVENT) {
				if(event.window.event == SDL_WINDOWEVENT_CLOSE)
					quit = true;
			} else if( event.type == SDL_MOUSEMOTION ){
				x = event.motion.x;
				y = event.motion.y;
			} 
		}

		render(renderer, state);
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

GameRenderer* init_renderer(int w, int h, int vsync)
{
	GameRenderer* renderer = (GameRenderer*)malloc(sizeof(GameRenderer));
	
	renderer->window = SDL_CreateWindow("The best game", 
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
		w, h, 
		SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

	if (renderer->window == NULL) {
		printf("Could not create window\n");
		free(renderer);
		renderer = NULL;
	} else {
		int ret = 0;
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 24);
		
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8); 

		renderer->gl_context = SDL_GL_CreateContext(renderer->window);
		ret = SDL_GL_MakeCurrent(renderer->window, renderer->gl_context);
		SDL_DisplayMode display_mode;
		SDL_GetCurrentDisplayMode(0, &display_mode);

		SDL_GL_SetSwapInterval(vsync);
	}

	renderer->frame = 0;
	return renderer;
}


//Player* load_player(const char* path, GameRenderer* renderer)
//{
//	Player* p = (Player*)malloc(sizeof(Player));
//	ZeroMemory(p, sizeof(Player));
//	
//	p->direction_sprites.sprite_sheet_texture = load_texture(path, renderer->renderer);
//			
//	if (p->direction_sprites.sprite_sheet_texture == NULL)
//	{
//		printf("Unable to load player sprite sheet %s! SDL_image Error: %s\n", path, IMG_GetError());
//		free(p);
//		p = NULL;
//	}
//	else
//	{
//		SDL_SetTextureBlendMode(p->direction_sprites.sprite_sheet_texture, SDL_BLENDMODE_BLEND);
//		Uint32 format;
//		int access, w, h;
//		
//		SDL_QueryTexture(p->direction_sprites.sprite_sheet_texture, &format, &access, &w, &h);
//		const char* format_name = SDL_GetPixelFormatName(format);
//
//		for (int i = 0; i < SPRITE_SHEET_ROWS; i++) {
//			for (int j = 0; j < SPRITE_ANIM_FRAMES; j++) {
//				p->direction_sprites.directions[i].frames[j] = { j * SPRITE_SIZE, i * SPRITE_SIZE };
//			}
//		}
//
//		p->moving = false;
//		p->location = { 0,0 };
//		p->anim_frame = 0;
//	}
//
//	return p;
//}


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