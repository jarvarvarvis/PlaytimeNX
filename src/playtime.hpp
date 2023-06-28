#pragma once

#include <string>

#include <switch.h>

class Playtime {
public:
    u64 hours, minutes, seconds;

    Playtime();
    Playtime(u64 hours, u64 minutes, u64 seconds);

    std::string toString();

    u64 totalSeconds();
};

