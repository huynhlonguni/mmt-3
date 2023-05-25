#define RAYLIB_NUKLEAR_IMPLEMENTATION
#define NK_INCLUDE_FIXED_TYPES
#define NK_ASSERT(...) (void)0
#define NK_BUTTON_TRIGGER_ON_RELEASE
#include "raylib-nuklear.h"
#include "fragment.h"
#include "types.h"
#include "client.h"
#include "key_visualizer.h"
#include <iostream>
#include <thread>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;


ScreenClient screen_client;
ControlClient control_client;
MulticastClient multicast_client;

#define PANEL_SIZE 38
#define UI_LINE_HEIGHT 30

nk_context *ctx = NULL;
enum View {
	VIEW_NONE = 0,
	VIEW_INIT,
	VIEW_APP,
	VIEW_PROCESS,
	VIEW_DIRECTORY,
	VIEW_SETTINGS,
};

struct nk_image
	pause_img, play_img, stop_img, //proceses
	file_img, folder_img, parent_img, drive_img, //directory
	open_img, copy_img, move_img, delete_img; //directory context menu

View current_view = VIEW_INIT;

Texture2D screen_texture = {0};

int mouse_x, mouse_y, mouse_width, mouse_height;
Texture2D mouse_texture = {0};

Camera2D camera = {0};
Shader shader = {0};

KeyVisualizer key_visualizer;

int view_begin(const char *name, bool forced = 0) {
	const int pad_x = 50;
	const int pad_y = 50;
	const struct nk_rect rc = nk_rect(pad_x, pad_y, screen_client.getWidth() / 2 - pad_x * 2, screen_client.getHeight() / 2 - pad_y * 2);
	if (!forced)
		return nk_begin(ctx, name, rc, NK_WINDOW_TITLE | NK_WINDOW_CLOSABLE | NK_WINDOW_MOVABLE);
	return nk_begin(ctx, name, rc, NK_WINDOW_TITLE);
}

#include "process_view.h"

#include "directory_view.h"

NK_API nk_bool nk_filter_ip_address(const struct nk_text_edit *box, nk_rune unicode) {
	NK_UNUSED(box);
	if ((unicode < '0' || unicode > '9') && unicode != '.')
		return nk_false;
	else return nk_true;
}

bool validate_ip_address(const char *ip_address) {
	int count = 0;
	for (int i = 0; ip_address[i] != '\0'; i++)
		if (ip_address[i] == '.') count++;
	
	if (count != 3) return 0;

	count = 0;
	for (int i = 0; ip_address[i] != '\0'; i++) {
		if (ip_address[i] == '.') {
			if (count <= 0 || count >= 4) return 0;
			count = 0;
		}
		else
			count++;
	}
	if (ip_address[TextLength(ip_address) - 1] == '.') return 0;

	return 1;
}

