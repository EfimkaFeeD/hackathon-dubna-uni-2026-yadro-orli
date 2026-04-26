from debian:bookworm-slim AS builder

run apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    wget \
    ca-certificates \
    libasio-dev \
    && rm -rf /var/lib/apt/lists/*

run wget -q -L -O /usr/local/include/crow.h \
    https://github.com/CrowCpp/Crow/releases/download/v1.2.0/crow_all.h

workdir /build
copy . .

run mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

from debian:bookworm-slim

run apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

workdir /app

copy --from=builder /build/build/storage_service .

expose 8081

cmd ["./storage_service"]