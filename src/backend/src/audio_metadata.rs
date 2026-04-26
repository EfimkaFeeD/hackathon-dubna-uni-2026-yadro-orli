use symphonia::core::{
    codecs::CODEC_TYPE_NULL,
    formats::FormatOptions,
    io::MediaSourceStream,
    meta::MetadataOptions,
    probe::Hint,
};
use std::io::Cursor;

#[derive(Debug, Clone)]
pub struct AudioMetadata {
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u8,
    pub duration_ms: u64,
    pub format: String,
}

impl Default for AudioMetadata {
    fn default() -> Self {
        Self {
            sample_rate: 44100,
            channels: 1,
            bits_per_sample: 16,
            duration_ms: 0,
            format: "unknown".to_string(),
        }
    }
}

pub fn get_audio_metadata(data: &[u8]) -> Result<AudioMetadata, String> {
    let data_vec = data.to_vec();
    let cursor = Cursor::new(data_vec);
    let mss = MediaSourceStream::new(Box::new(cursor), Default::default());

    let hint = Hint::new();
    let probed = symphonia::default::get_probe()
        .format(&hint, mss, &FormatOptions::default(), &MetadataOptions::default())
        .map_err(|e| format!("Probe error: {}", e))?;

    let format = probed.format;
    let track = format
        .tracks()
        .iter()
        .find(|t| t.codec_params.codec != CODEC_TYPE_NULL)
        .ok_or("No audio tracks found")?;

    let params = &track.codec_params;

    let sample_rate = params.sample_rate.unwrap_or(44100);

    let channels = params
        .channels
        .map(|ch| ch.count() as u16)
        .unwrap_or(2);

    let bits_per_sample = params
        .bits_per_sample
        .map(|bits| bits as u8)
        .unwrap_or(16);

    let duration_ms = params
        .n_frames
        .map(|frames| frames as u64 * 1000 / sample_rate as u64)
        .unwrap_or(0);

    let codec_name = format!("{:?}", params.codec);

    Ok(AudioMetadata {
        sample_rate,
        channels,
        bits_per_sample,
        duration_ms,
        format: codec_name,
    })
}
