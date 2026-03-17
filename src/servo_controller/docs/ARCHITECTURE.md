# 架构文档

## 1. 数据流向

### 控制下发 (上位机 → 执行器)

```
ROS话题 /control/raw  -->  ROS节点  -->  UDP  -->  执行器
(ID + 8字节数据)         打包+校验和      发送
```

数据包格式:
```
AA 55 0A 00 [ID:2字节] [数据:8字节] [校验和:1字节]
```

### 反馈获取 (执行器 → 上位机)

```
执行器  -->  UDP  -->  ROS节点  -->  ROS话题
            接收     解包+解析      /feedback/angle
                                    /feedback/status
                                    /feedback/raw
```

---

## 2. 模块说明

### 2.1 ProtocolParser (协议解析)

```cpp
// 构建发送数据包
std::vector<uint8_t> build(uint16_t id, const uint8_t* data);

// 解析接收数据
std::vector<DataBlock> parse(const uint8_t* buf, size_t len);

// 解析特定反馈
AngleFeedback parseAngle(const DataBlock& blk);
StatusFeedback parseStatus(const DataBlock& blk);
```

### 2.2 UdpComm (UDP通信)

```cpp
bool init(local_ip, local_port, remote_ip, remote_port);
bool send(data, len);
void setCallback(callback);  // 接收回调
void startReceive();         // 启动异步接收
```

### 2.3 ServoNode (ROS节点)

```cpp
// 初始化：加载参数、创建话题、启动UDP
bool init();

// 控制回调：接收ROS控制指令，转发到UDP
void onControl(RawUdpData msg);

// 接收回调：接收UDP数据，解析后发布到ROS
void onReceive(uint8_t* data, size_t len);
```

---

## 3. 消息格式

### 3.1 RawUdpData (控制+原始反馈)

```
uint16 packet_id    # 数据包ID
uint8[8] data       # 8字节数据
```

### 3.2 ServoAngleFeedback (角度反馈)

```
float32 azimuth_angle      # 方位角度(度)
float32 elevation_angle    # 俯仰角度(度)
int16   raw_azimuth        # 原始数据
int16   raw_elevation
```

### 3.3 ServoStatusFeedback (状态反馈)

```
uint8 self_check_status       # 0=无效,1=成功,2=失败
uint8 azimuth_motor_fault     # 0=无,1=有
uint8 elevation_motor_fault   # 0=无,1=有
```

---

## 4. 协议规范

### 4.1 数据包ID

| 方向 | ID | 功能 |
|------|----|------|
| 终端→伺服 | 0x6401 | 伺服控制(开/关/自检/模式) |
| 终端→伺服 | 0x6403 | 角度控制 |
| 伺服→终端 | 0x4603 | 角度反馈(50Hz) |
| 伺服→终端 | 0x4607 | 状态反馈 |

### 4.2 控制指令 (0x6401)

```
字节1: [伺服控制 bit3-2] [自检 bit1] [查询 bit0]
       00=无效 01=开 10=关   0=无 1=自检   0=无 1=查询

字节2: [归零控制 bit3-2]
       00=无效 01=方位 10=俯仰 11=双方位

字节3: [工作模式 bit7-5]
       000=无效 001=手柄 010=调转
```

### 4.3 角度指令 (0x6403)

```
字节1-2: 方位速度 [-9000,9000]
字节3-4: 俯仰速度 [-9000,9000]
字节5-6: 方位角度 [-18000,18000], 比例尺100
字节7-8: 俯仰角度 [-18000,18000], 比例尺100
```

### 4.4 角度反馈 (0x4603)

```
字节1-2: 方位角度, 比例尺100, 18100=无效
字节3-4: 俯仰角度, 比例尺100, 18100=无效
```

### 4.5 状态反馈 (0x4607)

```
字节1: 自检状态 0=无效,1=成功,2=失败
字节3: [故障 bit7] [故障码 bit6-0]
字节4: [故障 bit7] [故障码 bit6-0]
```

---

## 5. 配置

### 本地测试
```yaml
local_ip: "127.0.0.1"
local_port: 9000
remote_ip: "127.0.0.1"
remote_port: 9001
```

### 实际设备
```yaml
local_ip: "192.168.1.8"
remote_ip: "192.168.1.10"
```
