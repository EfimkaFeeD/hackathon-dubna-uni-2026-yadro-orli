use uuid::Uuid;

pub fn generate_session_id() -> String {
    format!("sess_{}", Uuid::new_v4())
}

pub fn validate_session_id(id: &str) -> bool {
    id.starts_with("sess_") && id.len() > 10 && Uuid::parse_str(&id[5..]).is_ok()
}

