#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ChunkType {
    Voice = 0,
    Silence = 1,
    WordStart = 2,
    SentenceStart = 3,
    ParagraphStart = 4,
    Unknown = 255,
}

impl From<u8> for ChunkType {
    fn from(val: u8) -> Self {
        match val {
            0 => ChunkType::Voice,
            1 => ChunkType::Silence,
            2 => ChunkType::WordStart,
            3 => ChunkType::SentenceStart,
            4 => ChunkType::ParagraphStart,
            _ => ChunkType::Unknown,
        }
    }
}

#[derive(Debug, Clone)]
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

// ДОБАВЛЯЕМ ЭТОТ БЛОК:
impl AudioSpec {
    pub fn is_mono(&self) -> bool {
        self.channels == 1
    }
}