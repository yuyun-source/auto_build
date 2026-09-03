#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
[[ -f "${ROOT_DIR}/config.env" ]] && { set -a; source "${ROOT_DIR}/config.env"; set +a; }
ROS_DISTRO="${ROS_DISTRO:-jazzy}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
COLCON_WORKERS="${COLCON_WORKERS:-0}"
CLEAN=0
RUN_ROSDEP=1
COLCON_ARGS=()

for arg in "$@"; do
  case "${arg}" in
    --clean) CLEAN=1 ;;
    --no-rosdep) RUN_ROSDEP=0 ;;
    *) COLCON_ARGS+=("${arg}") ;;
  esac
done

[[ -r "/opt/ros/${ROS_DISTRO}/setup.bash" ]] || { echo "请先运行 ./install.sh" >&2; exit 1; }
# shellcheck disable=SC1090
set +u
source "/opt/ros/${ROS_DISTRO}/setup.bash"
set -u

if (( CLEAN )); then
  rm -rf -- "${ROOT_DIR}/build" "${ROOT_DIR}/install" "${ROOT_DIR}/log"
fi

if (( RUN_ROSDEP )); then
  rosdep install --from-paths "${ROOT_DIR}/src" --ignore-src -r -y \
    --rosdistro "${ROS_DISTRO}" --os=ubuntu:noble
fi

PARALLEL_ARGS=()
if [[ "${COLCON_WORKERS}" =~ ^[1-9][0-9]*$ ]]; then
  PARALLEL_ARGS=(--parallel-workers "${COLCON_WORKERS}")
fi

cd "${ROOT_DIR}"
colcon build --symlink-install \
  --cmake-args "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  --event-handlers console_cohesion+ \
  "${PARALLEL_ARGS[@]}" "${COLCON_ARGS[@]}"

echo "构建完成。加载环境：source \"${ROOT_DIR}/setup.sh\""
