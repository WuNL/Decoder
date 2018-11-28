//
// Created by sdt on 18-10-30.
//

#include <include/h265Decoder.h>
#include <include/semtool.h>

#include "h265Decoder.h"
#include "config.h"

void h265Decoder::run ()
{
    started_ = true;
    pthread_create(&pthreadId_, nullptr, &h265Decoder::start, static_cast<void *>(this));
}

h265Decoder::h265Decoder (initParams p) : decoder(std::move(p)), mfxDEC(nullptr), width(static_cast<mfxU16>(p.v_width)),
                                          height(
                                                  static_cast<mfxU16>(p.v_height)),
                                          initFinish(false)
{

}

int h265Decoder::join ()
{
    return 0;
}

int h265Decoder::initDecoder ()
{
    sts = MFX_ERR_NONE;
    CmdOptions options{};

    // =====================================================================
    // Intel Media SDK decode pipeline setup
    // - In this example we are decoding an HEVC (H.265) stream
    // - For simplistic memory management, system memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    // - Before run the program, check if the input stream is HEVC 10bit, run "ffprobe" or other tool
    //   to confirm if the stream has the "HEVC Main 10" profile

    // 1. Read options from the command line (if any is given)
    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_DECODE;
    // Set default values:
    options.values.impl = MFX_IMPL_HARDWARE;

    // 2. Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    // OS specific notes
    // - On Windows both SW and HW libraries may present
    // - On Linux only HW library only is available
    //   If more recent API features are needed, change the version accordingly
    impl = options.values.impl;
    ver = {{0, 1}};

    sts = Initialize(impl, ver, &session, nullptr);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder
    mfxDEC = new MFXVideoDECODE(session);

    // 3. Set required video parameters for decode
    memset(&mfxVideoParams, 0, sizeof(mfxVideoParams));
    mfxVideoParams.mfx.CodecId = MFX_CODEC_HEVC; //The codecId must be set to HEVC in order to load the plugin
    mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    mfxVideoParams.AsyncDepth = 1;

    mfxVideoParams.mfx.FrameInfo.Width = MSDK_ALIGN32(width);
    mfxVideoParams.mfx.FrameInfo.Height = MSDK_ALIGN32(height);
    mfxVideoParams.mfx.FrameInfo.CropW = width;
    mfxVideoParams.mfx.FrameInfo.CropH = height;

    // 4. Load the HEVC plugin
    mfxPluginUID codecUID;
    bool success = true;
    codecUID = msdkGetPluginUID(impl, MSDK_VDECODE, mfxVideoParams.mfx.CodecId);

    if (AreGuidsEqual(codecUID, MSDK_PLUGINGUID_NULL))
    {
        printf("Get Plugin UID for HEVC is failed.\n");
        success = false;
    }

    printf("Loading HEVC plugin: %s\n", ConvertGuidToString(codecUID));

    //On the success of get the UID, load the plugin
    if (success)
    {
        sts = MFXVideoUSER_Load(session, &codecUID, ver.Major);
        if (sts < MFX_ERR_NONE)
        {
            printf("Loading HEVC plugin failed\n");
            success = false;
        }
    }

    // 5. Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = 10240 * 10240;
    mfxBS.Data = new mfxU8[mfxBS.MaxLength];
    MSDK_CHECK_POINTER(mfxBS.Data, MFX_ERR_MEMORY_ALLOC);

    char filename[200];
    sprintf(filename, "/home/sdt/Videos/Decoder_%d_%d", params.v_width, params.v_height);
//    MSDK_FOPEN(fRawSink, (filename + std::string(".yuv")).c_str(), "wb");
//    MSDK_FOPEN(fRawSink1, (filename + std::string("1.yuv")).c_str(), "wb");
#ifdef SAVE_CODECED
    MSDK_FOPEN(fSink, (filename + std::string(".265")).c_str(), "wb");
#endif
#ifdef SAVE_RAW
    MSDK_FOPEN(fRawSink, (filename + std::string(".yuv")).c_str(), "wb");
#endif

    return 0;
}

