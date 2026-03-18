#ifndef UDP_COMM_H
#define UDP_COMM_H

#include <string>
#include <functional>
#include <thread>
#include <atomic>

// 前向声明socket相关结构体
struct sockaddr_in;

class UdpComm {
public:
    // 接收回调函数类型: 数据指针, 数据长度
    using ReceiveCallback = std::function<void(const uint8_t*, size_t)>;
    
    UdpComm();
    ~UdpComm();
    
    /**
     * @brief 初始化UDP通信
     * @param local_ip 本地IP地址
     * @param local_port 本地端口（接收）
     * @param remote_ip 远程IP地址（发送目标）
     * @param remote_port 远程端口
     * @return true-成功 false-失败
     */
    bool init(const std::string& local_ip, int local_port,
              const std::string& remote_ip, int remote_port);
    
    /**
     * @brief 发送UDP数据
     * @param data 数据缓冲区
     * @param len 数据长度
     * @return true-成功 false-失败
     */
    bool send(const uint8_t* data, size_t len);
    
    /**
     * @brief 设置接收回调函数
     * @param cb 回调函数
     */
    void setCallback(ReceiveCallback cb);
    
    /**
     * @brief 启动接收线程
     * 在独立线程中循环接收数据，通过回调返回
     */
    void startReceive();
    
    /**
     * @brief 停止接收并关闭连接
     */
    void close();
    
private:
    /**
     * @brief 接收线程函数
     * 循环阻塞接收UDP数据
     */
    void receiveLoop();
    
    int sockfd_;                    // socket文件描述符
    sockaddr_in* local_addr_;       // 本地地址
    sockaddr_in* remote_addr_;      // 远程地址
    
    ReceiveCallback cb_;            // 接收回调函数
    std::thread recv_thread_;       // 接收线程
    std::atomic<bool> running_;     // 运行标志
    
    static constexpr size_t BUF_SIZE = 1024;  // 接收缓冲区大小
};

#endif
