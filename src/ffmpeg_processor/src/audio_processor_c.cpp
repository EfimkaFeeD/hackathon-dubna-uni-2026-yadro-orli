#include "audio_processor_c.h"

#include <cstddef>
#include <cstdint>

#include "consts.h"
#include "ffmpeg_processor.hpp"

static AudioSpec
toAudioSpec(const CAudioSpec& spec) {
  AudioSpec out{};
  out.sample_rate     = spec.sample_rate;
  out.is_mono         = spec.is_mono != 0;
  out.bits_per_sample = spec.bits_per_sample;
  out.is_signed       = spec.is_signed != 0;
  out.is_float        = spec.is_float != 0;
  return out;
}

ProcessorHandle
processor_create(const CAudioSpec* spec, float margin_db, int vad_mode, int noise_window_ms) {
  if (spec == nullptr) {
    return nullptr;
  }
  try {
    AudioSpec cppSpec = toAudioSpec(*spec);
    return new FfmpegProcessor(cppSpec, margin_db == 0.0F ? kDefaultMarginDb : margin_db,
                               vad_mode == -1 ? kDefaultVADMode : vad_mode,
                               noise_window_ms == -1 ? kNoiseWindowTargetMs : noise_window_ms);
  } catch (...) {
    return nullptr;
  }
}

PluginResult
processor_process(ProcessorHandle handle, uint32_t packet_num, const uint8_t* buffer, size_t buffer_len) {
  PluginResult res = {.packet_num = packet_num, .chunk_type = 1};
  if ((handle == nullptr) || (buffer == nullptr)) {
    return res;
  }
  auto* proc          = static_cast<FfmpegProcessor*>(handle);
  FfmpegResult cppRes = proc->process(packet_num, buffer, buffer_len);
  res.packet_num      = cppRes.packet_num;
  res.chunk_type      = cppRes.chunk_type;
  return res;
}

void
processor_destroy(ProcessorHandle handle) {
  delete static_cast<FfmpegProcessor*>(handle);
}
