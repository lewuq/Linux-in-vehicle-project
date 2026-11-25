#!/bin/sh
set -eu
APP_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-linuxfb}"
export TZ="${TZ:-Asia/Shanghai}"
export VEHICLE_CONFIG="${VEHICLE_CONFIG:-$APP_DIR/vehicle.ini}"
cd "$APP_DIR"
exec "$APP_DIR/VehicleTerminal" "$@"
