#pragma once

namespace vortex {
enum class TermKey {
    Left,
    Right,
    Home,
    End,
    Up,
    Down,
    Delete,
    Enter,
    Backspace,
    Char,
    Redraw // Special value to indicate a redraw request
};

using TermKeyHandler = void (*)(TermKey key, char32_t ch, void* user_data);
}