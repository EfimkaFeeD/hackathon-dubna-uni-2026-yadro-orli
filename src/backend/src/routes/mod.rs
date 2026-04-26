use axum::Router;
use crate::AppState;

pub mod health;
pub mod auth;
pub mod stream;

pub fn create_router(state: AppState) -> Router {
    Router::new()
        .route("/health", axum::routing::get(health::check))
        .route("/auth/login", axum::routing::post(auth::login))
        .route("/ws", axum::routing::get(stream::handler))
        .with_state(state)
}
