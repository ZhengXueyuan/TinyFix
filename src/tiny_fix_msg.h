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
    bool     FIELD_IN_ORDER = true;
    uint16_t DEFAULT_ALLOC_SIZE = 128;
};

template<typename TinyFixMsgComponentsT>
class TinyFixMsgBase
{
public:
    static constexpr bool        FIELD_IN_ORDER = TinyFixMsgComponentsT::FIELD_IN_ORDER;
    static constexpr uint16_t    DEFAULT_ALLOC_SIZE = TinyFixMsgComponentsT::DEFAULT_ALLOC_SIZE;
    inline static constexpr char Spiliter{ '\x01' };

    TinyFixMsgBase();
    TinyFixMsgBase( size_t size );

    // Quickfix Compatible Methods
    [[deprecated]]
    void setField( int field, const std::string& str );
    [[deprecated]]
    void setField( int field, const char* str, size_t strlen );

    // Zero Copy Style Interface
    std::tuple<char*,size_t> get_write_head();
    void  advance( size_t size );
    char* reserve_block( size_t size );

    void   gen_tail();
    size_t get_msg_size();
    void   expand( size_t target_size );
    int    getNumberLengthLog( int number );

private:
    std::unique_ptr<char[]> data_;
    uint16_t                data_len_;
    uint16_t                tail_pos_;
};

template<typename TinyFixMsgComponentsT>
TinyFixMsgBase<TinyFixMsgComponentsT>::TinyFixMsgBase()
    : data_( std::make_unique<char[]>( DEFAULT_ALLOC_SIZE ) )
    , data_len_( DEFAULT_ALLOC_SIZE )
    , tail_pos_( 0 )
{
}

template<typename TinyFixMsgComponentsT>
TinyFixMsgBase<TinyFixMsgComponentsT>::TinyFixMsgBase( size_t size )
    : data_( std::make_unique<char[]>( size ) )
    , data_len_( DEFAULT_ALLOC_SIZE )
    , tail_pos_( 0 )
{
}

template<typename TinyFixMsgComponentsT>
void TinyFixMsgBase<TinyFixMsgComponentsT>::expand( size_t target_size )
{
    size_t new_size = data_len_ * 2;
    while( new_size < target_size )
    {
        new_size *= 2;
    }
    std::unique_ptr<char[]> tmp{ std::make_unique<char[]>( new_size ) };
    memcpy( data_.get(), tmp.get(), data_len_ );
    std::swap( data_, tmp );
    data_len_ = new_size;
}

template<typename TinyFixMsgComponentsT>
int TinyFixMsgBase<TinyFixMsgComponentsT>::getNumberLengthLog( int number )
{
    if( number == 0 )
        return 1;
    if( number < 0 )
        number = -number;
    return static_cast<int>( std::floor( std::log10( number ) ) ) + 1;
}

template<typename TinyFixMsgComponentsT>
void TinyFixMsgBase<TinyFixMsgComponentsT>::setField( int field, const std::string& str )
{
    // expand buffer
    if( data_len_ - tail_pos_ < str.length() )
    {
        expand( str.length() + tail_pos_ );
    }
    sprintf( &data_[tail_pos_], "%d=%s\x01", field, str.c_str() );
    tail_pos_ += str.length() + getNumberLengthLog( field ) + 2;
}

template<typename TinyFixMsgComponentsT>
void TinyFixMsgBase<TinyFixMsgComponentsT>::setField( int field, const char* str, size_t strlen )
{
    // expand buffer
    if( data_len_ - tail_pos_ < strlen )
    {
        expand( strlen + tail_pos_ );
    }
    sprintf( &data_[tail_pos_], "%d=%s\x01", field, str );
    tail_pos_ += strlen + getNumberLengthLog( field ) + 2;
}

template<typename TinyFixMsgComponentsT>
std::tuple<char*,size_t> TinyFixMsgBase<TinyFixMsgComponentsT>::get_write_head()
{
    return std::make_tuple(&data_[tail_pos_], data_len_ - tail_pos_);
}

template<typename TinyFixMsgComponentsT>
void TinyFixMsgBase<TinyFixMsgComponentsT>::advance( size_t size )
{
    if( size + tail_pos_ > data_len_ )
    {
        expand( size + tail_pos_);
    }
    tail_pos_ += size;
}

template<typename TinyFixMsgComponentsT>
char* TinyFixMsgBase<TinyFixMsgComponentsT>::reserve_block( size_t size )
{
    if( size + tail_pos_ > data_len_ )
    {
        expand( size + tail_pos_);
    }
    auto tmp = &data_[tail_pos_];
    tail_pos_ += size;
    return tmp;
}

template<typename TinyFixMsgComponentsT>
void TinyFixMsgBase<TinyFixMsgComponentsT>::gen_tail()
{
    
}

template<typename TinyFixMsgComponentsT>
size_t TinyFixMsgBase<TinyFixMsgComponentsT>::get_msg_size()
{
    return tail_pos_;
}


} // namespace TinyFix