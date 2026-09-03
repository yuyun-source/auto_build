#include "fanuc_hmi/FanucRosBridge.h"
#include "fanuc_hmi/MainWindow.h"

#include <QApplication>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);

    QApplication app(argc, argv);

    FanucRosBridge bridge;
    MainWindow window(&bridge);

    bridge.start();
    window.show();

    const int result = app.exec();

    bridge.stop();
    rclcpp::shutdown();

    return result;
}
