use crate::storage_client::StorageClient;
use bytes::Bytes;
use std::sync::Arc;

#[derive(Clone)]
pub struct StorageService {
    client: Arc<StorageClient>,
}

impl StorageService {
    pub fn new(base_url: &str) -> Self {
        Self {
            client: Arc::new(StorageClient::new(base_url)),
        }
    }

    pub async fn save_chunk(&self, file_id: &str, packet_num: u32, data: &[u8]) -> Result<(), anyhow::Error> {
        let key = format!("chunk_{}_{:06}", file_id, packet_num);
        self.client.save(&key, Bytes::copy_from_slice(data)).await
    }

    pub async fn load_all_chunks(&self, file_id: &str) -> Result<Vec<u8>, anyhow::Error> {
        let mut result = Vec::new();
        let mut packet_num = 1;
        loop {
            let key = format!("chunk_{}_{:06}", file_id, packet_num);
            match self.client.get(&key).await? {
                Some(data) => {
                    result.extend_from_slice(&data);
                    packet_num += 1;
                }
                None => break,
            }
        }
        Ok(result)
    }
}
