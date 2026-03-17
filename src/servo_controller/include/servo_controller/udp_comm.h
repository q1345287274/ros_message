#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <string>
#include <functional>
#include <boost/asio.hpp>

class UdpComm {
public:
    using ReceiveCallback = std::function<void(const uint8_t*, size_t)>;
    
    UdpComm();
    ~UdpComm();
    
    // 初始化UDP
    bool init(const std::string& local_ip, int local_port,
              const std::string& remote_ip, int remote_port);
    
    // 发送数据
    bool send(const uint8_t* data, size_t len);
    
    // 设置接收回调
    void setCallback(ReceiveCallback cb);
    
    // 开始接收
    void startReceive();
    
    // 关闭
    void close();
    
private:
    void doReceive();
    
    boost::asio::io_context io_;
    boost::asio::ip::udp::socket socket_{io_};
    boost::asio::ip::udp::endpoint local_ep_;
    boost::asio::ip::udp::endpoint remote_ep_;
    boost::asio::ip::udp::endpoint sender_ep_;
    
    static constexpr size_t BUF_SIZE = 1024;
    uint8_t recv_buf_[BUF_SIZE];
    
    ReceiveCallback cb_;
    bool running_ = false;
};

#endif
