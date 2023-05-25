#include "raylib.h"
#include "key_names.h"
#include <deque>
#include <vector>
#include <string>
#include <iostream>

struct ShiftInfo {
	unsigned char key;
	char lower;
	char upper;
};

const ShiftInfo shiftTable[] = {
	{32, ' ', ' '},

	{48, '0', ')'},
	{49, '1', '!'},
	{50, '2', '@'},
	{51, '3', '#'},
	{52, '4', '$'},
	{53, '5', '%'},
	{54, '6', '^'},
	{55, '7', '&'},
	{56, '8', '*'},
	{57, '9', '('},

	{65, 'a', 'A'},
	{66, 'b', 'B'},
	{67, 'c', 'C'},
	{68, 'd', 'D'},
	{69, 'e', 'E'},
	{70, 'f', 'F'},
	{71, 'g', 'G'},
	{72, 'h', 'H'},
	{73, 'i', 'I'},
	{74, 'j', 'J'},
	{75, 'k', 'K'},
	{76, 'l', 'L'},
	{77, 'm', 'M'},
	{78, 'n', 'N'},
	{79, 'o', 'O'},
	{80, 'p', 'P'},
	{81, 'q', 'Q'},
	{82, 'r', 'R'},
	{83, 's', 'S'},
	{84, 't', 'T'},
	{85, 'u', 'U'},
	{86, 'v', 'V'},
	{87, 'w', 'W'},
	{88, 'x', 'X'},
	{89, 'y', 'Y'},
	{90, 'z', 'Z'},

	{96, '0', '0'},
	{97, '1', '1'},
	{98, '2', '2'},
	{99, '3', '3'},
	{100, '4', '4'},
	{101, '5', '5'},
	{102, '6', '6'},
	{103, '7', '7'},
	{104, '8', '8'},
	{105, '9', '9'},
	{106, '*', '*'},
	{107, '+', '+'},
	{108, ',', ','},
	{109, '-', '-'},
	{110, '.', '.'},
	{111, '/', '/'},

	{186, ';', ':'},
	{187, '=', '+'},
	{188, ',', '<'},
	{189, '-', '_'},
	{190, '.', '>'},
	{191, '/', '?'},
	{192, '`', '~'},

	{219, '[', '{'},
	{220, '\\', '|'},
	{221, ']', '}'},
	{222, '\'', '\"'}
};

const int shiftTableSize = sizeof(shiftTable) / sizeof(ShiftInfo);

class KeyVisualizer {
private:
	struct KeyEntry {
		unsigned char key;
		double time;
		bool is_ascii;
		bool shift;
	};
	std::deque<KeyEntry> key_entries;

	double visible_time = 5; //seconds
	float box_padding = 10;
	float lines_padding = 5;
	Font font = {0};
	int font_size = 0;

	bool isAscii(unsigned char key) {
		for (int i = 0; i < shiftTableSize; i++)
			if (key == shiftTable[i].key) return 1;
		return 0;
	}

	//Fun name
	char makeShift(KeyEntry entry) {
		for (int i = 0; i < shiftTableSize; i++)
			if (entry.key == shiftTable[i].key)
				return entry.shift ? shiftTable[i].upper : shiftTable[i].lower;
		return entry.key - 128;	
	}
	
public:
	void setKeys(const std::vector<unsigned char> &keys_pressed) {
		double time = GetTime();
		bool shift = 0;
		for (auto &key: keys_pressed)
			if (key == 16) shift = 1;

		for (auto &key: keys_pressed) {
			if (key == 16 || key == 17 || key == 18) continue; //skip non-sided shift, ctrl and alt
			key_entries.push_back({key, time, isAscii(key), shift});
		}
	}

	void update() {
		double time = GetTime();
		while (!key_entries.empty()) {
			KeyEntry key = key_entries.front();
			if (time - key.time >= visible_time)
				key_entries.pop_front();
			else //skip newer entries
				break;
		}
	}

	void setFont(Font fnt, int fnt_size) {
		font = fnt;
		font_size = fnt_size;
	}

	void draw() {
		bool ascii_flag = 0;
			
		std::vector<std::string> lines;
		for (auto &entry: key_entries) {
			if (entry.is_ascii) {
				if (!ascii_flag) { //previous character is not ascii
					lines.push_back(std::string(1, makeShift(entry)));
				}
				else { //previous character is ascii
					lines.back() += makeShift(entry);
				}
				ascii_flag = 1;
			}
			else {
				lines.push_back(std::string(key_names[entry.key]));
				ascii_flag = 0;
			}
		}

		int line_box_size = font_size + box_padding * 2;
		int y = GetScreenHeight() - (line_box_size + lines_padding) * lines.size();
		for (auto &line: lines) {
			float width = MeasureTextEx(font, line.c_str(), font_size, font_size / 10.0).x;
			DrawRectangle(lines_padding, y, width + box_padding * 2, line_box_size, {0,0,0,128});
			DrawTextEx(font, line.c_str(), {lines_padding + box_padding, y + box_padding}, font_size, font_size / 10.0, WHITE);
			y += line_box_size + lines_padding;
		}
	}

};