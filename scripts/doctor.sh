#!/usr/bin/env bash
set -uo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
ROS_DISTRO="${ROS_DISTRO:-jazzy}"
FAILURES=0
WARNINGS=0

ok() { printf '[ OK ] %s\n' "$*"; }
fail() { printf '[FAIL] %s\n' "$*"; FAILURES=$((FAILURES + 1)); }
warn() { printf '[WARN] %s\n' "$*"; WARNINGS=$((WARNINGS + 1)); }

[[ -r "/opt/ros/${ROS_DISTRO}/setup.bash" ]] && ok "ROS 2 ${ROS_DISTRO}" || fail "ROS 2 ${ROS_DISTRO} 未安装"
command -v qtpaths6 >/dev/null && ok "Qt $(qtpaths6 --qt-version 2>/dev/null)" || fail "Qt 6 开发环境不可用"
command -v colcon >/dev/null && ok "colcon" || fail "colcon 不可用"
command -v rosdep >/dev/null && ok "rosdep" || fail "rosdep 不可用"

if [[ -r "/opt/ros/${ROS_DISTRO}/setup.bash" ]]; then
  # shellcheck disable=SC1090
  set +u
  source "/opt/ros/${ROS_DISTRO}/setup.bash"
  [[ -r "${ROOT_DIR}/install/setup.bash" ]] && source "${ROOT_DIR}/install/setup.bash"
  set -u
  ros2 pkg prefix fanuc_hardware_interface >/dev/null 2>&1 \
    && ok "FANUC Driver 已构建" || fail "未找到 fanuc_hardware_interface"
  ros2 pkg prefix fanuc_hmi >/dev/null 2>&1 \
    && ok "fanuc_hmi 已构建" || warn "未找到 fanuc_hmi；请将源码放入 src/fanuc_hmi 后运行 ./build.sh"
fi

printf '\n自检结束：%d 个错误，%d 个警告。\n' "${FAILURES}" "${WARNINGS}"
(( FAILURES == 0 ))
