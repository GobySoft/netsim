#ifndef MODEMSIMGROUPS20170815H
#define MODEMSIMGROUPS20170815H

#include "goby/middleware/group.h"

namespace groups
{    
    constexpr goby::Group audio_in{"audio_in"};
    constexpr goby::Group audio_out{"audio_out"};
    constexpr goby::Group detector_data{"detector_data"};
    constexpr goby::Group impulse_request{"impulse_request"};
    constexpr goby::Group impulse_response{"impulse_response"};
}

#endif
