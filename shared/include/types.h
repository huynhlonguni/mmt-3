#pragma once
#include <string>
#include <vector>
#include "config.h"

enum OperationCode : uint16_t {
	NONE = 0,
	//Unused
	SCREEN_CONNECT,
	FRAME_DATA,
	CONTROL_DISCONNECT,
	//Processes and Applications
	PROCESS_LIST,
	PROCESS_SUSPEND,
	PROCESS_RESUME,
	PROCESS_KILL,
	//File system
	FS_INIT,
	FS_LIST,
	FS_CHECK_EXIST,
	FS_COPY,
	FS_MOVE,
	FS_WRITE,
	FS_DELETE
};

struct ScreenInfo {
	int width;
	int height;
};

struct FrameBundle {
	int num_keys_pressed;
	int mouse_x;
	int mouse_y;
	int mouse_changed;
	int mouse_width;
	int mouse_height;
	int mouse_center_x;
	int mouse_center_y;
	int mouse_size;
	int screen_changed;
	int screen_size;
};

struct FrameData {
	FrameBundle bd;
	std::vector<unsigned char> keys_pressed;
	void *mouse_data;
	void *screen_data;
};

enum FileEntry : char {
	ENTRY_FILE,
	ENTRY_FOLDER,
	ENTRY_PARENT,
	ENTRY_DRIVE
};

struct ProcessInfo {
	int pid;
	std::string name;
	char type;
};

struct FileInfo {
	std::string name;
	FileEntry type;
};