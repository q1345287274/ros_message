# 伺服控制器 ROS 包

基于ROS1和C++的伺服控制器，通过UDP与执行器通信。

## 功能概述

- **下发控制**: 从ROS话题获取原始数据(ID+8字节)，直接通过UDP转发给执行器
- **反馈获取**: 从执行器接收UDP数据，解析后通过ROS话题发布

## 协议说明

- **协议**: UDP
- **存储**: 小端模式
- **包头**: 0xAA 0x55
- **数据块**: ID(2字节) + 数据(8字节)

### IP配置

| 模式 | 上位机 | 执行器 |
|------|--------|--------|
| 本地测试 | 127.0.0.1:9000 | 127.0.0.1:9001 |
| 实际设备 | 192.168.1.8:9000 | 192.168.1.10:9001 |

默认使用本地测试配置，修改 `config/servo.yaml` 切换。

### 数据包ID

| 方向 | ID | 说明 |
|------|----|------|
| 终端→伺服 | 0x6401 | 伺服控制指令(开/关、自检、模式) |
| 终端→伺服 | 0x6403 | 角度控制指令 |
| 伺服→终端 | 0x4603 | 角度反馈(50Hz) |
| 伺服→终端 | 0x4607 | 状态反馈 |

## 文档

- [架构设计文档](docs/ARCHITECTURE.md)
- [测试指南文档](docs/TEST_GUIDE.md)

## 编译

```bash
cd /home/robot/wq_talk_ws
catkin_make
source devel/setup.bash
```

## 快速测试

### 本地测试（无需实际硬件）

需要开4个终端：

**终端1 - 启动ROS**
```bash
roscore
```

**终端2 - 启动控制器**
```bash
roslaunch servo_controller servo_controller.launch
```

**终端3 - 启动模拟执行器**
```bash
rosrun servo_controller mock_servo.py
```

**终端4 - 查看反馈**
```bash
rostopic echo /servo_controller/feedback/angle
```

然后发送测试命令：

```bash
# 开伺服 (0x04 = bit3-2=01)
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [4, 0, 0, 0, 0, 0, 0, 0]}" --once

# 自检
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [2, 0, 0, 0, 0, 0, 0, 0]}" --once

# 切换调转模式
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [0, 0, 64, 0, 0, 0, 0, 0]}" --once

# 发送角度 (30度方位, 15度俯仰)
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25603, data: [0, 0, 0, 0, 48, 117, 152, 58]}" --once
```

预期输出：
- 终端3显示：`[CMD] 开伺服`、`[CMD] 自检: 成功` 等
- 终端4显示：`azimuth_angle: 30.0`、`elevation_angle: 15.0`

### 使用交互式脚本

```bash
rosrun servo_controller test_control.py
```

输入：
- `s` - 开伺服
- `c` - 自检  
- `m` - 切换调转模式
- `a 30 15` - 发送角度
- `x` - 关伺服

### 使用一键测试脚本

```bash
rosrun servo_controller simple_test.sh
```

## 实际设备测试

1. 修改 `config/servo.yaml`：
```yaml
local_ip: "192.168.1.8"     # 你的电脑IP
remote_ip: "192.168.1.10"   # 执行器IP
```

2. 启动控制器：
```bash
roslaunch servo_controller servo_controller.launch
```

## 话题列表

| 话题名 | 类型 | 说明 |
|--------|------|------|
| /servo_controller/control/raw | RawUdpData | 控制下发（订阅） |
| /servo_controller/feedback/raw | RawUdpData | 原始反馈（发布） |
| /servo_controller/feedback/angle | ServoAngleFeedback | 角度反馈（发布） |
| /servo_controller/feedback/status | ServoStatusFeedback | 状态反馈（发布） |

## 消息类型

### RawUdpData
```
uint16 packet_id    # 数据包ID
uint8[8] data       # 8字节数据
```

### ServoAngleFeedback
```
float32 azimuth_angle      # 方位角度(度)
float32 elevation_angle    # 俯仰角度(度)
int16   raw_azimuth        # 原始数据
int16   raw_elevation
```

### ServoStatusFeedback
```
uint8 self_check_status       # 自检: 0=无效,1=成功,2=失败
uint8 azimuth_motor_fault     # 方位故障
uint8 elevation_motor_fault   # 俯仰故障
```
