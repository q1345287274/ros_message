#ifndef PROTOCOL_PARSER_H
#define PROTOCOL_PARSER_H

#include <cstdint>
#include <vector>
#include <array>

// 协议常量
namespace protocol
{
    static constexpr uint8_t HEAD1 = 0xAA;
    static constexpr uint8_t HEAD2 = 0x55;
    static constexpr size_t BLOCK_LEN = 10;  // ID(2B) + data(8B)
    static constexpr size_t MIN_PACK_SIZE = 5 + BLOCK_LEN; // head(2) + len(2) + block(10) + checksum(1)
    
    // IDs
    static constexpr uint16_t CMD_CONTROL = 0x6401;  // 控制指令
    static constexpr uint16_t CMD_ANGLE = 0x6403;    // 角度控制
    static constexpr uint16_t FB_ANGLE = 0x4603;     // 角度反馈
    static constexpr uint16_t FB_STATUS = 0x4607;    // 状态反馈
}

// 数据块
struct DataBlock {
    uint16_t id;
    std::array<uint8_t, 8> data;
    DataBlock() : id(0) { data.fill(0); }
};

// 角度反馈
struct AngleFeedback {
    float azimuth;      // 方位角度(度)
    float elevation;    // 俯仰角度(度)
    int16_t raw_az;     // 原始方位
    int16_t raw_el;     // 原始俯仰
    bool valid;
    AngleFeedback() : azimuth(0), elevation(0), raw_az(0), raw_el(0), valid(false) {}
};

// 状态反馈
struct StatusFeedback {
    uint8_t self_check;     // 自检: 0=无效,1=成功,2=失败
    uint8_t az_fault;       // 方位故障
    uint8_t az_fault_code;  // 方位故障码
    uint8_t el_fault;       // 俯仰故障
    uint8_t el_fault_code;  // 俯仰故障码
    std::array<uint8_t, 8> raw;
    bool valid;
    StatusFeedback() : self_check(0), az_fault(0), az_fault_code(0), 
                       el_fault(0), el_fault_code(0), valid(false) { raw.fill(0); }
};

class ProtocolParser {
public:
    ProtocolParser();
    ~ProtocolParser();
    
    // 解析UDP数据，返回数据块列表
    std::vector<DataBlock> parse(const uint8_t* buf, size_t len);
    
    // 构建发送数据包
    std::vector<uint8_t> build(uint16_t id, const uint8_t* data);
    
    // 解析角度反馈
    static AngleFeedback parseAngle(const DataBlock& blk);
    
    // 解析状态反馈
    static StatusFeedback parseStatus(const DataBlock& blk);
    
    // 小端转换
    static uint16_t toLE16(uint16_t v);
    static uint16_t fromLE16(const uint8_t* p);
    static int16_t fromLE16S(const uint8_t* p);
    
private:
    bool checkSum(const uint8_t* buf, size_t len, uint8_t sum);
    uint8_t calcSum(const uint8_t* buf, size_t len);
};

#endif
