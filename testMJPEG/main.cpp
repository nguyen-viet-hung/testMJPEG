#include <iostream>
#include <conio.h>
#include "mjpegstreamer.h"

#pragma warning(disable:4996)

class test : public MJPEGStreamer
{
public:
	test() {}
	virtual ~test() {}
	virtual void onJPEGImage(const unsigned char* buff, size_t size);
};

DWORD dwStart;
size_t fps = 0;

void test::onJPEGImage(const unsigned char* buff, size_t size) {
	fps++;
	if (GetTickCount() - dwStart >= 1000) {
		dwStart = GetTickCount();
		std::cout << "Get img fps = '" << fps << "'" << std::endl;
		fps = 0;
	}
}

int main(int argc, char** argv) {

	test buffer;
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

	//buffer.setURL("http://192.168.61.151:9901/video.mjpeg");
	buffer.setURL("http://root:elcom@192.168.61.60/axis-cgi/mjpg/video.cgi");
	buffer.start();
	int c = 0;
	do {
		if (kbhit()) {
			c = getch();
		}
	} while (c != 27);

	buffer.stop();

	curl_global_cleanup();
	return 0;
}