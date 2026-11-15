#include "stdint.h"
#include "config.h"
#include "hw.h"
#include "log.h"

#if SERIAL_PRINT
#include "uart.h"

static enum log_level min_level = LOG_INFO;

static const char *const level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"
};

static void log_msg(enum log_level level, const char *msg, int newline)
{
    if (level < min_level)
        return;
    uart_send_str("[");
    uart_send_uint64(get_date_ms());
    uart_send_str("][");
    uart_send_str(level_names[level]);
    uart_send_str("] ");
    uart_send_str(msg);
    if (newline) {
        uart_send_str("\r\n");
    }
}
#else
static enum log_level min_level = LOG_INFO;
static void log_msg(enum log_level level, const char *msg, int newline)
{
    (void)level; (void)msg; (void)newline;
}
#endif

void log_set_level(enum log_level level) { min_level = level; }

#define DEFINE_LOG(name, level) \
    void log_##name(const char *msg) { log_msg(level, msg, 0); } \
    void log_##name##_ln(const char *msg) { log_msg(level, msg, 1); }

DEFINE_LOG(trace, LOG_TRACE)
DEFINE_LOG(debug, LOG_DEBUG)
DEFINE_LOG(info, LOG_INFO)
DEFINE_LOG(warn, LOG_WARN)
DEFINE_LOG(error, LOG_ERROR)
DEFINE_LOG(critical, LOG_CRITICAL)
