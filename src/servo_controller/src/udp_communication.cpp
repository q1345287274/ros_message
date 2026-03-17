#include "servo_controller/udp_comm.h"
#include <iostream>
#include <thread>

UdpComm::UdpComm() {}

UdpComm::~UdpComm() {
    close();
}

bool UdpComm::init(const std::string& local_ip, int local_port,
                   const std::string& remote_ip, int remote_port) {
    try {
        // 创建本地端点
        local_ep_ = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(local_ip), local_port);
        
        // 创建远程端点
        remote_ep_ = boost::asio::ip::udp::endpoint(
            boost::asio::ip::address::from_string(remote_ip), remote_port);
        
        // 创建并绑定socket
        socket_.open(boost::asio::ip::udp::v4());
        socket_.bind(local_ep_);
        socket_.set_option(boost::asio::socket_base::reuse_address(true));
        
        running_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "UDP init error: " << e.what() << std::endl;
        return false;
    }
}

bool UdpComm::send(const uint8_t* data, size_t len) {
    if (!socket_.is_open()) {
        return false;
    }
    
    try {
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

void UdpComm::startReceive() {
    if (!socket_.is_open() || !cb_) {
        return;
    }
    doReceive();
}

void UdpComm::doReceive() {
    if (!running_) return;
    
    socket_.async_receive_from(
        boost::asio::buffer(recv_buf_, BUF_SIZE),
        sender_ep_,
        [this](boost::system::error_code ec, size_t len) {
            if (!ec && len > 0) {
                if (cb_) {
                    cb_(recv_buf_, len);
                }
            }
            if (running_) {
                doReceive();
            }
        }
    );
    
    // 在单独的线程中运行io_context
    std::thread([this]() {
        try {
            io_.run();
        } catch (...) {}
    }).detach();
}

void UdpComm::close() {
    running_ = false;
    try {
        if (socket_.is_open()) {
            socket_.close();
        }
        io_.stop();
    } catch (...) {}
}
