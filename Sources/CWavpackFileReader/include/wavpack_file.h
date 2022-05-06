#pragma once

#include <inttypes.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // Success (no error)
    WAVPACK_FILE_RESULT_SUCCESS = 0,

    // Failed to open the wav file
    WAVPACK_FILE_RESULT_OPEN_FAILED,

    // The specified file was not a valid wav file or a file read/seek operation failed
    WAVPACK_FILE_RESULT_FILE_ERROR,

    // The properties (i.e. channel count) of the wav file are not (yet) supported
    WAVPACK_FILE_RESULT_UNSUPPORTED,

    // An invalid parameter was passed
    WAVPACK_FILE_RESULT_INVALID_PARAM,
} wavpack_file_result_t;

struct wavpack_file;
typedef struct wavpack_file* _Null_unspecified wavpack_file_handle_t;

wavpack_file_result_t wavpack_file_open(const char* _Nonnull wv_path, const char* _Nonnull wvc_path, wavpack_file_handle_t* _Nonnull wavpack_file_out);

uint16_t wavpack_file_get_num_channels(wavpack_file_handle_t wavpack_file);

uint32_t wavpack_file_get_sample_rate(wavpack_file_handle_t wavpack_file);

double wavpack_file_get_duration(wavpack_file_handle_t wavpack_file);

wavpack_file_result_t wavpack_file_set_seek(wavpack_file_handle_t wavpack_file, double position);

wavpack_file_result_t wavpack_file_set_offset(wavpack_file_handle_t wavpack_file, uint32_t offset);

uint32_t wavpack_file_read(wavpack_file_handle_t wavpack_file, float* const _Nonnull* _Nonnull data, uint32_t max_num_frames);

void wavpack_file_close(wavpack_file_handle_t wavpack_file);

#ifdef __cplusplus
};
#endif
