use uuid::Uuid;

pub fn generate_session_id() -> String {
    format!("sess_{}", Uuid::new_v4())
}

pub fn validate_session_id(id: &str) -> bool {
    if !id.starts_with("sess_") || id.len() < 10 {
        return false;
    }

    let uuid_part = &id[5..];
    Uuid::parse_str(uuid_part).is_ok()
}
