#define _DEFAULT_SOURCE
#include "gps.h"
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int coordinate(const char *text, const char *hemisphere, int latitude, double *out) {
    char *end;
    double raw = strtod(text, &end), degrees = floor(raw / 100.0), minutes = raw - degrees * 100.0;
    double limit = latitude ? 90.0 : 180.0;
    if (!*text || *end || !isfinite(raw) || raw < 0 || minutes >= 60 || degrees > limit ||
        (degrees == limit && minutes != 0) || strlen(hemisphere) != 1)
        return 0;
    if (latitude ? (*hemisphere != 'N' && *hemisphere != 'S')
                 : (*hemisphere != 'E' && *hemisphere != 'W'))
        return 0;
    *out = degrees + minutes / 60.0;
    if (*hemisphere == 'S' || *hemisphere == 'W')
        *out = -*out;
    return 1;
}
int gps_parse_sentence(const char *sentence, GpsFix *fix) {
    char line[128], *fields[24], *cursor, *star;
    unsigned int checksum = 0;
    size_t length, count = 0, i;
    GpsFix parsed = {0};
    if (!sentence || !fix || sentence[0] != '$')
        return 0;
    length = strlen(sentence);
    if (length >= sizeof(line))
        return 0;
    memcpy(line, sentence, length + 1);
    star = strchr(line, '*');
    if (!star || strlen(star + 1) < 2 || !isxdigit((unsigned char)star[1]) ||
        !isxdigit((unsigned char)star[2]))
        return 0;
    for (cursor = line + 1; cursor < star; ++cursor)
        checksum ^= (unsigned char)*cursor;
    {
        char hex[3] = {star[1], star[2], 0};
        if (checksum != strtoul(hex, NULL, 16))
            return 0;
    }
    for (cursor = star + 3; *cursor; ++cursor)
        if (*cursor != '\r' && *cursor != '\n')
            return 0;
    *star = 0;
    fields[count++] = line + 1;
    for (cursor = line + 1; *cursor; ++cursor)
        if (*cursor == ',') {
            if (count >= sizeof(fields) / sizeof(fields[0]))
                return 0;
            *cursor = 0;
            fields[count++] = cursor + 1;
        }
    if (strlen(fields[0]) != 5)
        return 0;
    i = 2;
    if (strcmp(fields[0] + 2, "GGA") == 0) {
        if (count < 8)
            return 0;
        parsed.valid = atoi(fields[6]) > 0;
        parsed.satellites = atoi(fields[7]);
    } else if (strcmp(fields[0] + 2, "RMC") == 0) {
        if (count < 7)
            return 0;
        parsed.valid = strcmp(fields[2], "A") == 0;
        i = 3;
    } else
        return 0;
    if (parsed.valid && (!coordinate(fields[i], fields[i + 1], 1, &parsed.latitude) ||
                         !coordinate(fields[i + 2], fields[i + 3], 0, &parsed.longitude)))
        return 0;
    *fix = parsed;
    return 1;
}
#ifdef __linux__
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
static pthread_t worker;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int running, started, serial_fd = -1;
static GpsFix latest;
static struct timespec received_at;
static void *collect(void *unused) {
    char line[128];
    size_t used = 0;
    (void)unused;
    for (;;) {
        struct pollfd descriptor = {serial_fd, POLLIN, 0};
        char bytes[256];
        ssize_t n, j;
        int keep_running;
        pthread_mutex_lock(&lock);
        keep_running = running;
        pthread_mutex_unlock(&lock);
        if (!keep_running)
            break;
        if (poll(&descriptor, 1, 200) <= 0)
            continue;
        if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL))
            break;
        n = read(serial_fd, bytes, sizeof(bytes));
        for (j = 0; j < n; ++j) {
            char c = bytes[j];
            if (c == '$') {
                used = 0;
                line[used++] = c;
            } else if (used && c == '\n') {
                GpsFix fix;
                line[used] = 0;
                if (gps_parse_sentence(line, &fix)) {
                    pthread_mutex_lock(&lock);
                    if (!fix.satellites && fix.valid)
                        fix.satellites = latest.satellites;
                    latest = fix;
                    clock_gettime(CLOCK_MONOTONIC, &received_at);
                    pthread_mutex_unlock(&lock);
                }
                used = 0;
            } else if (used && used < sizeof(line) - 1)
                line[used++] = c;
            else
                used = 0;
        }
    }
    pthread_mutex_lock(&lock);
    latest.valid = 0;
    pthread_mutex_unlock(&lock);
    return NULL;
}
int gps_start(const char *device, unsigned int baud) {
    struct termios options;
    speed_t speed;
    if (started)
        return 0;
    switch (baud) {
    case 9600:
        speed = B9600;
        break;
    case 38400:
        speed = B38400;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        return -1;
    }
    serial_fd = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (serial_fd < 0)
        return -1;
    if (tcgetattr(serial_fd, &options) != 0) {
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }
    cfmakeraw(&options);
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    options.c_cflag |= CLOCAL | CREAD;
    if (tcsetattr(serial_fd, TCSANOW, &options) != 0) {
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }
    running = 1;
    if (pthread_create(&worker, NULL, collect, NULL) != 0) {
        running = 0;
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }
    started = 1;
    return 0;
}
void gps_stop(void) {
    if (!started)
        return;
    pthread_mutex_lock(&lock);
    running = 0;
    pthread_mutex_unlock(&lock);
    pthread_join(worker, NULL);
    close(serial_fd);
    serial_fd = -1;
    started = 0;
    pthread_mutex_lock(&lock);
    memset(&latest, 0, sizeof(latest));
    memset(&received_at, 0, sizeof(received_at));
    pthread_mutex_unlock(&lock);
}
int gps_snapshot(GpsFix *fix) {
    if (!fix)
        return 0;
    pthread_mutex_lock(&lock);
    *fix = latest;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if (now.tv_sec - received_at.tv_sec > 5)
        fix->valid = 0;
    pthread_mutex_unlock(&lock);
    return fix->valid;
}
#else
int gps_start(const char *device, unsigned int baud) {
    (void)device;
    (void)baud;
    return -1;
}
void gps_stop(void) {}
int gps_snapshot(GpsFix *fix) {
    if (fix)
        memset(fix, 0, sizeof(*fix));
    return 0;
}
#endif
