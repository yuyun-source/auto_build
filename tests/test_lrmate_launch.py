"""Check launch routing without requiring a local ROS installation."""
import ast
from ipaddress import IPv4Address
from pathlib import Path
import tempfile
import unittest


class Configuration:
    def __init__(self, name):
        self.name = name

    def perform(self, context):
        return context[self.name]


class LaunchRoutingTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for name in (
            "fanuc_hardware_interface/launch/fanuc_mock_control.launch.py",
            "fanuc_hardware_interface/launch/fanuc_physical_control.launch.py",
            "fanuc_hardware_interface/robot/6dof_robot.urdf.xacro",
            "fanuc_lrmate_description/urdf/lrmate200id_urdf_macro.xacro",
        ):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        source = Path(__file__).resolve().parents[1] / (
            "src/auto_build_bringup/launch/lrmate200id.launch.py")
        tree = ast.parse(source.read_text(encoding="utf-8"))
        function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                        and n.name == "launch_setup")
        namespace = dict(
            Path=Path, IPv4Address=IPv4Address, LaunchConfiguration=Configuration,
            get_package_share_directory=lambda name: str(self.root / name),
            PythonLaunchDescriptionSource=lambda path: path,
            IncludeLaunchDescription=lambda path, launch_arguments:
                (path, dict(launch_arguments)),
        )
        exec(compile(ast.Module(body=[function], type_ignores=[]), str(source), "exec"), namespace)
        self.launch = namespace["launch_setup"]

    def test_mock_needs_no_ip_and_never_selects_physical(self):
        path, args = self.launch(dict(use_mock="true", robot_ip=""))[0]
        self.assertTrue(path.endswith("fanuc_mock_control.launch.py"))
        self.assertEqual(args["robot_model"], "lrmate200id")
        self.assertEqual(args["robot_series"], "lrmate")
        self.assertNotIn("robot_ip", args)

    def test_physical_routes_ip_and_controller(self):
        path, args = self.launch(dict(use_mock="false", robot_ip="192.168.1.10"))[0]
        self.assertTrue(path.endswith("fanuc_physical_control.launch.py"))
        self.assertEqual(args["robot_ip"], "192.168.1.10")
        self.assertEqual(args["initial_controller"], "joint_trajectory_controller")

    def test_physical_rejects_missing_or_invalid_ip(self):
        for value in ("", "invalid", "999.1.1.1"):
            with self.subTest(value=value), self.assertRaises((RuntimeError, ValueError)):
                self.launch(dict(use_mock="false", robot_ip=value))

    def test_missing_model_fails_before_driver_start(self):
        (self.root / "fanuc_lrmate_description/urdf/lrmate200id_urdf_macro.xacro").unlink()
        with self.assertRaisesRegex(RuntimeError, "Missing"):
            self.launch(dict(use_mock="true", robot_ip=""))


if __name__ == "__main__":
    unittest.main()
