# fanuc_hmi

Qt6 + ROS 2 HMI for the FANUC `fanuc_driver` mock robot.

Confirmed interfaces used by this package:

- Joint state topic: `/joint_states`
- Trajectory action: `/joint_trajectory_controller/follow_joint_trajectory`
- Action type: `control_msgs/action/FollowJointTrajectory`

## Build

```bash
cd ~/ws_fanuc
source /opt/ros/jazzy/setup.bash
rm -rf build/fanuc_hmi install/fanuc_hmi
colcon build --packages-select fanuc_hmi --symlink-install
source install/setup.bash
```

## Run

Keep the FANUC mock / MoveIt launch running in another terminal. Then:

```bash
cd ~/ws_fanuc
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run fanuc_hmi fanuc_hmi
```

The HMI subscribes to `/joint_states`, discovers the first six joint names,
and sends 5-second joint trajectories to the active FANUC trajectory controller.
