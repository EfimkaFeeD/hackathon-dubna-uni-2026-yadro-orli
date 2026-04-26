use axum::extract::State;
use axum::response::Json;
use serde_json::json;
use crate::AppState;
use crate::auth;

pub async fn login(State(state): State<AppState>) -> Json<serde_json::Value> {
    let token = auth::generate_token("user", state.config.jwt_secret.as_bytes())
        .unwrap_or_default();
    Json(json!({
        "token": token,
        "token_type": "Bearer"
    }))
}
