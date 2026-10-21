// Regression: real PS5 scePadRead returns 0 between input samples.
// Holding RIGHT must continue moving and must NOT generate fake X taps.
#include "pad_state.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
using namespace surgepreview;
struct MockPad {
    std::uint32_t buttons=0;
    std::uint8_t left_x=128;
    bool connected=true;
};
static constexpr std::uint32_t R=hui::pad_bits::kRight;
static constexpr std::uint32_t X=hui::pad_bits::kCross;
static constexpr std::uint32_t S=hui::pad_bits::kSquare;
int main(){
    PadState pad;
    MockPad right{R,128,true};
    auto f=pad.update(&right,1);
    assert(f.pressed==R && f.held==R);
    std::uint32_t fake_jump_edges=0;
    int held_right_frames=0;
    for(int i=0;i<600;++i){
        // No new scePadRead packet in 3 out of 4 render frames.
        f=i%4 ? pad.update(&right,0) : pad.update(&right,1);
        if(f.held&R)++held_right_frames;
        if(f.pressed&X)++fake_jump_edges;
        if(f.pressed&R)assert(false && "Repeated RIGHT press while still held");
    }
    assert(held_right_frames==600 && fake_jump_edges==0);
    assert(pad.empty_polls()==450);
    // CROSS is pressed exactly once and stays held while the pad is silent.
    MockPad both{R|X,128,true};
    f=pad.update(&both,1);
    assert((f.pressed&X)!=0 && (f.held&R));
    for(int i=0;i<30;++i){
        f=pad.update(&both,0);
        assert(f.held==(R|X) && f.pressed==0);
    }
    f=pad.update(&both,1);
    assert(f.pressed==0);
    f=pad.update(&right,1);
    assert(f.released&X);
    f=pad.update(&both,1);
    assert(f.pressed==X);
    // A tap and release *within* one scePadRead batch is still one tap.
    MockPad batch[2]={{R|S,128,true},{R,128,true}};
    f=pad.update(batch,2);
    assert((f.pressed&S)!=0 && (f.held&S)==0);
    f=pad.update(batch,0);
    assert((f.pressed&S)==0);
    // Analog right gets a held direction bit, but NEVER invents cross.
    MockPad stick{0,255,true};
    f=pad.update(&stick,1);
    assert((f.held&R)!=0 && (f.held&X)==0);
    f=pad.update(&stick,0);
    assert((f.held&R)!=0);
    MockPad disconnect{0,128,false};
    f=pad.update(&disconnect,1);
    assert(f.held==0 && !f.connected);
    f=pad.update(&disconnect,0);
    assert(f.held==0);
    std::printf("PASS PAD hold-right=600/600 fake-jumps=%u empty-polls=%llu real-disconnects=%llu\n",
      fake_jump_edges,(unsigned long long)pad.empty_polls(),
      (unsigned long long)pad.disconnects());
}
