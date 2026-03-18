#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""模拟执行器 - 用于本地测试"""

import socket
import struct
import time
import sys

def build_packet(packet_id, data):
    """构建数据包: AA 55 + 长度 + ID + 数据 + 校验和"""
    packet = bytearray([0xAA, 0x55, 0x0A, 0x00])  # 包头 + 长度(10)
    packet.append(packet_id & 0xFF)               # ID低字节
    packet.append((packet_id >> 8) & 0xFF)        # ID高字节
    packet.extend(data)                           # 8字节数据
    checksum = sum(packet[4:14]) & 0xFF           # 校验和
    packet.append(checksum)
    return bytes(packet)

class MockServo:
    def __init__(self):
        # 创建UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(('127.0.0.1', 9001))
        self.sock.settimeout(0.05)
        
        # 伺服状态
        self.angle_az = 0
        self.angle_el = 0
        self.target_az = 0
        self.target_el = 0
        self.servo_on = False
        self.self_check_status = 0
        
        print("=" * 50)
        print("模拟执行器已启动")
        print("监听: 127.0.0.1:9001")
        print("发送到: 127.0.0.1:9000")
        print("=" * 50)
    
    def handle_command(self, data):
        """处理接收到的命令"""
        if len(data) < 15 or data[0] != 0xAA or data[1] != 0x55:
            return
        
        packet_id = data[4] | (data[5] << 8)
        cmd = data[6:14]
        
        if packet_id == 0x6401:  # 控制指令
            # 伺服开关 (bit3-2)
            servo_ctrl = (cmd[0] >> 2) & 0x03
            if servo_ctrl == 1:
                self.servo_on = True
                print("[CMD] 开伺服")
            elif servo_ctrl == 2:
                self.servo_on = False
                print("[CMD] 关伺服")
            
            # 自检 (bit1)
            if (cmd[0] >> 1) & 0x01:
                self.self_check_status = 1 if self.servo_on else 2
                print("[CMD] 自检: %s" % ("成功" if self.self_check_status == 1 else "失败"))
                # 发送状态反馈
                status_data = bytes([self.self_check_status, 0, 0, 0, 0, 0, 0, 0])
                self.sock.sendto(build_packet(0x4607, status_data), ('127.0.0.1', 9000))
            
            # 模式 (bit7-5)
            mode = (cmd[2] >> 5) & 0x07
            if mode == 2:
                print("[CMD] 切换调转模式")
            
            # 归零 (bit3-2 of byte2)
            homing = (cmd[1] >> 2) & 0x03
            if homing:
                self.target_az = 0
                self.target_el = 0
                print("[CMD] 归零")
        
        elif packet_id == 0x6403:  # 角度指令
            self.target_az = struct.unpack('<h', bytes(cmd[4:6]))[0]
            self.target_el = struct.unpack('<h', bytes(cmd[6:8]))[0]
            print("[CMD] 角度目标: az=%.2f, el=%.2f" % (self.target_az/100.0, self.target_el/100.0))
    
    def update(self):
        """更新位置和运动"""
        if not self.servo_on:
            return
        
        # 模拟运动：逐步接近目标
        for _ in range(5):  # 每次更新5步
            diff_az = self.target_az - self.angle_az
            diff_el = self.target_el - self.angle_el
            
            if abs(diff_az) > 5:
                self.angle_az += 5 if diff_az > 0 else -5
            else:
                self.angle_az = self.target_az
                
            if abs(diff_el) > 5:
                self.angle_el += 5 if diff_el > 0 else -5
            else:
                self.angle_el = self.target_el
    
    def send_feedback(self):
        """发送角度反馈"""
        if self.servo_on:
            angle_data = struct.pack('<hh', self.angle_az, self.angle_el) + bytes(4)
            self.sock.sendto(build_packet(0x4603, angle_data), ('127.0.0.1', 9000))
    
    def run(self):
        """主循环"""
        last_feedback = 0
        
        while True:
            try:
                # 接收命令
                data, addr = self.sock.recvfrom(1024)
                self.handle_command(data)
            except socket.timeout:
                pass
            except KeyboardInterrupt:
                break
            
            # 更新位置
            self.update()
            
            # 50Hz 发送反馈 (每20ms)
            now = time.time()
            if now - last_feedback >= 0.02:
                self.send_feedback()
                last_feedback = now
        
        self.sock.close()
        print("\n模拟执行器已停止")

if __name__ == '__main__':
    try:
        MockServo().run()
    except Exception as e:
        print("错误:", e)
        sys.exit(1)
