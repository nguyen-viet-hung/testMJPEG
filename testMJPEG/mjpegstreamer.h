#ifndef MJPEGSTREAMER_H__
#define MJPEGSTREAMER_H__

#include <thread>
#include <string>
#include <curl.h>

class MJPEGStreamer
{
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

protected:
	CURL* mHandle;
	MJPEGBuffer mBuffer;
	std::string mURL;
	std::string mUser;
	std::string mPassword;
	std::thread* mThreadHandle;
	volatile bool mRunning;
	volatile bool mStarted;
	CURLcode mRes;

	static size_t onCURLData(void* ptr, size_t size, size_t nmemb, void* data);
	static int onProgress(void *p,
		curl_off_t dltotal, curl_off_t dlnow,
		curl_off_t ultotal, curl_off_t ulnow);
	static void performThread(void* data);

public:
	MJPEGStreamer() : mHandle(NULL), mThreadHandle(NULL), mRunning(false) {}
	virtual ~MJPEGStreamer() { stop(); }

	void setURL(const char* url) { mURL = url; }
	void setUser(const char* user) { mUser = user; }
	void setPassword(const char* password) { mPassword = password; }

	int start();
	int stop();

	virtual void onJPEGImage(const unsigned char* buffer, size_t size) {}
};

#endif//MJPEGSTREAMER_H__