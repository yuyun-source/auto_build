# FANUC HMI 一键环境

适用于 Ubuntu 24.04、ROS 2 Jazzy 和 Qt 6。本仓库本身就是 ROS 2 工作空间，
可在新系统上自动安装依赖、下载 FANUC 源码并构建 `fanuc_hmi`。

## 仓库结构

```text
auto_build/
├── install.sh                 # 新系统首次安装、下载源码并构建
├── build.sh                   # 日常增量或清理构建
├── setup.sh                   # 加载 ROS 2 与本工作空间环境
├── update_sources.sh          # 更新外部源码
├── fanuc_jazzy.repos          # 外部源码地址和版本
├── config.env.example         # 可选本机配置示例
├── scripts/
│   └── doctor.sh              # 环境与构建结果自检
└── src/
    └── fanuc_hmi/             # 自有 ROS 2/Qt HMI 包
```

首次安装时，脚本还会在 `src/` 中下载：

```text
src/
├── fanuc_hmi/
├── fanuc_driver/
└── fanuc_description/
```

两个外部源码仓库来自 `yuyun-source` 账号下的 fork，不提交进本仓库。

## 新 Ubuntu 一键安装

系统必须是 Ubuntu 24.04，并能访问 Ubuntu、ROS 和 GitHub 软件源。

```bash
cd ~
git clone git@github.com:yuyun-source/auto_build.git
cd auto_build
./install.sh
source setup.sh
```

如果新电脑尚未配置 GitHub SSH key，可以使用 HTTPS：

```bash
git clone https://github.com/yuyun-source/auto_build.git
```

`install.sh` 会依次：

1. 校验 Ubuntu 24.04 和 ROS 2 Jazzy；
2. 配置 ROS 2 apt 软件源；
3. 安装 ROS 2、Qt 6、colcon、rosdep、vcstool 和 Git LFS；
4. 下载 FANUC Driver、机器人描述及其 Git submodule；
5. 使用 rosdep 安装工作空间依赖；
6. 使用 colcon 构建完整工作空间；
7. 运行环境自检。

脚本可以重复执行。安装中断后，修复对应问题并重新执行 `./install.sh` 即可，
一般不需要删除工作空间或重新安装系统。

## 安装结果检查

```bash
cd ~/auto_build
source setup.sh
./scripts/doctor.sh

ros2 pkg prefix fanuc_hmi
ros2 pkg prefix fanuc_hardware_interface
ros2 pkg prefix fanuc_crx_description
```

三个 `ros2 pkg prefix` 命令都应返回 `~/auto_build/install/` 中的路径。

## 启动 CRX-10iA Mock

在第一个终端运行：

```bash
cd ~/auto_build
source setup.sh
ros2 launch fanuc_moveit_config fanuc_moveit.launch.py \
  robot_model:=crx10ia \
  use_mock:=true
```

保持该终端运行。停止时使用 `Ctrl+C`，不要使用会暂停进程的 `Ctrl+Z`。

## 连接真实机器人

连接真机与 Mock 相比，必须先完成网络配置和真实硬件连接验证。首次联调应在
FANUC 专业人员或经过授权的现场人员指导下进行，不要直接发送运动目标。

### 1. 配置同网段静态 IP

使用专用以太网线连接 Ubuntu 电脑网卡和 FANUC 控制柜。电脑与控制柜必须在
同一子网，但不能使用相同 IP。例如控制柜地址是 `192.168.1.10` 时，可以把
Ubuntu 对应网卡配置为：

```text
Ubuntu PC                 FANUC 控制柜
192.168.1.100             192.168.1.10
子网掩码 255.255.255.0     子网掩码 255.255.255.0
```

先查看网卡及 NetworkManager 连接名称：

```bash
nmcli device status
nmcli connection show
```

假设连接 FANUC 控制柜的连接名称是 `Wired connection 1`，执行：

```bash
sudo nmcli connection modify "Wired connection 1" \
  ipv4.method manual \
  ipv4.addresses 192.168.1.100/24 \
  ipv4.gateway "" \
  ipv4.dns ""

sudo nmcli connection down "Wired connection 1"
sudo nmcli connection up "Wired connection 1"
```

