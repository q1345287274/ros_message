# 伺服角度反馈（解析后）
# 对应协议 ID: 0x4603

# 方位角度: 单位为度，比例尺100
# 原始数据范围 [-18000, 18000]，转换为实际角度需要除以100
float32 azimuth_angle

# 俯仰角度: 单位为度，比例尺100
# 原始数据范围 [-18000, 18000]，转换为实际角度需要除以100
float32 elevation_angle

# 原始数据（调试用）
int16 raw_azimuth
int16 raw_elevation
