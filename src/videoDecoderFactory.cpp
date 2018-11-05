//
// Created by wunl on 18-8-7.
//

#include <include/videoDecoderFactory.h>

#include "videoDecoderFactory.h"
#include "config.h"

std::unique_ptr<decoder> video_decoder_factory::create (initParams &p)
{
    std::unique_ptr<decoder> ret;
#ifdef USE_INTEL
    if (p.codec == std::string("h264"))
    {
        ret = std::unique_ptr<decoder>(new h264Decoder(p));
        return ret;
    }
    if (p.codec == std::string("h265"))
    {
        ret = std::unique_ptr<decoder>(new h265Decoder(p));
        return ret;
    } else
    {
//        ret = std::unique_ptr<decoder>(new fakeDecoder(p));
//        return ret;
    }
    return nullptr;
#else
    ret = std::unique_ptr<decoder>(new fakeDecoder(p));
    return ret;
#endif
}
