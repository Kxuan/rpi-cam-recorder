
#include "V4l2Device.h"
#include <unistd.h>
#include <fcntl.h>
#include <system_error>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>

using namespace std;

FrameBuffer::FrameBuffer(int fd, int index)
		: index(index)
{
	struct v4l2_buffer buf = {0};

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;

	if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
		throw system_error(errno, system_category(), "VIDIOC_QUERYBUF");

	size = buf.length;
	ptr = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

	if (MAP_FAILED == ptr) {
		throw system_error(errno, system_category(), "mmap");
	}
}

FrameBuffer::~FrameBuffer()
{
	munmap(ptr, size);
}

V4l2Device::V4l2Device(const char *devname)
		: devname(devname)
{
	if ((fd = open(devname, O_RDWR)) < 0) {
		throw system_error(errno, system_category(), "open");
	}

	struct v4l2_format fmt;

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	/* Preserve original settings as set by v4l2-ctl for example */
	if (-1 == ioctl(fd, VIDIOC_G_FMT, &fmt)) {
		throw system_error(errno, system_category(), "VIDIOC_G_FMT");
	}

	struct v4l2_requestbuffers req = {0};

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req)) {
		throw system_error(errno, system_category(), "VIDIOC_REQBUFS");
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on device\n");
		exit(EXIT_FAILURE);
	}

	fb.reserve(req.count);
	for (int i = 0; i < req.count; ++i) {
		fb.emplace_back(fd, i);
	}
}

V4l2Device::~V4l2Device()
{
	stop();
	fb.clear();
	close(fd);
}

void V4l2Device::start()
{
	if (started) {
		throw std::logic_error("Already started");
	}

	vbufs.assign(fb.size(), v4l2_buffer{});

	for (int i = 0; i < fb.size(); ++i) {
		struct v4l2_buffer &buf = vbufs[i];

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
			throw system_error(errno, system_category(), "VIDIOC_QBUF");
		pvbufs.push(&buf);
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
		throw system_error(errno, system_category(), "VIDIOC_STREAMON");
	started = true;
}

void V4l2Device::stop()
{
	if (!started) {
		return;
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
		throw system_error(errno, system_category(), "VIDIOC_STREAMOFF");
	std::queue<struct v4l2_buffer *> empty;
	pvbufs.swap(empty);
}

FrameBuffer &V4l2Device::deque()
{
	if (!started) {
		throw std::logic_error("Device has not been started");
	}

	if (pvbufs.empty()) {
		throw std::logic_error("All buffers are in use.");
	}
	struct v4l2_buffer *buf = pvbufs.front();
	pvbufs.pop();
	unsigned int i;

	if (-1 == ioctl(fd, VIDIOC_DQBUF, buf)) {
		throw system_error(errno, system_category(), "VIDIOC_DQBUF");
	}

	assert(buf->index < fb.size());
	auto &frame = fb[buf->index];
	frame.vbuf = buf;

	return frame;
}

void V4l2Device::queue(FrameBuffer &frame)
{
	if (!frame.vbuf) {
		throw std::logic_error("There is no data on this frame buffer.");
	}
	if (-1 == ioctl(fd, VIDIOC_QBUF, frame.vbuf))
		throw system_error(errno, system_category(), "VIDIOC_QBUF");
	pvbufs.push(frame.vbuf);
	frame.vbuf = nullptr;
}
