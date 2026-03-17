# 项目学习指南

## 推荐阅读顺序

### 第一步：了解整体（5分钟）

**1. 阅读 README.md**
```bash
cat src/servo_controller/README.md
```

**目标**：
- 项目是什么？干什么用的？
- 有哪些文件？
- 怎么编译和运行？

---

### 第二步：看接口定义（10分钟）

**2. 阅读消息定义文件**

按顺序阅读：
1. `msg/RawUdpData.msg` - 最基础的消息格式
2. `msg/ServoAngleFeedback.msg` - 角度反馈
3. `msg/ServoStatusFeedback.msg` - 状态反馈

**目标**：
- 理解上下位机之间交换什么数据
- 记住关键字段含义

---

### 第三步：理解协议（15分钟）

**3. 阅读架构文档**
```bash
cat src/servo_controller/docs/ARCHITECTURE.md
```

重点看：
- **第2节 数据流向** - 理解数据怎么流动
- **第4节 协议规范** - 理解数据包格式和ID含义

**4. 对照图片理解**
- 网络数据格式说明.png
- 终端-伺服.png
- 伺服-终端.png

**目标**：
- 明白协议数据包怎么组成
- 明白控制指令和反馈指令的区别

---

### 第四步：看头文件（15分钟）

**5. 阅读 protocol_parser.h**

```cpp
// 关注这几部分：
namespace protocol {
    // 常量定义（包头、ID）
}

struct DataBlock {
    // 数据块结构
};

struct AngleFeedback {
    // 解析后的角度
};

class ProtocolParser {
    // 类接口
};
```

**6. 阅读 udp_comm.h**

```cpp
class UdpComm {
    // UDP通信接口
};
```

**目标**：
- 知道每个类是干什么的
- 知道有哪些函数和参数

---

### 第五步：读实现代码（30分钟）

**7. 先看 protocol_parser.cpp**

这是基础，建议细读：

```cpp
// 先看 build() 函数 - 看怎么打包数据
// 再看 parse() 函数 - 看怎么解包数据
// 最后看 parseAngle() 和 parseStatus() - 看怎么解析特定数据
```

**关键理解**：
- 数据包格式：`AA 55` + 长度 + ID + 数据 + 校验和
- 小端字节序怎么转换
- 校验和怎么计算

**8. 再看 udp_communication.cpp**

了解Boost.Asio怎么用：

```cpp
// 重点看 init() - 初始化
// 和 doReceive() - 异步接收机制
```

**9. 最后看 servo_controller_node.cpp**

这是整合层，理解整体流程：

```cpp
// 重点看：
init()          // 初始化流程
onControl()     // 控制下发流程
onReceive()     // 反馈接收流程
```

**目标**：
- 明白ROS和UDP怎么衔接
- 明白回调函数的工作方式

---

### 第六步：看测试代码（20分钟）

**10. 阅读 mock_servo.py**

```bash
cat src/servo_controller/scripts/mock_servo.py
```

**建议**：边读边运行，加深理解

**11. 阅读 test_control.py**

了解如何发送控制指令

**目标**：
- 明白实际怎么使用这个库
- 明白协议字节怎么设置

---

### 第七步：动手实践（60分钟）

**12. 按测试文档跑一遍**

```bash
# 终端1
roscore

# 终端2
roslaunch servo_controller servo_controller.launch

# 终端3
rosrun servo_controller mock_servo.py

# 终端4
rosrun servo_controller test_control.py
```

**13. 修改代码尝试**

- 改一下输出日志，重新编译看效果
- 尝试添加一个新的控制指令
- 修改角度比例尺试试

---

## 学习路径总结

```
整体了解(README)
    ↓
接口定义(MSG文件)
    ↓
协议理解(ARCHITECTURE.md + 图片)
    ↓
类设计(头文件)
    ↓
实现细节(CPP文件)
    │   ├─ protocol_parser.cpp (基础)
    │   ├─ udp_communication.cpp (通信)
    │   └─ servo_controller_node.cpp (整合)
    ↓
使用示例(Python脚本)
    ↓
动手实践
```

## 重点难点

| 部分 | 难度 | 建议 |
|------|------|------|
| 消息定义 | ⭐ 简单 | 直接看，秒懂 |
| 协议格式 | ⭐⭐ 中等 | 对照图片看 |
| 小端转换 | ⭐⭐ 中等 | 记住公式，动手算几个例子 |
| Boost.Asio | ⭐⭐⭐ 较难 | 了解异步概念即可，不必深究 |
| 回调机制 | ⭐⭐ 中等 | 理解数据流向 |
| 位操作 | ⭐⭐ 中等 | 用计算器验证 |

## 常见问题

**Q: 看不懂Boost.Asio怎么办？**
> 不用深究，只需要知道它是异步接收UDP数据的，重点看数据怎么从UDP传到ROS。

**Q: 小端字节序怎么理解？**
> 举个例子：3000 = 0x0BB8
> - 大端：0B B8（高位在前）
> - 小端：B8 0B（低位在前）
> 本项目用小端，所以发送 `{184, 11}`

**Q: 校验和怎么算？**
> 从长度字段后开始，到校验和前所有字节相加，取低8位。

## 推荐学习资源

1. **ROS基础**：先学会 `rostopic`, `rosnode` 命令
2. **网络编程**：了解UDP基础
3. **C++11**：了解 lambda, smart pointer

## 检验学习成果

能回答这些问题说明学会了：
1. 控制指令怎么发送到执行器？
2. 角度反馈怎么解析出来的？
3. 如果要加一个新的控制指令，改哪里？
4. 怎么把比例尺从100改成1000？
