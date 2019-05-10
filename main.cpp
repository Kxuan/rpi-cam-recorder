#include <string.h>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "V4l2Device.h"

int main(int argc, char *argv[])
{
	V4l2Device dev{"/dev/video0"};
	dev.start();
	for (int i = 0; i < 10; i++) {
		auto frame = dev.deque();
		write(STDOUT_FILENO, frame.ptr, frame.vbuf->bytesused);
		dev.queue(frame);
	}
	return 0;
}