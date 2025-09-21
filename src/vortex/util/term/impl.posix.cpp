#ifndef _WIN32
#include <vortex/util/term/impl.posix.h>

// On Create
{
    if (!isatty(app.inFd)) {
        std::cerr << "stdin is not a TTY\n";
        return 1;
    }
    if (!enableRawInput(app)) {
        std::cerr << "failed to set raw mode\n";
        return 1;
    }
}

static bool enableRawInput(App& app)
{
    if (!isatty(app.inFd)) {
        return false;
    }
    if (tcgetattr(app.inFd, &app.origTerm) != 0) {
        return false;
    }
    termios raw = app.origTerm;
    cfmakeraw(&raw);
    // Keep ISIG so Ctrl+C still interrupts; remove if you want to handle it as input
    raw.c_lflag |= ISIG;
    if (tcsetattr(app.inFd, TCSANOW, &raw) != 0) {
        return false;
    }

    // Non-blocking fd
    int flags = fcntl(app.inFd, F_GETFL, 0);
    fcntl(app.inFd, F_SETFL, flags | O_NONBLOCK);
    return true;
}
static void restoreConsole(App& app)
{
    tcsetattr(app.inFd, TCSANOW, &app.origTerm);
}
static std::atomic_bool gotSigInt{ false };
static void handle_sigint(int)
{
    gotSigInt = true;
}



struct Utf8StreamDecoder {
    uint32_t codepoint = 0;
    int expected = 0;

    std::optional<char32_t> feed(uint8_t byte)
    {
        if (expected == 0) {
            if ((byte & 0x80) == 0) {
                return static_cast<char32_t>(byte);
            } else if ((byte & 0xE0) == 0xC0) {
                codepoint = byte & 0x1F;
                expected = 1;
                return std::nullopt;
            } else if ((byte & 0xF0) == 0xE0) {
                codepoint = byte & 0x0F;
                expected = 2;
                return std::nullopt;
            } else if ((byte & 0xF8) == 0xF0) {
                codepoint = byte & 0x07;
                expected = 3;
                return std::nullopt;
            } else {
                // Invalid start byte; drop
                return std::nullopt;
            }
        } else {
            if ((byte & 0xC0) != 0x80) {
                // Invalid continuation; reset
                expected = 0;
                return std::nullopt;
            }
            codepoint = (codepoint << 6) | (byte & 0x3F);
            expected--;
            if (expected == 0) {
                return static_cast<char32_t>(codepoint);
            }
            return std::nullopt;
        }
    }
};


static void write_bytes(App& app, const void* data, size_t len)
{
    const uint8_t* p = static_cast<const uint8_t*>(data);
    while (len > 0) {
        ssize_t n = ::write(app.outFd, p, len);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        p += n;
        len -= static_cast<size_t>(n);
    }
}
static void write_utf8(App& app, const std::string& s)
{
    if (!s.empty()) {
        write_bytes(app, s.data(), s.size());
    }
}

struct ParsedKey {
    SpecialKey type;
    char32_t ch = U'\0';
};

static std::optional<ParsedKey> try_parse_escape(std::vector<uint8_t>& q)
{
    if (q.size() < 2) {
        return std::nullopt;
    }
    if (q[0] != 0x1B) {
        return std::nullopt;
    }

    if (q[1] == '[') {
        // CSI sequences
        if (q.size() >= 3) {
            uint8_t c = q[2];
            if (c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'H' || c == 'F') {
                ParsedKey pk;
                switch (c) {
                case 'A':
                    pk.type = SpecialKey::Up;
                    break;
                case 'B':
                    pk.type = SpecialKey::Down;
                    break;
                case 'C':
                    pk.type = SpecialKey::Right;
                    break;
                case 'D':
                    pk.type = SpecialKey::Left;
                    break;
                case 'H':
                    pk.type = SpecialKey::Home;
                    break;
                case 'F':
                    pk.type = SpecialKey::End;
                    break;
                }
                q.erase(q.begin(), q.begin() + 3);
                return pk;
            }
            // CSI n~
            size_t i = 2;
            while (i < q.size() && (q[i] >= '0' && q[i] <= '9')) {
                ++i;
            }
            if (i < q.size() && q[i] == '~') {
                std::string num(reinterpret_cast<char*>(&q[2]), reinterpret_cast<char*>(&q[i]));
                ParsedKey pk;
                if (num == "1") {
                    pk.type = SpecialKey::Home;
                } else if (num == "4") {
                    pk.type = SpecialKey::End;
                } else if (num == "3") {
                    pk.type = SpecialKey::Delete;
                } else {
                    q.erase(q.begin(), q.begin() + (i + 1));
                    return std::nullopt;
                }
                q.erase(q.begin(), q.begin() + (i + 1));
                return pk;
            }
        }
        // Unknown/partial: consume ESC to avoid stalling
        q.erase(q.begin());
        return std::nullopt;
    } else if (q[1] == 'O') {
        // SS3 sequences
        if (q.size() >= 3) {
            uint8_t c = q[2];
            if (c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'H' || c == 'F') {
                ParsedKey pk;
                switch (c) {
                case 'A':
                    pk.type = SpecialKey::Up;
                    break;
                case 'B':
                    pk.type = SpecialKey::Down;
                    break;
                case 'C':
                    pk.type = SpecialKey::Right;
                    break;
                case 'D':
                    pk.type = SpecialKey::Left;
                    break;
                case 'H':
                    pk.type = SpecialKey::Home;
                    break;
                case 'F':
                    pk.type = SpecialKey::End;
                    break;
                }
                q.erase(q.begin(), q.begin() + 3);
                return pk;
            }
            q.erase(q.begin());
            return std::nullopt;
        }
        return std::nullopt;
    } else {
        q.erase(q.begin());
        return std::nullopt;
    }
}

