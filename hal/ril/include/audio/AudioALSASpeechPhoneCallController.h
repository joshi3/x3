#ifndef ANDROID_AUDIO_ALSA_SPEECH_PHONE_CALL_CONTROLLER_H
#define ANDROID_AUDIO_ALSA_SPEECH_PHONE_CALL_CONTROLLER_H

#include <tinyalsa/asoundlib.h> // TODO(Harvey): move it
#include <media/AudioParameter.h>

#include "AudioType.h"
#include "SpeechType.h"

#include <AudioLock.h>
#include "AudioVolumeInterface.h"
#include "SpeechDriverInterface.h"
#include "AudioTypeExt.h"

#if defined(SPEECH_PMIC_RESET)
#include <pthread.h>
#include <utils/threads.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#endif

namespace android {

class AudioALSAHardwareResourceManager;
class AudioALSAStreamManager;
class SpeechDriverFactory;
class AudioBTCVSDControl;
class AudioALSAVolumeController;

class AudioALSASpeechPhoneCallController {
public:
    virtual ~AudioALSASpeechPhoneCallController();

    static AudioALSASpeechPhoneCallController *getInstance();

    virtual audio_devices_t getInputDeviceForPhoneCall(const audio_devices_t output_devices);

    virtual status_t        open(const audio_mode_t audio_mode, const audio_devices_t output_devices, const audio_devices_t input_device);
    virtual status_t        close();
    virtual status_t        routing(const audio_devices_t new_output_devices, const audio_devices_t new_input_device);
    virtual audio_devices_t getPhonecallInputDevice();
    virtual audio_devices_t getPhonecallOutputDevice();

    virtual bool            checkTtyNeedOn() const;
    virtual bool            checkSideToneFilterNeedOn(const audio_devices_t output_device) const;

    inline tty_mode_t       getTtyMode() const { return mTtyMode; }
    virtual status_t        setTtyMode(const tty_mode_t tty_mode);
    inline audio_devices_t  getRoutingForTty() const { return mRoutingForTty; }
    inline void             setRoutingForTty(const audio_devices_t new_device) { mRoutingForTty = new_device; }
    virtual void            setTtyInOutDevice(audio_devices_t routing_device);
    virtual int             setRttCallType(const int rttCallType);

    inline unsigned int     getSampleRate() const { return mConfig.rate; }

    virtual void            setVtNeedOn(const bool vt_on);
    virtual void            setMicMute(const bool mute_on);

    virtual void            setBTMode(const int mode);
    virtual void            setDlMute(const bool mute_on);
    virtual void            setUlMute(const bool mute_on);
    virtual void            getRFInfo();
    virtual status_t        setParam(const String8 &keyParamPairs);
    inline bool             isAudioTaste() { return bAudioTaste; };
    inline uint32_t         getSpeechDVT_SampleRate() { return mSpeechDVT_SampleRate; };
    inline uint32_t         getSpeechDVT_MD_IDX() { return mSpeechDVT_MD_IDX; };

    virtual void            muteDlCodecForShutterSound(const bool mute_on);
    virtual void            updateVolume();
    virtual bool            checkReopen(const modem_index_t rilMappedMDIdx);
    virtual int             setPhoneId(const phone_id_t phoneId);

    inline bool isModeInPhoneCall() {
        return (mAudioMode == AUDIO_MODE_IN_CALL);
    }
    inline phone_id_t getPhoneId() { return mPhoneId; };
    inline modem_index_t getIdxMDByPhoneId(uint8_t PhoneId) { return mIdxMDByPhoneId[PhoneId]; };

protected:
    AudioALSASpeechPhoneCallController();

    /**
     * init audio hardware
     */
    virtual status_t init();

    inline uint32_t calculateSampleRate(const bool bt_device_on) {
#if defined(SPH_SR32K)
        return (bt_device_on == false) ? 32000 : (mBTMode == 0) ? 8000 : 16000;
#elif defined(SPH_SR48K)
        return (bt_device_on == false) ? 48000 : (mBTMode == 0) ? 8000 : 16000;
#else
        return (bt_device_on == false) ? 16000 : (mBTMode == 0) ? 8000 : 16000;
#endif
    }
    AudioALSAHardwareResourceManager *mHardwareResourceManager;
    AudioALSAStreamManager *mStreamManager;
    AudioVolumeInterface *mAudioALSAVolumeController;

    SpeechDriverFactory *mSpeechDriverFactory;
    AudioBTCVSDControl *mAudioBTCVSDControl;

    AudioLock mLock;
#if defined(SPEECH_PMIC_RESET)
    AudioLock mThreadLock;
#endif
    audio_mode_t mAudioMode;
    bool mMicMute;
    bool mDlMute;
    bool mUlMute;
    bool mVtNeedOn;
    bool bAudioTaste;
    tty_mode_t mTtyMode;
    audio_devices_t mRoutingForTty;
    audio_devices_t mPhonecallInputDevice;
    audio_devices_t mPhonecallOutputDevice;
    int mBTMode; // BT mode, 0:NB, 1:WB
    modem_index_t mIdxMD; // Modem Index, 0:MD1, 1:MD2, 2: MD3
    struct pcm_config mConfig; // TODO(Harvey): move it to AudioALSAHardwareResourceManager later
    struct pcm *mPcmIn; // TODO(Harvey): move it to AudioALSAHardwareResourceManager later
    struct pcm *mPcmOut; // TODO(Harvey): move it to AudioALSAHardwareResourceManager later
    uint16_t mRfInfo, mRfMode, mASRCNeedOn;
    uint32_t mSpeechDVT_SampleRate;
    uint32_t mSpeechDVT_MD_IDX;

private:
    static AudioALSASpeechPhoneCallController *mSpeechPhoneCallController; // singleton
#if defined(SPEECH_PMIC_RESET)
    bool mEnable_PMIC_Reset;
    bool StartPMIC_Reset();
    static void *thread_PMIC_Reset(void *arg);
    pthread_t hThread_PMIC_Reset;
#endif

    modem_index_t updatePhysicalModemIdx(const audio_mode_t audio_mode);
    void muteDlUlForRouting(const int muteCtrl);

    bool            mIsSidetoneEnable;
    phone_id_t      mPhoneId;
    modem_index_t   mIdxMDByPhoneId[NUM_PHONE_ID];
    /*
     * flag of dynamic enable verbose/debug log
     */
    int             mLogEnable;

    static void    *muteDlCodecForShutterSoundThread(void *arg);
    pthread_t       hMuteDlCodecForShutterSoundThread;
    bool            mMuteDlCodecForShutterSoundThreadEnable;

    AudioLock       mMuteDlCodecForShutterSoundLock;
    uint32_t        mMuteDlCodecForShutterSoundCount;
    bool            mDownlinkMuteCodec;

    static void    *muteDlUlForRoutingThread(void *arg);
    pthread_t       mMuteDlUlForRoutingThread;
    bool            mMuteDlUlForRoutingThreadEnable;
    AudioLock       mMuteDlUlForRoutingLock;
    int             mMuteDlUlForRoutingState;
    int             mMuteDlUlForRoutingCtrl;

    int mRttCallType;
    int mRttMode;

};

} // end namespace android

#endif // end of ANDROID_AUDIO_ALSA_SPEECH_PHONE_CALL_CONTROLLER_H
