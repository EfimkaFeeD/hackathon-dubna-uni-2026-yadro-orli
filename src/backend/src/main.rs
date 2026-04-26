mod config;
mod errors;
mod auth;
mod db;
mod storage;
mod storage_client;
mod plugin;
mod session;
mod models;
mod routes;

use config::AppConfig;
use sqlx::SqlitePool;
use storage::StorageService;
use tower_http::services::ServeDir;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt, EnvFilter};
use std::fs;

#[derive(Clone)]
pub struct AppState{
    pub db: SqlitePool,
    pub config: AppConfig,
    pub storage: StorageService,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::registry()
        .with(EnvFilter::try_from_default_env().unwrap_or_else(|_| "info".into()))
        .with(tracing_subscriber::fmt::layer())
        .init();

    tracing::info!("Server starting");

    fs::create_dir_all("data")?;

    let config = AppConfig::from_env();
    let db = db::init_pool(&config.database_url).await?;
    tracing::info!("DataBase connected");
    let storage = StorageService::new(&config.storage_url);
    tracing::info!("Storage service started");

    let state = AppState{db, config: config.clone(), storage};
    let static_files = ServeDir::new("../../frontend/").append_index_html_on_directories(true);
    let app = routes::create_router(state).fallback_service(static_files);

    let addr = format!("127.0.0.1:{}", config.server_port);
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    tracing::info!("Listening: {}", addr);

    axum::serve(listener, app).await?;
    Ok(())
}

