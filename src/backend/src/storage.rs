use crate::storage_client::StorageHttpClient;
use bytes::Bytes;
use std::sync::Arc;

#[derive(Clone)]
pub struct StorageService {
    client: Arc<StorageHttpClient>,
}

impl StorageService {
    pub fn new(base_url: &str) -> Self {
        Self {
            client: Arc::new(StorageHttpClient::new(base_url)),
        }
    }

    pub async fn save_chunk(&self, file_id: u64, key: &str, data: &[u8]) -> Result<(), anyhow::Error> {
        self.client.put_chunk(file_id, key, Bytes::copy_from_slice(data)).await
    }

    pub async fn load_chunk(&self, file_id: u64, key: &str) -> Result<Option<Bytes>, anyhow::Error> {
        self.client.get_chunk(file_id, key).await
    }

    pub async fn delete_chunk(&self, file_id: u64, key: &str) -> Result<bool, anyhow::Error> {
        self.client.delete_chunk(file_id, key).await
    }

    pub async fn delete_all_chunks(&self, file_id: u64) -> Result<bool, anyhow::Error> {
        self.client.delete_all_chunks(file_id).await
    }

    pub async fn list_chunks(&self, file_id: u64) -> Result<Vec<String>, anyhow::Error> {
        self.client.list_chunks(file_id).await
    }

    pub async fn list_all_files(&self) -> Result<Vec<u64>, anyhow::Error> {
        self.client.list_all_files().await
    }

    pub async fn health_check(&self) -> Result<bool, anyhow::Error> {
        self.client.health_check().await
    }
}
