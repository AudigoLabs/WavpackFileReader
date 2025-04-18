#include "wavpack-js.h"

wavpack_file_result_t read_wavpack_file(const char *wv_path, const char *wvc_path, float* const* data, uint32_t max_num_frames) {
  wavpack_file_handle_t handle;
  wavpack_file_result_t result = wavpack_file_open(wv_path, wvc_path, &handle);
  printf("result: %d\n", result);
  if (result != WAVPACK_FILE_RESULT_SUCCESS) {
    fprintf(stderr, "failed to open file\n");
    return result;
  }
  uint32_t result2 = wavpack_file_read(handle, data, max_num_frames);
  printf("result: %d\n", result2);
  return WAVPACK_FILE_RESULT_SUCCESS;
}

wavpack_file_result_t read_wavpack_buffer(const char *wv_data, int32_t wv_size, const char *wvc_data, int32_t wvc_size, float* const* data) {
  wavpack_file_handle_t handle;
  wavpack_file_result_t result = wavpack_file_open_raw(wv_data, wv_size, wvc_data, wvc_size, &handle);
  printf("result: %d\n", result);
  if (result != WAVPACK_FILE_RESULT_SUCCESS) {
    fprintf(stderr, "failed to open file\n");
    return result;
  }
  uint32_t num_samples = wavpack_file_get_num_samples(&result);
  printf("num_samples: %d\n", num_samples);
  uint32_t result2 = wavpack_file_read(handle, data, num_samples);
  printf("result: %d\n", result2);
  return WAVPACK_FILE_RESULT_SUCCESS;
}
