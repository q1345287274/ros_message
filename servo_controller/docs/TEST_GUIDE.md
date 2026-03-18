# 快速测试指南

## 1. 编译

```bash
cd /home/robot/wq_talk_ws
catkin_make
source devel/setup.bash
```

## 2. 本地测试（3个终端）

### 终端1 - 启动控制器
```bash
roslaunch servo_controller servo_controller.launch
```

### 终端2 - 启动模拟执行器
```bash
rosrun servo_controller mock_servo.py
```

### 终端3 - 查看话题
```bash
# 查看角度反馈
rostopic echo /servo_controller/feedback/angle
```

## 3. 发送测试命令

在**新终端**中执行：

```bash
# 开伺服 (0x04: bit3-2=01)
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

# 关伺服 (0x08: bit3-2=10)
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [8, 0, 0, 0, 0, 0, 0, 0]}" --once
```

## 4. 使用交互式测试脚本

```bash
rosrun servo_controller test_control.py
```

输入命令：
- `s` - 开伺服
- `c` - 自检
- `m` - 切换调转模式
- `a 30 15` - 发送角度（30度方位，15度俯仰）
- `x` - 关伺服
- `q` - 退出

## 5. 预期输出

在终端2（模拟执行器）应看到：
```
[CMD] Servo ON
[CMD] Self-check: SUCCESS
[CMD] Mode: Rotation
[CMD] Angle target: az=30.00, el=15.00
```

在终端3（话题回显）应看到：
```
azimuth_angle: 30.0
elevation_angle: 15.0
```

## 6. 实际设备测试

修改 `config/servo.yaml`：
```yaml
local_ip: "192.168.1.8"     # 你的电脑IP
remote_ip: "192.168.1.10"   # 执行器IP
```

然后直接启动控制器（不需要模拟执行器）：
```bash
roslaunch servo_controller servo_controller.launch
```
