# 伺服自检和故障反馈（解析后）
# 对应协议 ID: 0x4607

# 自检状态
uint8 SELF_CHECK_INVALID = 0
uint8 SELF_CHECK_SUCCESS = 1
uint8 SELF_CHECK_FAILED = 2

uint8 self_check_status    # 自检状态: 0=无效, 1=成功, 2=失败

# 方位电机故障
uint8 azimuth_motor_fault  # 0=无故障, 1=有故障
uint8 azimuth_fault_code   # 故障码

# 俯仰电机故障
uint8 elevation_motor_fault  # 0=无故障, 1=有故障
uint8 elevation_fault_code   # 故障码

# 故障码定义
uint8 FAULT_BUS_OVERVOLTAGE = 1    # 母线电压过压
uint8 FAULT_BUS_UNDERVOLTAGE = 2   # 母线电压欠压
uint8 FAULT_OTHER = 255            # 其他故障类型

# 原始数据（调试用）
uint8[8] raw_data
