#ifndef LIBTORREST_H
#define LIBTORREST_H

#include <stddef.h>
#include <stdint.h>

struct String {
    const char *ptr;
    size_t size;
};

typedef void (*log_callback_fn)(int, String);

#ifdef __cplusplus
extern "C"
{
#endif

int start_with_env();
int start(uint16_t port, String settings_path, int global_log_level);
void stop();
void clear_logging_sinks();
void add_logging_stdout_sink();
void add_logging_file_sink(String file_path, bool truncate);
void add_logging_callback_sink(log_callback_fn callback);

#ifdef __cplusplus
}
#endif

#endif //LIBTORREST_H
