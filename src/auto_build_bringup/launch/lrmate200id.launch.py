"""Launch the same LR Mate model with mock or physical ros2_control hardware."""

from ipaddress import IPv4Address
from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def launch_setup(context):
    use_mock = LaunchConfiguration("use_mock").perform(context) == "true"
    robot_ip = LaunchConfiguration("robot_ip").perform(context).strip()
    if not use_mock:
        if not robot_ip:
            raise RuntimeError("Physical mode requires robot_ip:=<controller IPv4 address>")
        IPv4Address(robot_ip)

    driver = Path(get_package_share_directory("fanuc_hardware_interface"))
    description = Path(get_package_share_directory("fanuc_lrmate_description"))
    mode = "mock" if use_mock else "physical"
    entry = driver / "launch" / f"fanuc_{mode}_control.launch.py"
    for required in (
        entry,
        driver / "robot" / "6dof_robot.urdf.xacro",
        description / "urdf" / "lrmate200id_urdf_macro.xacro",
    ):
        if not required.is_file():
            raise RuntimeError(f"Missing {required}; update FANUC sources and rebuild auto_build")

    arguments = {
        "robot_series": "lrmate",
        "robot_model": "lrmate200id",
        "launch_rviz": LaunchConfiguration("launch_rviz"),
    }
    if not use_mock:
        arguments.update(
            robot_ip=robot_ip,
            initial_controller="joint_trajectory_controller",
        )
    return [IncludeLaunchDescription(
        PythonLaunchDescriptionSource(str(entry)),
        launch_arguments=arguments.items(),
    )]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument("use_mock", default_value="true", choices=["true", "false"]),
        DeclareLaunchArgument("robot_ip", default_value="", description="Required for physical mode"),
        DeclareLaunchArgument("launch_rviz", default_value="true", choices=["true", "false"]),
        OpaqueFunction(function=launch_setup),
    ])
