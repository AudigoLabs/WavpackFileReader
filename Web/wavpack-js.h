#include "stdio.h"
#include "stdint.h"
#include "wavpack_file.h"

wavpack_file_result_t read_wavpack_file(const char *wv_path, const char * wvc_path, float* const* data, uint32_t max_num_frames);
