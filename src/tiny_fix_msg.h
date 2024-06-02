#include <iostream>
#include <string>
#include <stdlib.h>

namespace TinyFix {

enum class HeatBeatStyle : uint8_t
{
    Aggressive =
        0,   // send heartbeat at each timer event, even if other message is sent in the period
    Passive, // if some msgs are send, heartbeat will be skipped

    HeatBeatStyle_Count
};

struct DefaultTinyFixMsgComponents
{
    bool FIELD_IN_ORDER = true;
    uint16_t DEFAULT_ALLOC_SIZE = 128;
};

template<typename TinyFixMsgComponentsT>
class TinyFixMsgBase
{
public:
    static constexpr bool FIELD_IN_ORDER = TinyFixMsgComponentsT::FIELD_IN_ORDER;
    static constexpr uint16_t DEFAULT_ALLOC_SIZE = TinyFixMsgComponentsT::DEFAULT_ALLOC_SIZE;

    TinyFixMsgBase();
    TinyFixMsgBase( size_t size );
    TinyFixMsgBase( char* memblk, size_t size );

    // Quickfix Compatible Methods
    [[deprecated]]
    void setField( int field, const std::string& str );
    [[deprecated]]
    void setField( int field, std::string_view str );
    [[deprecated]]
    void setField( int field, const char* str, size_t strlen );

    // Zero Copy Style Interface
    char* get_write_head();
    void advance(size_t size);
    char* reserve_block(size_t size);

    void gen_tail();
    size_t get_msg_size();

private:
    std::unique_ptr<char[]> memblk_;
    char* const data_;
    uint16_t data_len_;
    uint16_t tail_pos_;
};

template<typename TinyFixMsgComponentsT>
TinyFixMsgBase<TinyFixMsgComponentsT>::TinyFixMsgBase()
: memblk_
{}

template<typename TinyFixMsgComponentsT>
TinyFixMsgBase<TinyFixMsgComponentsT>::TinyFixMsgBase(size_t size )
{}

template<typename TinyFixMsgComponentsT>
TinyFixMsgBase<TinyFixMsgComponentsT>::TinyFixMsgBase(char* memblk, size_t size)
{}


} // namespace TinyFix