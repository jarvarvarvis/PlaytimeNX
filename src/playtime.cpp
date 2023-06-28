#include "playtime.hpp"

#include "string_format.hpp"

Playtime::Playtime() {}

Playtime::Playtime(u64 hours, u64 minutes, u64 seconds)
    : hours(hours), minutes(minutes), seconds(seconds) {
}

std::string Playtime::toString() {
    return string_format("%02d:%02d:%02d", this->hours, this->minutes, this->seconds);
}

u64 Playtime::totalSeconds() {
    return this->seconds + this->minutes * 60 + this->hours * 60 * 60;
}

