#!/usr/bin/env bash
# 必须 source 本文件：source ./setup.sh

_fanuc_setup_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
_fanuc_ros_distro="${ROS_DISTRO:-jazzy}"

if [[ ! -r "/opt/ros/${_fanuc_ros_distro}/setup.bash" ]]; then
  echo "错误：未找到 /opt/ros/${_fanuc_ros_distro}/setup.bash，请先运行 ./install.sh" >&2
  return 1 2>/dev/null || exit 1
fi

# shellcheck disable=SC1090
_fanuc_nounset_was_on=0
case $- in *u*) _fanuc_nounset_was_on=1; set +u ;; esac
source "/opt/ros/${_fanuc_ros_distro}/setup.bash"
if [[ -r "${_fanuc_setup_dir}/install/setup.bash" ]]; then
  # shellcheck disable=SC1091
  source "${_fanuc_setup_dir}/install/setup.bash"
fi
(( _fanuc_nounset_was_on )) && set -u

export FANUC_HMI_WS="${_fanuc_setup_dir}"
unset _fanuc_setup_dir _fanuc_ros_distro _fanuc_nounset_was_on
