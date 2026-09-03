#pragma once

#include <QMainWindow>
#include <array>

class QLabel;
class QDoubleSpinBox;
class QPushButton;
class FanucRosBridge;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(FanucRosBridge *bridge, QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onJointStateUpdated(double j1, double j2, double j3,
                             double j4, double j5, double j6);
    void onConnectionChanged(bool connected);
    void onControllerReadyChanged(bool ready);
    void onMotionStatusChanged(const QString &message, bool success);
    void onMoveClicked();
    void onHomeClicked();
    void copyCurrentToTarget();

private:
    FanucRosBridge *bridge_;

    QLabel *ros_status_;
    QLabel *controller_status_;
    QLabel *motion_status_;

    std::array<QLabel *, 6> current_labels_{};
    std::array<QDoubleSpinBox *, 6> target_spins_{};

    QPushButton *move_button_;
    QPushButton *home_button_;
    QPushButton *copy_button_;
};
