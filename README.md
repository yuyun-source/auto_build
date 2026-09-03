# FANUC HMI 一键环境（Ubuntu 24.04 / ROS 2 Jazzy / Qt 6）

这组文件用于在一台全新的 Ubuntu 24.04 机器上安装 ROS 2 Jazzy、Qt 6、
FANUC 官方 ROS 2 Driver，并构建当前工作空间中的 `fanuc_hmi`。

## 目录约定

```text
auto_build/
├── install.sh                 # 新机器首次执行
├── build.sh                   # 日常构建
├── setup.sh                   # 当前终端加载环境
├── update_sources.sh          # 更新外部源码
├── fanuc_jazzy.repos          # 外部 Git 仓库清单
├── config.env.example         # 可选的本机配置
├── scripts/doctor.sh          # 环境自检
└── src/
    └── fanuc_hmi/             # 把自己的 ROS 2/Qt 包放在这里
```

## 新电脑一键安装

先把整个目录复制或 clone 到 Ubuntu 24.04，然后：

```bash
cd /path/to/auto_build
cp config.env.example config.env       # 可选：按需修改
chmod +x install.sh build.sh setup.sh update_sources.sh scripts/doctor.sh
./install.sh
```

脚本可重复执行。它会：

1. 校验 Ubuntu 24.04；
2. 配置 ROS 官方 apt 源；
3. 安装 ROS 2 Jazzy Desktop、开发工具和 Qt 6；
4. 用 `vcs` 导入 FANUC 官方机器人描述和 Jazzy 驱动；
5. 用 `rosdep` 安装工作空间依赖；
6. 用 `colcon` 构建并运行自检；
7. 可选地将工作空间环境写入 `~/.bashrc`。

安装后新开终端，或执行：

```bash
source ./setup.sh
ros2 pkg list | grep -E 'fanuc|fanuc_hmi'
```

## 日常使用

```bash
./build.sh                  # 更新 rosdep 并增量构建
./build.sh --clean          # 清理 build/install/log 后重新构建
./update_sources.sh         # 更新 fanuc_jazzy.repos 中的外部仓库
./scripts/doctor.sh         # 检查 ROS、Qt、驱动和 HMI 包
```

只构建指定包时，可把 colcon 参数直接传入：

```bash
./build.sh --packages-select fanuc_hmi
```

## 重要说明

- `fanuc_hmi` 源码位于 `src/fanuc_hmi`；Qt 6 系统依赖由本安装包安装。
- 默认导入 FANUC CORPORATION 官方 `fanuc_driver` 的 `main` 分支；该分支面向
  ROS 2 Jazzy。若使用公司的 fork，在 `fanuc_jazzy.repos` 中替换 URL/version。
- 真机连接仍需按机器人型号配置 FANUC 控制器、RMI/网络及机器人描述文件；
  本脚本只完成 Ubuntu 端的软件环境和编译。
- 默认不自动修改 `~/.bashrc`。设置 `AUTO_SOURCE_BASHRC=1` 后再运行
  `./install.sh` 可启用。

参考：[ROS 2 Jazzy Ubuntu 安装文档](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)、
[FANUC ROS 2 Driver](https://github.com/FANUC-CORPORATION/fanuc_driver)。
