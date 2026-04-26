from rust:1.93-slim-bookworm as builder

run apt-get update && apt-get install -y \
        libsqlite3-dev \
        pkg-config \
        libssl-dev \
        && rm -rf /var/lib/apt/lists/*

workdir /app

COPY src/backend/Cargo.toml src/backend/Cargo.lock ./

# Кеширование зависимостей
run mkdir src &&\
    echo "fn main() {}" > src/main.rs &&\
    cargo build --release &&\
    rm -rf src

COPY src/backend/src ./src

RUN cargo build --release && \
    strip /app/target/release/backend


from debian:bookworm-slim

run apt-get update && \
        apt-get install -y --no-install-recommends \
        ffmpeg \
        libsqlite3-0 \
        ca-certificates \
        libstdc++6 \
        git autoconf automake libtool build-essential pkg-config \
        && rm -rf /var/lib/apt/lists/*

run git clone https://github.com/dpirch/libfvad.git /tmp/libfvad && \
    cd /tmp/libfvad && \
    autoreconf -i && \
    ./configure --disable-examples && \
    make && make install && \
    ldconfig && \
    rm -rf /tmp/libfvad

workdir /app

run mkdir -p /app/data /app/storage/disk /app/plugins

copy --from=builder /app/target/release/backend /usr/local/bin/backend

env LD_LIBRARY_PATH=/usr/local/lib
env DATABASE_URL="sqlite:///app/data/backend.db"

expose 8080
cmd ["backend"]
