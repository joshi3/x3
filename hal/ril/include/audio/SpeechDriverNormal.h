#ifndef ANDROID_SPEECH_DRIVER_NORMAL_H
#define ANDROID_SPEECH_DRIVER_NORMAL_H

#include <SpeechDriverInterface.h>

#include <pthread.h>

#include <AudioLock.h>

#include <SpeechType.h>



namespace android {

/*
 * =============================================================================
 *                     ref struct
 * =============================================================================
 */

class SpeechMessageQueue;
class SpeechMessengerNormal;


/*
 * =============================================================================
 *                     typedef
 * =============================================================================
 */


/*
 * =============================================================================
 *                     class
 * =============================================================================
 */

class SpeechDriverNormal : public SpeechDriverInterface {
public:
    /** virtual dtor */
    virtual ~SpeechDriverNormal();

    /** singleton */
    static SpeechDriverNormal *GetInstance(modem_index_t modem_index);


    /** speech */
    virtual status_t SetModemSideSamplingRate(uint16_t sample_rate);
    virtual status_t SetSpeechMode(const audio_devices_t input_device, const audio_devices_t output_device);
    virtual status_t SpeechOn();
    virtual status_t SpeechOff();
    virtual status_t VideoTelephonyOn();
    virtual status_t VideoTelephonyOff();
    virtual status_t SpeechRouterOn();
    virtual status_t SpeechRouterOff();
    virtual status_t setMDVolumeIndex(int stream, int device, int index);


    /** record */
    virtual status_t RecordOn() { return -ENOSYS; }
    virtual status_t RecordOff() { return -ENOSYS; }

    virtual status_t VoiceMemoRecordOn();
    virtual status_t VoiceMemoRecordOff();

    virtual uint16_t GetRecordSampleRate() const;
    virtual uint16_t GetRecordChannelNumber() const;

    virtual status_t RecordOn(record_type_t type_record);
    virtual status_t RecordOff(record_type_t type_record);

    virtual status_t SetPcmRecordType(record_type_t type_record);


    /** background sound */
    virtual status_t BGSoundOn();
    virtual status_t BGSoundConfig(uint8_t ul_gain, uint8_t dl_gain);
    virtual status_t BGSoundOff();


    /** pcm 2 way */
    virtual status_t PCM2WayOn(const bool wideband_on);
    virtual status_t PCM2WayOff();


    /** tty ctm */
    virtual status_t TtyCtmOn(tty_mode_t ttyMode);
    virtual status_t TtyCtmOff();
    virtual status_t TtyCtmDebugOn(bool tty_debug_flag);

    /** rtt */
    virtual int RttConfig(int rttMode);

    /** acoustic loopback */
    virtual status_t SetAcousticLoopback(bool loopback_on);
    virtual status_t SetAcousticLoopbackBtCodec(bool enable_codec);

    virtual status_t SetAcousticLoopbackDelayFrames(int32_t delay_frames);
    virtual status_t setLpbkFlag(bool enableLpbk __unused) { return -ENOSYS; }

    /**
     * Modem Audio DVT and Debug
     */
    virtual status_t SetModemLoopbackPoint(uint16_t loopback_point);


    /** volume */
    virtual status_t SetDownlinkGain(int16_t gain);
    virtual status_t SetEnh1DownlinkGain(int16_t gain);
    virtual status_t SetUplinkGain(int16_t gain);
    virtual status_t SetDownlinkMute(bool mute_on);
    virtual status_t SetUplinkMute(bool mute_on);
    virtual status_t SetUplinkSourceMute(bool mute_on);
    virtual status_t SetSidetoneGain(int16_t gain __unused) { return -ENOSYS; }
    virtual status_t SetDSPSidetoneFilter(const bool dsp_stf_on __unused) { return -ENOSYS; }
    virtual status_t SetDownlinkMuteCodec(bool mute_on);


    /** speech enhancement */
    virtual status_t SetSpeechEnhancement(bool enhance_on);
    virtual status_t SetSpeechEnhancementMask(const sph_enh_mask_struct_t &mask);
    virtual uint16_t speechEnhancementMaskWrapper(const uint32_t enh_dynamic_mask);

    virtual status_t SetBtHeadsetNrecOn(const bool bt_headset_nrec_on);


    /** speech enhancement parameters setting */
    virtual status_t SetDynamicSpeechParameters(const int type, const void *param_arg);

    virtual status_t GetVibSpkParam(void *eVibSpkParam);
    virtual status_t SetVibSpkParam(void *eVibSpkParam);

    virtual status_t GetSmartpaParam(void *eParamSmartpa);
    virtual status_t SetSmartpaParam(void *eParamSmartpa);


    /** check whether modem is ready */
    virtual bool     CheckModemIsReady();



protected:
    /** hide ctor */
    SpeechDriverNormal() {}
    SpeechDriverNormal(modem_index_t modem_index);


    /** recover status (speech/record/bgs/vt/p2w/tty) */
    virtual void RecoverModemSideStatusToInitState();

