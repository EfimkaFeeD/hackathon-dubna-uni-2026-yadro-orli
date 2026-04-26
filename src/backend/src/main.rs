mod config;
mod errors;
mod auth;
mod db;
mod storage_client;
mod storage;
mod plugin;
mod session;
mod models;
mod routes;
mod audio_metadata;

use config::AppConfig;
use sqlx::SqlitePool;
use storage::StorageService;
use tower_http::services::ServeDir;
use tracing_subscriber::{layer::SubscriberExt, util::SubscriberInitExt, EnvFilter};
use std::fs;

#[derive(Clone)]
pub struct AppState {
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

    tracing::info!("Сервер запускается...");

    let current_dir = std::env::current_dir()?;
    tracing::info!("Текущая директория: {}", current_dir.display());

    let data_dir = current_dir.join("data");
    if !data_dir.exists() {
        fs::create_dir_all(&data_dir)?;
        tracing::info!("Создана папка для БД: {}", data_dir.display());
    }

    let config = AppConfig::from_env();
    tracing::info!("DATABASE_URL: {}", config.database_url);

    let db = db::init_pool(&config.database_url).await?;
    tracing::info!("База данных подключена");

    let storage = StorageService::new(&config.storage_url);
    tracing::info!("Storage сервис инициализирован");

    let state = AppState { db, config: config.clone(), storage };

    let static_files = ServeDir::new("../../../src/frontend").append_index_html_on_directories(true);

    let app = routes::create_router(state).fallback_service(static_files);

    let addr = format!("127.0.0.1:{}", config.server_port);
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    tracing::info!("Слушаю на http://{}", addr);

    axum::serve(listener, app).await?;
    Ok(())
}
