use axum::response::Json;
use serde_json::json;

pub async fn check() -> Json<serde_json::Value>{
    Json(json!({
        "status": "ok",
        "timestamp": time::OffsetDateTime::now_utc().unix_timestamp()
    }))
}
