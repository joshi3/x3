#ifndef ANDROID_SPEECH_MESSENGER_NORMAL_H
#define ANDROID_SPEECH_MESSENGER_NORMAL_H

#include <stdint.h>

#include <SpeechType.h>

#include <AudioLock.h>


namespace android {

struct ccci_msg_t;



class SpeechMessengerNormal {
public:
    SpeechMessengerNormal(const modem_index_t modem_index);
    virtual ~SpeechMessengerNormal();

    virtual bool checkModemReady();
    virtual bool checkModemAlive();


    virtual uint32_t getMaxApPayloadDataSize();
    virtual uint32_t getMaxMdPayloadDataSize();

    virtual int sendSpeechMessage(sph_msg_t *p_sph_msg);
    virtual int readSpeechMessage(sph_msg_t *p_sph_msg);

    virtual int resetShareMemoryIndex();


    virtual int writeSphParamToShareMemory(const void *p_sph_param,
                                           uint16_t sph_param_length,
                                           uint16_t *p_write_idx);


    virtual int writeApDataToShareMemory(const void *data_buf,
                                         uint16_t data_type,
                                         uint16_t data_size,
                                         uint16_t *p_payload_length,
                                         uint32_t *p_write_idx);


    virtual int readMdDataFromShareMemory(void *p_data_buf,
                                          uint16_t *p_data_type,
                                          uint16_t *p_data_size,
                                          uint16_t payload_length,
                                          uint32_t read_idx);




protected:
    SpeechMessengerNormal() {}


    virtual int openCcciDriver();
    virtual int closeCcciDriver();

    virtual int openShareMemory();
    virtual int closeShareMemory();

    virtual int checkCcciStatusAndRecovery();

    virtual int formatShareMemory();
    static void *formatShareMemoryThread(void *arg);
    pthread_t   hFormatShareMemoryThread;

    virtual int speechMessageToCcciMessage(sph_msg_t *p_sph_msg, ccci_msg_t *p_ccci_msg);
    virtual int ccciMessageToSpeechMessage(ccci_msg_t *p_ccci_msg, sph_msg_t *p_sph_msg);


    modem_index_t mModemIndex;


    int mCcciDeviceHandler;
    int mCcciShareMemoryHandler;
    AudioLock    mCcciHandlerLock;
    AudioLock    mShareMemoryHandlerLock;

    ccci_msg_t *mCcciMsgSend;
    AudioLock    mCcciMsgSendLock;


    ccci_msg_t *mCcciMsgRead;
    AudioLock    mCcciMsgReadLock;

    unsigned char *mShareMemoryBase;
    unsigned int mShareMemoryLength;

    sph_shm_t *mShareMemory;
    AudioLock    mShareMemoryLock;

    AudioLock    mShareMemorySpeechParamLock;
    AudioLock    mShareMemoryApDataLock;
    AudioLock    mShareMemoryMdDataLock;

    uint32_t shm_region_data_count(region_info_t *p_region);
    uint32_t shm_region_free_space(region_info_t *p_region);
    void shm_region_write_from_linear(region_info_t *p_region,
                                      const void *linear_buf,
                                      uint32_t count);
    void shm_region_read_to_linear(void *linear_buf,
                                   region_info_t *p_region,
                                   uint32_t count);

};

} /* end namespace android */

#endif /* end of ANDROID_SPEECH_MESSENGER_NORMAL_H */

