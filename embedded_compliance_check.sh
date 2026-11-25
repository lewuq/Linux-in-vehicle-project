#!/bin/sh
# Structural checks supplement, but do not replace, compilation and runtime tests.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$ROOT"
for file in VehicleTerminal.pro img.qrc vehicle.ini.example LICENSE tests/tests.pro; do
    test -f "$file" || { printf 'Missing required file: %s\n' "$file"; exit 1; }
done
if grep -RnE 'VerifyNone|http://|password.*qDebug|qDebug.*password|QEventLoop.*loop' Core Ui Map Monitor Music Weather --include='*.cpp' --include='*.h'; then
    printf 'Review the insecure or synchronous I/O patterns above.\n'
    exit 1
fi
printf 'Structural checks passed. Build and run tests/tests.pro; hardware validation remains required.\n'
