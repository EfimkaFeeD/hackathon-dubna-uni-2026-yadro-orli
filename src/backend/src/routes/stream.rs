use axum::extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State};
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use bytes::Bytes;
use crate::{AppState, db, plugin, models::{AudioSpec, ChunkType}};

pub async fn handler(State(state): State<AppState>, ws: WebSocketUpgrade) -> impl IntoResponse {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}
fn apply_fade_out(data: &mut [u8]) {
    let samples = unsafe {
        std::slice::from_raw_parts_mut(data.as_mut_ptr() as *mut i16, data.len() / 2)
    };
    let fade_len = 240;
    let len = samples.len();
    for i in 0..fade_len.min(len) {
        let ratio = 1.0 - (i as f32 / fade_len as f32);
        samples[len - 1 - i] = (samples[len - 1 - i] as f32 * ratio) as i16;
    }
}

fn apply_fade_in(data: &mut [u8]) {
    let samples = unsafe {
        std::slice::from_raw_parts_mut(data.as_mut_ptr() as *mut i16, data.len() / 2)
    };
    let fade_len = 240;
    for i in 0..fade_len.min(samples.len()) {
        let ratio = i as f32 / fade_len as f32;
        samples[i] = (samples[i] as f32 * ratio) as i16;
    }
}
fn generate_pure_pause(ms: u32, sample_rate: u32) -> Bytes {
    let size = (sample_rate * 1 * 2 * ms / 1000) as usize;
    Bytes::from(vec![0; size])
}

async fn handle_socket(socket: WebSocket, state: AppState) {
    let (mut tx, mut rx) = socket.split();
    let mut file_id = String::new();
    let mut processor: Option<plugin::AudioProcessor> = None;
    let mut packet_counter: u32 = 0;

    let mut pending_chunk: Option<Vec<u8>> = None;
    let mut last_type = ChunkType::Silence;

    let sample_rate = 44100;
    let frame_ms = 20;
    let chunk_size = (sample_rate * 1 * 2 * frame_ms / 1000) as usize; // 1764 байта

    while let Some(Ok(msg)) = rx.next().await {
        match msg {
            Message::Text(text) => {
                let json: serde_json::Value = serde_json::from_str(text.as_str()).unwrap_or_default();

                if json["type"] == "init" {
                    let session_id = json["sessionId"].as_str().unwrap_or("anon").to_string();
                    file_id = db::create_audio_file(&state.db, &session_id, "audio").await.unwrap_or_default();

                    processor = plugin::AudioProcessor::new(&AudioSpec::default()).ok();
                    let _ = tx.send(Message::Text(r#"{"extension":".wav"}"#.into())).await;
                }

                if json["type"] == "end" { break; }
            }

            Message::Binary(data) => {
                let mut offset = 0;
                while offset + chunk_size <= data.len() {
                    let mut current_data = data[offset..offset+chunk_size].to_vec();
                    offset += chunk_size;
                    packet_counter += 1;

                    let current_type = processor.as_ref()
                        .and_then(|p| p.process(packet_counter, &current_data).ok())
                        .unwrap_or(ChunkType::Voice);

                    match current_type {
                        ChunkType::Voice => {
                            if last_type == ChunkType::Silence {
                                apply_fade_in(&mut current_data);
                            }
                            if let Some(prev) = pending_chunk.take() {
                                let _ = tx.send(Message::Binary(prev.into())).await;
                            }
                            pending_chunk = Some(current_data);
                        }
                        ChunkType::WordStart | ChunkType::SentenceStart | ChunkType::ParagraphStart => {
                            if let Some(mut prev) = pending_chunk.take() {
                                apply_fade_out(&mut prev);
                                let _ = tx.send(Message::Binary(prev.into())).await;
                            }
                            let pause_ms = match current_type {
                                ChunkType::WordStart => 120,
                                ChunkType::SentenceStart => 450,
                                _ => 1000
                            };
                            let _ = tx.send(Message::Binary(generate_pure_pause(pause_ms, sample_rate))).await;
                            apply_fade_in(&mut current_data);
                            pending_chunk = Some(current_data);
                        }
                        ChunkType::Silence => {
                            if let Some(mut last_voice) = pending_chunk.take() {
                                apply_fade_out(&mut last_voice);
                                let _ = tx.send(Message::Binary(last_voice.into())).await;
                            }
                        }
                        _ => {}
                    }
                    last_type = current_type;
                }
            }

            _ => {}
        }
    }
    if let Some(mut last) = pending_chunk {
        apply_fade_out(&mut last);
        let _ = tx.send(Message::Binary(last.into())).await;
    }
}