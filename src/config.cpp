#include "config.h"

#include <cassert>

namespace tomsolver {

std::string ToString(double value) noexcept {
    char buf[64];
    int ret = -1;

    // 绝对值过大 或者 绝对值过小，应该使用科学计数法来表示
    if ((std::abs(value) >= 1.0e16 || std::abs(value) <= 1.0e-16) && value != 0.0) {
        ret = snprintf(buf, sizeof(buf), "%.16e", value);

        int state = 0;
        int stripPos = sizeof(buf) - 1;
        int ePos = sizeof(buf) - 1;
        for (int i = ret - 1; i >= 0; --i) {
            switch (state) {
            case 0:
                if (buf[i] == 'e') {
                    ePos = i;
                    state = 1;
                    break;
                }
                break;
            case 1:
                if (buf[i] == '0') {
                    stripPos = i;
                    continue;
                }
                if (buf[i] == '.') {
                    stripPos = i;
                }
                goto end_of_scientific_loop;
            }
        }
    end_of_scientific_loop:
        // 如果没有尾后0
        if (stripPos == sizeof(buf) - 1) {
            return buf;
        }

        if (stripPos > ePos)
            assert(0 && "err: stripPos>ePos");
        int i = stripPos;
        int j = ePos;
        while (1) {
            buf[i] = buf[j];

            if (buf[j] == 0) {
                break;
            }

            ++i;
            ++j;
        }

        return buf;
    } else {
        ret = snprintf(buf, sizeof(buf), GetConfig().GetDoubleFormatStr(), value);
    }

    int stripPos = sizeof(buf) - 1;
    for (int i = ret - 1; i >= 0; --i) {
        if (buf[i] == '0') {
            stripPos = i;
            continue;
        }
        if (buf[i] == '.') {
            stripPos = i;
            break;
        }
        break;
    }
    buf[stripPos] = 0;
    return buf;
}

Config &GetConfig() noexcept {
    static Config config;
    return config;
}

Config::Config() {
    Reset();
}

void Config::Reset() noexcept {
    throwOnInvalidValue = true;
    epsilon = 1.0e-9;
    logLevel = LogLevel::WARN;
    maxIterations = 100;
    nonlinearMethod = NonlinearMethod::NEWTON_RAPHSON;
    initialValue = 1.0;
}

const char *Config::GetDoubleFormatStr() const noexcept {
    return doubleFormatStr;
}

} // namespace tomsolver