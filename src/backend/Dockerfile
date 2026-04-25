from rust:1.93-slim-bookworm as builder

run apt-get update && apt-get install -y\
    libsqlite3-dev\
    && rm -rf /var/lib/apt/lists/*

workdir /app

copy Cargo.toml Cargo.lock ./

# Кеширование зависимостей
run mkdir src &&\
    echo "fn main() {}" > src/main.rs &&\
    cargo build --release &&\
    rm -rf src

copy src ./src
copy .sqlx ./.sqlx

env SQLX_OFFLINE=true

run cargo build --release


from debian:bookworm-slim

run apt-get update && \
    apt-get install -y \
    ffmpeg libsqlite3-0 ca-certificates && \
    rm -rf /var/lib/apt/lists/* \

workdir /app

run mkdir -p /app/data /app/storage/disk /app/storage/ram /app/plugins

copy --from=builder /app/target/release/backend /usr/local/bin/backend

env LD_LIBRARY_PATH=/usr/local/lib
env DATABASE_URL=""

expose 8080
cmd["backend"]