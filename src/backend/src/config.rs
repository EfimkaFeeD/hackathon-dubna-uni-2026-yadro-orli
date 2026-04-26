use serde::Deserialize;

#[derive(Clone, Deserialize)]
pub struct AppConfig {
    pub database_url: String,
    pub jwt_secret: String,
    pub server_port: u16,
    pub storage_url: String,
}

impl AppConfig {
    pub fn from_env() -> Self {
        dotenvy::dotenv().ok();
        Self {
            database_url: std::env::var("DATABASE_URL")
                .unwrap_or_else(|_| "sqlite://./data/backend.db?mode=rwc".into()),
            jwt_secret: std::env::var("JWT_SECRET")
                .unwrap_or_else(|_| "ruFzVtgAxWSqmPs0z5KEU2goUcbUecSs".into()),
            server_port: std::env::var("PORT")
                .unwrap_or_else(|_| "8080".into())
                .parse()
                .unwrap(),
            storage_url: std::env::var("STORAGE_URL")
                .unwrap_or_else(|_| "http://localhost:8081".into()),
        }
    }
}