    /** delay time */
    virtual int  getBtDelayTime(uint16_t *p_bt_delay_ms);
    virtual int  getUsbDelayTime(uint8_t *p_usb_delay_ms);


    virtual int configSpeechInfo(sph_info_t *p_sph_info);

    virtual int configMailBox(
        sph_msg_t *p_sph_msg,
        uint16_t msg_id,
        uint16_t param_16bit,
        uint32_t param_32bit);


    virtual int configPayload(
        sph_msg_t *p_sph_msg,
        uint16_t msg_id,
        uint16_t data_type,
        void    *data_buf,
        uint16_t data_size);

    virtual int sendSpeechMessageToQueue(sph_msg_t *p_sph_msg);

    virtual int sendSpeechMessageAckToQueue(sph_msg_t *p_sph_msg);

    static int sendSpeechMessageToModemWrapper(void *arg, sph_msg_t *p_sph_msg);
    virtual int sendSpeechMessageToModem(sph_msg_t *p_sph_msg);

    static int errorHandleSpeechMessageWrapper(void *arg, sph_msg_t *p_sph_msg);
    virtual int errorHandleSpeechMessage(sph_msg_t *p_sph_msg);


    virtual int sendMailbox(sph_msg_t *p_sph_msg,
                            uint16_t msg_id,
                            uint16_t param_16bit,
                            uint32_t param_32bit);

    virtual int sendPayload(sph_msg_t *p_sph_msg,
                            uint16_t msg_id,
                            uint16_t data_type,
                            void    *data_buf,
                            uint16_t data_size);


    virtual int readSpeechMessageFromModem(sph_msg_t *p_sph_msg);


    AudioLock    mReadMessageLock;


    /* not speech on/off but new/delete */
    virtual void createThreads();
    virtual void joinThreads();
    bool mEnableThread;

    static void *readSpeechMessageThread(void *arg);
    pthread_t hReadSpeechMessageThread;


    /* speech on/off */
    virtual void createThreadsDuringSpeech();
    virtual void joinThreadsDuringSpeech();
    bool mEnableThreadDuringSpeech;

    static void *modemStatusMonitorThread(void *arg);
    pthread_t hModemStatusMonitorThread;
    AudioLock    mModemStatusMonitorThreadLock;


    virtual int parseRawRecordPcmBuffer(void *raw_buf, void *parsed_buf, uint16_t *p_data_size);

    virtual int processModemMessage(sph_msg_t *p_sph_msg);
    virtual int processModemAckMessage(sph_msg_t *p_sph_msg);
    virtual int processModemControlMessage(sph_msg_t *p_sph_msg);
    virtual int processModemDataMessage(sph_msg_t *p_sph_msg);

    virtual void processModemEPOF();
    virtual void processModemAlive();
    virtual void processNetworkCodecInfo(sph_msg_t *p_sph_msg);


    SpeechMessengerNormal *mSpeechMessenger;

    uint8_t  mSampleRateEnum;


    SpeechMessageQueue *mSpeechMessageQueue;
    AudioLock           mWaitAckLock;


    bool getModemSideModemStatus(const modem_status_mask_t modem_status_mask) const;
    void setModemSideModemStatus(const modem_status_mask_t modem_status_mask);
    void resetModemSideModemStatus(const modem_status_mask_t modem_status_mask);
    void cleanAllModemSideModemStatus();


    virtual int parseSpeechParam(const int type, int *param_arg);
    virtual int writeAllSpeechParametersToModem(uint16_t *p_length, uint16_t *p_index);


    uint32_t mModemSideModemStatus; // value |= modem_status_mask_t
    AudioLock    mModemSideModemStatusLock;


    virtual int SpeechOnByApplication(const uint8_t application);
    virtual int SpeechOffByApplication(const uint8_t application);
    bool isSpeechApplicationOn() { return (mApplication != SPH_APPLICATION_INVALID); }


    uint32_t kMaxApPayloadDataSize;
    uint32_t kMaxMdPayloadDataSize;

    void *mBgsBuf;
    void *mVmRecBuf;
    void *mRawRecBuf;
    void *mParsedRecBuf;
    void *mP2WUlBuf;
    void *mP2WDlBuf;
    void *mTtyDebugBuf;

    uint8_t mApplication;
    speech_mode_t mSpeechMode;
    audio_devices_t mInputDevice;
    audio_devices_t mOutputDevice;


    AudioLock mRecordTypeLock;


    AudioLock    mSpeechParamLock;
    sph_param_buf_t mSpeechParam[NUM_AUDIO_TYPE_SPEECH_TYPE];
    void *mSpeechParamConcat;


    bool mTtyDebugEnable;

    bool mApResetDuringSpeech;
    bool mModemResetDuringSpeech;


    /* loopback delay frames (1 frame = 20 ms) */
    uint8_t mModemLoopbackDelayFrames;

    /* RTT Mode */
    int mRttMode;


private:
    /** singleton */
    static SpeechDriverNormal *mSpeechDriver;
};

} /* end of namespace android */

#endif /* end of ANDROID_SPEECH_DRIVER_NORMAL_H */

