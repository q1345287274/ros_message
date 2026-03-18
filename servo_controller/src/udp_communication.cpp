#include "servo_controller/udp_comm.h"
#include <iostream>
#include <cstring>

// socket头文件
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

UdpComm::UdpComm() 
    : sockfd_(-1)
    , local_addr_(nullptr)
    , remote_addr_(nullptr)
    , running_(false) 
{
}

UdpComm::~UdpComm() {
    close();
}

bool UdpComm::init(const std::string& local_ip, int local_port,
                   const std::string& remote_ip, int remote_port) {
    // 1. 创建UDP socket
    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // 2. 设置地址可重用
    int reuse = 1;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    // 3. 分配并设置本地地址
    local_addr_ = new sockaddr_in;
    memset(local_addr_, 0, sizeof(sockaddr_in));
    local_addr_->sin_family = AF_INET;
    local_addr_->sin_addr.s_addr = inet_addr(local_ip.c_str());
    local_addr_->sin_port = htons(local_port);
    
    // 4. 绑定本地地址
    if (bind(sockfd_, (struct sockaddr*)local_addr_, sizeof(sockaddr_in)) < 0) {
        std::cerr << "Failed to bind to " << local_ip << ":" << local_port << std::endl;
        close();
        return false;
    }
    
    // 5. 分配并设置远程地址（发送目标）
    remote_addr_ = new sockaddr_in;
    memset(remote_addr_, 0, sizeof(sockaddr_in));
    remote_addr_->sin_family = AF_INET;
    remote_addr_->sin_addr.s_addr = inet_addr(remote_ip.c_str());
    remote_addr_->sin_port = htons(remote_port);
    
    running_ = true;
    
    std::cout << "UDP initialized: " << local_ip << ":" << local_port 
              << " -> " << remote_ip << ":" << remote_port << std::endl;
    return true;
}

bool UdpComm::send(const uint8_t* data, size_t len) {
    if (sockfd_ < 0 || !remote_addr_) {
        return false;
    }
    
    // 发送数据到远程地址
    ssize_t sent = sendto(sockfd_, data, len, 0,
                          (struct sockaddr*)remote_addr_, sizeof(sockaddr_in));
    
    if (sent < 0) {
        std::cerr << "Failed to send UDP data" << std::endl;
        return false;
    }
    
    return static_cast<size_t>(sent) == len;
}

void UdpComm::setCallback(ReceiveCallback cb) {
    cb_ = cb;
}

void UdpComm::startReceive() {
    if (!running_ || !cb_) {
        std::cerr << "Cannot start receive: not initialized or no callback" << std::endl;
        return;
    }
    
    // 创建接收线程
    recv_thread_ = std::thread(&UdpComm::receiveLoop, this);
}

void UdpComm::receiveLoop() {
    uint8_t buffer[BUF_SIZE];
    sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sockaddr_in);
    
    std::cout << "UDP receive thread started" << std::endl;
    
    while (running_) {
        // 阻塞接收数据
        ssize_t len = recvfrom(sockfd_, buffer, BUF_SIZE, 0,
                               (struct sockaddr*)&sender_addr, &addr_len);
        
        if (len > 0) {
            // 收到数据，调用回调
            if (cb_) {
                cb_(buffer, static_cast<size_t>(len));
            }
        } else if (len < 0) {
            // 出错
            if (running_) {
                std::cerr << "UDP receive error" << std::endl;
            }
            break;
        }
        // len == 0 继续循环
    }
    
    std::cout << "UDP receive thread stopped" << std::endl;
}

void UdpComm::close() {
    running_ = false;
    
    // 关闭socket，这会中断阻塞的recvfrom
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
    
    // 等待接收线程结束
    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }
    
    // 释放地址内存
    if (local_addr_) {
        delete local_addr_;
        local_addr_ = nullptr;
    }
    if (remote_addr_) {
        delete remote_addr_;
        remote_addr_ = nullptr;
    }
}
