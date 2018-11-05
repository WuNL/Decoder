//
// Created by wunl on 18-8-7.
//

#ifndef ENCODER_VIDEOENCODERFACTORY_H
#define ENCODER_VIDEOENCODERFACTORY_H

#include <bits/unique_ptr.h>
//#include "decoder.h"
#include "h264Decoder.h"
#include "h265Decoder.h"
#include "fakeDecoder.h"

class video_decoder_factory
{
public:
    static std::unique_ptr<decoder> create (initParams &p);
};


#endif //ENCODER_VIDEOENCODERFACTORY_H
