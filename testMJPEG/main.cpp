extern "C" {
#include <curl.h>
}

#include <iostream>
#include <string>

#pragma warning(disable:4996)

class MJPEGBuffer {
private:
	unsigned char* mBuffer;
	size_t mIndex;
	size_t mBufferSize;
public:
	MJPEGBuffer();
	virtual ~MJPEGBuffer();
	int pushData(void* ptr, size_t size);
	unsigned char* foundStartJPEG(unsigned char* from = NULL);
	unsigned char* foundStopJPEG(unsigned char* from = NULL);
	void cleanUp(const unsigned char* toPrt);
};


MJPEGBuffer::MJPEGBuffer()
{
	mBufferSize = 1000 * 1000;
	mBuffer = new unsigned char[mBufferSize];
	mIndex = 0;
}

MJPEGBuffer::~MJPEGBuffer()
{
	if (mBuffer) {
		delete[] mBuffer;
		mBuffer = NULL;
	}

	mBufferSize = mIndex = 0;
}

int MJPEGBuffer::pushData(void* ptr, size_t size)
{
	if (mIndex + size > mBufferSize) {
		size_t tmpSize = ((mIndex + size) / 1000) * 1000;
		unsigned char* newBuf = new unsigned char[tmpSize];
		if (!newBuf) {
			std::cout << "Error in allocating new buffer" << std::endl;
			return -1;
		}

		memcpy(newBuf, mBuffer, mIndex);
		delete[] mBuffer;
		mBuffer = newBuf;
	}

	memcpy(mBuffer + mIndex, ptr, size);
	mIndex += size;

	return 0;
}

unsigned char* MJPEGBuffer::foundStartJPEG(unsigned char* from)
{
	if (!from)
		from = mBuffer;
	size_t skip = from - mBuffer;
	if (skip > mIndex) // invalid from pointer
		return NULL;

	size_t search_range = mIndex - skip;
	for (size_t i = 0; i < search_range - 1; i++) {
		if (from[i] == 0xff && from[i + 1] == 0xd8)
			return from + i;
	}
		
	return NULL;
}

unsigned char* MJPEGBuffer::foundStopJPEG(unsigned char* from)
{
	if (!from)
		from = mBuffer;
	size_t skip = from - mBuffer;
	if (skip > mIndex) // invalid from pointer
		return NULL;

	size_t search_range = mIndex - skip;
	for (size_t i = 0; i < search_range - 1; i++) {
		if (from[i] == 0xff && from[i + 1] == 0xd9)
			return from + i + 2;
	}

	return NULL;
}

void MJPEGBuffer::cleanUp(const unsigned char* toPrt)
{
	size_t skip = toPrt - mBuffer;
	if (skip > mIndex) // invalid from pointer
		return;

	size_t move_range = mIndex - skip;

	memmove(mBuffer, toPrt, move_range);
	mIndex = move_range;
}

size_t current = 0;
DWORD dwStart;
size_t fps = 0;

size_t onData(void* ptr, size_t size, size_t nmemb, void* data)
{
	size_t realsize = size * nmemb;
	MJPEGBuffer* mem = (MJPEGBuffer*)data;
	mem->pushData(ptr, realsize);
	unsigned char* start, *stop;
	start = stop = NULL;
	do {
		start = mem->foundStartJPEG(start);
		stop = mem->foundStopJPEG(start);

		if (start && stop) {
			// write to file
			std::string fileName = std::to_string(current) + std::string(".jpg");
			FILE* f = fopen(fileName.c_str(), "wb");
			if (f) {
				fwrite(start, stop - start, 1, f);
				fclose(f);
			}
			mem->cleanUp(stop);
			start = NULL;
			current++;
			fps++;
			//std::cout << "Got img with name = '" << fileName << "'" << std::endl;
			if (GetTickCount() - dwStart >= 1000) {
				dwStart = GetTickCount();
				std::cout << "Got img fps = '" << fps << "'" << std::endl;
				fps = 0;
			}
		}
	} while (start && stop);

	if (current >= 2000)
		exit(0);

	return realsize;
}

int main(int argc, char** argv) {

	MJPEGBuffer buffer;
	CURL *curl;
	CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
	if (res != CURLE_OK)
	{
		std::cout << "CURLE NOT OK," << " Reason:" << curl_easy_strerror(res) << std::endl;
		exit(0);
	}

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.61.151:9901/video.mjpeg");
		//curl_easy_setopt(curl, CURLOPT_URL, "http://root:elcom@192.168.61.60/axis-cgi/mjpg/video.cgi");
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, onData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&buffer);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

		dwStart = GetTickCount();
		res = curl_easy_perform(curl);
	}

	while (1) {
		Sleep(1);
	}
	return 0;
}