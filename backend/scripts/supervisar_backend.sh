#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BACKEND_BINARY="$ROOT_DIR/backend/bin/riego_backend"
CHILD_PID_FILE="$ROOT_DIR/.backend.child.pid"
LOG_DIR="$ROOT_DIR/backend/logs"
STDOUT_LOG="$LOG_DIR/backend.out.log"
STDERR_LOG="$LOG_DIR/backend.err.log"
PORT="${1:-8080}"
STOP_REQUESTED=0
CHILD_PID=""

mkdir -p "$LOG_DIR"

# Compatible with macOS bash 3.2 (no printf '%(%T)T' — that is bash 4+ only)
log_info() {
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$1" >> "$STDOUT_LOG"
}

log_error() {
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$1" >> "$STDERR_LOG"
}

cleanup() {
  STOP_REQUESTED=1

  if [[ -n "${CHILD_PID:-}" ]] && kill -0 "$CHILD_PID" 2>/dev/null; then
    kill "$CHILD_PID" 2>/dev/null || true
    wait "$CHILD_PID" 2>/dev/null || true
  fi

  rm -f "$CHILD_PID_FILE"
}

trap cleanup INT TERM EXIT

while true; do
  log_info "Iniciando backend en puerto $PORT"
  "$BACKEND_BINARY" --port "$PORT" >>"$STDOUT_LOG" 2>>"$STDERR_LOG" &
  CHILD_PID=$!
  echo "$CHILD_PID" > "$CHILD_PID_FILE"

  set +e
  wait "$CHILD_PID"
  EXIT_CODE=$?
  set -e

  rm -f "$CHILD_PID_FILE"

  if [[ "$STOP_REQUESTED" -eq 1 ]]; then
    log_info "Supervisor detenido de forma controlada."
    break
  fi

  log_error "El backend termino inesperadamente con codigo $EXIT_CODE. Reiniciando en 1 segundo."
  sleep 1
done