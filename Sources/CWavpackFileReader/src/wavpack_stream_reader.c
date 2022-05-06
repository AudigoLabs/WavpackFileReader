
#include "wavpack_file.h"
//#include "wav_file_types.h"
//
//#include <Accelerate/Accelerate.h>
//#include <errno.h>
//#include <inttypes.h>
//#include <stdbool.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//
//#define MIN_NUM_CHANNELS        1
//#define MAX_NUM_CHANNELS        2
//#define MAX_BYTES_PER_SAMPLE    4
//#define READ_BUFFER_SIZE_FRAMES 1024
//
//#define _STR1(S) #S
//#define _STR2(S) _STR1(S)
//#define _LOG_LOCATION __FILE__ ":" _STR2(__LINE__)
//#define LOG_ERROR(FMT, ...) printf("ERROR [" _LOG_LOCATION "]" FMT "\n", ##__VA_ARGS__)
//


#include "lib/wavpack.h"

#include <stdio.h>
#include <stdlib.h>

#define _STR1(S) #S
#define _STR2(S) _STR1(S)
#define _LOG_LOCATION __FILE__ ":" _STR2(__LINE__)
#define LOG_ERROR(FMT, ...) printf("ERROR [" _LOG_LOCATION "]" FMT "\n", ##__VA_ARGS__)

static int32_t wavpack_read_bytes(void* id, void *data, int32_t bcount) {
    FILE* file = (FILE*)id;
    return (int32_t)fread(data, 1, bcount, file);
}

static int64_t wavpack_get_pos(void* id) {
    FILE* file = (FILE*)id;
    return ftell(file);
}
static int wavpack_set_pos_abs(void* id, int64_t pos) {
    FILE* file = (FILE*)id;
    fseek(file, pos, SEEK_SET);
    return 0;
}
static int wavpack_set_pos_rel(void* id, int64_t delta, int mode) {
    FILE* file = (FILE*)id;
    fseek(file, delta, mode);
    return 0;
}
static int wavpack_push_back_byte(void* id, int c) {
    FILE* file = (FILE*)id;
    fseek(file, -1, SEEK_CUR);
    return c;
}
static int64_t wavpack_get_length(void* id) {
    FILE* file = (FILE*)id;
    const long prevOffset = ftell(file);
    fseek(file, 0, SEEK_END);
    const int64_t length = ftell(file);
    fseek(file, prevOffset, SEEK_SET);
    return length;
}
static int wavpack_can_seek(void* id) {
    return 1;
}
static int wavpack_close(void* id) {
    FILE* file = (FILE*)id;
    fclose(file);
    return 0;
}

static WavpackStreamReader64 reader = {
    .read_bytes = wavpack_read_bytes,
    .write_bytes = NULL,
    .get_pos = wavpack_get_pos,
    .set_pos_abs = wavpack_set_pos_abs,
    .set_pos_rel = wavpack_set_pos_rel,
    .push_back_byte = wavpack_push_back_byte,
    .get_length = wavpack_get_length,
    .can_seek = wavpack_can_seek,
    .truncate_here = NULL,
    .close = wavpack_close,
};

WavpackStreamReader64* wavpack_stream_reader_get(void) {
    return &reader;
}
