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

#define BUFFER_SIZE 4096

struct wavpack_file {
    WavpackContext* context;
    bool is_writing;
    int32_t* buffer;
    uint32_t raw_frames;
    uint32_t raw_offset;
    FILE* wv_file;
    FILE* wvc_file;
};

_Static_assert(sizeof(int32_t) == sizeof(float), "Invalid int32 / float sizes");

static int wavpack_write_handler(void* context, void* data, int32_t length) {
    return fwrite(data, length, 1, (FILE*)context) == 1;
}

wavpack_file_result_t wavpack_file_open_for_writing(const wavpack_write_config_t* config, const char* wv_path, const char* wvc_path, wavpack_file_handle_t* wavpack_file_out) {
    // Open the files
    FILE* wv_file = fopen(wv_path, "w");
    if (!wv_file) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    FILE* wvc_file = NULL;
    if (wvc_path) {
        wvc_file = fopen(wvc_path, "w");
        if (!wvc_file) {
            fclose(wv_file);
            return WAVPACK_FILE_RESULT_OPEN_FAILED;
        }
    }
    WavpackContext* context = WavpackOpenFileOutput(wavpack_write_handler, wv_file, wvc_file);
    if (!context) {
        fclose(wv_file);
        if (wvc_file) {
            fclose(wvc_file);
        }
        LOG_ERROR("Failed to open wavpack file");
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }
    WavpackConfig wavpack_config = {
        .bits_per_sample = config->bits_per_sample,
        .bytes_per_sample = config->bits_per_sample / 8,
        .num_channels = config->num_channels,
        .sample_rate = config->sample_rate,
        .flags = CONFIG_FAST_FLAG | (wvc_path ? (CONFIG_HYBRID_FLAG | CONFIG_CREATE_WVC | CONFIG_OPTIMIZE_WVC) : 0),
        .bitrate = 512,
        .shaping_weight = 0,
    };
    if (!WavpackSetConfiguration64(context, &wavpack_config, -1, NULL) || !WavpackPackInit(context)) {
        fclose(wv_file);
        if (wvc_file) {
            fclose(wvc_file);
        }
        LOG_ERROR("Failed to configure wavpack: %s", WavpackGetErrorMessage(context));
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    // Allocate a file handle
    wavpack_file_handle_t wavpack_file = malloc(sizeof(struct wavpack_file));
    memset(wavpack_file, 0, sizeof(struct wavpack_file));
    wavpack_file->context = context;
    wavpack_file->is_writing = true;
    wavpack_file->buffer = malloc(sizeof(*wavpack_file->buffer) * wavpack_file_get_num_channels(wavpack_file) * BUFFER_SIZE);
    wavpack_file->wv_file = wv_file;
    wavpack_file->wvc_file = wvc_file;

    // Return the handle
    *wavpack_file_out = wavpack_file;
    return WAVPACK_FILE_RESULT_SUCCESS;
}

wavpack_file_result_t wavpack_file_open_for_reading(const char* wv_path, const char* wvc_path, wavpack_file_handle_t* wavpack_file_out) {
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
    const int flags = wvc_path ? OPEN_WVC : 0;
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
    wavpack_file->buffer = malloc(sizeof(*wavpack_file->buffer) * wavpack_file_get_num_channels(wavpack_file) * BUFFER_SIZE);
    wavpack_file->wv_file = wv_file;
    wavpack_file->wvc_file = wvc_file;

    // Return the handle
    *wavpack_file_out = wavpack_file;
    return WAVPACK_FILE_RESULT_SUCCESS;
}

wavpack_file_result_t wavpack_file_open_for_reading_raw(const void* wv_data, int32_t wv_size, const void* wvc_data, int32_t wvc_size, wavpack_file_handle_t* wavpack_file_out) {
    if (!wv_data) {
        return WAVPACK_FILE_RESULT_OPEN_FAILED;
    }

    const uint64_t block_offset = (uint64_t)((const uint32_t*)wv_data)[4] | (((uint64_t)((const uint8_t*)wv_data)[10]) << 32);
    if (block_offset > UINT32_MAX) {
        LOG_ERROR("Unsupported block offset: %llu", block_offset);
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }

    char error_buffer[81];
    const int flags = 0;
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
    wavpack_file->buffer = malloc(sizeof(*wavpack_file->buffer) * wavpack_file_get_num_channels(wavpack_file) * wavpack_file->raw_frames);
    const uint32_t num_frames_unpacked = WavpackUnpackSamples(wavpack_file->context, wavpack_file->buffer, wavpack_file->raw_frames);
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
    if (wavpack_file->is_writing) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    if (position > wavpack_file_get_duration(wavpack_file)) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    return wavpack_file_set_offset(wavpack_file, position * wavpack_file_get_sample_rate(wavpack_file) + 0.5);
}

wavpack_file_result_t wavpack_file_set_offset(wavpack_file_handle_t wavpack_file, uint32_t offset) {
    if (wavpack_file->is_writing) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    if (offset > wavpack_file_get_num_samples(wavpack_file)) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }
    return WavpackSeekSample64(wavpack_file->context, offset) ? WAVPACK_FILE_RESULT_SUCCESS : WAVPACK_FILE_RESULT_FILE_ERROR;
}

