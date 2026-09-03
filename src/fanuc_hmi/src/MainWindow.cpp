#include "fanuc_hmi/MainWindow.h"
#include "fanuc_hmi/FanucRosBridge.h"

#include <QDoubleSpinBox>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(FanucRosBridge *bridge, QWidget *parent)
    : QMainWindow(parent),
      bridge_(bridge)
{
    setWindowTitle(QStringLiteral("FANUC Mock Robot HMI"));
    resize(720, 620);

    auto *central = new QWidget(this);
    auto *main_layout = new QVBoxLayout(central);

    auto *title = new QLabel(QStringLiteral("FANUC Robot HMI"), central);
    QFont title_font = title->font();
    title_font.setPointSize(20);
    title_font.setBold(true);
    title->setFont(title_font);
    main_layout->addWidget(title);

    auto *status_group = new QGroupBox(QStringLiteral("系统状态"), central);
    auto *status_layout = new QGridLayout(status_group);

    status_layout->addWidget(new QLabel(QStringLiteral("ROS /joint_states:")), 0, 0);
    ros_status_ = new QLabel(QStringLiteral("等待连接"), status_group);
    status_layout->addWidget(ros_status_, 0, 1);

    status_layout->addWidget(new QLabel(QStringLiteral("Trajectory Controller:")), 1, 0);
    controller_status_ = new QLabel(QStringLiteral("等待连接"), status_group);
    status_layout->addWidget(controller_status_, 1, 1);

    status_layout->addWidget(new QLabel(QStringLiteral("Motion:")), 2, 0);
    motion_status_ = new QLabel(QStringLiteral("Idle"), status_group);
    status_layout->addWidget(motion_status_, 2, 1);

    main_layout->addWidget(status_group);

    auto *current_group = new QGroupBox(QStringLiteral("当前关节角度"), central);
    auto *current_layout = new QGridLayout(current_group);

    for (int i = 0; i < 6; ++i)
    {
        current_layout->addWidget(
            new QLabel(QStringLiteral("J%1").arg(i + 1), current_group),
            i, 0);

        current_labels_[i] = new QLabel(QStringLiteral("0.000°"), current_group);
        current_layout->addWidget(current_labels_[i], i, 1);
    }

    main_layout->addWidget(current_group);

    auto *target_group = new QGroupBox(QStringLiteral("目标关节角度"), central);
    auto *target_layout = new QGridLayout(target_group);

    for (int i = 0; i < 6; ++i)
    {
        target_layout->addWidget(
            new QLabel(QStringLiteral("J%1").arg(i + 1), target_group),
            i, 0);

        target_spins_[i] = new QDoubleSpinBox(target_group);
        target_spins_[i]->setRange(-360.0, 360.0);
        target_spins_[i]->setDecimals(3);
        target_spins_[i]->setSuffix(QStringLiteral(" °"));
        target_spins_[i]->setSingleStep(1.0);
        target_layout->addWidget(target_spins_[i], i, 1);
    }

    main_layout->addWidget(target_group);

    auto *buttons = new QHBoxLayout();

    move_button_ = new QPushButton(QStringLiteral("Move"), central);
    home_button_ = new QPushButton(QStringLiteral("Home (0°)"), central);
    copy_button_ = new QPushButton(QStringLiteral("当前值 → 目标值"), central);

    buttons->addWidget(move_button_);
    buttons->addWidget(home_button_);
    buttons->addWidget(copy_button_);

    main_layout->addLayout(buttons);
    main_layout->addStretch();

    setCentralWidget(central);

    connect(move_button_, &QPushButton::clicked,
            this, &MainWindow::onMoveClicked);

    connect(home_button_, &QPushButton::clicked,
            this, &MainWindow::onHomeClicked);

    connect(copy_button_, &QPushButton::clicked,
            this, &MainWindow::copyCurrentToTarget);

    connect(bridge_, &FanucRosBridge::jointStateUpdated,
            this, &MainWindow::onJointStateUpdated);

    connect(bridge_, &FanucRosBridge::connectionChanged,
            this, &MainWindow::onConnectionChanged);

    connect(bridge_, &FanucRosBridge::controllerReadyChanged,
            this, &MainWindow::onControllerReadyChanged);

    connect(bridge_, &FanucRosBridge::motionStatusChanged,
            this, &MainWindow::onMotionStatusChanged);
}

void MainWindow::onJointStateUpdated(
    double j1, double j2, double j3,
    double j4, double j5, double j6)
{
    const std::array<double, 6> values{j1, j2, j3, j4, j5, j6};

    for (int i = 0; i < 6; ++i)
        current_labels_[i]->setText(
            QStringLiteral("%1°").arg(values[i], 0, 'f', 3));
}

void MainWindow::onConnectionChanged(bool connected)
{
    ros_status_->setText(
        connected ? QStringLiteral("Connected")
                  : QStringLiteral("Disconnected"));
}

void MainWindow::onControllerReadyChanged(bool ready)
{
    controller_status_->setText(
        ready ? QStringLiteral("Ready")
              : QStringLiteral("Not Ready"));
}

void MainWindow::onMotionStatusChanged(
    const QString &message, bool)
{
    motion_status_->setText(message);
}

void MainWindow::onMoveClicked()
{
    bridge_->moveJoints(
        target_spins_[0]->value(),
        target_spins_[1]->value(),
        target_spins_[2]->value(),
        target_spins_[3]->value(),
        target_spins_[4]->value(),
        target_spins_[5]->value());
}

void MainWindow::onHomeClicked()
{
    bridge_->moveHome();
}

void MainWindow::copyCurrentToTarget()
{
    for (int i = 0; i < 6; ++i)
    {
        QString text = current_labels_[i]->text();
        text.remove(QChar(0x00B0));

        bool ok = false;
        const double value = text.toDouble(&ok);

        if (ok)
            target_spins_[i]->setValue(value);
    }
}
