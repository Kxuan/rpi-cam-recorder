//
// Created by xuan on 5/10/19.
//

#ifndef V4L2_V4L2DEVICE_H
#define V4L2_V4L2DEVICE_H

#include <vector>
#include <queue>
#include <sys/types.h>

class FrameBuffer
{
public:
	FrameBuffer(int fd, int index);

	~FrameBuffer();

	int index;
	ssize_t size;
	struct v4l2_buffer *vbuf = nullptr;
	void *ptr;
};

class V4l2Device
{
public:
	V4l2Device(const char *devname);

	~V4l2Device();

	void start();

	void stop();

	FrameBuffer &deque();

	void queue(FrameBuffer &frame);

private:
	bool started = false;
	std::vector<FrameBuffer> fb;
	std::vector<struct v4l2_buffer> vbufs;
	std::queue<struct v4l2_buffer *> pvbufs;

	int fd;
	const char *devname;
};


#endif //V4L2_V4L2DEVICE_H
