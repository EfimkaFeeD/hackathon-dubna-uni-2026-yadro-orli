use bytes::Bytes;
use reqwest::Client;
use serde_json::Value;

#[derive(Clone)]
pub struct StorageHttpClient {
    client: Client,
    base_url: String,
}

impl StorageHttpClient {
    pub fn new(base_url: &str) -> Self {
        Self {
            client: Client::builder()
                .pool_max_idle_per_host(10)
                .timeout(std::time::Duration::from_secs(30))
                .build()
                .expect("Failed to build HTTP client"),
            base_url: base_url.to_string(),
        }
    }

    pub async fn put_chunk(&self, file_id: u64, key: &str, data: Bytes) -> Result<(), anyhow::Error> {
        let url = format!("{}/v1/objects/put/{}/{}", self.base_url, file_id, key);
        let response = self.client.put(&url).body(data).send().await?;
        match response.status() {
            reqwest::StatusCode::CREATED => Ok(()),
            status => Err(anyhow::anyhow!("Storage put failed: {}", status)),
        }
    }

    pub async fn get_chunk(&self, file_id: u64, key: &str) -> Result<Option<Bytes>, anyhow::Error> {
        let url = format!("{}/v1/objects/get/{}/{}", self.base_url, file_id, key);
        let response = self.client.get(&url).send().await?;
        match response.status() {
            reqwest::StatusCode::OK => Ok(Some(response.bytes().await?)),
            reqwest::StatusCode::NOT_FOUND => Ok(None),
            status => Err(anyhow::anyhow!("Storage get failed: {}", status)),
        }
    }

    pub async fn delete_chunk(&self, file_id: u64, key: &str) -> Result<bool, anyhow::Error> {
        let url = format!("{}/v1/objects/delete/{}/{}", self.base_url, file_id, key);
        let response = self.client.delete(&url).send().await?;
        match response.status() {
            reqwest::StatusCode::NO_CONTENT => Ok(true),
            reqwest::StatusCode::NOT_FOUND => Ok(false),
            status => Err(anyhow::anyhow!("Storage delete failed: {}", status)),
        }
    }

    pub async fn delete_all_chunks(&self, file_id: u64) -> Result<bool, anyhow::Error> {
        let url = format!("{}/v1/objects/all/{}", self.base_url, file_id);
        let response = self.client.delete(&url).send().await?;
        match response.status() {
            reqwest::StatusCode::NO_CONTENT => Ok(true),
            reqwest::StatusCode::NOT_FOUND => Ok(false),
            status => Err(anyhow::anyhow!("Storage delete all failed: {}", status)),
        }
    }

    pub async fn list_chunks(&self, file_id: u64) -> Result<Vec<String>, anyhow::Error> {
        let url = format!("{}/v1/objects/list/{}", self.base_url, file_id);
        let response = self.client.get(&url).send().await?;
        if response.status().is_success() {
            let json: Value = response.json().await?;
            let chunks = json["chunks"]
                .as_array()
                .unwrap_or(&vec![])
                .iter()
                .filter_map(|v| v.as_str().map(String::from))
                .collect();
            Ok(chunks)
        } else {
            Err(anyhow::anyhow!("Storage list failed: {}", response.status()))
        }
    }

    pub async fn list_all_files(&self) -> Result<Vec<u64>, anyhow::Error> {
        let url = format!("{}/v1/objects/all", self.base_url);
        let response = self.client.get(&url).send().await?;
        if response.status().is_success() {
            let json: Value = response.json().await?;
            let file_ids = json["file_ids"]
                .as_array()
                .unwrap_or(&vec![])
                .iter()
                .filter_map(|v| v.as_u64())
                .collect();
            Ok(file_ids)
        } else {
            Err(anyhow::anyhow!("Storage list all failed: {}", response.status()))
        }
    }

    pub async fn health_check(&self) -> Result<bool, anyhow::Error> {
        let url = format!("{}/health", self.base_url);
        let response = self.client.get(&url).send().await?;
        Ok(response.status().is_success())
    }
}
