#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/input.h>
#include <string.h>
#include <errno.h>

static int scale(int v, int min, int max, int out) {
    if (max == min) return 0;
    long long num = (long long)(v - min) * out;
    return (int)(num / (max - min));
}

int main(int argc, char **argv) {
    const char *device_path = "/dev/input/event1";
    int fd = open(device_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        perror("Failed to open input device");
        return 1;
    }

    printf("Opened input device: %s (fd=%d)\n", device_path, fd);

    struct input_absinfo ax, ay;
    // if ((ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &ax) == 0)
    //     && (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &ay) == 0)) {
    //     printf("Touch X range: %d - %d\n", ax.minimum, ax.maximum);
    //     printf("Touch Y range: %d - %d\n", ay.minimum, ay.maximum);
    //     // continue;
    // }

    struct pollfd p = { .fd = fd, .events = POLLIN };
    struct input_event ev[64];
    for(;;) {
        int rc = poll(&p, 1, 2000);
        if (rc < 0) {
            perror("poll failed");
            close(fd);
            return 1;
        } else if (rc == 0) {
            continue;
        }

        ssize_t n = read(fd, ev, sizeof(ev));
        if (n < 0) {
            if (errno == EINTR) {
                continue; // Interrupted, retry
            }
            perror("Failed to read input events");
            close(fd);
            return 1;
        }

        int cnt = n / sizeof(ev[0]);
        for (int i = 0; i < cnt; i++) {
            if (ev[i].type == EV_ABS) {
                if (ev[i].code == ABS_MT_SLOT) {
                    printf("Touch slot changed: %d\n", ev[i].value);
                } else if (ev[i].code == ABS_MT_TRACKING_ID) {
                    printf("Touch tracking ID: %d\n", ev[i].value);
                } else if (ev[i].code == ABS_MT_POSITION_X) {
                    printf("Touch X position: %d\n", ev[i].value);
                } else if (ev[i].code == ABS_MT_POSITION_Y) {
                    printf("Touch Y position: %d\n", ev[i].value);
                }
            } else if (ev[i].type == EV_SYN && ev[i].code == SYN_REPORT) {
                printf("Touch report end\n");
            }
        }
    }
    close(fd);

    return 0;
}