use axum::extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State};
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use rand::Rng;
use crate::AppState;
use crate::{db, plugin};
use crate::audio_metadata;
use crate::models::{AudioSpec, ChunkType, PluginRequest};

pub async fn handler(State(state): State<AppState>, ws: WebSocketUpgrade) -> impl IntoResponse {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}

const CHUNK_DURATION_MS: u32 = 10;

fn get_pause_ms(chunk_type: ChunkType) -> u32 {
    let base_ms = match chunk_type {
        ChunkType::WordEnd => 75,
        ChunkType::SentenceEnd => 300,
        ChunkType::ParagraphEnd => 600,
        _ => 0,
    };
    if base_ms == 0 { return 0; }
    let mut rng = rand::thread_rng();
    let variation = rng.gen_range(0.9..1.1);
    (base_ms as f64 * variation) as u32
}

fn get_chunk_size_bytes(spec: &AudioSpec, duration_ms: u32) -> usize {
    let bytes_per_second = spec.sample_rate as usize
        * spec.channels as usize
        * (spec.bits_per_sample as usize / 8);
    (bytes_per_second as f64 * (duration_ms as f64 / 1000.0)) as usize
}

fn generate_silence(spec: &AudioSpec, duration_ms: u32) -> Vec<u8> {
    let byte_count = get_chunk_size_bytes(spec, duration_ms);
    vec![0u8; byte_count]
}

async fn handle_socket(socket: WebSocket, state: AppState) {
    tracing::info!("WebSocket подключён");
    let (mut tx, mut rx) = socket.split();

    let mut session_id = String::new();
    let mut file_id = String::new();
    let mut file_buffer = Vec::new();
    let mut metadata_received = false;
    let mut spec = AudioSpec::default();
    let mut chunk_size_10ms: usize = 0;
    let mut pending_silence_ms: u32 = 0;
    let mut packet_counter: u32 = 0;

    while let Some(Ok(msg)) = rx.next().await {
        match msg {
            Message::Text(text) if text.contains(r#""type":"init""#) => {
                let json: serde_json::Value = serde_json::from_str(&text).unwrap_or_default();
                let incoming_session = json["sessionId"].as_str().unwrap_or("anon");
                let _filename = json["fileName"].as_str().unwrap_or("audio.wav").to_string();

                session_id = incoming_session.to_string();
                tracing::info!("Сессия: {}", session_id);
                let _ = db::upsert_session(&state.db, &session_id).await;

                let _ = tx.send(Message::Text(r#"{"extension":".wav"}"#.into())).await;
            }

            Message::Binary(data) => {
                if !metadata_received {
                    file_buffer.extend_from_slice(&data);

                    if let Ok(metadata) = audio_metadata::get_audio_metadata(&file_buffer) {
                        spec = AudioSpec {
                            sample_rate: metadata.sample_rate,
                            channels: metadata.channels,
                            bits_per_sample: metadata.bits_per_sample,
                            is_signed: true,
                        };
                        chunk_size_10ms = get_chunk_size_bytes(&spec, CHUNK_DURATION_MS);
                        tracing::info!(
                            "Метаданные: {} кГц, {} канал(ов), {} бит, формат: {} | Чанк {} мс: {} байт",
                            spec.sample_rate / 1000,
                            spec.channels,
                            spec.bits_per_sample,
                            metadata.format,
                            CHUNK_DURATION_MS,
                            chunk_size_10ms
                        );

                        if let Ok(fid) = db::create_audio_file(&state.db, &session_id, "audio").await {
                            file_id = fid;
                            tracing::info!("Файл создан: {}", file_id);
                        }

                        metadata_received = true;
                    }
                } else if chunk_size_10ms > 0 {
                    let mut offset = 0;
                    while offset + chunk_size_10ms <= data.len() {
                        let chunk_data = data[offset..offset + chunk_size_10ms].to_vec();
                        offset += chunk_size_10ms;
                        packet_counter += 1;

                        let resp = plugin::analyze_chunk(PluginRequest {
                            packet_num: packet_counter,
                            buffer: chunk_data.clone(),
                            spec,
                        }).await;

                        let chunk_type_str = format!("{:?}", resp.chunk_type).to_lowercase();
                        let _ = db::log_chunk(&state.db, &file_id, packet_counter, &chunk_type_str, chunk_data.len()).await;

                        match resp.chunk_type {
                            ChunkType::Silence => continue,
                            ChunkType::Voice => {
                                if pending_silence_ms > 0 {
                                    let silence = generate_silence(&spec, pending_silence_ms);
                                    if tx.send(Message::Binary(silence.into())).await.is_err() {
                                        break;
                                    }
                                    pending_silence_ms = 0;
                                }
                                if tx.send(Message::Binary(resp.processed_buffer.into())).await.is_err() {
                                    break;
                                }
                            }
                            ChunkType::WordEnd | ChunkType::SentenceEnd | ChunkType::ParagraphEnd => {
                                pending_silence_ms = get_pause_ms(resp.chunk_type);
                            }
                        }
                    }

                    if offset < data.len() {
                        let remaining = data[offset..].to_vec();
                        packet_counter += 1;
                        let resp = plugin::analyze_chunk(PluginRequest {
                            packet_num: packet_counter,
                            buffer: remaining.clone(),
                            spec,
                        }).await;

                        if !matches!(resp.chunk_type, ChunkType::Silence) {
                            if pending_silence_ms > 0 {
                                let silence = generate_silence(&spec, pending_silence_ms);
                                let _ = tx.send(Message::Binary(silence.into())).await;
                                pending_silence_ms = 0;
                            }
                            let _ = tx.send(Message::Binary(resp.processed_buffer.into())).await;
                        }
                    }
                }
            }

            Message::Text(text) if text.contains(r#""type":"end""#) => {
                let _ = db::finish_audio_file(&state.db, &file_id).await;
                tracing::info!("Стрим завершён. Файл: {}, обработано пакетов: {}", file_id, packet_counter);
                break;
            }
            _ => {}
        }
    }
    tracing::info!("WebSocket отключён");
}
