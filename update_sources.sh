#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
command -v vcs >/dev/null || { echo "缺少 vcs，请先运行 ./install.sh" >&2; exit 1; }
mkdir -p "${ROOT_DIR}/src"
vcs import --skip-existing "${ROOT_DIR}/src" < "${ROOT_DIR}/fanuc_jazzy.repos"
vcs pull "${ROOT_DIR}/src"
while IFS= read -r -d '' gitmodules; do
  git -C "$(dirname "${gitmodules}")" submodule update --init --recursive
done < <(find "${ROOT_DIR}/src" -name .gitmodules -print0)
echo "源码已更新。运行 ./build.sh 重新构建。"
