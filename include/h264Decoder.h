//
// Created by sdt on 18-10-30.
//

#ifndef DECODER_H264DECODER_H
#define DECODER_H264DECODER_H

#include "decoder.h"
#include "config.h"

class h264Decoder: public decoder
{
public:
    void run () override;

    explicit h264Decoder (initParams p);

    virtual bool initialized () override;

    int join () override;

    int initDecoder () override;

    ~h264Decoder () override;

    void getDataAndSetMfxBSLengthZero (coded_video_buffer &codeced);

//    int updateBitrate (int target_kbps, int target_fps) override;

private:
    int decodeBuffer (void *in, void *out) override {}

    static void *start (void *threadarg);

    void decodeBuffer ();

    int decodeBuffer (raw_video_buffer &raw, coded_video_buffer &codeced);

private:
    mfxBitstream mfxBS;

    mfxStatus sts;
    mfxIMPL impl;
    mfxVersion ver;
    MFXVideoSession session;
    MFXVideoENCODE *mfxENC;
    MFXVideoVPP *mfxVPP;
    mfxVideoParam mfxEncParams;
    mfxVideoParam VPPParams;

    mfxFrameAllocRequest EncRequest;
    mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
    mfxFrameAllocResponse mfxResponseVPPIn;
    mfxFrameAllocResponse mfxResponseVPPOutEnc;
    mfxVideoParam par;


    mfxU16 nEncSurfNum;
    mfxU16 nSurfNumVPPIn;
    mfxU16 nSurfNumVPPOutEnc;

    mfxU16 width;
    mfxU16 height;
    mfxU8 bitsPerPixel;
    mfxU32 surfaceSize;
    mfxU8 *surfaceBuffers;
    mfxFrameSurface1 **pEncSurfaces;
    mfxFrameSurface1 **pVPPSurfacesIn;
    mfxFrameSurface1 **pVPPSurfacesOut;

    FILE *fSink;
    FILE *fRawSink;

    bool useVPP;
    bool insertIDR;
    bool updateBitrate_;
};


#endif //DECODER_H264DECODER_H
