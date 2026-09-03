#include "fanuc_hmi/FanucRosBridge.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>

#include <trajectory_msgs/msg/joint_trajectory_point.hpp>

using namespace std::chrono_literals;

namespace
{
constexpr double kPi = 3.14159265358979323846;

double radToDeg(double rad)
{
    return rad * 180.0 / kPi;
}

double degToRad(double deg)
{
    return deg * kPi / 180.0;
}
}

FanucRosBridge::FanucRosBridge(QObject *parent)
    : QObject(parent)
{
    node_ = std::make_shared<rclcpp::Node>("fanuc_hmi_node");

    joint_state_sub_ = node_->create_subscription<sensor_msgs::msg::JointState>(
        "/joint_states",
        rclcpp::SensorDataQoS(),
        [this](sensor_msgs::msg::JointState::SharedPtr msg)
        {
            handleJointState(std::move(msg));
        });

    trajectory_client_ =
        rclcpp_action::create_client<FollowJointTrajectory>(
            node_,
            "/joint_trajectory_controller/follow_joint_trajectory");
}

FanucRosBridge::~FanucRosBridge()
{
    stop();
}

void FanucRosBridge::start()
{
    if (running_.exchange(true))
        return;

    spin_thread_ = std::thread([this]()
    {
        rclcpp::WallRate rate(100.0);

        while (rclcpp::ok() && running_)
        {
            rclcpp::spin_some(node_);

            const bool ready = trajectory_client_->action_server_is_ready();
            if (ready != controller_ready_.exchange(ready))
                emit controllerReadyChanged(ready);

            rate.sleep();
        }
    });
}

void FanucRosBridge::stop()
{
    if (!running_.exchange(false))
        return;

    if (spin_thread_.joinable())
        spin_thread_.join();
}

void FanucRosBridge::handleJointState(
    const sensor_msgs::msg::JointState::SharedPtr msg)
{
    if (msg->name.size() < 6 || msg->position.size() < 6)
        return;

    std::array<double, 6> values{};

    {
        std::lock_guard<std::mutex> lock(joint_mutex_);

        if (joint_names_.empty())
        {
            // The FANUC mock publishes the controlled joints on /joint_states.
            // Store the first six names so commands use the robot's actual names.
            joint_names_.assign(msg->name.begin(), msg->name.begin() + 6);
        }

        // Read positions in the same name order saved above.
        for (std::size_t i = 0; i < 6; ++i)
        {
            auto it = std::find(msg->name.begin(), msg->name.end(), joint_names_[i]);
            if (it == msg->name.end())
                return;

            const auto index = static_cast<std::size_t>(
                std::distance(msg->name.begin(), it));

            if (index >= msg->position.size())
                return;

            values[i] = radToDeg(msg->position[index]);
        }
    }

    if (!received_joint_state_.exchange(true))
        emit connectionChanged(true);

    emit jointStateUpdated(
        values[0], values[1], values[2],
        values[3], values[4], values[5]);
}

std::vector<std::string> FanucRosBridge::getJointNames() const
{
    std::lock_guard<std::mutex> lock(joint_mutex_);
    return joint_names_;
}

void FanucRosBridge::moveHome()
{
    sendJointGoal({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
}

void FanucRosBridge::moveJoints(
    double j1_deg, double j2_deg, double j3_deg,
    double j4_deg, double j5_deg, double j6_deg)
{
    sendJointGoal({
        j1_deg, j2_deg, j3_deg,
        j4_deg, j5_deg, j6_deg
    });
}

void FanucRosBridge::sendJointGoal(
    const std::array<double, 6> &target_deg)
{
    if (!trajectory_client_->action_server_is_ready())
    {
        emit motionStatusChanged(
            QStringLiteral("Trajectory controller 未就绪"), false);
        return;
    }

    const auto names = getJointNames();
    if (names.size() != 6)
    {
        emit motionStatusChanged(
            QStringLiteral("尚未收到有效 /joint_states"), false);
        return;
    }

    FollowJointTrajectory::Goal goal;
    goal.trajectory.joint_names = names;

    trajectory_msgs::msg::JointTrajectoryPoint point;
    point.positions.resize(6);

    for (std::size_t i = 0; i < 6; ++i)
        point.positions[i] = degToRad(target_deg[i]);

    // Five-second point-to-point test motion.
    point.time_from_start.sec = 5;
    point.time_from_start.nanosec = 0;

    goal.trajectory.points.push_back(point);

    rclcpp_action::Client<FollowJointTrajectory>::SendGoalOptions options;

    options.goal_response_callback =
        [this](GoalHandle::SharedPtr goal_handle)
        {
            if (!goal_handle)
            {
                emit motionStatusChanged(
                    QStringLiteral("轨迹目标被控制器拒绝"), false);
                return;
            }

            emit motionStatusChanged(
                QStringLiteral("运动中..."), true);
        };

    options.result_callback =
        [this](const GoalHandle::WrappedResult &result)
        {
            switch (result.code)
            {
            case rclcpp_action::ResultCode::SUCCEEDED:
                emit motionStatusChanged(
                    QStringLiteral("运动完成"), true);
                break;

            case rclcpp_action::ResultCode::ABORTED:
                emit motionStatusChanged(
                    QStringLiteral("运动被中止"), false);
                break;

            case rclcpp_action::ResultCode::CANCELED:
                emit motionStatusChanged(
                    QStringLiteral("运动已取消"), false);
                break;

            default:
                emit motionStatusChanged(
                    QStringLiteral("未知运动结果"), false);
                break;
            }
        };

    emit motionStatusChanged(
        QStringLiteral("正在发送轨迹目标..."), true);

    trajectory_client_->async_send_goal(goal, options);
}
