//
// Created by sdt on 18-8-6.
//

#ifndef ENCODER_CONDUCTOR_H
#define ENCODER_CONDUCTOR_H

#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <boost/utility.hpp>

#include "decoder.h"


class conductor : public boost::noncopyable
{
public:
    typedef std::function<void ()> ErrHandleCallback;
    typedef std::function<void ()> NotifyCloseCallback;
    typedef std::function<void ()> InitDoneCallback;

    conductor ();

    void setErrHandleCallback (const ErrHandleCallback &cb)
    {
        errHandleCallback_ = cb;
    }

    void setInitDoneCallback_ (const InitDoneCallback &InitDoneCallback_)
    {
        conductor::InitDoneCallback_ = InitDoneCallback_;
    }

    const initParamsRet initDecoder (initParams &p);

    const bool destroyDecoder (destroyParams &p);

    const bool updateBitrate (initParams &p);

    const int getSize ()
    {
        return static_cast<const int>(decoderVec_.size());
    }

private:
    int findDecoderByName (std::string &name);

private:

    ErrHandleCallback errHandleCallback_;
    NotifyCloseCallback notifyCloseCallback_;
    InitDoneCallback InitDoneCallback_;

private:

    std::vector<std::unique_ptr<decoder> > decoderVec_;
};


#endif //ENCODER_CONDUCTOR_H
