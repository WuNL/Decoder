//
// Created by sdt on 18-8-6.
//

#include <include/conductor.h>
#include <cstring>
#include "videoDecoderFactory.h"

const initParamsRet conductor::initDecoder (initParams &p)
{
    initParamsRet ret;
    memset(&ret, 0x00, sizeof(initParamsRet));
    int index = findDecoderByName(p.decoder_name);
    if (index != - 1)
    {
        decoderVec_[index]->waitForInitFinish();
//        std::cout << "initDecoder waitForInitFinish done" << std::endl;
//        decoderVec_[index]->stop();
        decoderVec_.erase(decoderVec_.begin() + index);

        std::unique_ptr<decoder> e = video_decoder_factory::create(p);
        decoderVec_.push_back(std::move(e));
        ret.decoder_name = p.decoder_name;
        ret.yuv_shmID = decoderVec_.back()->getRawId();
        ret.out_shmID = decoderVec_.back()->getCodecedId();
        ret.success = true;
        decoderVec_.back()->run();
    } else
    {
        std::unique_ptr<decoder> e = video_decoder_factory::create(p);
        decoderVec_.push_back(std::move(e));
        ret.decoder_name = p.decoder_name;
        ret.yuv_shmID = decoderVec_.back()->getRawId();
        ret.out_shmID = decoderVec_.back()->getCodecedId();
        ret.success = true;
        decoderVec_.back()->run();
    }

    if (InitDoneCallback_)
    {
        InitDoneCallback_();
    }

    return ret;
}

int conductor::findDecoderByName (std::string &name)
{
    for (int i = 0; i < decoderVec_.size(); ++ i)
    {
//        std::cout << "decoderVec name:" << decoderVec_[i]->getName() << std::endl;
        if (decoderVec_[i]->getName() == name)
            return i;
    }
    return - 1;
}

conductor::conductor () : errHandleCallback_(nullptr),
                          notifyCloseCallback_(nullptr),
                          InitDoneCallback_(nullptr) {}

const bool conductor::destroyDecoder (destroyParams &p)
{
    int index = findDecoderByName(p.decoder_name);
    if (- 1 != index)
    {
        decoderVec_[index]->waitForInitFinish();
//        std::cout << "destroyDecoder waitForInitFinish done" << std::endl;
        decoderVec_.erase(decoderVec_.begin() + index);
    } else
    {
        std::cout << "destroyDecoder not find decoder" << std::endl;
    }
    std::cout << "destroyDecoder finish" << std::endl;
    return true;
}

const bool conductor::updateBitrate (initParams &p)
{
    int index = findDecoderByName(p.decoder_name);
    if (- 1 != index)
    {
        decoderVec_[index]->waitForInitFinish();
        std::cout << "updateBitrate waitForInitFinish done" << std::endl;
//        decoderVec_[index]->updateBitrate(p.bitrate, p.framerate);
    } else
    {
        std::cout << "updateBitrate not find decoder" << std::endl;
    }
    std::cout << "updateBitrate finish" << std::endl;
    return true;
}
