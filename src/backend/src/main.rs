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
use crate::plugin::init_plugin;

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

    let config = AppConfig::from_env();
    tracing::info!("DATABASE_URL: {}", config.database_url);
    tracing::info!("STATIC_DIR: {}", config.static_dir);

    let db = db::init_pool(&config.database_url).await?;
    tracing::info!("База данных подключена и проинициализирована");

    if let Err(e) = init_plugin("/app/plugins/libpodcast_silence_remover.so") {
        tracing::warn!("⚠️ Не удалось загрузить плагин: {}", e);
    } else {
        tracing::info!("✅ Аудио-плагин C++ успешно загружен!");
    }

    let storage = StorageService::new(&config.storage_url);
    let state = AppState { db, config: config.clone(), storage };

    let static_files = ServeDir::new(&config.static_dir)
        .append_index_html_on_directories(true);

    let app = routes::create_router(state).fallback_service(static_files);

    let addr = format!("{}:{}", config.server_host, config.server_port);
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    tracing::info!("Сервер запущен по адресу http://{}", addr);

    axum::serve(listener, app).await?;
    Ok(())
}
