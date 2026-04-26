use axum::extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State};
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use crate::AppState;
use crate::plugin::AudioProcessor;
use crate::models::{AudioSpec, ChunkType};
// use crate::db; // Раскомментируйте, если записываете историю

pub async fn handler(State(state): State<AppState>, ws: WebSocketUpgrade) -> impl IntoResponse {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}

async fn handle_socket(socket: WebSocket, _state: AppState) {
    tracing::info!("🔗 WebSocket подключён");
    let (mut tx, mut rx) = socket.split();

    let mut received_init = false;
    let mut packet_counter: u32 = 0;

    // Хранит инстанс C++ процессора для текущего файла
    let mut processor: Option<AudioProcessor> = None;
    let spec = AudioSpec::default();

    while let Some(Ok(msg)) = rx.next().await {
        match msg {
            Message::Text(text) if text.contains(r#""type":"init""#) => {
                tracing::info!("📥 Получен Init: {}", text);
                received_init = true;

                // Создаем C++ обработчик
                match AudioProcessor::new(&spec) {
                    Ok(p) => processor = Some(p),
                    Err(e) => tracing::error!("❌ Ошибка загрузки плагина: {}", e),
                }

                let response = r#"{"extension":".wav"}"#;
                if tx.send(Message::Text(response.into())).await.is_err() {
                    break;
                }
            }
            Message::Binary(data) => {
                if !received_init {
                    continue;
                }

                packet_counter += 1;

                if let Some(ref proc) = processor {
                    match proc.process(packet_counter, &data) {
                        Ok(ChunkType::Voice) => {
                            // Это голос! Отправляем чанк обратно на фронт (или в хранилище)
                            if tx.send(Message::Binary(data)).await.is_err() {
                                break;
                            }
                        }
                        Ok(ChunkType::Silence) => {
                            // Пропускаем этот чанк, тем самым "ускоряя" аудио и удаляя тишину.
                            // Можно включить лог для дебага:
                            // tracing::debug!("🔇 Пропущена тишина, чанк #{}", packet_counter);
                        }
                        _ => {
                            // Если неизвестно — перестраховываемся и отправляем
                            let _ = tx.send(Message::Binary(data)).await;
                        }
                    }
                } else {
                    // Если плагин не загрузился, работаем как простой эхо-сервер (pass-through)
                    let _ = tx.send(Message::Binary(data)).await;
                }
            }
            Message::Text(text) if text.contains(r#""type":"end""#) => {
                tracing::info!("🏁 Стрим завершён. Обработано пакетов: {}", packet_counter);
                break;
            }
            _ => {}
        }
    }
    tracing::info!("👋 WebSocket отключён");
}