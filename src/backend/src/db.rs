use sqlx::SqlitePool;
use time::OffsetDateTime;

pub async fn init_pool(url: &str) -> sqlx::Result<SqlitePool> {
    let pool = SqlitePool::connect(url).await?;
    sqlx::query(
        r#"
        CREATE TABLE IF NOT EXISTS sessions (
            id TEXT PRIMARY KEY,
            created_at INTEGER NOT NULL DEFAULT (unixepoch()),
            last_active INTEGER NOT NULL DEFAULT (unixepoch())
        );
        
        CREATE TABLE IF NOT EXISTS audio_files (
            id TEXT PRIMARY KEY,
            session_id TEXT NOT NULL,
            filename TEXT NOT NULL,
            total_chunks INTEGER DEFAULT 0,
            status TEXT DEFAULT 'processing',
            created_at INTEGER DEFAULT (unixepoch()),
            finished_at INTEGER,
            FOREIGN KEY(session_id) REFERENCES sessions(id)
        );
        
        CREATE TABLE IF NOT EXISTS audio_chunks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id TEXT NOT NULL,
            packet_num INTEGER NOT NULL,
            chunk_type TEXT NOT NULL,
            size_bytes INTEGER NOT NULL,
            created_at INTEGER DEFAULT (unixepoch()),
            FOREIGN KEY(file_id) REFERENCES audio_files(id)
        );
        "#,
    )
    .execute(&pool)
    .await?;
    Ok(pool)
}

pub async fn upsert_session(pool: &SqlitePool, session_id: &str) -> sqlx::Result<()> {
    let now = OffsetDateTime::now_utc().unix_timestamp();
    sqlx::query(
        r#"
        INSERT INTO sessions (id, created_at, last_active) 
        VALUES (?, ?, ?) 
        ON CONFLICT(id) DO UPDATE SET last_active = ?
        "#,
    )
    .bind(session_id)
    .bind(now)
    .bind(now)
    .bind(now)
    .execute(pool)
    .await?;
    Ok(())
}

pub async fn create_audio_file(
    pool: &SqlitePool,
    session_id: &str,
    filename: &str,
) -> sqlx::Result<String> {
    let file_id = uuid::Uuid::new_v4().to_string();
    let now = OffsetDateTime::now_utc().unix_timestamp();
    sqlx::query(
        r#"
        INSERT INTO audio_files (id, session_id, filename, status, created_at) 
        VALUES (?, ?, ?, 'processing', ?)
        "#,
    )
    .bind(&file_id)
    .bind(session_id)
    .bind(filename)
    .bind(now)
    .execute(pool)
    .await?;
    Ok(file_id)
}

pub async fn log_chunk(
    pool: &SqlitePool,
    file_id: &str,
    packet_num: u32,
    chunk_type: &str,
    size: usize,
) -> sqlx::Result<()> {
    let now = OffsetDateTime::now_utc().unix_timestamp();
    sqlx::query(
        r#"
        INSERT INTO audio_chunks (file_id, packet_num, chunk_type, size_bytes, created_at) 
        VALUES (?, ?, ?, ?, ?)
        "#,
    )
    .bind(file_id)
    .bind(packet_num as i64)
    .bind(chunk_type)
    .bind(size as i64)
    .bind(now)
    .execute(pool)
    .await?;
    Ok(())
}

pub async fn finish_audio_file(pool: &SqlitePool, file_id: &str) -> sqlx::Result<()> {
    let now = OffsetDateTime::now_utc().unix_timestamp();
    sqlx::query(
        r#"
        UPDATE audio_files 
        SET status = 'completed', finished_at = ? 
        WHERE id = ?
        "#,
    )
    .bind(now)
    .bind(file_id)
    .execute(pool)
    .await?;
    Ok(())
}
