#ifndef PODCAST_SILENCE_REMOVER_SRC_AUDIO_PROCESSOR_C_H
#define PODCAST_SILENCE_REMOVER_SRC_AUDIO_PROCESSOR_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  typedef void* ProcessorHandle;

  typedef struct {
    uint32_t packet_num;
    uint8_t chunk_type;
  } PluginResult;

  typedef struct {
    uint32_t sample_rate;
    int is_mono;
    uint8_t bits_per_sample;
    int is_signed;
    int is_float;
  } CAudioSpec;

  ProcessorHandle processor_create(const CAudioSpec* spec, float margin_db, int vad_mode, int noise_window_ms);

  PluginResult processor_process(ProcessorHandle handle, uint32_t packet_num, const uint8_t* buffer, size_t buffer_len);

  void processor_destroy(ProcessorHandle handle);

#ifdef __cplusplus
}
#endif

#endif // PODCAST_SILENCE_REMOVER_SRC_AUDIO_PROCESSOR_C_H