uint32_t wavpack_file_read(wavpack_file_handle_t wavpack_file, float* const* data, uint32_t max_num_frames) {
    if (wavpack_file->is_writing) {
        return 0;
    }
    const uint16_t num_channels = wavpack_file_get_num_channels(wavpack_file);
    const float MAX_VALUE = 1 << (WavpackGetBitsPerSample(wavpack_file->context) - 1);

    if (wavpack_file->raw_frames) {
        if (max_num_frames != wavpack_file_get_num_samples(wavpack_file)) {
            return 0;
        }
        // Convert the samples to floats and write them into the passed buffer
        for (uint16_t channel = 0; channel < num_channels; channel++) {
            // Convert the fixed-point data into a float
            vDSP_vflt32(&wavpack_file->buffer[channel], num_channels, data[channel], 1, max_num_frames);
            // Scale the float data to be within the range [-1,1]
            vDSP_vsdiv(data[channel], 1, &MAX_VALUE, data[channel], 1, max_num_frames);
        }
        return max_num_frames;
    }

    uint32_t total_frames_unpacked = 0;
    while (total_frames_unpacked < max_num_frames) {
        uint32_t chunk_frames = max_num_frames - total_frames_unpacked;
        chunk_frames = chunk_frames <= BUFFER_SIZE ? chunk_frames : BUFFER_SIZE;

        // Unpack into a temp buffer
        const uint32_t num_frames_unpacked = WavpackUnpackSamples(wavpack_file->context, wavpack_file->buffer, chunk_frames);
        if (num_frames_unpacked == 0) {
            // Reached the end of the file
            break;
        }

        // Convert the samples to floats and write them into the passed buffer
        for (uint16_t channel = 0; channel < num_channels; channel++) {
            // Convert the fixed-point data into a float
            vDSP_vflt32(&wavpack_file->buffer[channel], num_channels, &data[channel][total_frames_unpacked], 1, num_frames_unpacked);
            // Scale the float data to be within the range [-1,1]
            vDSP_vsdiv(&data[channel][total_frames_unpacked], 1, &MAX_VALUE, &data[channel][total_frames_unpacked], 1, num_frames_unpacked);
        }

        total_frames_unpacked += num_frames_unpacked;
    }
    return total_frames_unpacked;
}

wavpack_file_result_t wavpack_file_write(wavpack_file_handle_t wavpack_file, float* const* data, uint32_t num_frames) {
    if (!wavpack_file->is_writing) {
        return WAVPACK_FILE_RESULT_INVALID_PARAM;
    }

    const uint16_t num_channels = wavpack_file_get_num_channels(wavpack_file);
    const float MAX_VALUE = 1 << (WavpackGetBitsPerSample(wavpack_file->context) - 1);

    uint32_t total_frames_written = 0;
    while (total_frames_written < num_frames) {
        uint32_t chunk_frames = num_frames - total_frames_written;
        chunk_frames = chunk_frames <= BUFFER_SIZE ? chunk_frames : BUFFER_SIZE;

        // Convert the samples to integers and store in our buffer
        for (uint16_t channel = 0; channel < num_channels; channel++) {
            // Scale the float data to be within the integer range and copy into the buffer
            vDSP_vsmul(&data[channel][total_frames_written], 1, &MAX_VALUE, (float*)&wavpack_file->buffer[channel], num_channels, chunk_frames);
        }
        // Convert the float data into fixed-point data
        vDSP_vfixr32((float*)wavpack_file->buffer, 1, wavpack_file->buffer, 1, chunk_frames * num_channels);

        if (!WavpackPackSamples(wavpack_file->context, wavpack_file->buffer, chunk_frames)) {
            return WAVPACK_FILE_RESULT_FILE_ERROR;
        }
        total_frames_written += chunk_frames;
    }
    return WAVPACK_FILE_RESULT_SUCCESS;
}

void wavpack_file_close(wavpack_file_handle_t wavpack_file) {
    if (wavpack_file->is_writing) {
        WavpackFlushSamples(wavpack_file->context);
    }
    WavpackCloseFile(wavpack_file->context);
    fflush(wavpack_file->wv_file);
    fclose(wavpack_file->wv_file);
    if (wavpack_file->wvc_file) {
        fflush(wavpack_file->wvc_file);
        fclose(wavpack_file->wvc_file);
    }
    free(wavpack_file->buffer);
    free(wavpack_file);
}
