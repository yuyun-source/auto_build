# FANUC HMI 一键环境

适用于 Ubuntu 24.04、ROS 2 Jazzy 和 Qt 6。本仓库本身就是 ROS 2 工作空间，
可在新系统上自动安装依赖、下载 FANUC 源码并构建 `fanuc_hmi`。

同一个 HMI 也可用于 Stäubli TX2-60L Mock，操作见下方
[Stäubli Mock 与 HMI 联调](#staubli-mock)。现有一键安装脚本只下载 FANUC
外部源码，Stäubli 需要按该节手动添加。

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

## LR Mate 200iD：Mock 与真机

现场标准型号使用 `lrmate200id`（不适用于 7L 等其他变型）。统一入口为
`auto_build_bringup lrmate200id.launch.py`，自动选择驱动的 Mock 或真机 launch，
固定 `robot_series:=lrmate`。HMI 直接连接轨迹控制器；此入口不启动 MoveIt，
RViz 用于显示模型和关节姿态，不提供路径规划或碰撞检查。

### 更新和构建

```bash
cd ~/auto_build
git pull --ff-only
./build.sh
source setup.sh
ros2 pkg prefix auto_build_bringup
ros2 pkg prefix fanuc_lrmate_description
```

如果提示缺少 `6dof_robot.urdf.xacro` 或 LR Mate 模型，先检查外部源码是否有
本地修改，再用 `./update_sources.sh` 更新并重新构建。不要修改 `install/` 中
生成的文件，也不要仅向 CRX MoveIt launch 的型号列表添加 `lrmate200id`。

### 终端 1：选择 Mock 或真机之一

Mock 不连接控制柜：

```bash
cd ~/auto_build
source setup.sh
ros2 launch auto_build_bringup lrmate200id.launch.py use_mock:=true
```

驱动还会打开滑块窗口，使用 HMI 时不要同时通过滑块发送命令。

真机模式连接现场控制柜，启动前停止 Mock 和旧 HMI：

```bash
cd ~/auto_build
source setup.sh
read -r -p '控制柜 IPv4 地址: ' ROBOT_IP
ros2 launch auto_build_bringup lrmate200id.launch.py \
  use_mock:=false robot_ip:="$ROBOT_IP"
```

真机模式必须指定 IP，不会因连接失败自动退回 Mock。它会启动真机驱动并激活
`joint_trajectory_controller`，不会由本项目主动发送测试运动目标。

电脑与控制柜 IP 连通是网络条件；当前 FANUC 驱动还要求控制柜支持 Stream
Motion 和 Remote Motion。官方要求 J519 + R912，或包含两者的 S636；支持的
控制柜及最低软件版本见 [驱动系统要求](https://fanuc-corporation.github.io/fanuc_driver_doc/main/docs/environment/system_requirements.html)。
这些功能不能通过修改电脑端 launch 补出来。控制柜还需按
[FANUC 驱动文档](https://fanuc-corporation.github.io/fanuc_driver_doc/main/index.html)
完成对应版本的外部控制设置。

### 终端 2：先检查反馈，再操作 HMI

```bash
cd ~/auto_build
source setup.sh
ros2 control list_controllers
ros2 topic echo /joint_states --once
ros2 action info /joint_trajectory_controller/follow_joint_trajectory
ros2 run fanuc_hmi fanuc_hmi
```

两个控制器 `joint_state_broadcaster`、`joint_trajectory_controller` 应为
`active`，Action 应有一个服务端。真机时先将六轴反馈与示教器姿态核对，
HMI 中点击“当前值 → 目标值”，再按现场允许的低速、小幅运动方式测试。
不要将界面初始全零值直接作为真机目标，也不要使用未确认的 `Home (0°)`。

当前 HMI 发送 5 秒关节目标，界面 ±360° 是输入范围，不是 LR Mate 的实际
关节限位；目标必须符合模型和现场限位。反馈超时会阻止新目标发送，但这不等于
取消正在执行的运动，软件连接状态也不等于安全停止；现场停止使用控制柜的
停止/急停装置。Mock 验证只能证明接口流程，不能替代真机联调。

若真机启动失败，查看终端 1 中最早出现的连接或协议错误：

| 现象 | 下一步 |
| --- | --- |
| 参数不接受 `lrmate200id` | 确认使用本节入口，而非 CRX 的 `fanuc_moveit.launch.py`。 |
| 找不到模型或 launch | 更新并构建外部 FANUC 源码，检查 `ros2 pkg prefix` 是否指向当前工作空间。 |
| IP 能 ping 通但驱动连接失败 | 检查控制柜通信功能、软件版本、外部控制设置及端口网络连通性；ping 不验证运动协议。 |
| 有反馈但控制器未激活 | 检查驱动和控制器启动日志，确认真机没有报警或外部控制条件未满足。 |
| HMI 拒绝发送，提示反馈断开 | 恢复持续有效的关节反馈；不要使用缓存姿态继续发送。 |

## 启动 CRX-10iA Mock（其他型号示例）

在第一个终端运行：

```bash
cd ~/auto_build
source setup.sh
ros2 launch fanuc_moveit_config fanuc_moveit.launch.py \
  robot_model:=crx10ia \
  use_mock:=true
```

保持该终端运行。停止时使用 `Ctrl+C`，不要使用会暂停进程的 `Ctrl+Z`。

<a id="staubli-mock"></a>

## Stäubli Mock 与 HMI 联调

本节目标是跑通以下链路：

```text
Qt HMI → FollowJointTrajectory → joint_trajectory_controller
       → Stäubli Mock Hardware → /joint_states → HMI / RViz
```

以下命令在 Ubuntu 24.04 / ROS 2 Jazzy 的 Bash 终端执行，假设本仓库位于
`~/auto_build`；如实际路径不同，请统一替换。首次使用新系统，先完成上面的
一键安装。此流程是 Mock 联调，不需要机器人 IP，也不需要另外启动 MoveIt。

### 1. 下载驱动、安装依赖并构建

```bash
cd ~/auto_build
git clone https://github.com/yuyun-source/staubli_driver_ros2.git src/staubli_driver_ros2

source /opt/ros/jazzy/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install
source ./install/setup.bash

ros2 pkg list | grep staubli
ros2 pkg prefix staubli_bringup
ros2 pkg prefix fanuc_hmi
```

如果 `src/staubli_driver_ros2` 已存在，跳过 clone。依赖安装有报错时应先解决，
再继续构建。构建成功后应能找到 `staubli_bringup` 等 Stäubli 包，两个 prefix
应指向当前 `auto_build/install/`。Stäubli 源码独立保留在它自己的 Git 仓库中，
不要将其作为嵌套仓库提交到本仓库；当前 `update_sources.sh` 不负责更新它。

所有运行终端统一使用 `auto_build` 工作空间环境。不要再加载其他工作空间的
`install/setup.bash`，否则同名包可能来自旧工作空间。
若已经混用，打开干净终端，并检查 `~/.bashrc` 是否自动加载旧环境。

### 2. 终端 1：启动 Stäubli Mock

先用 `Ctrl+C` 停止原来的 FANUC launch 和 HMI，再执行：

```bash
cd ~/auto_build
source setup.sh
ros2 launch staubli_bringup launch_robot_control.launch.py \
  robot_model:=tx2_60l \
  use_mock_hardware:=true \
  start_controller:=joint_trajectory_controller \
  gui:=true
```

保持终端 1 运行，RViz 应显示 Stäubli 模型。必须显式指定
`start_controller:=joint_trajectory_controller`，确保用于 HMI 的轨迹控制器激活。
不要同时启动使用相同话题和 Action 名称的 FANUC 与 Stäubli 实例。

### 3. 终端 2：检查接口并发送测试轨迹

```bash
cd ~/auto_build
source setup.sh
ros2 control list_controllers
ros2 topic echo /joint_states --once
ros2 action list -t
ros2 action info /joint_trajectory_controller/follow_joint_trajectory
```

继续前确认：

- `joint_state_broadcaster` 和 `joint_trajectory_controller` 均为 `active`。
- `/joint_states` 包含 `joint_1` 至 `joint_6` 及对应的 `position`。
- Action 为 `/joint_trajectory_controller/follow_joint_trajectory`，类型为
  `control_msgs/action/FollowJointTrajectory`，且 `Action servers: 1`。

确认当前运行的是 Mock 后，在终端 2 发送一次测试目标：

```bash
ros2 action send_goal \
  /joint_trajectory_controller/follow_joint_trajectory \
  control_msgs/action/FollowJointTrajectory \
  "{
    trajectory: {
      joint_names: ['joint_1', 'joint_2', 'joint_3', 'joint_4', 'joint_5', 'joint_6'],
      points: [
        {
          positions: [0.2, -0.3, 0.4, 0.0, 0.3, 0.0],
          time_from_start: {sec: 3}
        }
      ]
    }
  }"

ros2 topic echo /joint_states --once
```

这里的角度单位为 **弧度（rad）**，目标时间为 3 秒。预期目标被接受并成功完成
（`SUCCEEDED`，结果 `error_code: 0`），RViz 姿态变化，关节状态按名称对应接近
目标值。若实际关节名不同，必须以当前控制器配置和关节状态为准。

### 4. 终端 3：启动 HMI 并验证闭环

首次完整构建已包含 HMI。只有修改 HMI 源码后，才需要先增量构建：

```bash
cd ~/auto_build
source /opt/ros/jazzy/setup.bash
colcon build --packages-select fanuc_hmi --symlink-install
```

在终端 3 启动：

```bash
cd ~/auto_build
source setup.sh
ros2 run fanuc_hmi fanuc_hmi
```

HMI 应显示 `Connected` 和 `Ready`。核对六轴顺序后，输入一个 Mock 测试目标，
例如 J1～J6 为 `[5, 0, 0, 0, 0, 0]`，点击 `Move`。HMI 输入和显示单位为
**度（°）**，当前代码将目标转换成弧度并发送 5 秒轨迹。

验收标准：HMI 显示运动完成，RViz 姿态同步变化，`/joint_states` 更新，HMI
反馈角度接近所填目标。停止 Mock 后，HMI 应在关节状态超时后显示
`Disconnected`，Action 服务消失后显示 `Not Ready`。

### 5. FANUC 与 Stäubli 切换时，HMI 是否需要改代码

当前 `FanucRosBridge.cpp` 已订阅 `/joint_states`，使用上述标准轨迹 Action，
首次接收有效状态时保存前六个关节名，后续按名称匹配位置并发送目标。因此，
在这六个名称恰好是控制器接受的六轴、接口名称一致且控制器激活时，可以复用
当前 HMI 控制逻辑。包名仍为 `fanuc_hmi`，节点名仍为 `fanuc_hmi_node`；
通用命名和关节日志属于后续可选改进，当前代码尚未实施。

切换流程是：停止 HMI 和旧机器人 launch → 启动目标机器人的 launch →
检查控制器、话题和 Action → 重新启动 HMI。**必须重启 HMI**，因为它不会在
切换机器人后自动清空首次保存的关节名。若还有夹爪等额外关节，不能假设消息
前六项就是机械臂六轴，应先调整关节选择逻辑。

当前 `Home` 发送六轴全零目标；这只是测试目标，不代表真实机器人的安全回零
姿态。真机接入还需单独核对对应驱动、控制柜和机器人模型配置，不能仅把本节
的 Mock 参数改成 false 就视为完成真机联调。

### 常见问题

| 现象 | 检查与处理 |
| --- | --- |
| 找不到 `staubli_bringup` | 确认源码位于 `src/staubli_driver_ros2`、构建成功，并重新 `source setup.sh`。 |
| 轨迹控制器为 `inactive` | 检查 launch 的 `start_controller` 参数；已加载时可执行 `ros2 control set_controller_state joint_trajectory_controller active`。 |
| HMI 显示 `Not Ready` | 检查轨迹控制器和 Action server；仅有 RViz 窗口不代表控制器已经就绪。 |
| HMI 显示 `Disconnected` | 检查 `/joint_states` 是否持续发布、是否包含六个有效关节；各终端应使用相同 `ROS_DOMAIN_ID`。 |
| 目标被拒绝或中止 | 查看 launch 日志及 Action 的错误结果，核对关节名、目标限位和控制器状态。 |
| 运行到旧版 HMI | 用 `ros2 pkg prefix fanuc_hmi` 检查来源，清理旧工作空间的自动 source 配置后重新开终端。 |
| 切换机器人后关节不匹配 | 停止两个机器人实例和 HMI，仅启动一个 Mock，并重新启动 HMI。 |

驱动安装和型号参数以 [Stäubli 驱动仓库](https://github.com/yuyun-source/staubli_driver_ros2)
为准；若版本变化，可运行 `ros2 launch staubli_bringup launch_robot_control.launch.py --show-args`
查看本机安装版本的参数。

## 连接真实机器人（FANUC）

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