void InitView(nk_context *ctx) {
	static std::string message = "";
	static char ip_address[16];
	if (screen_client.getLastConnectError() == 1 || control_client.getLastConnectError() == 1)
		message = "Connection failed";
	if (nk_begin(ctx, "Init", nk_rect(0, 0, GetScreenWidth(), GetScreenHeight()), 0)) {
		nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
		nk_select_label(ctx, "Welcome to 3ChangeDev's remote desktop control", NK_TEXT_LEFT, 0);
		nk_select_label(ctx, "Input the server's IP address in the box below", NK_TEXT_LEFT, 0);
		nk_select_label(ctx, "It may takes up to 15 seconds to connect", NK_TEXT_LEFT, 0);
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 3);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 4);
		nk_layout_row_template_end(ctx);
		nk_select_label(ctx, "IP Address", NK_TEXT_CENTERED, 0);
		nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, ip_address, 16, nk_filter_ip_address);
		if (nk_button_label(ctx, "Connect")) {
			if (validate_ip_address(ip_address)) {
				if (screen_client.connect(ip_address) && control_client.connect(ip_address)) {
					message = "Connecting to " + std::string(ip_address);
				}
				else {
					message = "Cannot connect to server at address " + std::string(ip_address);
				}
			}
			else {
				message = "The address you typed is not valid";
			}
		}
		
		nk_layout_row_template_begin(ctx, UI_LINE_HEIGHT);
		nk_layout_row_template_push_static(ctx, UI_LINE_HEIGHT * 3);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_end(ctx);
		nk_select_label(ctx, "", NK_TEXT_LEFT, 0); //Pad
		nk_select_label(ctx, message.c_str(), NK_TEXT_LEFT, 0);

		std::vector<std::string> ips = multicast_client.getAddresses();
		if (ips.size()) {
			nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
			nk_select_label(ctx, "Alternatively, these addresses were detected on your network", NK_TEXT_LEFT, 0);
			nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 2);
			for (const auto &ip: ips) {
				if (nk_button_label(ctx, ip.c_str())) {
					if (screen_client.connect(ip.c_str()) && control_client.connect(ip.c_str())) {
						message = "Connecting to " + ip;
					}
					else {
						message = "Cannot connect to server at address " + ip;
					}
				}
			}
		}
	}
	nk_end(ctx);

	if (screen_client.isConnected() && control_client.isConnected()) {
		IoContextRun();

		screen_client.init();

		UnloadTexture(screen_texture);
		//Screen texture Init
		Image screen_image = GenImageColor(screen_client.getWidth(), screen_client.getHeight(), BLANK);
		screen_texture = LoadTextureFromImage(screen_image);
		UnloadImage(screen_image);
		SetTextureFilter(screen_texture, TEXTURE_FILTER_BILINEAR);
		SetTextureWrap(screen_texture, TEXTURE_WRAP_CLAMP);

		SetWindowMinSize(screen_texture.width / 2, screen_texture.height / 2 + PANEL_SIZE);
		SetWindowSize(screen_texture.width / 2, screen_texture.height / 2 + PANEL_SIZE);

		UnloadTexture(mouse_texture);
		//Mouse texture init
		Image mouse_image = GenImageColor(32, 32, BLANK);
		mouse_texture = LoadTextureFromImage(mouse_image);
		UnloadImage(mouse_image);
		SetTextureFilter(mouse_texture, TEXTURE_FILTER_BILINEAR);
		SetTextureWrap(mouse_texture, TEXTURE_WRAP_CLAMP);

		current_view = VIEW_NONE;
		message = "";
	}
}

void SettingsView(nk_context *ctx) {
	if (view_begin("Settings")) {
		nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 1);
		if (nk_button_label(ctx, "Disconnect")) {
			current_view = VIEW_INIT;
			screen_client.disconnect();
			control_client.disconnect();
			SetWindowMinSize(500, 300);
			SetWindowSize(500, 300);
		}

	} else {
		current_view = VIEW_NONE;
	}
	nk_end(ctx);
}

void NuklearView(nk_context *ctx) {
	UpdateNuklear(ctx);
	if (current_view == VIEW_INIT) return InitView(ctx);

	if (nk_begin(ctx, "Nuklear", nk_rect(0, 0, GetScreenWidth(), PANEL_SIZE), NK_WINDOW_NO_SCROLLBAR)) {
		nk_layout_row_dynamic(ctx, UI_LINE_HEIGHT, 4);
		if (nk_button_label(ctx, "Applications")) {
			if (current_view == VIEW_APP) current_view = VIEW_NONE;
			else current_view = VIEW_APP;
		}
		if (nk_button_label(ctx, "Procceses")) {
			if (current_view == VIEW_PROCESS) current_view = VIEW_NONE;
			else current_view = VIEW_PROCESS;
		}
		if (nk_button_label(ctx, "Files")) {
			if (current_view == VIEW_DIRECTORY) current_view = VIEW_NONE;
			else current_view = VIEW_DIRECTORY;
		}
		if (nk_button_label(ctx, "Settings")) {
			if (current_view == VIEW_SETTINGS) current_view = VIEW_NONE;
			else current_view = VIEW_SETTINGS;
		}
	}
	nk_end(ctx);

	switch (current_view) {
		case VIEW_APP:
			ProcessesView(ctx, 1);
			break;
		case VIEW_PROCESS:
			ProcessesView(ctx, 0);
			break;
		case VIEW_DIRECTORY:
			from_path == "" ? DirectoryView(ctx) : FileOperationPopup(ctx);
			break;
		case VIEW_SETTINGS:
			SettingsView(ctx);
			break;
	}

	//Clear dropped queue
	if (IsFileDropped()) {
		FilePathList files_dropped = LoadDroppedFiles();
		UnloadDroppedFiles(files_dropped);
	}
}

