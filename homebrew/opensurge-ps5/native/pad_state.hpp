// SPDX-License-Identifier: GPL-3.0-or-later
// Native PS5 buffered scePadRead bridge: sample-free polls are NOT releases.
// Preserve held input on frames where the pad has no *new* samples.
#pragma once
#include "core/input.hpp"
#include <cstddef>
#include <cstdint>

namespace surgepreview {

struct PadFrame {
    std::uint32_t held=0;
    std::uint32_t pressed=0;
    std::uint32_t released=0;
    std::uint32_t raw_buttons=0;
    bool connected=false;
};

class PadState {
public:
    template <typename Sample>
    PadFrame update(const Sample* samples,std::size_t count) {
        PadFrame frame{};
        // scePadRead returns 0 when there are no NEW events, not when the
        // user releases all buttons. Last known state must persist.
        if(count==0) {
            ++empty_polls_;
            frame.held=held_;
            frame.raw_buttons=raw_buttons_;
            frame.connected=connected_;
            return frame;
        }
        for(std::size_t i=0;i<count;++i) {
            const auto& s=samples[i];
            if(!s.connected || (s.buttons & hui::pad_bits::kIntercepted)!=0) {
                frame.released|=held_;
                held_=0;raw_buttons_=0;connected_=false;
                ++disconnects_;
                continue;
            }
            connected_=true;
            raw_buttons_=s.buttons;
            std::uint32_t bits=s.buttons & ~hui::pad_bits::kIntercepted;
            // D-pad takes precedence if the stick points the other way.
            if((bits&(hui::pad_bits::kLeft|hui::pad_bits::kRight))==0){
                if(s.left_x<64)bits|=hui::pad_bits::kLeft;
                else if(s.left_x>192)bits|=hui::pad_bits::kRight;
            }
            frame.pressed |= bits & ~held_;
            frame.released |= held_ & ~bits;
            held_=bits;
            ++samples_seen_;
        }
        frame.held=held_;
        frame.raw_buttons=raw_buttons_;
        frame.connected=connected_;
        return frame;
    }
    std::uint64_t empty_polls() const { return empty_polls_; }
    std::uint64_t samples_seen() const { return samples_seen_; }
    std::uint64_t disconnects() const { return disconnects_; }

private:
    std::uint32_t held_=0;
    std::uint32_t raw_buttons_=0;
    bool connected_=false;
    std::uint64_t empty_polls_=0,samples_seen_=0,disconnects_=0;
};
} // namespace surgepreview
