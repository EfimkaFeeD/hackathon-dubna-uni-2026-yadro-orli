mod config;
mod errors;
mod auth;
mod db;
mod storage;
mod plugin;
mod routes;

use axum::{Router, routing::get, response::Html};
use tokio::net::TcpListener;

fn init_router() -> Router {
    Router::new()
        .route("/", get(hello_world))
}

async fn hello_world() -> Html<&'static str> {
    Html("<h1>Hello, World!</h1>")
}

#[tokio::main]
async fn main() {
    println!("Hello, world!");
    let app = init_router();

    let listener = TcpListener::bind("127.0.0.1:3000")
        .await
        .expect("Failed to bind");

    println!("Listening on: {}", listener.local_addr().unwrap());
    axum::serve(listener, app).await.expect("Failed to run server");
}
