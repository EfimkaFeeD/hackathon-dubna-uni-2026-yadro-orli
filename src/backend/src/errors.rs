use axum::response::{IntoResponse, Response};
use axum::http::StatusCode;

#[derive(Debug, thiserror::Error)]
pub enum AppError {
    #[error("Error Database")]
    Database(#[from] sqlx::Error),
    #[error("Error WebSocket: {0}")]
    WebSocket(String),
    #[error("Error plugin: {0}")]
    Plugin(String),
    #[error("Error storage: {0}")]
    Storage(String),
    #[error("Error backend")]
    Internal,
}

impl IntoResponse for AppError {
    fn into_response(self) -> Response {
        let status = match &self {
            AppError::Database(_)  => StatusCode::INTERNAL_SERVER_ERROR,
            AppError::WebSocket(_) => StatusCode::BAD_REQUEST,
            AppError::Plugin(_) => StatusCode::INTERNAL_SERVER_ERROR,
            AppError::Storage(_) => StatusCode::INTERNAL_SERVER_ERROR,
            AppError::Internal => StatusCode::INTERNAL_SERVER_ERROR,
        };
        
        tracing::error!("{}", self);
        (status, self.to_string()).into_response()
    }
}

