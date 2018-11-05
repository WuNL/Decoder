//
// Created by sdt on 18-10-30.
//

#ifndef DECODER_H265DECODER_H
#define DECODER_H265DECODER_H

#include "decoder.h"
#include "config.h"

class h265Decoder: public decoder
{
public:
    void run () override;

    explicit h265Decoder (initParams p);

    virtual bool initialized () override;

    int join () override;

    int initDecoder () override;

    ~h265Decoder () override;

    void getDataAndSetMfxBSLengthZero (coded_video_buffer &codeced);

//    int updateBitrate (int target_kbps, int target_fps) override;

private:
    int decodeBuffer (void *in, void *out) {}

    static void *start (void *threadarg);

    void decodeBuffer ();

    int decodeBuffer (raw_video_buffer &raw, coded_video_buffer &codeced);

private:

    mfxStatus sts;
    mfxIMPL impl;
    mfxVersion ver;
    MFXVideoSession session;
    MFXVideoDECODE* mfxDEC;
    mfxVideoParam mfxVideoParams;
    mfxBitstream mfxBS;

    mfxFrameSurface1** pmfxSurfaces;
    mfxU8 *surfaceBuffers;
    mfxU16 numSurfaces;

    FILE *fSink;
    FILE *fRawSink;

    bool initFinish;

};


#endif //DECODER_H265DECODER_H
