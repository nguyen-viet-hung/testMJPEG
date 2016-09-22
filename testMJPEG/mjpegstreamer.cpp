#include "mjpegstreamer.h"

MJPEGStreamer::MJPEGBuffer::MJPEGBuffer()
{
	mBufferSize = 1000 * 1000;
	mBuffer = new unsigned char[mBufferSize];
	mIndex = 0;
}

MJPEGStreamer::MJPEGBuffer::~MJPEGBuffer()
{
	if (mBuffer) {
		delete[] mBuffer;
		mBuffer = NULL;
	}

	mBufferSize = mIndex = 0;
}

int MJPEGStreamer::MJPEGBuffer::pushData(void* ptr, size_t size)
{
	if (mIndex + size > mBufferSize) {
		size_t tmpSize = ((mIndex + size) / 1000) * 1000;
		unsigned char* newBuf = new unsigned char[tmpSize];
		if (!newBuf) {
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

unsigned char* MJPEGStreamer::MJPEGBuffer::foundStartJPEG(unsigned char* from)
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

unsigned char* MJPEGStreamer::MJPEGBuffer::foundStopJPEG(unsigned char* from)
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

void MJPEGStreamer::MJPEGBuffer::cleanUp(const unsigned char* toPrt)
{
	size_t skip = toPrt - mBuffer;
	if (skip > mIndex) // invalid from pointer
		return;

	size_t move_range = mIndex - skip;

	memmove(mBuffer, toPrt, move_range);
	mIndex = move_range;
}

size_t MJPEGStreamer::onCURLData(void* ptr, size_t size, size_t nmemb, void* data)
{
	MJPEGStreamer* pThis = (MJPEGStreamer*)data;

	size_t realsize = size * nmemb;
	pThis->mBuffer.pushData(ptr, realsize);

	unsigned char* start, *stop;
	start = stop = NULL;
	do {
		start = pThis->mBuffer.foundStartJPEG(start);
		stop = pThis->mBuffer.foundStopJPEG(start);

		if (start && stop) {
			pThis->onJPEGImage(start, stop - start);
			pThis->mBuffer.cleanUp(stop);
			start = NULL;
		}
	} while (start && stop);

	return realsize;
}

int MJPEGStreamer::onProgress(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
	MJPEGStreamer* pThis = (MJPEGStreamer*)p;
	if (!pThis->mRunning)
		return 1; //break;
	return 0;
}

void MJPEGStreamer::performThread(void* data)
{
	MJPEGStreamer* pThis = (MJPEGStreamer*)data;
	if (pThis->mHandle) {
		pThis->mRes = curl_easy_perform(pThis->mHandle);
	}
}

int MJPEGStreamer::start()
{
	mHandle = curl_easy_init();
	if (mHandle) {
		mRes = curl_easy_setopt(mHandle, CURLOPT_URL, mURL.c_str());

		if (mUser.size() && mPassword.size()) {
			std::string user_pass = mUser + ":" + mPassword;
			mRes = curl_easy_setopt(mHandle, CURLOPT_USERPWD, user_pass.c_str());
			mRes = curl_easy_setopt(mHandle, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY);
		}

		mRes = curl_easy_setopt(mHandle, CURLOPT_VERBOSE, 1L);
		mRes = curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, onCURLData);
		mRes = curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, (void*)this);
		mRes = curl_easy_setopt(mHandle, CURLOPT_TCP_KEEPALIVE, 1L);
		mRes = curl_easy_setopt(mHandle, CURLOPT_XFERINFOFUNCTION, onProgress);
		mRes = curl_easy_setopt(mHandle, CURLOPT_XFERINFODATA, this);
		mRes = curl_easy_setopt(mHandle, CURLOPT_NOPROGRESS, 0L);
		mRunning = true;
		mThreadHandle = new std::thread(performThread, this);
	}
	else {
		return -1;
	}

	return 0;
	
}

int MJPEGStreamer::stop()
{
	if (mHandle) {

		if (mThreadHandle) {
			mRunning = false;
			mThreadHandle->join();
			delete mThreadHandle;
			mThreadHandle = NULL;
		}

		curl_easy_cleanup(mHandle);
		mHandle = NULL;
	}

	return 0;
}