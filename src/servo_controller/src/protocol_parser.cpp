#include "servo_controller/protocol_parser.h"
#include <cstring>

ProtocolParser::ProtocolParser() {}
ProtocolParser::~ProtocolParser() {}

std::vector<DataBlock> ProtocolParser::parse(const uint8_t* buf, size_t len) {
    std::vector<DataBlock> blocks;
    
    // 检查最小长度
    if (len < protocol::MIN_PACK_SIZE) {
        return blocks;
    }
    
    // 检查包头
    if (buf[0] != protocol::HEAD1 || buf[1] != protocol::HEAD2) {
        return blocks;
    }
    
    // 获取数据长度
    uint16_t data_len = fromLE16(buf + 2);
    
    // 验证总长度
    size_t total_len = 2 + 2 + data_len + 1;  // head + len + data + checksum
    if (len < total_len) {
        return blocks;
    }
    
    // 验证校验和
    uint8_t recv_sum = buf[total_len - 1];
    if (!checkSum(buf + 4, data_len, recv_sum)) {
        return blocks;
    }
    
    // 解析数据块
    size_t offset = 4;
    while (offset + protocol::BLOCK_LEN <= 4 + data_len) {
        DataBlock blk;
        blk.id = fromLE16(buf + offset);
        memcpy(blk.data.data(), buf + offset + 2, 8);
        blocks.push_back(blk);
        offset += protocol::BLOCK_LEN;
    }
    
    return blocks;
}

std::vector<uint8_t> ProtocolParser::build(uint16_t id, const uint8_t* data) {
    std::vector<uint8_t> packet;
    
    // 包头
    packet.push_back(protocol::HEAD1);
    packet.push_back(protocol::HEAD2);
    
    // 数据长度 (1个数据块 = 10字节)
    uint16_t len = protocol::BLOCK_LEN;
    packet.push_back(len & 0xFF);
    packet.push_back((len >> 8) & 0xFF);
    
    // 数据块: ID (小端)
    packet.push_back(id & 0xFF);
    packet.push_back((id >> 8) & 0xFF);
    
    // 数据块: data (8字节)
    for (int i = 0; i < 8; i++) {
        packet.push_back(data[i]);
    }
    
    // 校验和
    uint8_t sum = calcSum(packet.data() + 4, protocol::BLOCK_LEN);
    packet.push_back(sum);
    
    return packet;
}

AngleFeedback ProtocolParser::parseAngle(const DataBlock& blk) {
    AngleFeedback fb;
    
    if (blk.id != protocol::FB_ANGLE) {
        return fb;
    }
    
    // 解析方位角度 (字节1-2)
    fb.raw_az = fromLE16S(blk.data.data());
    // 18100 是无效值
    if (fb.raw_az == 18100) {
        fb.valid = false;
    } else {
        fb.azimuth = fb.raw_az / 100.0f;
        fb.valid = true;
    }
    
    // 解析俯仰角度 (字节3-4)
    fb.raw_el = fromLE16S(blk.data.data() + 2);
    if (fb.raw_el == 18100) {
        fb.valid = false;
    } else {
        fb.elevation = fb.raw_el / 100.0f;
    }
    
    return fb;
}

StatusFeedback ProtocolParser::parseStatus(const DataBlock& blk) {
    StatusFeedback fb;
    
    if (blk.id != protocol::FB_STATUS) {
        return fb;
    }
    
    memcpy(fb.raw.data(), blk.data.data(), 8);
    
    // 字节1: 自检反馈 (bit1-0)
    fb.self_check = blk.data[0] & 0x03;
    
    // 字节3: 方位电机故障
    fb.az_fault = (blk.data[2] >> 7) & 0x01;
    fb.az_fault_code = blk.data[2] & 0x7F;
    
    // 字节4: 俯仰电机故障
    fb.el_fault = (blk.data[3] >> 7) & 0x01;
    fb.el_fault_code = blk.data[3] & 0x7F;
    
    fb.valid = true;
    return fb;
}

bool ProtocolParser::checkSum(const uint8_t* buf, size_t len, uint8_t sum) {
    return calcSum(buf, len) == sum;
}

uint8_t ProtocolParser::calcSum(const uint8_t* buf, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += buf[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

uint16_t ProtocolParser::toLE16(uint16_t v) {
    return v;  // 小端系统直接返回
}

uint16_t ProtocolParser::fromLE16(const uint8_t* p) {
    return p[0] | (p[1] << 8);
}

int16_t ProtocolParser::fromLE16S(const uint8_t* p) {
    return static_cast<int16_t>(fromLE16(p));
}
