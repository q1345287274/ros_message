#include <ros/ros.h>
#include <thread>
#include "servo_controller/udp_comm.h"
#include "servo_controller/protocol_parser.h"
#include "servo_controller/RawUdpData.h"
#include "servo_controller/ServoAngleFeedback.h"
#include "servo_controller/ServoStatusFeedback.h"

class ServoNode {
public:
    ServoNode() : nh_("~") {}
    
    bool init() {
        // 获取参数
        std::string local_ip, remote_ip;
        int local_port, remote_port;
        
        nh_.param<std::string>("local_ip", local_ip, "127.0.0.1");
        nh_.param<int>("local_port", local_port, 9000);
        nh_.param<std::string>("remote_ip", remote_ip, "127.0.0.1");
        nh_.param<int>("remote_port", remote_port, 9001);
        
        // 创建发布者
        pub_raw_ = nh_.advertise<servo_controller::RawUdpData>("feedback/raw", 10);
        pub_angle_ = nh_.advertise<servo_controller::ServoAngleFeedback>("feedback/angle", 10);
        pub_status_ = nh_.advertise<servo_controller::ServoStatusFeedback>("feedback/status", 10);
        
        // 创建订阅者
        sub_control_ = nh_.subscribe("control/raw", 10, &ServoNode::onControl, this);
        
        // 初始化UDP
        if (!udp_.init(local_ip, local_port, remote_ip, remote_port)) {
            ROS_ERROR("UDP init failed");
            return false;
        }
        
        // 设置接收回调
        udp_.setCallback([this](const uint8_t* data, size_t len) {
            this->onReceive(data, len);
        });
        udp_.startReceive();
        
        ROS_INFO("Servo controller initialized");
        ROS_INFO("Local: %s:%d, Remote: %s:%d", 
                 local_ip.c_str(), local_port, remote_ip.c_str(), remote_port);
        
        return true;
    }
    
    void run() {
        ros::spin();
    }

private:
    // 接收回调
    void onReceive(const uint8_t* data, size_t len) {
        // 解析数据
        auto blocks = parser_.parse(data, len);
        
        for (const auto& blk : blocks) {
            // 发布原始数据
            servo_controller::RawUdpData raw_msg;
            raw_msg.packet_id = blk.id;
            memcpy(raw_msg.data.data(), blk.data.data(), 8);
            pub_raw_.publish(raw_msg);
            
            // 根据ID解析并发布
            if (blk.id == protocol::FB_ANGLE) {
                auto fb = ProtocolParser::parseAngle(blk);
                if (fb.valid) {
                    servo_controller::ServoAngleFeedback msg;
                    msg.azimuth_angle = fb.azimuth;
                    msg.elevation_angle = fb.elevation;
                    msg.raw_azimuth = fb.raw_az;
                    msg.raw_elevation = fb.raw_el;
                    pub_angle_.publish(msg);
                }
            }
            else if (blk.id == protocol::FB_STATUS) {
                auto fb = ProtocolParser::parseStatus(blk);
                if (fb.valid) {
                    servo_controller::ServoStatusFeedback msg;
                    msg.self_check_status = fb.self_check;
                    msg.azimuth_motor_fault = fb.az_fault;
                    msg.azimuth_fault_code = fb.az_fault_code;
                    msg.elevation_motor_fault = fb.el_fault;
                    msg.elevation_fault_code = fb.el_fault_code;
                    memcpy(msg.raw_data.data(), fb.raw.data(), 8);
                    pub_status_.publish(msg);
                }
            }
        }
    }
    
    // 控制回调
    void onControl(const servo_controller::RawUdpData::ConstPtr& msg) {
        // 构建并发送UDP包
        auto packet = parser_.build(msg->packet_id, msg->data.data());
        if (!udp_.send(packet.data(), packet.size())) {
            ROS_WARN_THROTTLE(1.0, "Failed to send UDP packet");
        }
    }
    
    ros::NodeHandle nh_;
    ros::Publisher pub_raw_, pub_angle_, pub_status_;
    ros::Subscriber sub_control_;
    
    UdpComm udp_;
    ProtocolParser parser_;
};

int main(int argc, char** argv) {
    ros::init(argc, argv, "servo_controller");
    
    ServoNode node;
    if (!node.init()) {
        return 1;
    }
    
    node.run();
    return 0;
}