void UpdateFrame() {
	NuklearView(ctx);
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
		last_click = GetTime();
	key_visualizer.update();

	if (screen_client.isConnected()) {
		if (screen_client.isFrameChanged()) {
			screen_client.getMouseInfo(&mouse_x, &mouse_y);
			if (screen_client.isMouseImgChanged()) {
				unsigned char *mouse_data = screen_client.getMouseData(&mouse_width, &mouse_height);
				if (mouse_width != mouse_texture.width || mouse_height != mouse_texture.height) {
					//Size changed, create new texture
					UnloadTexture(mouse_texture);
					Image mouse_image = GenImageColor(mouse_width, mouse_height, BLANK);
					mouse_texture = LoadTextureFromImage(mouse_image);
					UnloadImage(mouse_image);
					SetTextureFilter(mouse_texture, TEXTURE_FILTER_BILINEAR);
					SetTextureWrap(mouse_texture, TEXTURE_WRAP_CLAMP);
				}
				UpdateTexture(mouse_texture, mouse_data);
			}
			unsigned char *screen_data = screen_client.getScreenData();
			UpdateTexture(screen_texture, screen_data);
			key_visualizer.setKeys(screen_client.getKeys());
		}
	}
	//Get mouse location and whether mouse image has changed

	if (IsWindowResized()) {
		//Calculate new zoom so the screen view is contained and centered by the window
		if ((GetScreenWidth()) / (float) (GetScreenHeight() - PANEL_SIZE) < screen_texture.width / (float)screen_texture.height)
			camera.zoom = (float) (GetScreenWidth()) / screen_texture.width;
		else
			camera.zoom = (float) (GetScreenHeight() - PANEL_SIZE) / screen_texture.height;
		camera.offset.x = ((GetScreenWidth()) - camera.zoom * screen_texture.width) / 2;
		camera.offset.y = ((GetScreenHeight() - PANEL_SIZE) - camera.zoom * screen_texture.height) / 2 + PANEL_SIZE;
	}

	BeginDrawing();
		ClearBackground(GRAY);
		BeginMode2D(camera);
			BeginShaderMode(shader);
				DrawTexture(screen_texture, 0, 0, WHITE);
				DrawTexture(mouse_texture, mouse_x, mouse_y, WHITE);
			EndShaderMode();
		EndMode2D();
		key_visualizer.draw();
		DrawNuklear(ctx);
		//DrawFPS(10, GetScreenHeight() - 20);
	EndDrawing();
}

int main(void) {
	std::thread socket_thread(IoContextPoll);

	IoContextRun();
	multicast_client.connect();
	multicast_client.doReceive();
	//Raylib Window Creation
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
	SetTargetFPS(60);
	InitWindow(500, 300, "3ChangDev");
	SetWindowMinSize(500, 300);
	SetExitKey(0); //Prevent app from closing when pressing ESC key

	//Nuklear GUI Init
	const int fontSize = 16;
	Font font = LoadFontEx("resource/Roboto-Medium.ttf", fontSize, NULL, 0);
	ctx = InitNuklearEx(font, fontSize);
	key_visualizer.setFont(font, fontSize);
	pause_img = LoadNuklearImage("resource/pause.png");
	play_img = LoadNuklearImage("resource/play.png");
	stop_img = LoadNuklearImage("resource/stop.png");
	file_img = LoadNuklearImage("resource/file.png");
	folder_img = LoadNuklearImage("resource/folder.png");
	parent_img = LoadNuklearImage("resource/parent.png");
	drive_img = LoadNuklearImage("resource/drive.png");
	open_img = LoadNuklearImage("resource/open.png");
	copy_img = LoadNuklearImage("resource/copy.png");
	move_img = LoadNuklearImage("resource/move.png");
	delete_img = LoadNuklearImage("resource/delete.png");

	//Shader to flip RGB channel
	shader = LoadShaderFromMemory(NULL, fragment_shader);

	while (!WindowShouldClose()) {
		UpdateFrame();
	}
	//Stop socket listening and thread
	IoContextStop();
	socket_thread.detach();
	//Free resources
	UnloadNuklear(ctx);
	UnloadFont(font);
	UnloadNuklearImage(pause_img);
	UnloadNuklearImage(play_img);
	UnloadNuklearImage(stop_img);
	UnloadNuklearImage(file_img);
	UnloadNuklearImage(folder_img);
	UnloadNuklearImage(parent_img);
	UnloadNuklearImage(drive_img);
	UnloadNuklearImage(open_img);
	UnloadNuklearImage(copy_img);
	UnloadNuklearImage(move_img);
	UnloadNuklearImage(delete_img);
	UnloadTexture(screen_texture);
	UnloadTexture(mouse_texture);
	CloseWindow();
	return 0;
}