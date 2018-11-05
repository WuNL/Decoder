//
// Created by sdt on 18-10-29.
//

#ifndef DECODER_DECODER_H
#define DECODER_DECODER_H
#include <iostream>
#include <string>
#include <functional>
#include <boost/utility.hpp>
#include <vector>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "config.h"

#ifdef USE_INTEL

#include "../common/common_utils.h"
#include "../common/cmd_options.h"

#endif
#include "shmfifo.h"

#define BUFFERLEN 1920*1080*3/2
#define BUFFERSIZE 30

typedef struct videoBuffer
{
    int size;
    char buffer[BUFFERLEN];
} coded_video_buffer;

typedef struct yuvBuffer
{
    int size;
    int width;
    int height;
    char buffer[BUFFERLEN];
} raw_video_buffer;

enum
{
    h264,
    h264_hw,
    h265,
    h265_hw
};

typedef struct InitParams
{
    std::string decoder_name;
    std::string codec;
    int v_width;
    int v_height;
    int bitrate;
    int framerate;
    int v_gop;
    int packetMode;
} initParams;

typedef struct InitParamsRet
{
    std::string decoder_name;
    bool success;
    int yuv_shmID;
    int out_shmID;
} initParamsRet;

typedef struct DestroyParams
{
    std::string decoder_name;
    bool success;
} destroyParams;

class decoder
{
public:
    typedef std::function<void ()> ErrHandleCallback;
    typedef std::function<void ()> NotifyCloseCallback;

    explicit decoder (initParams p);

    virtual ~decoder ();

    virtual bool initialized() = 0;

    void setErrHandleCallback_ (const ErrHandleCallback &errHandleCallback_)
    {
        decoder::errHandleCallback_ = errHandleCallback_;
    }

    void setNotifyCloseCallback_ (const NotifyCloseCallback &notifyCloseCallback_)
    {
        decoder::notifyCloseCallback_ = notifyCloseCallback_;
    }

    const std::string &getName ()
    {
        return decoder_name;
    }

    bool started () const { return started_; }

    bool ifInitFinished () const { return initFinished; }

    void waitForInitFinish ()
    {
        while (! initFinished)
        {
            pthread_mutex_lock(&count_mutex);

            pthread_cond_wait(&count_threshold_cv, &count_mutex);

            pthread_mutex_unlock(&count_mutex);
        }
    }

    void stop ()
    {
        started_ = false;
        std::cout << "start stop" << std::endl;
        while (true)
        {
            pthread_mutex_lock(&count_mutex);

            pthread_cond_wait(&finish_threshold_cv, &count_mutex);

            pthread_mutex_unlock(&count_mutex);

            break;
        }

        std::cout << "stop" << std::endl;
        pthread_exit(nullptr);

    }

    pid_t tid () const { return tid_; }

    virtual void run () = 0;

    virtual int join () = 0; // return pthread_join()

    virtual int initDecoder () {};

//    virtual int updateBitrate (int target_kbps, int target_fps) {};

    virtual int getRawId ()
    {
        return raw_shm_id_local;
    }

    virtual int getCodecedId ()
    {
        return codeced_shm_id_local;
    }

private:
    /// \brief
    /// \param in
    /// \param out
    /// \return
    virtual int decodeBuffer (void *in, void *out) = 0;

private:
    ErrHandleCallback errHandleCallback_;
    NotifyCloseCallback notifyCloseCallback_;

private:


protected:
    initParams params;
    std::string decoder_name;

    static int raw_shm_id;
    static int codeced_shm_id;

    int raw_shm_id_local;
    int codeced_shm_id_local;

    bool started_;
    bool joined_;
    bool initFinished;
    bool readyToEndThread;
    pthread_t pthreadId_;
    pid_t tid_;

    pthread_mutex_t count_mutex;
    pthread_cond_t count_threshold_cv;
    pthread_cond_t finish_threshold_cv;

    shmfifo_t *raw_video_fifo_;
    shmfifo_t *coded_video_fifo_;

    raw_video_buffer raw_video_buffer_;
    coded_video_buffer coded_video_buffer_;
};


#endif //DECODER_DECODER_H
