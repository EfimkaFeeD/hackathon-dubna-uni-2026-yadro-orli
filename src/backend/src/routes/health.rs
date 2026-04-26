use axum::response::Json;
use serde_json::json;
use time::OffsetDateTime;

pub async fn check() -> Json<serde_json::Value> {
    Json(json!({
        "status": "ok",
        "timestamp": OffsetDateTime::now_utc().unix_timestamp()
    }))
}