必须把 `Wired connection 1` 替换成实际连接控制柜的连接名称。不要修改用于
访问互联网的另一块网卡。`192.168.1.100/24` 中的 `/24` 等同于子网掩码
`255.255.255.0`；专用直连网卡通常不需要设置网关和 DNS。

配置完成后检查网卡地址、路由，并在控制柜允许 ICMP 的情况下测试网络：

```bash
ip -br addr
ip route
ping -c 4 192.168.1.10
```

如果需要把这条连接恢复为自动获取 IP：

```bash
sudo nmcli connection modify "Wired connection 1" \
  ipv4.method auto \
  ipv4.addresses ""
sudo nmcli connection down "Wired connection 1"
sudo nmcli connection up "Wired connection 1"
```

### 2. 启动 ROS 2 真机驱动并验证连接

先确认机器人型号、控制柜 IP、所需 FANUC 软件选项及安全条件。控制柜应清除
报警，按现场要求设置运行模式和示教器状态，机械臂周围必须没有人员或障碍物。

以 CRX-10iA、控制柜 IP `192.168.1.10` 为例：

```bash
cd ~/auto_build
source setup.sh
ros2 launch fanuc_moveit_config fanuc_moveit.launch.py \
  robot_model:=crx10ia \
  robot_ip:=192.168.1.10
```

这条命令启动 ROS 2 真机驱动并连接控制柜，本身不代表已经安全地执行机械臂
运动。保持该终端运行，在另一个终端先验证只读状态和控制器接口：

```bash
cd ~/auto_build
source setup.sh
ros2 topic echo /joint_states --once
ros2 control list_controllers
ros2 action list | grep follow_joint_trajectory
```

当前 HMI 使用的轨迹 Action 是：

```text
/joint_trajectory_controller/follow_joint_trajectory
```

只有在 `/joint_states` 能反映真实姿态、相关控制器为 `active`，并完成风险评估、
限位检查和低速测试准备后，才可以通过 RViz 或 HMI 尝试小幅运动。其他机器人
系列可能还需要传入对应的 `robot_series` 和 `robot_model` 参数。

## 启动 HMI

在第二个终端运行：

```bash
cd ~/auto_build
source setup.sh
ros2 run fanuc_hmi fanuc_hmi
```

HMI 使用以下接口：

```text
/joint_states
/joint_trajectory_controller/follow_joint_trajectory
```

Mock 关闭后，HMI 会在 `/joint_states` 超时后显示 `Disconnected`，轨迹控制器
显示 `Not Ready`。

## 日常构建和源码更新

```bash
cd ~/auto_build
./build.sh                  # 安装缺失依赖并增量构建
./build.sh --clean          # 清理 build/install/log 后重新构建
./build.sh --packages-select fanuc_hmi
./update_sources.sh         # 从配置的 fork 更新外部源码
./scripts/doctor.sh
```

`fanuc_jazzy.repos` 当前使用：

```text
https://github.com/yuyun-source/fanuc_driver.git
https://github.com/yuyun-source/fanuc_description.git
```

GitHub fork 不会自动跟随上游更新。只有确认新版本兼容后，才应主动同步官方
仓库，然后重新执行 `./update_sources.sh` 和 `./build.sh`。

## 可选配置

需要修改构建类型、并发数、代理或自动加载环境时：

```bash
cd ~/auto_build
cp config.env.example config.env
```

编辑 `config.env`。该文件只用于本机并已被 Git 忽略。设置
`AUTO_SOURCE_BASHRC=1` 后重新执行 `./install.sh`，可将环境加载命令写入
`~/.bashrc`。

## 真机说明

Mock 环境不需要连接真实机器人。真机运行还需要根据机器人型号配置 FANUC
控制器软件选项、控制模式、RMI、网络地址、机器人描述和安全条件。本安装器
只负责 Ubuntu 端环境、源码和构建。

参考：

- [ROS 2 Jazzy Ubuntu 安装文档](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
- [FANUC ROS 2 Driver 文档](https://fanuc-corporation.github.io/fanuc_driver_doc/main/index.html)
