use axum::{
    extract::ws::{Message, WebSocket, WebSocketUpgrade},
    response::IntoResponse,
    routing::get,
    Router,
};
use futures::{SinkExt, StreamExt};
use std::net::SocketAddr;
use tower_http::services::ServeDir;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt, EnvFilter};

#[tokio::main]
async fn main() {
    tracing_subscriber::registry()
        .with(EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()))
        .with(tracing_subscriber::fmt::layer())
        .init();

    println!("Server starting...");

    let static_files = ServeDir::new("../../frontend").append_index_html_on_directories(true);

    let app = Router::new()
        .route("/ws", get(ws_handler))
        .fallback_service(static_files);

    let addr = SocketAddr::from(([127, 0, 0, 1], 8080));
    let listener = tokio::net::TcpListener::bind(addr).await.unwrap();
    println!("Listening on http://{}", addr);

    axum::serve(listener, app).await.unwrap();
}

async fn ws_handler(ws: WebSocketUpgrade) -> impl IntoResponse {
    ws.on_upgrade(handle_socket)
}

async fn handle_socket(socket: WebSocket) {
    println!("WebSocket connected");
    let (mut tx, mut rx) = socket.split();

    while let Some(Ok(msg)) = rx.next().await {
        match msg {
            Message::Text(text) if text.contains(r#""type":"init""#) => {
                println!("Init received");
                let _ = tx.send(Message::Text(r#"{"extension":".wav"}"#.into())).await;
            }
            Message::Text(text) if text.contains(r#""type":"end""#) => {
                println!("Stream ended");
                break;
            }
            Message::Binary(data) => {
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
