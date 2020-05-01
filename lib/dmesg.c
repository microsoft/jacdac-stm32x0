#include "lib.h"

#if DEVICE_DMESG_BUFFER_SIZE > 0

struct CodalLogStore codalLogStore;

static void logwrite(const char *msg);

static void logwriten(const char *msg, int l) {
    if (codalLogStore.ptr + l >= sizeof(codalLogStore.buffer)) {
#if 1
        codalLogStore.buffer[0] = '.';
        codalLogStore.buffer[1] = '.';
        codalLogStore.buffer[2] = '.';
        codalLogStore.ptr = 3;
#else
    // this messes with timings too much
        const int jump = sizeof(codalLogStore.buffer) / 4;
        codalLogStore.ptr -= jump;
        memmove(codalLogStore.buffer, codalLogStore.buffer + jump, codalLogStore.ptr);
        // zero-out the rest so it looks OK in the debugger
        memset(codalLogStore.buffer + codalLogStore.ptr, 0,
               sizeof(codalLogStore.buffer) - codalLogStore.ptr);
#endif
    }
    if (l + codalLogStore.ptr >= sizeof(codalLogStore.buffer)) {
        logwrite("DMESG line too long!\n");
        return;
    }
    memcpy(codalLogStore.buffer + codalLogStore.ptr, msg, l);
    codalLogStore.ptr += l;
    codalLogStore.buffer[codalLogStore.ptr] = 0;
}

static void logwrite(const char *msg) {
    logwriten(msg, strlen(msg));
}

static void writeNum(char *buf, uint32_t n, bool full) {
    int i = 0;
    int sh = 28;
    while (sh >= 0) {
        int d = (n >> sh) & 0xf;
        if (full || d || sh == 0 || i) {
            buf[i++] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        sh -= 4;
    }
    buf[i] = 0;
}

static void logwritenum(uint32_t n, bool full, bool hex) {
    char buff[20];

    if (hex) {
        writeNum(buff, n, full);
        logwrite("0x");
    } else {
        itoa(n, buff);
    }

    logwrite(buff);
}

void codal_dmesg(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    codal_vdmesg(format, arg);
    va_end(arg);
}

void codal_dmesgf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    codal_vdmesg(format, arg);
    va_end(arg);
    codal_dmesg_flush();
}

void codal_vdmesg(const char *format, va_list ap) {
    const char *end = format;

    target_disable_irq();
    while (*end) {
        if (*end++ == '%') {
            logwriten(format, end - format - 1);
            uint32_t val = va_arg(ap, uint32_t);
            switch (*end++) {
            case 'c':
                logwriten((const char *)&val, 1);
                break;
            case 'd':
                logwritenum(val, false, false);
                break;
            case 'x':
                logwritenum(val, false, true);
                break;
            case 'p':
            case 'X':
                logwritenum(val, true, true);
                break;
            case 's':
                logwrite((char *)(void *)val);
                break;
            case '%':
                logwrite("%");
                break;
            default:
                logwrite("???");
                break;
            }
            format = end;
        }
    }
    logwriten(format, end - format);
    logwrite("\r\n");
    target_enable_irq();
}

#endif
