#pragma once
#include <stdint.h>
struct VSConfig {
 uint32_t magic, version, p1, p2, stage, state, step, error;
};
static_assert(sizeof(VSConfig)==32, "Launcher protocol size");
inline bool valid_mecha(uint32_t id) { return id<103 && id!=10 && id!=20 && id!=23 && id!=49; }
inline bool valid_config(const VSConfig& c) {
 return c.magic==0x42565331 && c.version==1 && valid_mecha(c.p1) && valid_mecha(c.p2)
     && c.stage<=220 && c.stage!=9 && c.state==1;
}
