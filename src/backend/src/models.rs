use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub struct AudioSpec {
    pub sample_rate: u32,
    pub channels: u16,
    pub bits_per_sample: u8,
    pub is_signed: bool,
}

impl Default for AudioSpec {
    fn default() -> Self {
        Self {
            sample_rate: 44100,
            channels: 1,
            bits_per_sample: 16,
            is_signed: true,
        }
    }
}

impl AudioSpec {
    pub fn is_mono(&self) -> bool {
        self.channels == 1
    }

    pub fn bytes_per_sample(&self) -> usize {
        (self.bits_per_sample as usize) / 8
    }

    pub fn bytes_per_second(&self) -> usize {
        self.sample_rate as usize * self.channels as usize * self.bytes_per_sample()
    }

    pub fn chunk_size_for_ms(&self, ms: u32) -> usize {
        (self.bytes_per_second() as f64 * (ms as f64 / 1000.0)) as usize
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Serialize, Deserialize)]
pub enum ChunkType {
    Silence = 0,
    Voice = 1,
    WordEnd = 2,
    SentenceEnd = 3,
    ParagraphEnd = 4,
    Unknown = 255,
}

impl From<u8> for ChunkType {
    fn from(val: u8) -> Self {
        match val {
            0 => ChunkType::Silence,
            1 => ChunkType::Voice,
            2 => ChunkType::WordEnd,
            3 => ChunkType::SentenceEnd,
            4 => ChunkType::ParagraphEnd,
            _ => ChunkType::Unknown,
        }
    }
}