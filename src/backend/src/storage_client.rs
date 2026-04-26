use bytes::Bytes;
use reqwest::Client;

pub struct StorageClient {
    client: Client,
    base_url: String,
}

impl StorageClient {
    pub fn new(base_url: &str) -> Self {
        Self {
            client: Client::new(),
            base_url: base_url.to_string(),
        }
    }

    pub async fn save(&self, key: &str, data: Bytes) -> Result<(), anyhow::Error> {
        let url = format!("{}/storage/save/{}", self.base_url, key);
        let response: reqwest::Response = self.client.post(&url).body(data).send().await?;  // ← добавлен тип
        if response.status().is_success() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("Storage save failed: {}", response.status()))
        }
    }

    pub async fn get(&self, key: &str) -> Result<Option<Bytes>, anyhow::Error> {
        let url = format!("{}/storage/get/{}", self.base_url, key);
        let response: reqwest::Response = self.client.get(&url).send().await?;  // ← добавлен тип
        if response.status().is_success() {
            Ok(Some(response.bytes().await?))
        } else if response.status() == reqwest::StatusCode::NOT_FOUND {
            Ok(None)
        } else {
            Err(anyhow::anyhow!("Storage get failed: {}", response.status()))
        }
    }

    pub async fn delete(&self, key: &str) -> Result<(), anyhow::Error> {
        let url = format!("{}/storage/delete/{}", self.base_url, key);
        let response: reqwest::Response = self.client.delete(&url).send().await?;  // ← добавлен тип
        if response.status().is_success() {
            Ok(())
        } else {
            Err(anyhow::anyhow!("Storage delete failed: {}", response.status()))
        }
    }
}
