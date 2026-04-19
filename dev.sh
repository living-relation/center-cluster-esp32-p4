#!/usr/bin/env bash

set -euo pipefail

IDF_HELPER="${IDF_HELPER:-/home/ubuntu/.esp/use-idf54.sh}"
DEFAULT_PORT_GLOBS=(
  "/dev/ttyUSB*"
  "/dev/ttyACM*"
  "/dev/cu.usb*"
  "/dev/tty.usb*"
)

print_usage() {
  echo "Usage: $0 [--port <serial-port>] [--build-only]"
  echo
  echo "Examples:"
  echo "  $0"
  echo "  $0 --port /dev/ttyUSB0"
  echo "  $0 --build-only"
}

detect_port() {
  local glob
  local candidate
  for glob in "${DEFAULT_PORT_GLOBS[@]}"; do
    for candidate in $glob; do
      if [ -e "$candidate" ]; then
        echo "$candidate"
        return 0
      fi
    done
  done
  return 1
}

PORT=""
BUILD_ONLY="0"

while [ "$#" -gt 0 ]; do
  case "$1" in
    --port)
      if [ "$#" -lt 2 ]; then
        echo "Error: --port requires a value." >&2
        print_usage
        exit 1
      fi
      PORT="$2"
      shift 2
      ;;
    --build-only)
      BUILD_ONLY="1"
      shift
      ;;
    -h|--help)
      print_usage
      exit 0
      ;;
    *)
      echo "Error: unknown argument '$1'." >&2
      print_usage
      exit 1
      ;;
  esac
done

if [ ! -f "$IDF_HELPER" ]; then
  echo "Error: ESP-IDF helper script not found at '$IDF_HELPER'." >&2
  echo "Run VM setup first or set IDF_HELPER to a valid script path." >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$IDF_HELPER"

if [ "$BUILD_ONLY" = "1" ]; then
  echo "Running build only..."
  exec idf.py build
fi

if [ -z "$PORT" ]; then
  if ! PORT="$(detect_port)"; then
    echo "Error: No serial device found automatically." >&2
    echo "Connect your board or pass --port <serial-port>." >&2
    exit 1
  fi
fi

echo "Using serial port: $PORT"
exec idf.py -p "$PORT" flash monitor