static void handleParsedKey(App& app, const ParsedKey& pk)
{
    switch (pk.type) {
    case SpecialKey::Left:
        app.cursor = prev_cp(app.input, app.cursor);
        break;
    case SpecialKey::Right:
        app.cursor = next_cp(app.input, app.cursor);
        break;
    case SpecialKey::Home:
        app.cursor = 0;
        break;
    case SpecialKey::End:
        app.cursor = app.input.size();
        break;
    case SpecialKey::Up:
        historyUp(app);
        return; // redraw done
    case SpecialKey::Down:
        historyDown(app);
        return; // redraw done
    case SpecialKey::Delete:
        abortHistoryBrowseOnEdit(app);
        erase_next_cp(app.input, app.cursor);
        break;
    case SpecialKey::Backspace:
        abortHistoryBrowseOnEdit(app);
        app.cursor = erase_prev_cp(app.input, app.cursor);
        break;
    case SpecialKey::Enter: {
        std::string line = app.input;
        addToHistory(app, line);
        app.input.clear();
        app.cursor = 0;
        app.histIndex.reset();
        app.savedBeforeHistory.clear();
        printMessage(app, "You typed: " + line);
        break;
    }
    case SpecialKey::Char:
        abortHistoryBrowseOnEdit(app);
        insert_cp_utf8(app.input, app.cursor, pk.ch);
        app.cursor = next_cp(app.input, app.cursor);
        break;
    }
    redrawInput(app);
}

static void inputLoop(App& app)
{
    redrawInput(app);

    std::vector<uint8_t> q;
    q.reserve(256);
    Utf8StreamDecoder dec;

    while (app.running.load(std::memory_order_relaxed)) {
        if (gotSigInt.load(std::memory_order_relaxed)) {
            app.running = false;
            break;
        }
        struct pollfd pfd{ app.inFd, POLLIN, 0 };
        int rc = ::poll(&pfd, 1, 50);
        if (rc > 0 && (pfd.revents & POLLIN)) {
            uint8_t buf[256];
            ssize_t n = ::read(app.inFd, buf, sizeof(buf));
            if (n > 0) {
                q.insert(q.end(), buf, buf + n);
                while (!q.empty()) {
                    if (q[0] == 0x1B) {
                        auto pk = try_parse_escape(q);
                        if (pk.has_value()) {
                            handleParsedKey(app, *pk);
                        } else {
                            break; // need more
                        }
                        continue;
                    }
                    uint8_t b = q[0];

                    // Enter (CR/LF)
                    if (b == '\r' || b == '\n') {
                        q.erase(q.begin());
                        handleParsedKey(app, ParsedKey{ SpecialKey::Enter });
                        continue;
                    }
                    // Backspace (DEL or BS)
                    if (b == 0x7F || b == 0x08) {
                        q.erase(q.begin());
                        handleParsedKey(app, ParsedKey{ SpecialKey::Backspace });
                        continue;
                    }

                    // Decode UTF-8 to complete a code point, then insert it
                    auto maybe = dec.feed(b);
                    q.erase(q.begin());
                    if (maybe.has_value()) {
                        char32_t cp = *maybe;
                        if (cp >= U' ' || cp == U'\t') {
                            handleParsedKey(app, ParsedKey{ SpecialKey::Char, cp });
                        }
                    }
                }
            }
        } else if (rc == 0) {
            // timeout
        } else if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
    }
}
#endif