#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
伺服控制器测试脚本
提供简单的控制命令发送和反馈接收测试
"""

import rospy
import struct
from servo_controller.msg import RawUdpData, ServoAngleFeedback, ServoStatusFeedback

# 协议ID
CMD_CONTROL = 0x6401  # 伺服控制指令
CMD_ANGLE = 0x6403    # 角度控制

class ServoTester:
    def __init__(self):
        rospy.init_node('servo_tester')
        
        # 创建发布者
        self.pub = rospy.Publisher('/servo_controller/control/raw', RawUdpData, queue_size=10)
        
        # 创建订阅者
        rospy.Subscriber('/servo_controller/feedback/angle', ServoAngleFeedback, self.on_angle)
        rospy.Subscriber('/servo_controller/feedback/status', ServoStatusFeedback, self.on_status)
        
        rospy.loginfo("Servo tester initialized")
        rospy.loginfo("Available commands:")
        rospy.loginfo("  s - 开伺服")
        rospy.loginfo("  x - 关伺服")
        rospy.loginfo("  c - 自检")
        rospy.loginfo("  m - 切换至调转模式")
        rospy.loginfo("  a <az> <el> - 发送角度指令(度)")
        rospy.loginfo("  q - 退出")
    
    def send_control(self, byte1, byte2=0, byte3=0):
        """发送控制指令"""
        msg = RawUdpData()
        msg.packet_id = CMD_CONTROL
        msg.data = [byte1, byte2, byte3, 0, 0, 0, 0, 0]
        self.pub.publish(msg)
        rospy.loginfo("Control sent: [0x%02X, 0x%02X, 0x%02X]" % (byte1, byte2, byte3))
    
    def open_servo(self):
        """开伺服"""
        # bit3-2 = 01 (开伺服) -> 0x04
        self.send_control(0x04)
        rospy.loginfo("Open servo command sent")
    
    def close_servo(self):
        """关伺服"""
        # bit3-2 = 10 (关伺服) -> 0x08
        self.send_control(0x08)
        rospy.loginfo("Close servo command sent")
    
    def self_check(self):
        """自检"""
        # bit1 = 1 (自检)
        self.send_control(0x02)
        rospy.loginfo("Self-check command sent")
    
    def set_rotation_mode(self):
        """切换至调转模式"""
        # bit7-5 = 010 (调转模式)
        self.send_control(0, 0, 0x40)
        rospy.loginfo("Rotation mode command sent")
    
    def send_angle(self, azimuth, elevation):
        """发送角度控制指令"""
        msg = RawUdpData()
        msg.packet_id = CMD_ANGLE
        
        # 转换为原始数据 (比例尺100)
        az_raw = int(azimuth * 100)
        el_raw = int(elevation * 100)
        
        # 小端打包
        data = struct.pack('<hhhh', 0, 0, az_raw, el_raw)
        msg.data = [ord(b) for b in data] if isinstance(data, str) else list(data)
        
        self.pub.publish(msg)
        rospy.loginfo("Angle command sent: az=%.2f, el=%.2f" % (azimuth, elevation))
    
    def on_angle(self, msg):
        """角度反馈回调"""
        rospy.loginfo("Angle feedback: az=%.2f, el=%.2f [raw: %d, %d]" % 
                     (msg.azimuth_angle, msg.elevation_angle, 
                      msg.raw_azimuth, msg.raw_elevation))
    
    def on_status(self, msg):
        """状态反馈回调"""
        status_map = {0: "无效", 1: "成功", 2: "失败"}
        rospy.loginfo("Status feedback: self_check=%s, az_fault=%d, el_fault=%d" % 
                     (status_map.get(msg.self_check_status, "未知"),
                      msg.azimuth_motor_fault, msg.elevation_motor_fault))
    
    def run(self):
        """交互式运行"""
        import sys
        
        while not rospy.is_shutdown():
            try:
                cmd = raw_input("\nEnter command: ").strip().split()
            except:
                cmd = input("\nEnter command: ").strip().split()
            
            if not cmd:
                continue
            
            key = cmd[0].lower()
            
            if key == 'q':
                break
            elif key == 's':
                self.open_servo()
            elif key == 'x':
                self.close_servo()
            elif key == 'c':
                self.self_check()
            elif key == 'm':
                self.set_rotation_mode()
            elif key == 'a' and len(cmd) >= 3:
                try:
                    az = float(cmd[1])
                    el = float(cmd[2])
                    self.send_angle(az, el)
                except ValueError:
                    rospy.logerr("Invalid angle values")
            else:
                rospy.loginfo("Unknown command: %s" % key)

if __name__ == '__main__':
    tester = ServoTester()
    tester.run()