h265Decoder::~h265Decoder ()
{
    printf("~h265Decoder\n");

    if (mfxDEC)
        mfxDEC->Close();
    delete mfxDEC;
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    for (int i = 0; i < numSurfaces; i ++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    MSDK_SAFE_DELETE_ARRAY(surfaceBuffers);

    Release();

    if (fSink)
        fclose(fSink);
    if (fRawSink)
        fclose(fRawSink);
}

void h265Decoder::getDataAndSetMfxBSLengthZero (coded_video_buffer &codeced)
{

}


void *h265Decoder::start (void *threadarg)
{
    auto this_ptr = static_cast<h265Decoder *>(threadarg);

    this_ptr->decodeBuffer();
    return nullptr;
}

void h265Decoder::decodeBuffer ()
{
    initDecoder();
    while (started_)
    {
        memset(&raw_video_buffer_, 0, sizeof(raw_video_buffer));
        if (! raw_video_fifo_)
            continue;
        memset(&coded_video_buffer_, 0, sizeof(coded_video_buffer));
        if (! coded_video_fifo_)
            continue;

        shmfifo_get(coded_video_fifo_, &coded_video_buffer_);


        decodeBuffer(raw_video_buffer_, coded_video_buffer_);
//        getDataAndSetMfxBSLengthZero(coded_video_buffer_);

        shmfifo_put(raw_video_fifo_, &raw_video_buffer_);
    }
    pthread_cond_signal(&finish_threshold_cv);
}

int h265Decoder::decodeBuffer (raw_video_buffer &raw, coded_video_buffer &codeced)
{
//    printf("decode \n");
#ifdef SAVE_CODECED
    fwrite(codeced.buffer, 1, static_cast<size_t>(codeced.size), fSink);
#endif

    if (! initFinish)
    {
        // Read a chunk of data from stream file into bit stream buffer
        // - Parse bit stream, searching for header and fill video parameters structure
        // - Abort if bit stream header is not found in the first bit stream buffer chunk
        sts = ReadBitStreamDataFromBuffer(&mfxBS, codeced.buffer, static_cast<size_t>(codeced.size));
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        sts = mfxDEC->DecodeHeader(&mfxBS, &mfxVideoParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // Validate video decode parameters (optional)
        sts = mfxDEC->Query(&mfxVideoParams, &mfxVideoParams);

        // Query number of required surfaces for decoder
        mfxFrameAllocRequest Request;
        memset(&Request, 0, sizeof(Request));
        sts = mfxDEC->QueryIOSurf(&mfxVideoParams, &Request);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        numSurfaces = Request.NumFrameSuggested;

        // 6. Allocate surfaces for decoder
        // - Width and height of buffer must be aligned, a multiple of 32
        // - Frame surface array keeps pointers all surface planes and general frame info
        // - For HEVC 10 bit, the bytes per pixel is doubled, so the width is multiplied by 2.
        width = (mfxU16) MSDK_ALIGN32(Request.Info.Width);
//        width = (mfxU16) MSDK_ALIGN32(1920);
//        height = (mfxU16) MSDK_ALIGN32(1080);
        height = (mfxU16) MSDK_ALIGN32(Request.Info.Height);
        printf("%d %d\n", width, height);
        mfxU8 bitsPerPixel = 12; // Note: YUV P010 format is a 24 bits per pixel format, but we have doubled the width
        auto surfaceSize = static_cast<mfxU32>(width * height * bitsPerPixel / 8);
        surfaceBuffers = (mfxU8 *) new mfxU8[surfaceSize * numSurfaces];

        // Allocate surface headers (mfxFrameSurface1) for decoder, noticed the width has been doubled for P010.
        pmfxSurfaces = new mfxFrameSurface1 *[numSurfaces];
        MSDK_CHECK_POINTER(pmfxSurfaces, MFX_ERR_MEMORY_ALLOC);
        for (int i = 0; i < numSurfaces; i ++)
        {
            pmfxSurfaces[i] = new mfxFrameSurface1;
            memset(pmfxSurfaces[i], 0, sizeof(mfxFrameSurface1));
            memcpy(&(pmfxSurfaces[i]->Info), &(mfxVideoParams.mfx.FrameInfo), sizeof(mfxFrameInfo));
            pmfxSurfaces[i]->Data.Y = &surfaceBuffers[surfaceSize * i];
            pmfxSurfaces[i]->Data.U = pmfxSurfaces[i]->Data.Y + width * height;
            pmfxSurfaces[i]->Data.V = pmfxSurfaces[i]->Data.U + 1;
            pmfxSurfaces[i]->Data.Pitch = width;
        }

        // 7. Initialize the Media SDK decoder
        sts = mfxDEC->Init(&mfxVideoParams);
        MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
        MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

        // ===============================================================
        // 8. Start decoding the frames
        //

        mfxTime tStart, tEnd;
        mfxGetTime(&tStart);

        mfxSyncPoint syncp;
        mfxFrameSurface1 *pmfxOutSurface = nullptr;
        int nIndex = 0;
        mfxU32 nFrame = 0;

        initFinish = true;
    }


    // ===============================================================
    // 8. Start decoding the frames
    //

    mfxTime tStart, tEnd;
    mfxGetTime(&tStart);

    mfxSyncPoint syncp;
    mfxFrameSurface1 *pmfxOutSurface = nullptr;
    int nIndex = 0;
    static mfxU32 nFrame = 0;

    sts = MFX_ERR_NONE;
    //
    // Stage 1: Main decoding loop
    //
    sts = ReadBitStreamDataFromBuffer(&mfxBS, codeced.buffer, static_cast<size_t>(codeced.size));
    MSDK_RETURN_ON_ERROR(sts);


    if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
    {
        nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
    }
    // Decode a frame asychronously (returns immediately)
    //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
    sts = mfxDEC->DecodeFrameAsync(&mfxBS, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);
    if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts)
    {
        mfxVideoParam tmpparam;
        memset(&tmpparam, 0, sizeof(tmpparam));
        mfxDEC->GetVideoParam(&tmpparam);
        printf("sts:%d , tmpparam:%d %d\n", sts, tmpparam.mfx.FrameInfo.CropW, tmpparam.mfx.FrameInfo.CropH);

        reallocate();
        initFinish = false;
        return 0;
    }

    // Ignore warnings if output is available,
    // if no output and no action required just repeat the DecodeFrameAsync call
    if (MFX_ERR_NONE < sts && syncp)
        sts = MFX_ERR_NONE;

    if (MFX_ERR_NONE == sts)
        sts = session.SyncOperation(syncp, 60000);      // Synchronize. Wait until decoded frame is ready

    if (MFX_ERR_NONE == sts)
    {
        ++ nFrame;

        mfxFrameInfo *pInfo = &pmfxOutSurface->Info;
        mfxFrameData *pData = &pmfxOutSurface->Data;

        uint32_t offset = 0;
        uint32_t offset1 = 0;
//        Y
        for (int i = 0; i < pInfo->CropH; i ++)
        {
            memcpy((void *) (raw.buffer + offset), pData->Y + offset1, pInfo->CropW);
            offset += pInfo->CropW;
            offset1 += pData->Pitch;
        }
        offset1 = 0;
//            offset = pInfo->CropH * pInfo->CropW;
        for (int i = 0; i < pInfo->CropH / 2; i ++)
        {
            memcpy((void *) (raw.buffer + offset), pData->UV + offset1, pInfo->CropW);
            offset += pInfo->CropW;
            offset1 += pData->Pitch;
        }

        raw.width = pInfo->CropW;
        raw.height = pInfo->CropH;
        raw.size = pmfxOutSurface->Info.CropW * pmfxOutSurface->Info.CropH * 3 / 2;

        return 0;
#ifdef SAVE_RAW
        sts = WriteRawFrame(pmfxOutSurface, fRawSink);

            mfxFrameInfo *pInfo = &pmfxOutSurface->Info;
            mfxFrameData *pData = &pmfxOutSurface->Data;

            uint32_t offset = 0;
            uint32_t offset1 = 0;
//        Y
            for (int i = 0; i < pInfo->CropH; i ++)
            {
                memcpy((void*)(raw.buffer+offset),pData->Y+offset1,pInfo->CropW);
                offset += pInfo->CropW;
                offset1 += pInfo->CropW;
            }
            offset1 = 0;
            for (int i = 0; i < pInfo->CropH/2; i ++)
            {
                memcpy((void*)(raw.buffer+offset),pData->UV+offset1,pInfo->CropW);
                offset += pInfo->CropW;
                offset1 += pInfo->CropW;
            }

//            sts = WriteRawFrameToBuffer(pmfxOutSurface, raw.buffer);
            raw.width = pmfxOutSurface->Info.CropW;
            raw.height = pmfxOutSurface->Info.CropH;
            raw.size = pmfxOutSurface->Info.CropW * pmfxOutSurface->Info.CropH * 3 / 2;
#endif
    }

    // MFX_ERR_MORE_DATA means that file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
//    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered decoded frames
    //
    if (MFX_WRN_DEVICE_BUSY == sts) MSDK_SLEEP(
            1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

    nIndex = GetFreeSurfaceIndex(pmfxSurfaces, numSurfaces);        // Find free frame surface
    MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

    // Decode a frame asychronously (returns immediately)
    sts = mfxDEC->DecodeFrameAsync(nullptr, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

    // Ignore warnings if output is available,
    // if no output and no action required just repeat the DecodeFrameAsync call
    if (MFX_ERR_NONE < sts && syncp)
        sts = MFX_ERR_NONE;

    if (MFX_ERR_NONE == sts)
        sts = session.SyncOperation(syncp, 60000);      // Synchronize. Waits until decoded frame is ready

    if (MFX_ERR_NONE == sts)
    {
        ++ nFrame;

        mfxFrameInfo *pInfo = &pmfxOutSurface->Info;
        mfxFrameData *pData = &pmfxOutSurface->Data;

        uint32_t offset = 0;
        uint32_t offset1 = 0;
//        Y
        for (int i = 0; i < pInfo->CropH; i ++)
        {
            memcpy((void *) (raw.buffer + offset), pData->Y + offset1, pInfo->CropW);
            offset += pInfo->CropW;
            offset1 += pData->Pitch;
        }
        offset1 = 0;
//            offset = pInfo->CropH * pInfo->CropW;
        for (int i = 0; i < pInfo->CropH / 2; i ++)
        {
            memcpy((void *) (raw.buffer + offset), pData->UV + offset1, pInfo->CropW);
            offset += pInfo->CropW;
            offset1 += pData->Pitch;
        }

        raw.width = pInfo->CropW;
        raw.height = pInfo->CropH;
        raw.size = pmfxOutSurface->Info.CropW * pmfxOutSurface->Info.CropH * 3 / 2;

#ifdef SAVE_RAW
        sts = WriteRawFrame(pmfxOutSurface, fRawSink);
//            fwrite(raw.buffer, 1, static_cast<size_t>(raw.size), fRawSink);
//            printf("rest Frame number: %d %d %d\n", nFrame, raw.width, raw.height);
//            fflush(stdout);
#endif
    }

    // MFX_ERR_MORE_DATA indicates that all buffers has been fetched, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    return 0;
}

bool h265Decoder::initialized ()
{
    return mfxDEC != nullptr;
}

void h265Decoder::reallocate ()
{
    printf("~h265Decoder\n");

    if (mfxDEC)
        mfxDEC->Close();
    delete mfxDEC;
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    for (int i = 0; i < numSurfaces; i ++)
        delete pmfxSurfaces[i];
    MSDK_SAFE_DELETE_ARRAY(pmfxSurfaces);
    MSDK_SAFE_DELETE_ARRAY(mfxBS.Data);

    MSDK_SAFE_DELETE_ARRAY(surfaceBuffers);

    Release();

    initDecoder();
}
