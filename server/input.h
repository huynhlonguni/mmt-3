#include <Windows.h>

bool GetKeySinceLastCall(int key) {
	static bool key_last_called[256] = {0}; 

	bool key_pressed = GetAsyncKeyState(key) & 0x8000;  // Most significant bit

	if (key_pressed && key_pressed != key_last_called[key]) {
		key_last_called[key] = key_pressed;
		return 1;
	}
	key_last_called[key] = key_pressed;
	return 0;
}
