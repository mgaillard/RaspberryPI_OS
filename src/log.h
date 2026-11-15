#ifndef LOG_H
#define LOG_H

enum log_level {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRITICAL
};

// Sets the minimum level of the messages that are logged
void log_set_level(enum log_level level);

// Log without trailing newline: "[date_ms][LEVEL] msg"
void log_trace(const char *msg);
void log_debug(const char *msg);
void log_info(const char *msg);
void log_warn(const char *msg);
void log_error(const char *msg);
void log_critical(const char *msg);

// Same as above, but append "\r\n"
void log_trace_ln(const char *msg);
void log_debug_ln(const char *msg);
void log_info_ln(const char *msg);
void log_warn_ln(const char *msg);
void log_error_ln(const char *msg);
void log_critical_ln(const char *msg);

#endif
