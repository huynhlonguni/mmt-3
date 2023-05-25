#include <string>
#include <vector>
#include <atomic>
#include <system_error>
#include "types.h"

class ScreenClient {
private:
	bool connected = 0;

	int connect_error = 0;

	ScreenInfo screen_info = {1080, 720};
	OperationCode opcode; //dummy for async operations

	std::vector<unsigned char> keys_pressed;

	unsigned char *screen_data = nullptr;
	std::atomic<bool> screen_changed = 0;

	unsigned char *mouse_data = nullptr;
	std::atomic<bool> mouse_changed = 0;
	std::atomic<int> mouse_x = 0;
	std::atomic<int> mouse_y = 0;
	std::atomic<int> mouse_width = 0;
	std::atomic<int> mouse_height = 0;
	std::atomic<int> mouse_center_x = 0;
	std::atomic<int> mouse_center_y = 0;

	void getData(void *data, int size);

	void handleRead(std::error_code error);
public:
	ScreenClient();

	bool connect(const char *address);

	void disconnect();

	bool isConnected();

	int getLastConnectError();

	void init();

	int getWidth();

	int getHeight();

	bool isFrameChanged();

	bool isMouseImgChanged();

	std::vector<unsigned char> getKeys();

	unsigned char* getScreenData();

	void getMouseInfo(int *x, int *y);

	unsigned char* getMouseData(int *width, int *height);

	~ScreenClient();
};

class ControlClient {
private:
	bool connected = 0;

	int connect_error = 0;

	void getData(void *data, int size);

	void getString(std::string *str);

	void sendData(void *data, int size);

	void sendString(std::string str);

	void sendControl(OperationCode opcode);
public:
	ControlClient();

	bool connect(const char *address);

	void disconnect();

	bool isConnected();

	int getLastConnectError();

	void suspendProcess(int pid);

	void resumeProcess(int pid);

	void terminateProcess(int pid);

	std::vector<ProcessInfo> getProcesses();

	std::string getDefaultLocation();

	std::vector<FileInfo> listDir(std::string path);

	bool checkExist(std::string from, std::string to);

	void requestCopy(std::string from, std::string to, bool overwrite);

	void requestMove(std::string from, std::string to, bool overwrite);

	void requestWrite(std::string path, bool overwrite, int size, void *data);

	void requestDelete(std::string path);

	~ControlClient();
};

class MulticastClient {
private:
	char data[16 * 50] = {0};

	int size = 0;

	std::vector<std::string> ipFromBytes(const char *raw_data, int size);
public:
	MulticastClient();

	void connect();

	void doReceive();

	std::vector<std::string> getAddresses();

	~MulticastClient();
};

void IoContextPoll();

void IoContextRun();

void IoContextStop();