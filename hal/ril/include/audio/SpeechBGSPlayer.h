#ifndef ANDROID_SPEECH_BACKGROUND_SOUND_PLAYER_H
#define ANDROID_SPEECH_BACKGROUND_SOUND_PLAYER_H

#include <pthread.h>
#include "AudioType.h"
#include "AudioUtility.h"
#include "MtkAudioComponent.h"
#include <AudioLock.h>

namespace android {
// for debug
//#define DUMP_BGS_DATA
//#define DUMP_BGS_BLI_BUF
//#define BGS_USE_SINE_WAVE


// BGS ring buffer: 80 ms (modem request data: 20~40ms)(ping-pong)
#if defined(SPH_SR48K)
#define BGS_TARGET_SAMPLE_RATE  (48000)
#define BGS_PLAY_BUFFER_LEN     (7680) // 2byte * 1ch * 48000Hz * 80/1000 = 7,680 bytes
#elif defined(SPH_SR32K)
#define BGS_TARGET_SAMPLE_RATE  (32000)
#define BGS_PLAY_BUFFER_LEN     (5120) // 2byte * 1ch * 32000Hz * 80/1000 = 5,120 bytes
#else
#define BGS_TARGET_SAMPLE_RATE  (16000)
#define BGS_PLAY_BUFFER_LEN     (2560) // 2byte * 1ch * 16000Hz * 80/1000 = 2,560 bytes
#endif

/*=============================================================================
 *                              Class definition
 *===========================================================================*/

class BGSPlayer;
class SpeechDriverInterface;

class BGSPlayBuffer {
private:
    BGSPlayBuffer();
    virtual ~BGSPlayBuffer(); // only destroied by friend class BGSPlayer
    friend          class BGSPlayer;
    status_t        InitBGSPlayBuffer(BGSPlayer *playPointer, uint32_t sampleRate, uint32_t chNum, int32_t mFormat);
    uint32_t        Write(char *buf, uint32_t num);
    bool        IsBGSBlisrcDumpEnable();

    int32_t         mFormat;
    RingBuf         mRingBuf;
    MtkAudioSrcBase     *mBliSrc;
    char           *mBliOutputLinearBuffer;

    AudioLock       mBGSPlayBufferRuningMutex;
    AudioLock       mBGSPlayBufferMutex;

    bool            mExitRequest;

    bool        mIsBGSBlisrcDumpEnable;
    FILE        *pDumpFile;
};

class BGSPlayer {
public:
    virtual ~BGSPlayer();

    static BGSPlayer        *GetInstance();
    BGSPlayBuffer        *CreateBGSPlayBuffer(uint32_t sampleRate, uint32_t chNum, int32_t format);
    void        DestroyBGSPlayBuffer(BGSPlayBuffer *pBGSPlayBuffer);
    bool        Open(SpeechDriverInterface *pSpeechDriver, uint8_t uplink_gain, uint8_t downlink_gain);
    bool        Close();
    bool        IsBGSDumpEnable();
    uint32_t        Write(BGSPlayBuffer *pBGSPlayBuffer, void *buf, uint32_t num);
    uint32_t        PutData(BGSPlayBuffer *pBGSPlayBuffer, char *target_ptr, uint16_t num_data_request);
    uint16_t        PutDataToSpeaker(char *target_ptr, uint16_t num_data_request);

    AudioLock   mBGSMutex; // use for create/destroy bgs buffer & ccci bgs data request

private:
    BGSPlayer();

    static BGSPlayer        *mBGSPlayer; // singleton
    SpeechDriverInterface        *mSpeechDriver;
    SortedVector<BGSPlayBuffer *> mBGSPlayBufferVector;
    char        *mBufBaseTemp;

    AudioLock    mBGSPlayBufferVectorLock;
    uint16_t     mCount;

    bool        mIsBGSDumpEnable;
    FILE        *pDumpFile;
};


} // end namespace android

#endif //ANDROID_SPEECH_BACKGROUND_SOUND_PLAYER_H
