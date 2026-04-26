set windows-powershell := true
os_type  := os()
is_win   := if os_type == "windows" { "true" } else { "false" }

mkdir    := if is_win == "true" { "New-Item -ItemType Directory -Force" } else { "mkdir -p" }
cp_env   := if is_win == "true" { "if (!(Test-Path .env)) { Copy-Item .env.example .env }" } else { "cp -n .env.example .env || true" }
rm_rf    := if is_win == "true" { "Remove-Item -Recurse -Force" } else { "rm -rf" }

default:
    @just --list


setup:
    @echo "[INFO] Starting initialization for OS: {{ os_type }}"
    @echo "[1/2] Creating required directories..."
    @{{ mkdir }} data, storage/disk, storage/ram, shared_plugins > {{ if is_win == "true" { "$null" } else { "/dev/null" } }}
    @echo "[2/2] Checking environment configuration (.env)..."
    @{{ cp_env }}
    @echo "[DONE] Project initialization completed."

build-plugin:
    @echo "[INFO] Starting isolated build for C++ plugin..."
    docker-compose up --build plugin-compiler
    @echo "[DONE] Plugin compiled and exported to ./shared_plugins directory."

dev:
    @echo "[INFO] Building and starting all Docker services..."
    docker-compose up -d --build
    @echo "[INFO] Access Point: http://localhost:8080"
    @echo "[INFO] Following backend logs..."
    docker-compose logs -f backend

down:
    @echo "[INFO] Stopping all active services..."
    docker-compose down
    @echo "[DONE] Services stopped."

clean: down
    @echo "[INFO] Starting workspace cleanup..."
    @echo "[PROGRESS] Removing data files and temporary artifacts..."
    @{{ rm_rf }} data/*, storage/disk/*, shared_plugins/*
    @echo "[DONE] Workspace is now clean."