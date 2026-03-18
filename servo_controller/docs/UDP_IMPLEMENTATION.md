# UDP通信实现说明

## 实现方式变更

### 旧版本：Boost.Asio 异步方式

```cpp
// 使用Boost.Asio的异步接口
io_context io_;
udp::socket socket_(io_);

// 启动异步接收
socket_.async_receive_from(buffer, endpoint, callback);
io_.run();  // 事件循环
```

**问题**：
- 依赖Boost库，编译复杂
- 异步编程理解难度大
- 调试困难

### 新版本：原生Socket + 线程

```cpp
// 创建原生socket
sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
bind(sockfd_, local_addr, ...);

// 创建接收线程
std::thread(&UdpComm::receiveLoop, this);

// 线程中循环接收
recvfrom(sockfd_, buffer, ...);  // 阻塞等待
```

**优点**：
- 无外部依赖（只用系统API）
- 代码简洁易懂
- 同步编程逻辑清晰

---

## 核心代码对比

### 初始化

**Boost.Asio方式**：
```cpp
io_context io_;
udp::socket socket_(io_);
udp::endpoint local_ep(address::from_string(ip), port);
socket_.open(udp::v4());
socket_.bind(local_ep);
```

**原生Socket方式**：
```cpp
sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
struct sockaddr_in addr;
addr.sin_family = AF_INET;
addr.sin_addr.s_addr = inet_addr(ip);
addr.sin_port = htons(port);
bind(sockfd_, (struct sockaddr*)&addr, sizeof(addr));
```

### 发送数据

**Boost.Asio方式**：
```cpp
socket_.send_to(buffer(data, len), remote_ep);
```

**原生Socket方式**：
```cpp
sendto(sockfd_, data, len, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
```

### 接收数据

**Boost.Asio方式**：
```cpp
// 异步回调方式
socket_.async_receive_from(buffer, sender_ep,
    [](error_code ec, size_t len) {
        // 处理数据
    });
```

**原生Socket方式**：
```cpp
// 同步阻塞+线程方式
void receiveLoop() {
    while (running_) {
        len = recvfrom(sockfd_, buffer, sizeof(buffer), 0, ...);
        if (len > 0) {
            callback(buffer, len);  // 调用回调
        }
    }
}
```

---

## 关键API说明

| API | 功能 | 说明 |
|-----|------|------|
| `socket()` | 创建socket | `AF_INET`=IPv4, `SOCK_DGRAM`=UDP |
| `bind()` | 绑定本地地址 | 指定接收端口 |
| `sendto()` | 发送数据 | 指定目标地址 |
| `recvfrom()` | 接收数据 | 阻塞直到收到数据 |
| `htons()` | 端口转换 | 主机字节序→网络字节序 |
| `inet_addr()` | IP转换 | 字符串→二进制 |
| `close()` | 关闭socket | 释放资源 |

---

## 线程安全

### 当前设计

- **主线程**：ROS回调（`onControl`），调用 `UdpComm::send()`
- **接收线程**：`receiveLoop()`，调用 `recvfrom()` 和回调

### 安全分析

- `send()` 和 `receive()` 使用不同方向，无冲突
- socket 文件描述符创建后不再修改，线程安全
- 回调函数在主线程？不，在接收线程，需要注意ROS的线程安全

**注意**：ROS的 `publish()` 是线程安全的，可以在接收线程直接调用。

---

## 编译依赖

### 旧版本需要
```cmake
find_package(Boost REQUIRED COMPONENTS system)
target_link_libraries(... Boost::system)
```

### 新版本
```cmake
# 无需额外依赖
# 原生socket API已包含在系统库中
```

---

## 调试技巧

### 查看socket状态
```bash
# 查看UDP端口
netstat -anu | grep 9000
ss -anu | grep 9000

# 抓包
sudo tcpdump -i lo udp port 9000 -X
```

### 测试连通性
```bash
# 终端1（监听）
nc -u -l 9001

# 终端2（发送）
echo "test" | nc -u 127.0.0.1 9001
```
