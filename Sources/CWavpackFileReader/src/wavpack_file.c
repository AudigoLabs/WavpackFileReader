#include "wavpack_file.h"
#include "wavpack_stream_reader.h"

#include <Accelerate/Accelerate.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _STR1(S) #S
#define _STR2(S) _STR1(S)
#define _LOG_LOCATION __FILE__ ":" _STR2(__LINE__)
#define LOG_ERROR(FMT, ...) printf("ERROR [" _LOG_LOCATION "]" FMT "\n", ##__VA_ARGS__)

#define UNPACK_BUFFER_SIZE  4096

struct wavpack_file {
    WavpackContext* context;
    int32_t* unpack_buffer;
};

wavpack_file_result_t wavpack_file_open(const char* wv_path, const char* wvc_path, wavpack_file_handle_t* wavpack_file_out) {
    // Open the files
    FILE* wv_file = fopen(wv_path, "r");
    if (!wv_file) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    FILE* wvc_file = NULL;
    if (wvc_path) {
        wvc_file = fopen(wvc_path, "r");
        if (!wvc_file) {
            fclose(wv_file);
            return WAVPACK_FILE_RESULT_OPEN_FAILED;
        }
    }
    char error_buffer[81];
    WavpackContext* context = WavpackOpenFileInputEx64(wavpack_stream_reader_get(), wv_file, wvc_file, error_buffer, wvc_path ? OPEN_WVC : 0, 0);
    if (!context) {
        fclose(wv_file);
        if (wvc_file) {
            fclose(wvc_file);
        }
        LOG_ERROR("Failed to open wavpack file: %s", error_buffer);
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    // Allocate a file handle
    wavpack_file_handle_t wavpack_file = malloc(sizeof(struct wavpack_file));
    memset(wavpack_file, 0, sizeof(struct wavpack_file));
    wavpack_file->context = context;
    wavpack_file->unpack_buffer = malloc(sizeof(*wavpack_file->unpack_buffer) * wavpack_file_get_num_channels(wavpack_file) * UNPACK_BUFFER_SIZE);

    // Return the handle
    *wavpack_file_out = wavpack_file;
    return WAVPACK_FILE_RESULT_SUCCESS;
}

uint16_t wavpack_file_get_num_channels(wavpack_file_handle_t wavpack_file) {
    return (uint16_t)WavpackGetNumChannels(wavpack_file->context);
}

uint32_t wavpack_file_get_sample_rate(wavpack_file_handle_t wavpack_file) {
    return WavpackGetSampleRate(wavpack_file->context);
}

double wavpack_file_get_duration(wavpack_file_handle_t wavpack_file) {
    return (double)WavpackGetNumSamples(wavpack_file->context) / wavpack_file_get_sample_rate(wavpack_file);
}

uint16_t wavpack_file_get_bits_per_sample(wavpack_file_handle_t wavpack_file) {
    return (uint16_t)WavpackGetBitsPerSample(wavpack_file->context);
}

wavpack_file_result_t wavpack_file_set_seek(wavpack_file_handle_t wavpack_file, double position) {
    if (position > wavpack_file_get_duration(wavpack_file)) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    return wavpack_file_set_offset(wavpack_file, position * wavpack_file_get_sample_rate(wavpack_file) + 0.5);
}

wavpack_file_result_t wavpack_file_set_offset(wavpack_file_handle_t wavpack_file, uint32_t offset) {
    if (offset > WavpackGetNumSamples(wavpack_file->context)) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    return WavpackSeekSample64(wavpack_file->context, offset) ? WAVPACK_FILE_RESULT_SUCCESS : WAVPACK_FILE_RESULT_FILE_ERROR;
}

uint32_t wavpack_file_read(wavpack_file_handle_t wavpack_file, float* const* data, uint32_t max_num_frames) {
    const uint16_t num_channels = wavpack_file_get_num_channels(wavpack_file);
    const float MAX_VALUE = 1 << (WavpackGetBitsPerSample(wavpack_file->context) - 1);
    uint32_t total_frames_unpacked = 0;
    while (total_frames_unpacked < max_num_frames) {
        uint32_t chunk_frames = max_num_frames - total_frames_unpacked;
        chunk_frames = chunk_frames <= UNPACK_BUFFER_SIZE ? chunk_frames : UNPACK_BUFFER_SIZE;

        // Unpack into a temp buffer
        const uint32_t num_frames_unpacked = WavpackUnpackSamples(wavpack_file->context, wavpack_file->unpack_buffer, chunk_frames);
        if (num_frames_unpacked == 0) {
            // Reached the end of the file
            break;
        }

        // Convert the samples to floats and write them into the passed buffer
        for (uint16_t channel = 0; channel < num_channels; channel++) {
            // Convert the fixed-point data into a float
            vDSP_vflt32(&wavpack_file->unpack_buffer[channel], num_channels, &data[channel][total_frames_unpacked], 1, num_frames_unpacked);
            // Scale the float data to be within the range [-1,1]
            vDSP_vsdiv(&data[channel][total_frames_unpacked], 1, &MAX_VALUE, &data[channel][total_frames_unpacked], 1, num_frames_unpacked);
        }

        total_frames_unpacked += num_frames_unpacked;
    }
    return total_frames_unpacked;
}

void wavpack_file_close(wavpack_file_handle_t wavpack_file) {
    WavpackCloseFile(wavpack_file->context);
    if (wavpack_file->unpack_buffer) {
        free(wavpack_file->unpack_buffer);
    }
    free(wavpack_file);
}

static int wavpack_write_handler(void* id, void* data, int32_t length) {
    return (int)fwrite(data, 1, length, (FILE*)id);
}

wavpack_file_result_t wavpack_file_open_for_writing(const char* wv_path, const char* wvc_path, uint8_t num_channels, uint8_t bits_per_sample, uint32_t sample_rate, wavpack_file_handle_t* wavpack_file_out) {
    // Open the files
    FILE* wv_file = fopen(wv_path, "w");
    if (!wv_file) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    FILE* wvc_file = fopen(wvc_path, "w");
    if (!wvc_file) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    WavpackContext* context = WavpackOpenFileOutput(wavpack_write_handler, wv_file, wvc_file);
    if (!context) {
        fclose(wv_file);
        fclose(wvc_file);
        LOG_ERROR("Failed to open wavpack file");
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    WavpackConfig config_wavpack = {
        .bits_per_sample = bits_per_sample,
        .bytes_per_sample = bits_per_sample / 8,
        .num_channels = num_channels,
        .sample_rate = sample_rate,
        .flags = CONFIG_FAST_FLAG | CONFIG_HYBRID_FLAG | CONFIG_CREATE_WVC | CONFIG_OPTIMIZE_WVC,
        .bitrate = 512,
        .shaping_weight = 0,
    };
    if (!WavpackSetConfiguration(context, &config_wavpack, UINT32_MAX)) {
        fclose(wv_file);
        fclose(wvc_file);
        WavpackCloseFile(context);
        LOG_ERROR("Failed to set configuration");
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    if (!WavpackPackInit(context)) {
        fclose(wv_file);
        fclose(wvc_file);
        WavpackCloseFile(context);
        LOG_ERROR("Failed to initialize");
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    // Allocate a file handle
    wavpack_file_handle_t wavpack_file = malloc(sizeof(struct wavpack_file));
    memset(wavpack_file, 0, sizeof(struct wavpack_file));
    wavpack_file->context = context;

    // Return the handle
    *wavpack_file_out = wavpack_file;
    return WAVPACK_FILE_RESULT_SUCCESS;
}

wavpack_file_result_t wavpack_file_write(wavpack_file_handle_t wavpack_file, int32_t* samples, uint32_t num_frames) {
    return WavpackPackSamples(wavpack_file->context, samples, num_frames) ? WAVPACK_FILE_RESULT_SUCCESS : WAVPACK_FILE_RESULT_FILE_ERROR;
}
