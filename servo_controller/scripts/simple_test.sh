#!/bin/bash
# 简单集成测试脚本

echo "=========================================="
echo "    伺服控制器 - 简单测试脚本"
echo "=========================================="
echo ""
echo "请确保："
echo "  1. roscore 已在运行"
echo "  2. 控制器节点已启动 (roslaunch servo_controller servo_controller.launch)"
echo "  3. 模拟执行器已启动 (rosrun servo_controller mock_servo.py)"
echo ""
read -p "按回车键开始测试..."

echo ""
echo ">>> 1. 开伺服"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [4, 0, 0, 0, 0, 0, 0, 0]}" --once
sleep 1

echo ""
echo ">>> 2. 自检"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [2, 0, 0, 0, 0, 0, 0, 0]}" --once
sleep 1

echo ""
echo ">>> 3. 切换调转模式"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [0, 0, 64, 0, 0, 0, 0, 0]}" --once
sleep 1

echo ""
echo ">>> 4. 发送角度 (30度方位, 15度俯仰)"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25603, data: [0, 0, 0, 0, 48, 117, 152, 58]}" --once
sleep 3

echo ""
echo ">>> 5. 归零"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [0, 16, 0, 0, 0, 0, 0, 0]}" --once
sleep 3

echo ""
echo ">>> 6. 关伺服"
rostopic pub /servo_controller/control/raw servo_controller/RawUdpData \
  "{packet_id: 25601, data: [8, 0, 0, 0, 0, 0, 0, 0]}" --once

echo ""
echo ">>> 测试完成！"
