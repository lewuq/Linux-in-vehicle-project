/* Copyright © Deng Zhimao Co., Ltd. 2021-2030. All rights reserved.
 * Original project: video_server, 2021-11-19; www.openedv.com.
 * Adapted: bounded polling, negotiated buffers, RAII teardown and frame backpressure. */
#include "capture_thread.h"
#include "Core/appconfig.h"
#ifdef Q_OS_LINUX
#include <cerrno>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>
namespace {
struct Mapping {
    void *data = MAP_FAILED;
    size_t size = 0;
};
struct CameraDevice {
    int fd = -1;
    bool streaming = false;
    std::vector<Mapping> buffers;
    ~CameraDevice() {
        if (fd < 0)
            return;
        if (streaming) {
            v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
        }
        for (const auto &buffer : buffers)
            if (buffer.data != MAP_FAILED)
                munmap(buffer.data, buffer.size);
        close(fd);
    }
};
} // namespace
#endif
void CaptureThread::run() {
    framePending = false;
#ifdef Q_OS_LINUX
    if (!AppConfig::hardwareEnabled()) {
        emit captureError(QStringLiteral("摄像头未连接"));
        return;
    }
    CameraDevice camera;
    camera.fd =
        open(AppConfig::value("hardware/camera_device", "/dev/video1").toLocal8Bit().constData(),
             O_RDWR | O_NONBLOCK);
    if (camera.fd < 0) {
        emit captureError(QStringLiteral("无法打开摄像头，请检查连接与权限"));
        return;
    }
    v4l2_format format{};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
    format.fmt.pix.field = V4L2_FIELD_ANY;
    if (ioctl(camera.fd, VIDIOC_S_FMT, &format) < 0 ||
        format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
        emit captureError(QStringLiteral("摄像头不支持 RGB565 格式"));
        return;
    }
    v4l2_requestbuffers request{};
    request.count = 3;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    if (ioctl(camera.fd, VIDIOC_REQBUFS, &request) < 0 || request.count < 2 || request.count > 32) {
        emit captureError(QStringLiteral("摄像头缓冲区申请失败"));
        return;
    }
    camera.buffers.resize(request.count);
    for (unsigned int i = 0; i < request.count; ++i) {
        v4l2_buffer buffer{};
        buffer.type = request.type;
        buffer.memory = request.memory;
        buffer.index = i;
        if (ioctl(camera.fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            emit captureError(QStringLiteral("摄像头缓冲区读取失败"));
            return;
        }
        auto &mapping = camera.buffers[i];
        mapping.size = buffer.length;
        mapping.data = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, camera.fd,
                            buffer.m.offset);
        if (mapping.data == MAP_FAILED || ioctl(camera.fd, VIDIOC_QBUF, &buffer) < 0) {
            emit captureError(QStringLiteral("摄像头缓冲区初始化失败"));
            return;
        }
    }
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(camera.fd, VIDIOC_STREAMON, &type) < 0) {
        emit captureError(QStringLiteral("摄像头启动失败"));
        return;
    }
    camera.streaming = true;
    while (!isInterruptionRequested()) {
        pollfd descriptor{camera.fd, POLLIN, 0};
        const int ready = poll(&descriptor, 1, 200);
        if (ready < 0 && errno == EINTR)
            continue;
        if (ready < 0 || (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))) {
            emit captureError(QStringLiteral("摄像头连接已中断"));
            break;
        }
        if (!ready)
            continue;
        v4l2_buffer buffer{};
        buffer.type = type;
        buffer.memory = V4L2_MEMORY_MMAP;
        if (ioctl(camera.fd, VIDIOC_DQBUF, &buffer) < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            emit captureError(QStringLiteral("摄像头取帧失败"));
            break;
        }
        if (buffer.index >= camera.buffers.size()) {
            emit captureError(QStringLiteral("摄像头缓冲区索引异常"));
            break;
        }
        const auto &mapping = camera.buffers[buffer.index];
        const unsigned int stride =
            format.fmt.pix.bytesperline ? format.fmt.pix.bytesperline : format.fmt.pix.width * 2;
        if (stride < format.fmt.pix.width * 2 ||
            size_t(stride) * format.fmt.pix.height > mapping.size) {
            emit captureError(QStringLiteral("摄像头帧尺寸异常"));
            break;
        }
        if (!framePending.exchange(true)) {
            QImage image(static_cast<const uchar *>(mapping.data), int(format.fmt.pix.width),
                         int(format.fmt.pix.height), int(stride), QImage::Format_RGB16);
            emit imageReady(image.copy());
        }
        if (ioctl(camera.fd, VIDIOC_QBUF, &buffer) < 0) {
            emit captureError(QStringLiteral("摄像头回收帧失败"));
            break;
        }
    }
#else
    emit captureError(QStringLiteral("当前平台未连接 V4L2 摄像头"));
#endif
}
