use crate::models::{ChunkType, PluginRequest, PluginResponse};
use tokio::task;
use std::time::Duration;


pub async fn analyze_chunk(req: PluginRequest) -> PluginResponse {
    let packet_num = req.packet_num;
    let buffer = req.buffer;
    let _spec = req.spec;

    let result = task::spawn_blocking(move || {
        // Имитация работы FFmpeg
        std::thread::sleep(Duration::from_micros(500));

        let chunk_type = if buffer.iter().all(|&b| b == 0 || b < 10) {
            ChunkType::Silence
        } else if packet_num % 20 == 0 {
            ChunkType::SentenceEnd
        } else if packet_num % 5 == 0 {
            ChunkType::WordEnd
        } else {
            ChunkType::Voice
        };

        PluginResponse {
            packet_num,
            chunk_type,
            processed_buffer: buffer,
        }
    });

    match result.await {
        Ok(response) => response,
        Err(e) => {
            tracing::error!("❌ Паника в заглушке плагина: {}", e);
            PluginResponse {
                packet_num,
                chunk_type: ChunkType::Voice,
                processed_buffer: Vec::new(),
            }
        }
    }
}
