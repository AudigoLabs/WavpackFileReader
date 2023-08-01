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
    uint32_t raw_frames;
    uint32_t raw_offset;
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
    const int flags =
        (wvc_path ? OPEN_WVC : 0) |
        (4 << OPEN_THREADS_SHFT);
    WavpackContext* context = WavpackOpenFileInputEx64(wavpack_stream_reader_get(), wv_file, wvc_file, error_buffer, flags, 0);
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

wavpack_file_result_t wavpack_file_open_raw(const void* wv_data, int32_t wv_size, const void* wvc_data, int32_t wvc_size, wavpack_file_handle_t* wavpack_file_out) {
    if (!wv_data) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    const uint64_t block_offset = (uint64_t)((const uint32_t*)wv_data)[4] | (((uint64_t)((const uint8_t*)wv_data)[10]) << 32);
    if (block_offset > UINT32_MAX) {
        LOG_ERROR("Unsupported block offset: %llu", block_offset);
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }

    char error_buffer[81];
    const int flags = (4 << OPEN_THREADS_SHFT);
    WavpackContext* context = WavpackOpenRawDecoder((void*)wv_data, wv_size, (void*)wvc_data, wvc_size, 0x0, error_buffer, flags, 0);
    if (!context) {
        LOG_ERROR("Failed to open raw wavpack decoder: %s", error_buffer);
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    // Allocate a file handle
    wavpack_file_handle_t wavpack_file = malloc(sizeof(struct wavpack_file));
    memset(wavpack_file, 0, sizeof(struct wavpack_file));
    wavpack_file->context = context;
    wavpack_file->raw_frames = ((const uint32_t*)wv_data)[5];
    wavpack_file->raw_offset = (uint32_t)block_offset;
    wavpack_file->unpack_buffer = malloc(sizeof(*wavpack_file->unpack_buffer) * wavpack_file_get_num_channels(wavpack_file) * wavpack_file->raw_frames);
    const uint32_t num_frames_unpacked = WavpackUnpackSamples(wavpack_file->context, wavpack_file->unpack_buffer, wavpack_file->raw_frames);
    if (num_frames_unpacked != wavpack_file->raw_frames) {
        LOG_ERROR("Failed to unpack data: %d, %d", wavpack_file->raw_frames, num_frames_unpacked);
        wavpack_file_close(wavpack_file);
        return WAVPACK_FILE_RESULT_FILE_ERROR;
    }

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

uint32_t wavpack_file_get_raw_block_offset(wavpack_file_handle_t wavpack_file) {
    return wavpack_file->raw_offset;
}

uint32_t wavpack_file_get_num_samples(wavpack_file_handle_t wavpack_file) {
    return wavpack_file->raw_frames ? wavpack_file->raw_frames : WavpackGetNumSamples(wavpack_file->context);
}

double wavpack_file_get_duration(wavpack_file_handle_t wavpack_file) {
    return (double)wavpack_file_get_num_samples(wavpack_file) / wavpack_file_get_sample_rate(wavpack_file);
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
    if (offset > wavpack_file_get_num_samples(wavpack_file)) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    return WavpackSeekSample64(wavpack_file->context, offset) ? WAVPACK_FILE_RESULT_SUCCESS : WAVPACK_FILE_RESULT_FILE_ERROR;
}

uint32_t wavpack_file_read(wavpack_file_handle_t wavpack_file, float* const* data, uint32_t max_num_frames) {
    const uint16_t num_channels = wavpack_file_get_num_channels(wavpack_file);
    const float MAX_VALUE = 1 << (WavpackGetBitsPerSample(wavpack_file->context) - 1);

    if (wavpack_file->raw_frames) {
        if (max_num_frames != wavpack_file_get_num_samples(wavpack_file)) {
            return WAVPACK_FILE_RESULT_INVALID_PARAM;
        }
        // Convert the samples to floats and write them into the passed buffer
        for (uint16_t channel = 0; channel < num_channels; channel++) {
            // Convert the fixed-point data into a float
            vDSP_vflt32(&wavpack_file->unpack_buffer[channel], num_channels, data[channel], 1, max_num_frames);
            // Scale the float data to be within the range [-1,1]
            vDSP_vsdiv(data[channel], 1, &MAX_VALUE, data[channel], 1, max_num_frames);
        }
        return max_num_frames;
    }

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
    free(wavpack_file->unpack_buffer);
    free(wavpack_file);
}
