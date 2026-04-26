use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct AudioSpec {
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u8,
    pub is_signed: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub enum ChunkType {
    Voice,
    Silence,
    WordEnd,
    SentenceEnd,
    ParagraphEnd,
}

#[derive(Debug, Clone)]
pub struct PluginRequest {
    pub packet_num: u32,
    pub buffer: Vec<u8>,
    pub spec: AudioSpec,
}

#[derive(Debug, Clone)]
pub struct PluginResponse {
    pub packet_num: u32,
    pub chunk_type: ChunkType,
    pub processed_buffer: Vec<u8>,
}
