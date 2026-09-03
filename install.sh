#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "${ROOT_DIR}/config.env" ]]; then
  set -a
  # shellcheck disable=SC1091
  source "${ROOT_DIR}/config.env"
  set +a
fi

ROS_DISTRO="${ROS_DISTRO:-jazzy}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
AUTO_SOURCE_BASHRC="${AUTO_SOURCE_BASHRC:-0}"
export DEBIAN_FRONTEND=noninteractive

log() { printf '\n\033[1;32m==> %s\033[0m\n' "$*"; }
die() { printf '\n错误：%s\n' "$*" >&2; exit 1; }
trap 'printf "\n安装失败（第 %s 行）。修复问题后可直接重新运行 ./install.sh。\n" "$LINENO" >&2' ERR

[[ "$(uname -s)" == Linux ]] || die "此安装器仅支持 Linux。"
[[ -r /etc/os-release ]] || die "无法识别操作系统。"
# shellcheck disable=SC1091
source /etc/os-release
[[ "${ID:-}" == ubuntu && "${VERSION_ID:-}" == 24.04 ]] || \
  die "需要 Ubuntu 24.04，当前为 ${PRETTY_NAME:-未知系统}。"
[[ "${ROS_DISTRO}" == jazzy ]] || die "Ubuntu 24.04 部署包固定支持 ROS_DISTRO=jazzy。"

log "安装基础工具并启用 Ubuntu Universe"
sudo apt-get update
sudo apt-get install -y locales software-properties-common curl ca-certificates gnupg git
sudo add-apt-repository -y universe
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

log "配置 ROS 2 官方 apt 源"
if ! dpkg-query -W -f='${Status}' ros2-apt-source 2>/dev/null | grep -q "ok installed"; then
  ROS_APT_SOURCE_VERSION="$(curl -fsSL https://api.github.com/repos/ros-infrastructure/ros-apt-source/releases/latest | sed -n 's/.*"tag_name": *"\([^"]*\)".*/\1/p' | head -n1)"
  [[ -n "${ROS_APT_SOURCE_VERSION}" ]] || die "无法获取 ros2-apt-source 最新版本。"
  ROS_APT_DEB="ros2-apt-source_${ROS_APT_SOURCE_VERSION}.noble_all.deb"
  curl -fL "https://github.com/ros-infrastructure/ros-apt-source/releases/download/${ROS_APT_SOURCE_VERSION}/${ROS_APT_DEB}" -o "/tmp/${ROS_APT_DEB}"
  sudo dpkg -i "/tmp/${ROS_APT_DEB}"
fi

log "安装 ROS 2 Jazzy、构建工具和 Qt 6"
sudo apt-get update
sudo apt-get install -y \
  ros-jazzy-desktop ros-dev-tools python3-rosdep python3-vcstool \
  python3-colcon-common-extensions \
  qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-tools-dev-tools \
  libgl1-mesa-dev libxkbcommon-x11-0 ninja-build

if [[ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]]; then
  log "初始化 rosdep"
  sudo rosdep init
fi
rosdep update --rosdistro "${ROS_DISTRO}"

log "导入 FANUC Driver 源码"
mkdir -p "${ROOT_DIR}/src"
# shellcheck disable=SC1090
source "/opt/ros/${ROS_DISTRO}/setup.bash"
vcs import --skip-existing "${ROOT_DIR}/src" < "${ROOT_DIR}/fanuc_jazzy.repos"
while IFS= read -r -d '' gitmodules; do
  git -C "$(dirname "${gitmodules}")" submodule update --init --recursive
done < <(find "${ROOT_DIR}/src" -name .gitmodules -print0)

log "安装工作空间依赖"
rosdep install --from-paths "${ROOT_DIR}/src" --ignore-src -r -y \
  --rosdistro "${ROS_DISTRO}" --os=ubuntu:noble

log "构建工作空间"
BUILD_TYPE="${BUILD_TYPE}" bash "${ROOT_DIR}/build.sh" --no-rosdep

if [[ "${AUTO_SOURCE_BASHRC}" == 1 ]]; then
  SOURCE_LINE="source \"${ROOT_DIR}/setup.sh\""
  grep -Fqx "${SOURCE_LINE}" "${HOME}/.bashrc" 2>/dev/null || printf '\n%s\n' "${SOURCE_LINE}" >> "${HOME}/.bashrc"
  log "已将工作空间环境加入 ~/.bashrc"
fi

bash "${ROOT_DIR}/scripts/doctor.sh"
log "安装完成。执行：source \"${ROOT_DIR}/setup.sh\""
