# 代码注释文档

## 目录
- [头文件](#头文件)
  - [protocol_parser.h](#protocol_parserh)
  - [udp_comm.h](#udp_commh)
- [源文件](#源文件)
  - [protocol_parser.cpp](#protocol_parsercpp)
  - [udp_communication.cpp](#udp_communicationcpp)
  - [servo_controller_node.cpp](#servo_controller_nodecpp)
- [消息定义](#消息定义)
- [Python脚本](#python脚本)

---

## 头文件

### protocol_parser.h

```cpp
#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include <cstdint>
#include <vector>
#include <array>

// 协议常量命名空间
namespace protocol {
    // 包头定义
    static constexpr uint8_t HEAD1 = 0xAA;  // 包头第1字节
    static constexpr uint8_t HEAD2 = 0x55;  // 包头第2字节
    
    // 数据块长度：ID(2字节) + 数据(8字节) = 10字节
    static constexpr size_t BLOCK_LEN = 10;
    
    // 最小数据包长度：包头(2) + 长度(2) + 数据块(10) + 校验和(1) = 15字节
    static constexpr size_t MIN_PACK_SIZE = 5 + BLOCK_LEN;
    
    // 数据包ID定义
    static constexpr uint16_t CMD_CONTROL = 0x6401;  // 终端→伺服：控制指令
    static constexpr uint16_t CMD_ANGLE = 0x6403;    // 终端→伺服：角度控制
    static constexpr uint16_t FB_ANGLE = 0x4603;     // 伺服→终端：角度反馈
    static constexpr uint16_t FB_STATUS = 0x4607;    // 伺服→终端：状态反馈
}

/**
 * @brief 数据块结构
 * 每个数据块包含2字节ID和8字节数据
 */
struct DataBlock {
    uint16_t id;                    // 数据包ID（小端存储）
    std::array<uint8_t, 8> data;    // 8字节数据内容
    
    DataBlock() : id(0) { data.fill(0); }
};

/**
 * @brief 角度反馈结构
 * 解析0x4603数据块得到的角度信息
 */
struct AngleFeedback {
    float azimuth;      // 方位角度（度），原始值/100
    float elevation;    // 俯仰角度（度），原始值/100
    int16_t raw_az;     // 原始方位数据（比例尺100）
    int16_t raw_el;     // 原始俯仰数据（比例尺100）
    bool valid;         // 数据是否有效（原始值!=18100）
    
    AngleFeedback() : azimuth(0), elevation(0), 
                      raw_az(0), raw_el(0), valid(false) {}
};

/**
 * @brief 状态反馈结构
 * 解析0x4607数据块得到的状态信息
 */
struct StatusFeedback {
    uint8_t self_check;     // 自检状态: 0=无效, 1=成功, 2=失败
    uint8_t az_fault;       // 方位电机故障: 0=无, 1=有
    uint8_t az_fault_code;  // 方位故障码
    uint8_t el_fault;       // 俯仰电机故障: 0=无, 1=有
    uint8_t el_fault_code;  // 俯仰故障码
    std::array<uint8_t, 8> raw;  // 原始数据备份
    bool valid;             // 数据是否有效
    
    StatusFeedback() : self_check(0), az_fault(0), az_fault_code(0), 
                       el_fault(0), el_fault_code(0), valid(false) { raw.fill(0); }
};

/**
 * @brief 协议解析类
 * 负责UDP数据包的打包、解包、校验和计算、数据解析
 */
class ProtocolParser {
public:
    ProtocolParser();
    ~ProtocolParser();
    
    /**
     * @brief 解析接收到的UDP数据
     * @param buf 接收缓冲区指针
     * @param len 数据长度
     * @return 解析出的数据块列表，校验失败返回空列表
     */
    std::vector<DataBlock> parse(const uint8_t* buf, size_t len);
    
    /**
     * @brief 构建发送数据包
     * @param id 数据包ID
     * @param data 8字节数据指针
     * @return 构建好的完整数据包（包含包头、长度、数据、校验和）
     */
    std::vector<uint8_t> build(uint16_t id, const uint8_t* data);
    
    /**
     * @brief 解析角度反馈数据块（ID=0x4603）
     * @param blk 数据块
     * @return 解析后的角度反馈结构
     */
    static AngleFeedback parseAngle(const DataBlock& blk);
    
    /**
     * @brief 解析状态反馈数据块（ID=0x4607）
     * @param blk 数据块
     * @return 解析后的状态反馈结构
     */
    static StatusFeedback parseStatus(const DataBlock& blk);
    
    // 小端字节序转换工具函数
    static uint16_t toLE16(uint16_t v);           // 转小端16位
    static uint16_t fromLE16(const uint8_t* p);   // 从小端解析16位无符号
    static int16_t fromLE16S(const uint8_t* p);   // 从小端解析16位有符号
    
private:
    /**
     * @brief 验证校验和
     * @param buf 数据缓冲区（不含包头和长度字段）
     * @param len 数据长度
     * @param sum 接收到的校验和
     * @return 校验是否通过
     */
    bool checkSum(const uint8_t* buf, size_t len, uint8_t sum);
    
    /**
     * @brief 计算校验和
     * @param buf 数据缓冲区
     * @param len 数据长度
     * @return 计算得到的校验和（所有字节和的低8位）
     */
    uint8_t calcSum(const uint8_t* buf, size_t len);
};

#endif
```

---

### udp_comm.h

```cpp
#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <string>
#include <functional>
#include <boost/asio.hpp>

/**
 * @brief UDP通信类
 * 封装Boost.Asio的UDP通信功能，提供异步收发接口
 */
class UdpComm {
public:
    // 接收回调函数类型：接收缓冲区指针，数据长度
    using ReceiveCallback = std::function<void(const uint8_t*, size_t)>;
    
    UdpComm();
    ~UdpComm();
    
    /**
     * @brief 初始化UDP通信
     * @param local_ip 本地IP地址
     * @param local_port 本地端口
     * @param remote_ip 远程IP地址（发送目标）
     * @param remote_port 远程端口
     * @return 初始化是否成功
     */
    bool init(const std::string& local_ip, int local_port,
              const std::string& remote_ip, int remote_port);
    
    /**
     * @brief 发送UDP数据
     * @param data 数据缓冲区
     * @param len 数据长度
     * @return 发送是否成功
     */
    bool send(const uint8_t* data, size_t len);
    
    /**
     * @brief 设置接收回调函数
     * @param cb 回调函数对象
     */
    void setCallback(ReceiveCallback cb);
    
    /**
     * @brief 启动异步接收
     * 在独立线程中运行io_context，通过回调函数返回接收到的数据
     */
    void startReceive();
    
    /**
     * @brief 关闭UDP连接
     */
    void close();
    
private:
    /**
     * @brief 执行异步接收操作
     * 递归调用实现持续接收
     */
    void doReceive();
    
    // Boost.Asio IO上下文
    boost::asio::io_context io_;
    
    // UDP Socket
    boost::asio::ip::udp::socket socket_{io_};
    
    // 本地端点（绑定地址）
    boost::asio::ip::udp::endpoint local_ep_;
    
    // 远程端点（发送目标）
    boost::asio::ip::udp::endpoint remote_ep_;
    
    // 发送方端点（接收时填充）
    boost::asio::ip::udp::endpoint sender_ep_;
    
    // 接收缓冲区大小
    static constexpr size_t BUF_SIZE = 1024;
    uint8_t recv_buf_[BUF_SIZE];  // 接收缓冲区
    
    // 接收回调函数
    ReceiveCallback cb_;
    
    // 运行标志
    bool running_ = false;
};

#endif
```

---

## 源文件

### protocol_parser.cpp

```cpp
#include "servo_controller/protocol_parser.h"
#include <cstring>

// 构造函数和析构函数
ProtocolParser::ProtocolParser() {}
ProtocolParser::~ProtocolParser() {}

/**
 * 解析UDP数据包流程：
 * 1. 检查最小长度
 * 2. 验证包头（0xAA 0x55）
 * 3. 解析数据长度字段
 * 4. 验证校验和
 * 5. 提取所有数据块
 */
std::vector<DataBlock> ProtocolParser::parse(const uint8_t* buf, size_t len) {
    std::vector<DataBlock> blocks;
    
    // 检查最小长度（15字节）
    if (len < protocol::MIN_PACK_SIZE) {
        return blocks;
    }
    
    // 验证包头
    if (buf[0] != protocol::HEAD1 || buf[1] != protocol::HEAD2) {
        return blocks;
    }
    
    // 获取数据长度（小端，字节2-3）
    uint16_t data_len = fromLE16(buf + 2);
    
    // 验证总长度 = 包头(2) + 长度(2) + 数据(data_len) + 校验和(1)
    size_t total_len = 2 + 2 + data_len + 1;
    if (len < total_len) {
        return blocks;
    }
    
    // 验证校验和（字节4到4+data_len-1）
    uint8_t recv_sum = buf[total_len - 1];
    if (!checkSum(buf + 4, data_len, recv_sum)) {
        return blocks;
    }
    
    // 解析数据块（每个10字节：ID 2字节 + 数据 8字节）
    size_t offset = 4;  // 从字节4开始（跳过包头和长度）
    while (offset + protocol::BLOCK_LEN <= 4 + data_len) {
        DataBlock blk;
        blk.id = fromLE16(buf + offset);           // 解析ID（小端）
        memcpy(blk.data.data(), buf + offset + 2, 8);  // 复制8字节数据
        blocks.push_back(blk);
        offset += protocol::BLOCK_LEN;
    }
    
    return blocks;
}

/**
 * 构建发送数据包流程：
 * 1. 添加包头（0xAA 0x55）
 * 2. 添加长度字段（10字节，小端）
 * 3. 添加ID（小端）
 * 4. 添加8字节数据
 * 5. 计算并添加校验和
 */
std::vector<uint8_t> ProtocolParser::build(uint16_t id, const uint8_t* data) {
    std::vector<uint8_t> packet;
    
    // 包头
    packet.push_back(protocol::HEAD1);
    packet.push_back(protocol::HEAD2);
    
    // 长度（10字节 = ID 2字节 + 数据 8字节，小端）
    uint16_t len = protocol::BLOCK_LEN;
    packet.push_back(len & 0xFF);
    packet.push_back((len >> 8) & 0xFF);
    
    // ID（小端）
    packet.push_back(id & 0xFF);
    packet.push_back((id >> 8) & 0xFF);
    
    // 数据（8字节）
    for (int i = 0; i < 8; i++) {
        packet.push_back(data[i]);
    }
    
    // 校验和（字节4到13的累加和低8位）
    uint8_t sum = calcSum(packet.data() + 4, protocol::BLOCK_LEN);
    packet.push_back(sum);
    
    return packet;
}

/**
 * 解析角度反馈（ID=0x4603）
 * 字节0-1：方位角度（有符号小端，比例尺100，无效值18100）
 * 字节2-3：俯仰角度（有符号小端，比例尺100，无效值18100）
 */
AngleFeedback ProtocolParser::parseAngle(const DataBlock& blk) {
    AngleFeedback fb;
    
    // ID检查
    if (blk.id != protocol::FB_ANGLE) {
        return fb;
    }
    
    // 解析方位角度
    fb.raw_az = fromLE16S(blk.data.data());
    if (fb.raw_az == 18100) {
        fb.valid = false;  // 无效值
    } else {
        fb.azimuth = fb.raw_az / 100.0f;  // 比例尺转换
        fb.valid = true;
    }
    
    // 解析俯仰角度
    fb.raw_el = fromLE16S(blk.data.data() + 2);
    if (fb.raw_el == 18100) {
        fb.valid = false;
    } else {
        fb.elevation = fb.raw_el / 100.0f;
    }
    
    return fb;
}

/**
 * 解析状态反馈（ID=0x4607）
 * 字节0：自检状态（bit1-0）
 * 字节2：方位故障（bit7=故障标志，bit6-0=故障码）
 * 字节3：俯仰故障（bit7=故障标志，bit6-0=故障码）
 */
StatusFeedback ProtocolParser::parseStatus(const DataBlock& blk) {
    StatusFeedback fb;
    
    // ID检查
    if (blk.id != protocol::FB_STATUS) {
        return fb;
    }
    
    // 保存原始数据
    memcpy(fb.raw.data(), blk.data.data(), 8);
    
    // 字节0：自检反馈（bit1-0）
    fb.self_check = blk.data[0] & 0x03;
    
    // 字节2：方位电机故障
    fb.az_fault = (blk.data[2] >> 7) & 0x01;  // bit7
    fb.az_fault_code = blk.data[2] & 0x7F;    // bit6-0
    
    // 字节3：俯仰电机故障
    fb.el_fault = (blk.data[3] >> 7) & 0x01;  // bit7
    fb.el_fault_code = blk.data[3] & 0x7F;    // bit6-0
    
    fb.valid = true;
    return fb;
}

// 校验和验证：计算校验和并与接收值比较
bool ProtocolParser::checkSum(const uint8_t* buf, size_t len, uint8_t sum) {
    return calcSum(buf, len) == sum;
}

// 计算校验和：所有字节累加，取低8位
uint8_t ProtocolParser::calcSum(const uint8_t* buf, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

// 小端转换（假设系统是小端，直接返回）
uint16_t ProtocolParser::toLE16(uint16_t v) {
    return v;
}

// 从小端字节数组解析16位无符号整数
uint16_t ProtocolParser::fromLE16(const uint8_t* p) {
    return p[0] | (p[1] << 8);
}

// 从小端字节数组解析16位有符号整数
int16_t ProtocolParser::fromLE16S(const uint8_t* p) {
    return static_cast<int16_t>(fromLE16(p));
}
```

---

### udp_communication.cpp

```cpp
#include "servo_controller/udp_comm.h"
#include <iostream>
#include <thread>

UdpComm::UdpComm() {}

UdpComm::~UdpComm() {
    close();
}

/**
 * UDP初始化流程：
 * 1. 创建本地端点（绑定地址）
 * 2. 创建远程端点（发送目标）
 * 3. 创建socket并绑定本地地址
 * 4. 设置地址复用选项
 */
bool UdpComm::init(const std::string& local_ip, int local_port,
                   const std::string& remote_ip, int remote_port) {
    try {
        // 本地端点（接收地址）
        local_ep_ = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(local_ip), local_port);
        
        // 远程端点（发送目标）
        remote_ep_ = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(remote_ip), remote_port);
        
        // 创建IPv4 UDP socket
        socket_.open(boost::asio::ip::udp::v4());
        // 绑定本地地址
        socket_.bind(local_ep_);
        // 设置地址复用，允许多个socket绑定同一地址
        socket_.set_option(boost::asio::socket_base::reuse_address(true));
        
        running_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UDP init error: " << e.what() << std::endl;
        return false;
    }
}

/**
 * 发送数据到远程端点
 */
bool UdpComm::send(const uint8_t* data, size_t len) {
    if (!socket_.is_open()) {
        return false;
    }
    
    try {
        // 同步发送数据到远程端点
        size_t sent = socket_.send_to(boost::asio::buffer(data, len), remote_ep_);
        return sent == len;
    } catch (const std::exception& e) {
        std::cerr << "UDP send error: " << e.what() << std::endl;
        return false;
    }
}

void UdpComm::setCallback(ReceiveCallback cb) {
    cb_ = cb;
}

/**
 * 启动异步接收
 * 注意：实际应该只调用一次，这里简化处理
 */
void UdpComm::startReceive() {
    if (!socket_.is_open() || !cb_) {
        return;
    }
    doReceive();
}

/**
 * 执行异步接收操作
 * 使用递归方式实现持续接收
 */
void UdpComm::doReceive() {
    if (!running_) return;
    
    // 启动异步接收操作
    socket_.async_receive_from(
        boost::asio::buffer(recv_buf_, BUF_SIZE),  // 接收缓冲区
        sender_ep_,                                // 填充发送方地址
        [this](boost::system::error_code ec, size_t len) {
            // 接收完成回调（lambda）
            if (!ec && len > 0) {
                if (cb_) {
                    cb_(recv_buf_, len);  // 调用用户回调
                }
            }
            // 递归调用，继续接收
            if (running_) {
                doReceive();
            }
        }
    );
    
    // 在独立线程中运行io_context事件循环
    std::thread([this]() {
        try {
            io_.run();
        } catch (...) {}
    }).detach();
}

/**
 * 关闭UDP连接
 */
void UdpComm::close() {
    running_ = false;
    try {
        if (socket_.is_open()) {
            socket_.close();
        }
        io_.stop();  // 停止io_context
    } catch (...) {}
}
```

---

### servo_controller_node.cpp

```cpp
#include <ros/ros.h>
#include <thread>
#include "servo_controller/udp_comm.h"
#include "servo_controller/protocol_parser.h"
#include "servo_controller/RawUdpData.h"
#include "servo_controller/ServoAngleFeedback.h"
#include "servo_controller/ServoStatusFeedback.h"

/**
 * @brief 伺服控制器ROS节点类
 * 
 * 功能：
 * 1. 订阅ROS控制话题，通过UDP转发给执行器
 * 2. 接收UDP反馈数据，解析后发布到ROS话题
 */
class ServoNode {
public:
    ServoNode() : nh_("~") {}  // 使用私有命名空间
    
    /**
     * @brief 初始化节点
     * 流程：加载参数→创建话题→初始化UDP→启动接收
     */
    bool init() {
        // 从参数服务器获取网络配置
        std::string local_ip, remote_ip;
        int local_port, remote_port;
        
        nh_.param<std::string>("local_ip", local_ip, "127.0.0.1");
        nh_.param<int>("local_port", local_port, 9000);
        nh_.param<std::string>("remote_ip", remote_ip, "127.0.0.1");
        nh_.param<int>("remote_port", remote_port, 9001);
        
        // 创建ROS发布者
        pub_raw_ = nh_.advertise<servo_controller::RawUdpData>("feedback/raw", 10);
        pub_angle_ = nh_.advertise<servo_controller::ServoAngleFeedback>("feedback/angle", 10);
        pub_status_ = nh_.advertise<servo_controller::ServoStatusFeedback>("feedback/status", 10);
        
        // 创建ROS订阅者（绑定回调函数）
        sub_control_ = nh_.subscribe("control/raw", 10, &ServoNode::onControl, this);
        
        // 初始化UDP通信
        if (!udp_.init(local_ip, local_port, remote_ip, remote_port)) {
            ROS_ERROR("UDP init failed");
            return false;
        }
        
        // 设置UDP接收回调（lambda绑定成员函数）
        udp_.setCallback([this](const uint8_t* data, size_t len) {
            this->onReceive(data, len);
        });
        udp_.startReceive();  // 启动异步接收
        
        ROS_INFO("Servo controller initialized");
        ROS_INFO("Local: %s:%d, Remote: %s:%d", 
                 local_ip.c_str(), local_port, remote_ip.c_str(), remote_port);
        
        return true;
    }
    
    /**
     * @brief 进入ROS事件循环
     */
    void run() {
        ros::spin();  // 阻塞，处理回调
    }

private:
    /**
     * @brief UDP接收回调
     * 流程：解析数据包→根据ID处理→发布到ROS话题
     * @param data 接收缓冲区
     * @param len 数据长度
     */
    void onReceive(const uint8_t* data, size_t len) {
        // 解析UDP数据包
        auto blocks = parser_.parse(data, len);
        
        // 遍历所有数据块
        for (const auto& blk : blocks) {
            // 1. 发布原始数据（所有数据块）
            servo_controller::RawUdpData raw_msg;
            raw_msg.packet_id = blk.id;
            memcpy(raw_msg.data.data(), blk.data.data(), 8);
            pub_raw_.publish(raw_msg);
            
            // 2. 根据ID解析并发布特定反馈
            if (blk.id == protocol::FB_ANGLE) {
                // 角度反馈
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
                // 状态反馈
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
    
    /**
     * @brief ROS控制回调
     * 将ROS控制消息转换为UDP数据包发送
     * @param msg ROS控制消息（ID + 8字节数据）
     */
    void onControl(const servo_controller::RawUdpData::ConstPtr& msg) {
        // 构建UDP数据包
        auto packet = parser_.build(msg->packet_id, msg->data.data());
        
        // 发送UDP数据
        if (!udp_.send(packet.data(), packet.size())) {
            ROS_WARN_THROTTLE(1.0, "Failed to send UDP packet");
        }
    }
    
    // ROS节点句柄
    ros::NodeHandle nh_;
    
    // ROS发布者
    ros::Publisher pub_raw_;     // 原始反馈数据
    ros::Publisher pub_angle_;   // 角度反馈
    ros::Publisher pub_status_;  // 状态反馈
    
    // ROS订阅者
    ros::Subscriber sub_control_;  // 控制指令
    
    // UDP通信对象
    UdpComm udp_;
    
    // 协议解析器
    ProtocolParser parser_;
};

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    // 初始化ROS节点
    ros::init(argc, argv, "servo_controller");
    
    // 创建并运行节点
    ServoNode node;
    if (!node.init()) {
        return 1;
    }
    
    node.run();
    return 0;
}
```

---

## 消息定义

### RawUdpData.msg

```
# 原始UDP数据消息
# 用于：
#   1. 控制下发：上位机→执行器
#   2. 原始反馈：执行器→上位机（未解析的原始数据）

# 数据包ID（2字节，小端）
# 控制指令ID：
#   0x6401 - 伺服控制（开/关/自检/模式切换）
#   0x6403 - 角度控制
# 反馈指令ID：
#   0x4603 - 角度反馈
#   0x4607 - 状态反馈
uint16 packet_id

# 数据内容（8字节）
# 根据packet_id有不同的含义
uint8[8] data
```

### ServoAngleFeedback.msg

```
# 伺服角度反馈消息（解析后）
# 对应协议ID：0x4603
# 发送频率：50Hz

# 方位角度（度）
# 计算方式：原始值 / 100
# 范围：-180.00 ~ +180.00
float32 azimuth_angle

# 俯仰角度（度）
# 计算方式：原始值 / 100
# 范围：-180.00 ~ +180.00
float32 elevation_angle

# 原始方位数据（调试用）
# 协议原始值，比例尺100
# 范围：-18000 ~ +18000
int16 raw_azimuth

# 原始俯仰数据（调试用）
int16 raw_elevation
```

### ServoStatusFeedback.msg

```
# 伺服状态反馈消息（解析后）
# 对应协议ID：0x4607
# 发送时机：触发式（如自检完成后）

# 自检状态
# 0 = 无效/未进行
# 1 = 自检成功
# 2 = 自检失败
uint8 self_check_status

# 方位电机故障标志
# 0 = 无故障
# 1 = 有故障
uint8 azimuth_motor_fault

# 方位故障码
# 1 = 母线电压过压
# 2 = 母线电压欠压
# 255 = 其他故障
uint8 azimuth_fault_code

# 俯仰电机故障标志
uint8 elevation_motor_fault

# 俯仰故障码
uint8 elevation_fault_code

# 原始数据（调试用）
uint8[8] raw_data
```

---

## Python脚本

### mock_servo.py

```python
#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
模拟执行器 - 用于本地测试

功能：
1. 监听UDP端口9001，接收控制指令
2. 模拟伺服运动（逐步接近目标角度）
3. 50Hz发送角度反馈到端口9000
"""

import socket
import struct
import time
import sys

def build_packet(packet_id, data):
    """
    构建协议数据包
    
    数据包格式：
    [0xAA][0x55][长度:2][ID:2][数据:8][校验和:1]
    
    Args:
        packet_id: 数据包ID（如0x4603）
        data: 8字节数据
    
    Returns:
        bytes: 完整数据包
    """
    packet = bytearray([0xAA, 0x55, 0x0A, 0x00])  # 包头 + 长度（10字节）
    packet.append(packet_id & 0xFF)               # ID低字节
    packet.append((packet_id >> 8) & 0xFF)        # ID高字节
    packet.extend(data)                           # 数据
    checksum = sum(packet[4:14]) & 0xFF           # 计算校验和（字节4-13）
    packet.append(checksum)
    return bytes(packet)

class MockServo:
    """模拟执行器类"""
    
    def __init__(self):
        # 创建UDP socket并绑定到本地端口9001
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 9001))
        self.sock.settimeout(0.05)  # 50ms超时，用于定期检查
        
        # 伺服状态变量
        self.angle_az = 0       # 当前方位角度（比例尺100）
        self.angle_el = 0       # 当前俯仰角度
        self.target_az = 0      # 目标方位角度
        self.target_el = 0      # 目标俯仰角度
        self.servo_on = False   # 伺服开关状态
        self.self_check_status = 0  # 自检状态
        
        print("=" * 50)
        print("模拟执行器已启动")
        print("监听: 127.0.0.1:9001")
        print("发送到: 127.0.0.1:9000")
        print("=" * 50)
    
    def handle_command(self, data):
        """
        处理接收到的控制指令
        
        协议解析：
        字节0-1: 包头 0xAA 0x55
        字节2-3: 数据长度（小端）
        字节4-5: 数据包ID（小端）
        字节6-13: 8字节数据
        字节14: 校验和
        
        Args:
            data: 接收到的UDP数据
        """
        # 基本验证
        if len(data) < 15 or data[0] != 0xAA or data[1] != 0x55:
            return
        
        # 解析ID和数据
        packet_id = data[4] | (data[5] << 8)  # 小端解析
        cmd = data[6:14]  # 8字节数据
        
        # 处理控制指令（0x6401）
        if packet_id == 0x6401:
            self._handle_control_cmd(cmd)
        
        # 处理角度指令（0x6403）
        elif packet_id == 0x6403:
            self._handle_angle_cmd(cmd)
    
    def _handle_control_cmd(self, cmd):
        """处理0x6401控制指令"""
        # 字节0: [伺服控制 bit3-2] [自检 bit1] [查询 bit0]
        # 伺服控制: 00=无效, 01=开(0x04), 10=关(0x08)
        servo_ctrl = (cmd[0] >> 2) & 0x03
        if servo_ctrl == 1:
            self.servo_on = True
            print("[CMD] 开伺服")
        elif servo_ctrl == 2:
            self.servo_on = False
            print("[CMD] 关伺服")
        
        # 自检控制
        if (cmd[0] >> 1) & 0x01:
            # 只有伺服开启时自检才成功
            self.self_check_status = 1 if self.servo_on else 2
            status_str = "成功" if self.self_check_status == 1 else "失败"
            print("[CMD] 自检: %s" % status_str)
            
            # 发送状态反馈（0x4607）
            status_data = bytes([self.self_check_status, 0, 0, 0, 0, 0, 0, 0])
            self.sock.sendto(build_packet(0x4607, status_data), ('127.0.0.1', 9000))
        
        # 工作模式（字节2的bit7-5）
        mode = (cmd[2] >> 5) & 0x07
        if mode == 2:
            print("[CMD] 切换调转模式")
        
        # 归零控制（字节1的bit3-2）
        homing = (cmd[1] >> 2) & 0x03
        if homing:
            self.target_az = 0
            self.target_el = 0
            print("[CMD] 归零")
    
    def _handle_angle_cmd(self, cmd):
        """处理0x6403角度指令"""
        # 字节4-5: 方位角度（小端有符号）
        self.target_az = struct.unpack('<h', bytes(cmd[4:6]))[0]
        # 字节6-7: 俯仰角度
        self.target_el = struct.unpack('<h', bytes(cmd[6:8]))[0]
        
        print("[CMD] 角度目标: az=%.2f, el=%.2f" % 
              (self.target_az/100.0, self.target_el/100.0))
    
    def update(self):
        """
        更新伺服位置（模拟运动）
        每次更新逐步接近目标角度
        """
        if not self.servo_on:
            return
        
        # 模拟运动：每次更新5步，每步最多移动5个单位
        for _ in range(5):
            diff_az = self.target_az - self.angle_az
            diff_el = self.target_el - self.angle_el
            
            # 方位移动
            if abs(diff_az) > 5:
                self.angle_az += 5 if diff_az > 0 else -5
            else:
                self.angle_az = self.target_az
            
            # 俯仰移动
            if abs(diff_el) > 5:
                self.angle_el += 5 if diff_el > 0 else -5
            else:
                self.angle_el = self.target_el
    
    def send_feedback(self):
        """发送角度反馈（0x4603）"""
        if not self.servo_on:
            return
        
        # 打包角度数据（小端）
        angle_data = struct.pack('<hh', self.angle_az, self.angle_el) + bytes(4)
        self.sock.sendto(build_packet(0x4603, angle_data), ('127.0.0.1', 9000))
    
    def run(self):
        """主循环"""
        last_feedback = 0  # 上次发送反馈的时间
        
        while True:
            try:
                # 接收控制指令（超时50ms）
                data, addr = self.sock.recvfrom(1024)
                self.handle_command(data)
            except socket.timeout:
                pass  # 超时继续
            except KeyboardInterrupt:
                break
            
            # 更新伺服位置
            self.update()
            
            # 50Hz发送反馈（每20ms）
            now = time.time()
            if now - last_feedback >= 0.02:
                self.send_feedback()
                last_feedback = now
        
        self.sock.close()
        print("\n模拟执行器已停止")

if __name__ == '__main__':
    try:
        MockServo().run()
    except Exception as e:
        print("错误:", e)
        sys.exit(1)
```

### test_control.py

```python
#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
交互式测试脚本

提供简单的命令行界面用于测试伺服控制器

命令：
    s - 开伺服
    x - 关伺服
    c - 自检
    m - 切换调转模式
    a <az> <el> - 发送角度指令
    q - 退出
"""

import rospy
import struct
from servo_controller.msg import RawUdpData, ServoAngleFeedback, ServoStatusFeedback

# 协议ID常量
CMD_CONTROL = 0x6401  # 控制指令
CMD_ANGLE = 0x6403    # 角度控制

class ServoTester:
    def __init__(self):
        rospy.init_node('servo_tester')
        
        # 创建发布者（发送控制指令）
        self.pub = rospy.Publisher('/servo_controller/control/raw', RawUdpData, queue_size=10)
        
        # 创建订阅者（接收反馈）
        rospy.Subscriber('/servo_controller/feedback/angle', ServoAngleFeedback, self.on_angle)
        rospy.Subscriber('/servo_controller/feedback/status', ServoStatusFeedback, self.on_status)
        
        # 打印帮助信息
        rospy.loginfo("伺服测试工具")
        rospy.loginfo("命令: s=开伺服 x=关伺服 c=自检 m=调转模式 a=角度 q=退出")
    
    def send_control(self, byte1, byte2=0, byte3=0):
        """
        发送控制指令（0x6401）
        
        Args:
            byte1: 伺服控制(bit3-2) + 自检(bit1) + 查询(bit0)
            byte2: 归零控制(bit3-2) + 清零控制(bit1-0)
            byte3: 工作模式(bit7-5)
        """
        msg = RawUdpData()
        msg.packet_id = CMD_CONTROL
        msg.data = [byte1, byte2, byte3, 0, 0, 0, 0, 0]
        self.pub.publish(msg)
        rospy.loginfo("发送控制: [%02X, %02X, %02X]" % (byte1, byte2, byte3))
    
    def open_servo(self):
        """开伺服：bit3-2 = 01 -> 0x04"""
        self.send_control(0x04)
        rospy.loginfo("→ 开伺服")
    
    def close_servo(self):
        """关伺服：bit3-2 = 10 -> 0x08"""
        self.send_control(0x08)
        rospy.loginfo("→ 关伺服")
    
    def self_check(self):
        """自检：bit1 = 1 -> 0x02"""
        self.send_control(0x02)
        rospy.loginfo("→ 自检")
    
    def set_rotation_mode(self):
        """切换调转模式：bit7-5 = 010 -> 0x40"""
        self.send_control(0, 0, 0x40)
        rospy.loginfo("→ 调转模式")
    
    def send_angle(self, azimuth, elevation):
        """
        发送角度指令（0x6403）
        
        数据格式：
        字节0-1: 方位速度
        字节2-3: 俯仰速度
        字节4-5: 方位角度（小端，比例尺100）
        字节6-7: 俯仰角度（小端，比例尺100）
        """
        msg = RawUdpData()
        msg.packet_id = CMD_ANGLE
        
        # 角度转原始值（×100）
        az_raw = int(azimuth * 100)
        el_raw = int(elevation * 100)
        
        # 小端打包：速度(0) + 速度(0) + 角度 + 角度
        data = struct.pack('<hhhh', 0, 0, az_raw, el_raw)
        msg.data = list(data)
        
        self.pub.publish(msg)
        rospy.loginfo("→ 角度指令: az=%.2f, el=%.2f" % (azimuth, elevation))
    
    def on_angle(self, msg):
        """角度反馈回调"""
        rospy.loginfo("反馈-角度: az=%.2f, el=%.2f" % 
                     (msg.azimuth_angle, msg.elevation_angle))
    
    def on_status(self, msg):
        """状态反馈回调"""
        status_map = {0: "无效", 1: "成功", 2: "失败"}
        rospy.loginfo("反馈-状态: 自检=%s, 方位故障=%d, 俯仰故障=%d" % 
                     (status_map.get(msg.self_check_status, "未知"),
                      msg.azimuth_motor_fault, msg.elevation_motor_fault))
    
    def run(self):
        """交互式命令循环"""
        while not rospy.is_shutdown():
            try:
                # Python2/3兼容输入
                cmd = raw_input("\n命令> ").strip().split()
            except:
                cmd = input("\n命令> ").strip().split()
            
            if not cmd:
                continue
            
            key = cmd[0].lower()
            
            if key == 'q':
                break
            elif key == 's':
                self.open_servo()
            elif key == 'x':
                self.close_servo()
            elif key == 'c':
                self.self_check()
            elif key == 'm':
                self.set_rotation_mode()
            elif key == 'a' and len(cmd) >= 3:
                try:
                    az = float(cmd[1])
                    el = float(cmd[2])
                    self.send_angle(az, el)
                except ValueError:
                    rospy.logerr("角度值无效")
            else:
                rospy.loginfo("未知命令: %s" % key)

if __name__ == '__main__':
    ServoTester().run()
```

---

## 总结

| 文件 | 功能 | 核心类/函数 |
|------|------|------------|
| `protocol_parser.h/cpp` | 协议解析 | `ProtocolParser::parse/build/parseAngle/parseStatus` |
| `udp_comm.h/cpp` | UDP通信 | `UdpComm::init/send/startReceive` |
| `servo_controller_node.cpp` | ROS节点 | `ServoNode::init/onControl/onReceive` |
| `mock_servo.py` | 模拟执行器 | `MockServo::handle_command/update/send_feedback` |
| `test_control.py` | 交互式测试 | `ServoTester::send_control/send_angle` |
