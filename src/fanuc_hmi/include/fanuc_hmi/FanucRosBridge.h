#pragma once

#include <QObject>
#include <QString>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <control_msgs/action/follow_joint_trajectory.hpp>

class FanucRosBridge : public QObject
{
    Q_OBJECT

public:
    explicit FanucRosBridge(QObject *parent = nullptr);
    ~FanucRosBridge() override;

    void start();
    void stop();

public slots:
    void moveHome();
    void moveJoints(double j1_deg, double j2_deg, double j3_deg,
                    double j4_deg, double j5_deg, double j6_deg);

signals:
    void jointStateUpdated(double j1_deg, double j2_deg, double j3_deg,
                           double j4_deg, double j5_deg, double j6_deg);
    void connectionChanged(bool connected);
    void controllerReadyChanged(bool ready);
    void motionStatusChanged(const QString &message, bool success);

private:
    using FollowJointTrajectory = control_msgs::action::FollowJointTrajectory;
    using GoalHandle = rclcpp_action::ClientGoalHandle<FollowJointTrajectory>;

    void sendJointGoal(const std::array<double, 6> &target_deg);
    void handleJointState(const sensor_msgs::msg::JointState::SharedPtr msg);
    std::vector<std::string> getJointNames() const;

    rclcpp::Node::SharedPtr node_;
    rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;
    rclcpp_action::Client<FollowJointTrajectory>::SharedPtr trajectory_client_;

    std::thread spin_thread_;
    std::atomic_bool running_{false};
    std::atomic_bool joint_state_connected_{false};
    std::atomic<std::int64_t> last_joint_state_ns_{0};
    std::atomic_bool controller_ready_{false};

    mutable std::mutex joint_mutex_;
    std::vector<std::string> joint_names_;
};
