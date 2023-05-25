#include <iostream>
#include <string>
#include <vector>
#include <asio.hpp>
#include <Windows.h>
#include <atomic>
#include "ScreenCapture.h"
#include "screen_server.h"
#include "control_server.h"
#include "multicast_server.h"
#include "input.h"
using asio::ip::tcp;

void IoContextRun(asio::io_context &io_context) {
	io_context.run();
}

int main() {
	auto mons = SL::Screen_Capture::GetMonitors();
	mons.resize(1);
	auto monitor = mons[0];
	std::atomic<int> screen_buffer_size = monitor.Width * monitor.Height * sizeof(SL::Screen_Capture::ImageBGRA);
	std::unique_ptr<unsigned char[]> screen_buffer(std::make_unique<unsigned char[]>(screen_buffer_size));
	memset(screen_buffer.get(), 0, screen_buffer_size); // create a black image to start with
	std::atomic<bool> screen_changed = false;

	std::atomic<int> mouse_buffer_size = 32 * 32 * 4;
	unsigned char *mouse_buffer = nullptr;
	std::atomic<bool> mouse_changed = false;

	std::atomic<int> mouse_x = 0;
	std::atomic<int> mouse_y = 0;
	std::atomic<int> mouse_width = 0;
	std::atomic<int> mouse_height = 0;
	std::atomic<int> mouse_center_x = 0;
	std::atomic<int> mouse_center_y = 0;

	auto framgrabber = SL::Screen_Capture::CreateCaptureConfiguration([&]() { return mons; })
		->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
			screen_changed = true;
			memcpy(screen_buffer.get(), StartSrc(img), screen_buffer_size);
			
		})
		->onMouseChanged([&](const SL::Screen_Capture::Image* img, const SL::Screen_Capture::MousePoint &mousepoint) {
			mouse_x = mousepoint.Position.x;
			mouse_y = mousepoint.Position.y;
			if (img) {
				mouse_changed = true;
				mouse_width = Width(*img);
				mouse_height = Height(*img);
				mouse_center_x = mousepoint.HotSpot.x;
				mouse_center_y = mousepoint.HotSpot.y;
				mouse_buffer_size = mouse_width * mouse_height * 4;
				mouse_buffer = (unsigned char*)realloc(mouse_buffer, mouse_buffer_size);
				memcpy(mouse_buffer, StartSrc(*img), mouse_buffer_size);
			}
		})
		->start_capturing();
	framgrabber->setFrameChangeInterval(std::chrono::milliseconds(SCREEN_CAPTURE_INTERVAL_MILLISECONDS));
	framgrabber->setMouseChangeInterval(std::chrono::milliseconds(SCREEN_CAPTURE_INTERVAL_MILLISECONDS));
	try {
		asio::io_context io_context;

		tcp::resolver resolver(io_context);
		tcp::resolver::query query(asio::ip::host_name(), "");
		tcp::resolver::iterator it=resolver.resolve(query);

		std::vector<std::string> local_ips;
		//List server's ip addresses
		while (it != tcp::resolver::iterator()) {
			asio::ip::address addr=(it++)->endpoint().address();
			if (addr.is_v4())
				local_ips.push_back(addr.to_string());
		}
		if (local_ips.size()) {
			std::cout << "The server is hosted on the following ";
			if (local_ips.size() > 1) std::cout << "IPs: " << std::endl;
			else std::cout << "IP: " << std::endl;
			for (auto &ip: local_ips)
				std::cout << ip << std::endl;
		}
		else {
			std::cout << "Error! Can't find your machine IP" << std::endl;
			return -2;
		}

		std::vector<unsigned char> keys_pressed;

		ScreenServer screen_server(io_context, {monitor.Width, monitor.Height}, [&](bool init) {
			FrameData frDt = {0};
			if (init) keys_pressed.clear();
			frDt.bd.mouse_x = mouse_x;
			frDt.bd.mouse_y = mouse_y;
			frDt.bd.mouse_changed = mouse_changed;
			frDt.bd.mouse_width = mouse_width;
			frDt.bd.mouse_height = mouse_height;
			frDt.bd.mouse_center_x = mouse_center_x;
			frDt.bd.mouse_center_y = mouse_center_y;
			frDt.bd.mouse_size = mouse_buffer_size;
			frDt.bd.screen_changed = screen_changed;
			frDt.bd.screen_size = screen_buffer_size;
			screen_changed = false;
			
			frDt.mouse_data = mouse_buffer;
			frDt.screen_data = screen_buffer.get();
			if (frDt.bd.screen_changed) {
				mouse_changed = false;
				frDt.keys_pressed = keys_pressed;
				frDt.bd.num_keys_pressed = frDt.keys_pressed.size();
				keys_pressed.clear();
			}
			return frDt;
		});
		ControlServer control_server(io_context, [&]() {

		});

		MulticastServer multicast_server(io_context, asio::ip::make_address(MULTICAST_ADDRESS), local_ips);

		std::thread socket_thread(IoContextRun, std::ref(io_context));
		
		while (1) {
			for (int i = 1; i <= 254; i++) {
				if (GetKeySinceLastCall(i))
					keys_pressed.push_back(i);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(SCREEN_CAPTURE_INTERVAL_MILLISECONDS));
		}

	}
	catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	free(mouse_buffer);

	return 0;
}