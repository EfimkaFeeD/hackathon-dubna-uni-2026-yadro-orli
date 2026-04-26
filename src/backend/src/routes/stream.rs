use axum::extract::{ws::{Message, WebSocket, WebSocketUpgrade}, State};
use axum::response::IntoResponse;
use futures::{SinkExt, StreamExt};
use crate::AppState;

pub async fn handler(State(_state): State<AppState>, ws: WebSocketUpgrade) -> impl IntoResponse {
    ws.on_upgrade(handle_socket)
}

async fn handle_socket(mut socket: WebSocket) {
    println!("WebSocket connected");
    let (mut tx, mut rx) = socket.split();
    
    let mut received_init = false;

    while let Some(Ok(msg)) = rx.next().await {
        match msg {
            Message::Text(text) if text.contains(r#""type":"init""#) => {
                println!("Init received: {}", text);
                received_init = true;
                let response = r#"{"extension":".wav"}"#;
                println!("Sending: {}", response);
                if tx.send(Message::Text(response.into())).await.is_err() {
                    break;
                }
            }
            Message::Text(text) if text.contains(r#""type":"end""#) => {
                println!("Stream ended");
                break;
            }
            Message::Binary(data) => {
                if !received_init {
                    continue;
                }
                if tx.send(Message::Binary(data)).await.is_err() {
                    println!("Client disconnected");
                    break;
                }
            }
            _ => {}
        }
    }
    println!("WebSocket disconnected");
}