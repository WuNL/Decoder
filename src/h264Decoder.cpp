//
// Created by sdt on 18-10-30.
//

#include <include/h264Decoder.h>

#include "h264Decoder.h"

void h264Decoder::run ()
{

}

h264Decoder::h264Decoder (initParams p): decoder(std::move(p)),
                                         insertIDR(false),
                                         updateBitrate_(false)
{

}

int h264Decoder::join ()
{
    return 0;
}

int h264Decoder::initDecoder ()
{
    return 0;
}

h264Decoder::~h264Decoder ()
{

}

void h264Decoder::getDataAndSetMfxBSLengthZero (coded_video_buffer &codeced)
{

}


void *h264Decoder::start (void *threadarg)
{
    return nullptr;
}

void h264Decoder::decodeBuffer ()
{

}

int h264Decoder::decodeBuffer (raw_video_buffer &raw, coded_video_buffer &codeced)
{
    return 0;
}

bool h264Decoder::initialized ()
{
    return false;
}

