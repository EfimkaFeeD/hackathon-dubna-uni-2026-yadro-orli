set windows-powershell := true

os_type  := os()
is_win   := if os_type == "windows" { "true" } else { "false" }

# Настройка команд
mkdir_cmd := if is_win == "true" { "New-Item -ItemType Directory -Force" } else { "mkdir -p" }
cp_env    := if is_win == "true" { "if (!(Test-Path .env)) { Copy-Item .env.example .env }" } else { "cp -n .env.example .env || true" }
rm_rf     := if is_win == "true" { "Remove-Item -Recurse -Force" } else { "rm -rf" }

# Кроссплатформенное скрытие вывода
null_redir := if is_win == "true" { "| Out-Null" } else { "> /dev/null 2>&1" }

# Списки путей: в Windows нужны запятые, в Linux — пробелы
dirs      := if is_win == "true" { "data, storage/disk, shared_plugins" } else { "data storage/disk shared_plugins" }
rm_paths  := if is_win == "true" { "data/*, storage/disk/*, shared_plugins/*" } else { "data/* storage/disk/* shared_plugins/*" }


default:
    @just --list

setup:
    @echo "[INFO] Starting initialization for OS: {{ os_type }}"
    @echo "[1/2] Creating required directories..."
    @{{ mkdir_cmd }} {{ dirs }} {{ null_redir }}
    @echo "[2/2] Checking environment configuration (.env)..."
    @{{ cp_env }}
    @echo "[DONE] Project initialization completed."

build-ffmpeg:
    @echo "[INFO] Starting isolated build for FFmpeg plugin..."
    docker-compose up --build plugin-compiler
    @echo "[DONE] FFmpeg plugin compiled to ./shared_plugins directory."

build-storage:
    @echo "[INFO] Rebuilding and starting Storage Service on port 8081..."
    docker-compose up -d --build storage-service

run-storage:
    @echo "[INFO] Starting Storage Service..."
    docker-compose up -d storage-service

stop-storage:
    @echo "[INFO] Stopping Storage Service..."
    docker-compose stop storage-service

logs-storage:
    @echo "[INFO] Following Storage Service logs..."
    docker-compose logs -f storage-service

dev:
    @echo "[INFO] Building and starting all Docker services..."
    docker-compose up -d --build
    @echo "[INFO] Backend: http://localhost:8080"
    @echo "[INFO] Storage: http://localhost:8081"
    @echo "[INFO] Following backend logs..."
    docker-compose logs -f backend

down:
    @echo "[INFO] Stopping all active services..."
    docker-compose down
    @echo "[DONE] Services stopped."

clean: down
    @echo "[INFO] Starting workspace cleanup..."
    @{{ rm_rf }} {{ rm_paths }}
    @echo "[DONE] Workspace is now clean."