#!/usr/bin/env bash
# PR 6c CI lint: forbid SharedArray/SharedSet reads inside _componentConstructor
# bodies in firefly NetworkIO sources.  Reads must happen at _componentSetup or
# later (after SharedObjectManager's all-published gate releases).  See
# pr-6-design.md §6 and §7 R-01 for rationale.
#
# Exits 0 when clean; non-zero with offending lines otherwise.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FIREFLY_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

TARGETS=()
for stem in nic hadesNetworkIO hadesStorageController nicRecvCtx nicNetworkIOStream; do
    for ext in cc h; do
        f="${FIREFLY_DIR}/${stem}.${ext}"
        [ -f "$f" ] && TARGETS+=("$f")
    done
done

if [ ${#TARGETS[@]} -eq 0 ]; then
    echo "lint_shared_no_ctor_read: no target source files found under ${FIREFLY_DIR}" >&2
    exit 2
fi

# Forbidden read patterns (subscript / find on SharedArray / SharedSet handles).
FORBIDDEN_RE='m_pool\[|m_permit\.find\(|m_permitSnapshot\.find\('

awk -v forbidden="${FORBIDDEN_RE}" '
    FNR == 1 {
        in_ctor = 0
        ctor_brace = 0
    }
    {
        line = $0
        if (in_ctor == 0) {
            if (line ~ /_componentConstructor|ComponentConstructor/) {
                in_ctor = 1
                ctor_brace = 0
            }
        }
        if (in_ctor) {
            n_open = gsub(/\{/, "{", line)
            n_close = gsub(/\}/, "}", line)
            ctor_brace += n_open - n_close
            if (ctor_brace > 0 && line ~ forbidden) {
                printf "%s:%d: %s\n", FILENAME, FNR, $0
                rc = 1
            }
            if (ctor_brace <= 0 && (n_open + n_close) > 0) {
                in_ctor = 0
            }
        }
    }
    END { exit (rc ? 1 : 0) }
' "${TARGETS[@]}"
