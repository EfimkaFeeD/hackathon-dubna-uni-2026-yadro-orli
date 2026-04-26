use serde::Deserialize;

#[derive(Clone, Deserialize)]
pub struct AppConfig {
    pub database_url: String,
    pub jwt_secret: String,
    pub server_port: u16,
    pub server_host: String,
    pub storage_url: String,
    pub static_dir: String,
}

impl AppConfig {
    pub fn from_env() -> Self {
        dotenvy::dotenv().ok();
        Self {
            database_url: std::env::var("DATABASE_URL")
                .unwrap_or_else(|_| "sqlite:///app/data/backend.db".into()),
            jwt_secret: std::env::var("JWT_SECRET")
                .unwrap_or_else(|_| "ruFzVtgAxWSqmPs0z5KEU2goUcbUecSs".into()),
            server_port: std::env::var("SERVER_PORT")
                .unwrap_or_else(|_| "8080".into())
                .parse().unwrap_or(8080),
            server_host: std::env::var("SERVER_HOST")
                .unwrap_or_else(|_| "0.0.0.0".into()), // Берем из .env
            storage_url: std::env::var("STORAGE_SERVICE_URL")
                .unwrap_or_else(|_| "http://storage-service:8081".into()),
            static_dir: std::env::var("STATIC_DIR")
                .unwrap_or_else(|_| "/static".into()),
        }
    }
}
