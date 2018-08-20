/*
 * Copyright (c) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "RILC-RP"

//#include <android/hardware/radio/deprecated/1.0/IOemHook.h>
//#include <android/hardware/radio/1.0/IRadio.h>
#include <vendor/mediatek/hardware/radio/2.3/IRadio.h>
#include <vendor/mediatek/hardware/radio/2.3/IRadioResponse.h>
#include <vendor/mediatek/hardware/radio/2.3/IRadioIndication.h>
#include <vendor/mediatek/hardware/radio/deprecated/1.1/IOemHook.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <ril_service.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/SystemClock.h>
#include <inttypes.h>
#include <telephony/librilutilsmtk.h>
#include <cutils/properties.h>

#define INVALID_HEX_CHAR 16

using namespace android::hardware::radio;
using namespace android::hardware::radio::V1_0;
using namespace android::hardware::radio::deprecated::V1_0;
using namespace vendor::mediatek::hardware::radio;
using namespace vendor::mediatek::hardware::radio::V2_0;
using namespace vendor::mediatek::hardware::radio::V2_1;
using namespace vendor::mediatek::hardware::radio::V2_2;
using namespace vendor::mediatek::hardware::radio::V2_3;
using namespace vendor::mediatek::hardware::radio::deprecated::V1_1;

namespace AOSP_V1_0 = android::hardware::radio::V1_0;
namespace AOSP_V1_1 = android::hardware::radio::V1_1;
namespace VENDOR_V2_0 = vendor::mediatek::hardware::radio::V2_0;
namespace VENDOR_V2_1 = vendor::mediatek::hardware::radio::V2_1;
namespace VENDOR_V2_2 = vendor::mediatek::hardware::radio::V2_2;
namespace VENDOR_V2_3 = vendor::mediatek::hardware::radio::V2_3;

namespace VENDOR_V1_1 =
        vendor::mediatek::hardware::radio::deprecated::V1_1;

using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_array;
using ::android::hardware::Void;
using android::CommandInfo;
using android::RequestInfo;
using android::requestToString;
using android::sp;

#define BOOL_TO_INT(x) (x ? 1 : 0)
#define ATOI_NULL_HANDLED(x) (x ? atoi(x) : -1)
#define ATOI_NULL_HANDLED_DEF(x, defaultVal) (x ? atoi(x) : defaultVal)

#if defined(ANDROID_MULTI_SIM)
#define CALL_ONREQUEST(a, b, c, d, e) \
        s_vendorFunctions->onRequest((a), (b), (c), (d), ((RIL_SOCKET_ID)(e)))
#define CALL_ONSTATEREQUEST(a) s_vendorFunctions->onStateRequest((RIL_SOCKET_ID)(a))
#else
#define CALL_ONREQUEST(a, b, c, d, e) s_vendorFunctions->onRequest((a), (b), (c), (d))
#define CALL_ONSTATEREQUEST(a) s_vendorFunctions->onStateRequest()
#endif

RIL_RadioFunctions *s_vendorFunctions = NULL;
static CommandInfo *s_commands;

// M @{
static CardStatus s_cardStatus[MAX_SIM_COUNT * DIVISION_COUNT];
// M @}

struct RadioImpl;
struct OemHookImpl;

//#if (SIM_COUNT >= 2)
sp<RadioImpl> radioService[MAX_SIM_COUNT * DIVISION_COUNT];
sp<OemHookImpl> oemHookService[MAX_SIM_COUNT * DIVISION_COUNT];
// counter used for synchronization. It is incremented every time response callbacks are updated.
volatile int32_t mCounterRadio[MAX_SIM_COUNT * DIVISION_COUNT];
volatile int32_t mCounterOemHook[MAX_SIM_COUNT * DIVISION_COUNT];

// To Compute IMS Slot Id
extern "C" int toRealSlot(int slotId);
extern "C" int isImsSlot(int slotId);
extern "C" int toImsSlot(int slotId);
static pthread_rwlock_t radioServiceRwlock = PTHREAD_RWLOCK_INITIALIZER;

static pthread_rwlock_t radioServiceRwlocks[] = { PTHREAD_RWLOCK_INITIALIZER,
                                                  PTHREAD_RWLOCK_INITIALIZER,
                                                  PTHREAD_RWLOCK_INITIALIZER,
                                                  PTHREAD_RWLOCK_INITIALIZER};

void convertRilHardwareConfigListToHal(void *response, size_t responseLen,
        hidl_vec<HardwareConfig>& records);

void convertRilRadioCapabilityToHal(void *response, size_t responseLen, RadioCapability& rc);

void convertRilLceDataInfoToHal(void *response, size_t responseLen, LceDataInfo& lce);

void convertRilSignalStrengthToHal(void *response, size_t responseLen,
        SignalStrength& signalStrength);

void convertRilDataCallToHal(RIL_Data_Call_Response_v11 *dcResponse,
        SetupDataCallResult& dcResult);

void convertRilDataCallToHalEx(RIL_Data_Call_Response_v11 *dcResponse,
        MtkSetupDataCallResult& dcResult);

void convertRilDataCallListToHal(void *response, size_t responseLen,
        hidl_vec<SetupDataCallResult>& dcResultList);

void convertRilDataCallListToHalEx(void *response, size_t responseLen,
        hidl_vec<MtkSetupDataCallResult>& dcResultList);

void convertRilCellInfoListToHal(void *response, size_t responseLen, hidl_vec<CellInfo>& records);

void convertRilDedicatedBearerInfoToHal(RIL_Dedicate_Data_Call_Struct *response,
        DedicateDataCall& ddcResult);

struct RadioImpl : public VENDOR_V2_3::IRadio {
    int32_t mSlotId;
    sp<AOSP_V1_0::IRadioResponse> mRadioResponse;
    sp<AOSP_V1_0::IRadioIndication> mRadioIndication;
    sp<AOSP_V1_1::IRadioResponse> mRadioResponseV1_1;
    sp<AOSP_V1_1::IRadioIndication> mRadioIndicationV1_1;
    sp<VENDOR_V2_0::IRadioResponse> mRadioResponseMtk;
    sp<VENDOR_V2_0::IRadioIndication> mRadioIndicationMtk;
    sp<VENDOR_V2_1::IRadioResponse> mRadioResponseMtkV2_1;
    sp<VENDOR_V2_1::IRadioIndication> mRadioIndicationMtkV2_1;
    sp<VENDOR_V2_2::IRadioResponse> mRadioResponseMtkV2_2;
    sp<VENDOR_V2_2::IRadioIndication> mRadioIndicationMtkV2_2;
    sp<VENDOR_V2_3::IRadioResponse> mRadioResponseMtkV2_3;
    sp<VENDOR_V2_3::IRadioIndication> mRadioIndicationMtkV2_3;
    sp<IImsRadioResponse> mRadioResponseIms;
    sp<IImsRadioIndication> mRadioIndicationIms;

    Return<void> setResponseFunctions(
            const ::android::sp<AOSP_V1_0::IRadioResponse>& radioResponse,
            const ::android::sp<AOSP_V1_0::IRadioIndication>& radioIndication);

    Return<void> setResponseFunctionsMtk(
            const ::android::sp<VENDOR_V2_0::IRadioResponse>& radioResponse,
            const ::android::sp<VENDOR_V2_0::IRadioIndication>& radioIndication);

    Return<void> setResponseFunctionsIms(
            const ::android::sp<IImsRadioResponse>& radioResponse,
            const ::android::sp<IImsRadioIndication>& radioIndication);

    Return<void> getIccCardStatus(int32_t serial);

    Return<void> supplyIccPinForApp(int32_t serial, const hidl_string& pin,
            const hidl_string& aid);

    Return<void> supplyIccPukForApp(int32_t serial, const hidl_string& puk,
            const hidl_string& pin, const hidl_string& aid);

    Return<void> supplyIccPin2ForApp(int32_t serial,
            const hidl_string& pin2,
            const hidl_string& aid);

    Return<void> supplyIccPuk2ForApp(int32_t serial, const hidl_string& puk2,
            const hidl_string& pin2, const hidl_string& aid);

    Return<void> changeIccPinForApp(int32_t serial, const hidl_string& oldPin,
            const hidl_string& newPin, const hidl_string& aid);

    Return<void> changeIccPin2ForApp(int32_t serial, const hidl_string& oldPin2,
            const hidl_string& newPin2, const hidl_string& aid);

    Return<void> supplyNetworkDepersonalization(int32_t serial, const hidl_string& netPin);

    Return<void> getCurrentCalls(int32_t serial);

    Return<void> dial(int32_t serial, const Dial& dialInfo);

    Return<void> getImsiForApp(int32_t serial,
            const ::android::hardware::hidl_string& aid);

    Return<void> hangup(int32_t serial, int32_t gsmIndex);

    Return<void> hangupWaitingOrBackground(int32_t serial);

    Return<void> hangupForegroundResumeBackground(int32_t serial);

    Return<void> switchWaitingOrHoldingAndActive(int32_t serial);

    Return<void> conference(int32_t serial);

    Return<void> rejectCall(int32_t serial);

    Return<void> getLastCallFailCause(int32_t serial);

    Return<void> getSignalStrength(int32_t serial);

    Return<void> getVoiceRegistrationState(int32_t serial);

    Return<void> getDataRegistrationState(int32_t serial);

    Return<void> getOperator(int32_t serial);

    Return<void> setRadioPower(int32_t serial, bool on);

    Return<void> sendDtmf(int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> sendSms(int32_t serial, const GsmSmsMessage& message);

    Return<void> sendSMSExpectMore(int32_t serial, const GsmSmsMessage& message);

    Return<void> setupDataCall(int32_t serial,
            RadioTechnology radioTechnology,
            const DataProfileInfo& profileInfo,
            bool modemCognitive,
            bool roamingAllowed,
            bool isRoaming);

    Return<void> iccIOForApp(int32_t serial,
            const IccIo& iccIo);

    Return<void> sendUssd(int32_t serial,
            const ::android::hardware::hidl_string& ussd);

    Return<void> cancelPendingUssd(int32_t serial);

    Return<void> getClir(int32_t serial);

    Return<void> setClir(int32_t serial, int32_t status);

    Return<void> getCallForwardStatus(int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> setCallForward(int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> setColp(int32_t serial, int32_t status);

    Return<void> setColr(int32_t serial, int32_t status);

    Return<void> queryCallForwardInTimeSlotStatus(int32_t serial,
            const CallForwardInfoEx& callInfoEx);

    Return<void> setCallForwardInTimeSlot(int32_t serial,
            const CallForwardInfoEx& callInfoEx);

    Return<void> runGbaAuthentication(int32_t serial,
            const hidl_string& nafFqdn, const hidl_string& nafSecureProtocolId,
            bool forceRun, int32_t netId);

    Return<void> getCallWaiting(int32_t serial, int32_t serviceClass);

    Return<void> setCallWaiting(int32_t serial, bool enable, int32_t serviceClass);

    Return<void> acknowledgeLastIncomingGsmSms(int32_t serial,
            bool success, SmsAcknowledgeFailCause cause);

    Return<void> acceptCall(int32_t serial);

    Return<void> deactivateDataCall(int32_t serial,
            int32_t cid, bool reasonRadioShutDown);

    Return<void> getFacilityLockForApp(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setFacilityLockForApp(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            bool lockState,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setBarringPassword(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& oldPassword,
            const ::android::hardware::hidl_string& newPassword);

    Return<void> getNetworkSelectionMode(int32_t serial);

    Return<void> setNetworkSelectionModeAutomatic(int32_t serial);

    Return<void> setNetworkSelectionModeManual(int32_t serial,
            const ::android::hardware::hidl_string& operatorNumeric);

    Return<void> setNetworkSelectionModeManualWithAct(int32_t serial,
            const ::android::hardware::hidl_string& operatorNumeric,
            const ::android::hardware::hidl_string& act,
            const ::android::hardware::hidl_string& mode);

    Return<void> getAvailableNetworks(int32_t serial);

    Return<void> getAvailableNetworksWithAct(int32_t serial);
    Return<void> startNetworkScan(int32_t serial, const AOSP_V1_1::NetworkScanRequest& request);

    Return<void> cancelAvailableNetworks(int32_t serial);
    Return<void> stopNetworkScan(int32_t serial);

    Return<void> startDtmf(int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> stopDtmf(int32_t serial);

    Return<void> getBasebandVersion(int32_t serial);

    Return<void> separateConnection(int32_t serial, int32_t gsmIndex);

    Return<void> setMute(int32_t serial, bool enable);

    Return<void> getMute(int32_t serial);

    Return<void> setBarringPasswordCheckedByNW(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& oldPassword,
            const ::android::hardware::hidl_string& newPassword,
            const ::android::hardware::hidl_string& cfmPassword);

    Return<void> getClip(int32_t serial);

    Return<void> setClip(int32_t serial, int32_t clipEnable);

    Return<void> getColp(int32_t serial);

    Return<void> getColr(int32_t serial);

    Return<void> sendCnap(int32_t serial, const ::android::hardware::hidl_string& cnapMessage);

    Return<void> getDataCallList(int32_t serial);

    Return<void> setSuppServiceNotifications(int32_t serial, bool enable);

    Return<void> writeSmsToSim(int32_t serial,
            const SmsWriteArgs& smsWriteArgs);

    Return<void> deleteSmsOnSim(int32_t serial, int32_t index);

    Return<void> setBandMode(int32_t serial, RadioBandMode mode);

    Return<void> getAvailableBandModes(int32_t serial);

    Return<void> sendEnvelope(int32_t serial,
            const ::android::hardware::hidl_string& command);

    Return<void> sendTerminalResponseToSim(int32_t serial,
            const ::android::hardware::hidl_string& commandResponse);

    Return<void> handleStkCallSetupRequestFromSim(int32_t serial, bool accept);

    Return<void> handleStkCallSetupRequestFromSimWithResCode(int32_t serial, int32_t resultCodet);

    Return<void> explicitCallTransfer(int32_t serial);

    Return<void> setPreferredNetworkType(int32_t serial, PreferredNetworkType nwType);

    Return<void> getPreferredNetworkType(int32_t serial);

    Return<void> getNeighboringCids(int32_t serial);

    Return<void> setLocationUpdates(int32_t serial, bool enable);

    Return<void> setCdmaSubscriptionSource(int32_t serial,
            CdmaSubscriptionSource cdmaSub);

    Return<void> setCdmaRoamingPreference(int32_t serial, CdmaRoamingType type);

    Return<void> getCdmaRoamingPreference(int32_t serial);

    Return<void> setTTYMode(int32_t serial, TtyMode mode);

    Return<void> getTTYMode(int32_t serial);

    Return<void> setPreferredVoicePrivacy(int32_t serial, bool enable);

    Return<void> getPreferredVoicePrivacy(int32_t serial);

    Return<void> sendCDMAFeatureCode(int32_t serial,
            const ::android::hardware::hidl_string& featureCode);

    Return<void> sendBurstDtmf(int32_t serial,
            const ::android::hardware::hidl_string& dtmf,
            int32_t on,
            int32_t off);

    Return<void> sendCdmaSms(int32_t serial, const CdmaSmsMessage& sms);

    Return<void> acknowledgeLastIncomingCdmaSms(int32_t serial,
            const CdmaSmsAck& smsAck);

    Return<void> getGsmBroadcastConfig(int32_t serial);

    Return<void> setGsmBroadcastConfig(int32_t serial,
            const hidl_vec<GsmBroadcastSmsConfigInfo>& configInfo);

    Return<void> setGsmBroadcastActivation(int32_t serial, bool activate);

    Return<void> getCdmaBroadcastConfig(int32_t serial);

    Return<void> setCdmaBroadcastConfig(int32_t serial,
            const hidl_vec<CdmaBroadcastSmsConfigInfo>& configInfo);

    Return<void> setCdmaBroadcastActivation(int32_t serial, bool activate);

    Return<void> getCDMASubscription(int32_t serial);

    Return<void> writeSmsToRuim(int32_t serial, const CdmaSmsWriteArgs& cdmaSms);

    Return<void> deleteSmsOnRuim(int32_t serial, int32_t index);

    Return<void> getDeviceIdentity(int32_t serial);

    Return<void> exitEmergencyCallbackMode(int32_t serial);

    Return<void> getSmscAddress(int32_t serial);

    Return<void> setSmscAddress(int32_t serial,
            const ::android::hardware::hidl_string& smsc);

    Return<void> reportSmsMemoryStatus(int32_t serial, bool available);

    Return<void> reportStkServiceIsRunning(int32_t serial);

    Return<void> getCdmaSubscriptionSource(int32_t serial);

    Return<void> requestIsimAuthentication(int32_t serial,
            const ::android::hardware::hidl_string& challenge);

    Return<void> acknowledgeIncomingGsmSmsWithPdu(int32_t serial,
            bool success,
            const ::android::hardware::hidl_string& ackPdu);

    Return<void> sendEnvelopeWithStatus(int32_t serial,
            const ::android::hardware::hidl_string& contents);

    Return<void> getVoiceRadioTechnology(int32_t serial);

    Return<void> getCellInfoList(int32_t serial);

    Return<void> setCellInfoListRate(int32_t serial, int32_t rate);

    Return<void> setInitialAttachApn(int32_t serial, const DataProfileInfo& dataProfileInfo,
            bool modemCognitive, bool isRoaming);
    Return<void> setInitialAttachApnEx(int32_t serial, const DataProfileInfo& dataProfileInfo,
            bool modemCognitive, bool isRoaming, bool canHandleIms);
    Return<void> getImsRegistrationState(int32_t serial);

    Return<void> sendImsSms(int32_t serial, const ImsSmsMessage& message);

    Return<void> iccTransmitApduBasicChannel(int32_t serial, const SimApdu& message);

    Return<void> iccOpenLogicalChannel(int32_t serial,
            const ::android::hardware::hidl_string& aid, int32_t p2);

    Return<void> iccCloseLogicalChannel(int32_t serial, int32_t channelId);

    Return<void> iccTransmitApduLogicalChannel(int32_t serial, const SimApdu& message);

    Return<void> nvReadItem(int32_t serial, NvItem itemId);

    Return<void> nvWriteItem(int32_t serial, const NvWriteItem& item);

    Return<void> nvWriteCdmaPrl(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& prl);

    Return<void> nvResetConfig(int32_t serial, ResetNvType resetType);

    Return<void> setUiccSubscription(int32_t serial, const SelectUiccSub& uiccSub);

    Return<void> setDataAllowed(int32_t serial, bool allow);

    Return<void> getHardwareConfig(int32_t serial);

    Return<void> requestIccSimAuthentication(int32_t serial,
            int32_t authContext,
            const ::android::hardware::hidl_string& authData,
            const ::android::hardware::hidl_string& aid);

    Return<void> setDataProfile(int32_t serial,
            const ::android::hardware::hidl_vec<DataProfileInfo>& profiles, bool isRoaming);

    Return<void> setDataProfileEx(int32_t serial,
            const ::android::hardware::hidl_vec<MtkDataProfileInfo>& profiles, bool isRoaming);

    Return<void> requestShutdown(int32_t serial);

    Return<void> getRadioCapability(int32_t serial);

    Return<void> setRadioCapability(int32_t serial, const RadioCapability& rc);

    Return<void> startLceService(int32_t serial, int32_t reportInterval, bool pullMode);

    Return<void> stopLceService(int32_t serial);

    Return<void> pullLceData(int32_t serial);

    Return<void> getModemActivityInfo(int32_t serial);

    Return<void> setAllowedCarriers(int32_t serial,
            bool allAllowed,
            const CarrierRestrictions& carriers);

    Return<void> getAllowedCarriers(int32_t serial);

    Return<void> sendDeviceState(int32_t serial, DeviceStateType deviceStateType, bool state);

    Return<void> setIndicationFilter(int32_t serial, int32_t indicationFilter);

    Return<void> startKeepalive(int32_t serial, const AOSP_V1_1::KeepaliveRequest& keepalive);

    Return<void> stopKeepalive(int32_t serial, int32_t sessionHandle);

    Return<void> setSimCardPower(int32_t serial, bool powerUp);
    Return<void> setSimCardPower_1_1(int32_t serial,
            const AOSP_V1_1::CardPowerState state);

    Return<void> responseAcknowledgement();

    Return<void> setCarrierInfoForImsiEncryption(int32_t serial,
            const AOSP_V1_1::ImsiEncryptionInfo& message);

    /// M: CC: call control ([IMS] common flow) @{
    Return<void> hangupAll(int32_t serial);
    Return<void> setCallIndication(int32_t serial,
            int32_t mode, int32_t callId, int32_t seqNumber);

    Return<void> emergencyDial(int32_t serial, const Dial& dialInfo);
    Return<void> setEccServiceCategory(int32_t serial, int32_t serviceCategory);
    // M: Set ECC ist to MD
    Return<void> setEccList(int32_t serial, const hidl_string& list1,
            const hidl_string& list2);
    /// @}

    /// M: CC: For 3G VT only @{
    Return<void> vtDial(int32_t serial, const Dial& dialInfo);
    Return<void> voiceAccept(int32_t serial, int32_t callId);
    Return<void> replaceVtCall(int32_t serial, int32_t index);
    /// @}
    /// M: CC: E911 request current status.
    Return<void> currentStatus(int32_t serial, int32_t airplaneMode,
            int32_t imsReg);
    /// M: CC: E911 request set ECC preferred RAT.
    Return<void> eccPreferredRat(int32_t serial, int32_t phoneType);

    void checkReturnStatus(Return<void>& ret);
    Return<void> setTrm(int32_t serial, int32_t mode);
    Return<void> videoCallAccept(int32_t serial, int32_t videoMode, int32_t callId);

    Return<void> imsEctCommand(int32_t serial, const hidl_string& number,
            int32_t type);

    Return<void> holdCall(int32_t serial, int32_t callId);

    Return<void> resumeCall(int32_t serial, int32_t callId);

    Return<void> imsDeregNotification(int32_t serial,int32_t cause);

    Return<void> setImsEnable(int32_t serial, bool isOn);

    Return<void> setVolteEnable(int32_t serial, bool isOn);

    Return<void> setWfcEnable(int32_t serial, bool isOn);

    Return<void> setVilteEnable(int32_t serial, bool isOn);

    Return<void> setViWifiEnable(int32_t serial, bool isOn);

    Return<void> setImsVoiceEnable(int32_t serial, bool isOn);

    Return<void> setImsVideoEnable(int32_t serial, bool isOn);

    Return<void> setImscfg(int32_t serial, bool volteEnable,
                 bool vilteEnable, bool vowifiEnable, bool viwifiEnable,
                 bool smsEnable, bool eimsEnable);

    Return<void> setModemImsCfg(int32_t serial, const hidl_string& keys,
            const hidl_string& values, int32_t type);

    Return<void> getProvisionValue(int32_t serial,
            const hidl_string& provisionstring);

    Return<void> setProvisionValue(int32_t serial,
            const hidl_string& provisionstring, const hidl_string& provisionValue);

    Return<void> addImsConferenceCallMember(int32_t serial, int32_t confCallId,
            const hidl_string& address, int32_t callToAdd);

    Return<void> removeImsConferenceCallMember(int32_t serial, int32_t confCallId,
            const hidl_string& address, int32_t callToRemove);

    Return<void> setWfcProfile(int32_t serial, int32_t wfcPreference);


    Return<void> conferenceDial(int32_t serial, const ConferenceDial& dailInfo);

    Return<void> vtDialWithSipUri(int32_t serial, const hidl_string& address);

    Return<void> dialWithSipUri(int32_t serial, const hidl_string& address);

    Return<void> sendUssi(int32_t serial, int32_t action, const hidl_string& ussiString);

    Return<void> cancelUssi(int32_t serial);

    Return<void> forceReleaseCall(int32_t serial, int32_t callId);

    Return<void> setImsRtpReport(int32_t serial, int32_t pdnId,
            int32_t networkId, int32_t timer);

    Return<void> imsBearerActivationDone(int32_t serial, int32_t aid, int32_t status);

    Return<void> imsBearerDeactivationDone(int32_t serial, int32_t aid, int32_t status);

    Return<void> pullCall(int32_t serial, const hidl_string& target, bool isVideoCall);

    Return<void> setImsRegistrationReport(int32_t serial);

    // MTK-START: SIM
    Return<void> getATR(int32_t serial);
    Return<void> setSimPower(int32_t serial, int32_t mode);
    // MTK-END

    // MTK-START: SIM GBA
    Return<void> doGeneralSimAuthentication(int32_t serial, const SimAuthStructure& simAuth);
    // MTK-END

  /// M: eMBMS feature
    Return<void> sendEmbmsAtCommand(int32_t serial, const hidl_string& data);
    /// M: eMBMS end

    // ATCI Start
    sp<IAtciResponse> mAtciResponse;
    sp<IAtciIndication> mAtciIndication;

    Return<void> setResponseFunctionsForAtci(
            const ::android::sp<IAtciResponse>& atciResponse,
            const ::android::sp<IAtciIndication>& atciIndication);

    Return<void> sendAtciRequest(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& data);
    // ATCI End

    Return<void> setModemPower(int32_t serial, bool isOn);

    // Femtocell feature
    Return<void> getFemtocellList(int32_t serial);
    Return<void> abortFemtocellList(int32_t serial);
    Return<void> selectFemtocell(int32_t serial,
        const ::android::hardware::hidl_string& operatorNumeric,
        const ::android::hardware::hidl_string& act,
        const ::android::hardware::hidl_string& csgId);
    Return<void> queryFemtoCellSystemSelectionMode(int32_t serial);
    Return<void> setFemtoCellSystemSelectionMode(int32_t serial, int32_t mode);
    // MTK-END: NW
    // SMS-START
    Return<void> getSmsParameters(int32_t serial);
    Return<void> setSmsParameters(int32_t serial, const SmsParams& message);
    Return<void> getSmsMemStatus(int32_t serial);
    Return<void> setEtws(int32_t serial, int32_t mode);
    Return<void> removeCbMsg(int32_t serial, int32_t channelId, int32_t serialId);
    Return<void> setGsmBroadcastLangs(int32_t serial,
            const ::android::hardware::hidl_string& langs);
    Return<void> getGsmBroadcastLangs(int32_t serial);
    Return<void> getGsmBroadcastActivation(int32_t serial);
    // SMS-END
    Return<void> setApcMode(int32_t serial, int32_t mode, int32_t reportMode, int32_t interval);
    Return<void> getApcInfo(int32_t serial);
    // MTK-START: RIL_REQUEST_SWITCH_MODE_FOR_ECC
    Return<void> triggerModeSwitchByEcc(int32_t serial, int32_t mode);
    // MTK-END
    Return<void> getSmsRuimMemoryStatus(int32_t serial);
    // FastDormancy
    Return<void> setFdMode(int32_t serial, int mode, int param1, int param2);

    // worldmode {
    Return<void> setResumeRegistration(int32_t serial, int32_t sessionId);

    Return<void> storeModemType(int32_t serial, int32_t modemType);

    Return<void> reloadModemType(int32_t serial, int32_t modemType);
    // worldmode }
    // PHB start
    Return<void> queryPhbStorageInfo(int32_t serial, int32_t type);
    Return<void> writePhbEntry(int32_t serial, const PhbEntryStructure& phbEntry);
    Return<void> readPhbEntry(int32_t serial, int32_t type, int32_t bIndex, int32_t eIndex);
    Return<void> queryUPBCapability(int32_t serial);
    Return<void> editUPBEntry(int32_t serial, const hidl_vec<hidl_string>& data);
    Return<void> deleteUPBEntry(int32_t serial, int32_t entryType, int32_t adnIndex, int32_t entryIndex);
    Return<void> readUPBGasList(int32_t serial, int32_t startIndex, int32_t endIndex);
    Return<void> readUPBGrpEntry(int32_t serial, int32_t adnIndex);
    Return<void> writeUPBGrpEntry(int32_t serial, int32_t adnIndex, const hidl_vec<int32_t>& grpIds);
    Return<void> getPhoneBookStringsLength(int32_t serial);
    Return<void> getPhoneBookMemStorage(int32_t serial);
    Return<void> setPhoneBookMemStorage(int32_t serial, const hidl_string& storage, const hidl_string& password);
    Return<void> readPhoneBookEntryExt(int32_t serial, int32_t index1, int32_t index2);
    Return<void> writePhoneBookEntryExt(int32_t serial, const PhbEntryExt& phbEntryExt);
    Return<void> queryUPBAvailable(int32_t serial, int32_t eftype, int32_t fileIndex);
    Return<void> readUPBEmailEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex);
    Return<void> readUPBSneEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex);
    Return<void> readUPBAnrEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex);
    Return<void> readUPBAasList(int32_t serial, int32_t startIndex, int32_t endIndex);
    // PHB End
    Return<void> resetRadio(int32_t serial);

    // M: Data Framework - common part enhancement
    Return<void> syncDataSettingsToMd(int32_t serial,
            const hidl_vec<int32_t>& settings);
    // M: Data Framework - Data Retry enhancement
    Return<void> resetMdDataRetryCount(int32_t serial,
            const hidl_string& apn);
    // M: Data Framework - CC 33
    Return<void> setRemoveRestrictEutranMode(int32_t serial, int32_t type);
    // MTK-START: SIM ME LOCK
    Return<void> queryNetworkLock(int32_t serial, int32_t category);
    Return<void> setNetworkLock(int32_t serial, int32_t category, int32_t lockop,
            const hidl_string& password, const hidl_string& data_imsi,
            const hidl_string& gid1, const hidl_string& gid2);
    // MTK-END
    // M: [LTE][Low Power][UL traffic shaping] @{
    Return<void> setLteAccessStratumReport(int32_t serial, int32_t enable);
    Return<void> setLteUplinkDataTransfer(int32_t serial, int32_t state, int32_t interfaceId);
    // M: [LTE][Low Power][UL traffic shaping] @}
    Return<void> setRxTestConfig(int32_t serial, int32_t antType);
    Return<void> getRxTestResult(int32_t serial, int32_t mode);

    Return<void> getPOLCapability(int32_t serial);
    Return<void> getCurrentPOLList(int32_t serial);
    Return<void> setPOLEntry(int32_t serial, int32_t index,
            const ::android::hardware::hidl_string& numeric,
            int32_t nAct);
    /// M: [Network][C2K] Sprint roaming control @{
    Return<void> setRoamingEnable(int32_t serial, const hidl_vec<int32_t>& config);
    Return<void> getRoamingEnable(int32_t serial, int32_t phoneId);
    /// @}
    // External SIM [Start]
    Return<void> sendVsimNotification(int32_t serial, uint32_t transactionId,
            uint32_t eventId, uint32_t simType);

    Return<void> sendVsimOperation(int32_t serial, uint32_t transactionId,
            uint32_t eventId, int32_t result, int32_t dataLength, const hidl_vec<uint8_t>& data);
    // External SIM [End]
    Return<void> setVoiceDomainPreference(int32_t serial, int32_t vdp);

    /// MwiService@{
    Return<void> setWifiEnabled(int32_t serial, int32_t phoneId,
            const hidl_string& ifName, int32_t isEnabled);
    Return<void> setWifiFlightModeEnabled(int32_t serial, int32_t phoneId,
            const hidl_string& ifName, int32_t isWifiEnabled, int32_t isFlightModeOn);
    Return<void> setWifiAssociated(int32_t serial, int32_t phoneId,
            const hidl_string& ifName, int32_t associated, const hidl_string& ssid,
            const hidl_string& apMac);
    Return<void> setWifiSignalLevel(int32_t serial, int32_t phoneId,
            int32_t rssi, int32_t snr);
    Return<void> setWifiIpAddress(int32_t serial, int32_t phoneId,
            const hidl_string& ifName, const hidl_string& ipv4Addr,
            const hidl_string& ipv6Addr);
    Return<void> setLocationInfo(int32_t serial, int32_t phoneId, const hidl_string& accountId,
            const hidl_string& broadcastFlag, const hidl_string& latitude,
            const hidl_string& longitude, const hidl_string& accuracy,
            const hidl_string& method, const hidl_string& city, const hidl_string& state,
            const hidl_string& zip, const hidl_string& countryCode);
    Return<void> setLocationInfoWlanMac(int32_t serial, int32_t phoneId,
            const hidl_string& accountId,
            const hidl_string& broadcastFlag, const hidl_string& latitude,
            const hidl_string& longitude, const hidl_string& accuracy,
            const hidl_string& method, const hidl_string& city, const hidl_string& state,
            const hidl_string& zip, const hidl_string& countryCode, const hidl_string& ueWlanMac);
    Return<void> setEmergencyAddressId(int32_t serial,
            int32_t phoneId, const hidl_string& aid);
    Return<void> setNattKeepAliveStatus(int32_t serial, int32_t phoneId,
            const hidl_string& ifName, bool enable,
            const hidl_string& srcIp, int32_t srcPort,
            const hidl_string& dstIp, int32_t dstPort);
    ///@}
    Return<void> setE911State(int32_t serial, int32_t state);
    Return<void> setServiceStateToModem(int32_t serial, int32_t voiceRegState,
            int32_t dataRegState, int32_t voiceRoamingType, int32_t dataRoamingType,
            int32_t rilVoiceRegState, int32_t rilDataRegState);
    Return<void> sendRequestRaw(int32_t serial, const hidl_vec<uint8_t>& data);
    Return<void> sendRequestStrings(int32_t serial, const hidl_vec<hidl_string>& data);
};

struct OemHookImpl : public VENDOR_V1_1::IOemHook {
    int32_t mSlotId;
    sp<IOemHookResponse> mOemHookResponse;
    sp<IOemHookIndication> mOemHookIndication;

    Return<void> setResponseFunctions(
            const ::android::sp<IOemHookResponse>& oemHookResponse,
            const ::android::sp<IOemHookIndication>& oemHookIndication);

    Return<void> sendRequestRaw(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendRequestStrings(int32_t serial,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);
};

void memsetAndFreeStrings(int numPointers, ...) {
    va_list ap;
    va_start(ap, numPointers);
    for (int i = 0; i < numPointers; i++) {
        char *ptr = va_arg(ap, char *);
        if (ptr) {
#ifdef MEMSET_FREED
            // TODO: Should pass in the maximum length of the string
            memsetString (ptr);
#endif
            free(ptr);
        }
    }
    va_end(ap);
}

void sendErrorResponse(RequestInfo *pRI, RIL_Errno err) {
    pRI->pCI->responseFunction((int) pRI->socket_id,
            (int) RadioResponseType::SOLICITED, pRI->token, err, NULL, 0);
}

/**
 * Copies over src to dest. If memory allocation fails, responseFunction() is called for the
 * request with error RIL_E_NO_MEMORY.
 * Returns true on success, and false on failure.
 */
bool copyHidlStringToRil(char **dest, const hidl_string &src, RequestInfo *pRI) {
    size_t len = src.size();
    if (len == 0) {
        *dest = NULL;
        return true;
    }
    *dest = (char *) calloc(len + 1, sizeof(char));
    if (*dest == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    strncpy(*dest, src.c_str(), len + 1);
    return true;
}

hidl_string convertCharPtrToHidlString(const char *ptr) {
    hidl_string ret;
    if (ptr != NULL) {
        // TODO: replace this with strnlen
        ret.setToExternal(ptr, strlen(ptr));
    }
    return ret;
}

bool dispatchVoid(int serial, int slotId, int request) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }
    CALL_ONREQUEST(request, NULL, 0, pRI, slotId);
    return true;
}

bool dispatchString(int serial, int slotId, int request, const char * str) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    char *pString;
    if (!copyHidlStringToRil(&pString, str, pRI)) {
        return false;
    }

    CALL_ONREQUEST(request, pString, sizeof(char *), pRI, slotId);

    memsetAndFreeStrings(1, pString);
    return true;
}

bool dispatchStrings(int serial, int slotId, int request, int countStrings, ...) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    char **pStrings;
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    va_list ap;
    va_start(ap, countStrings);
    for (int i = 0; i < countStrings; i++) {
        const char* str = va_arg(ap, const char *);
        if (!copyHidlStringToRil(&pStrings[i], hidl_string(str), pRI)) {
            va_end(ap);
            for (int j = 0; j < i; j++) {
                memsetAndFreeStrings(1, pStrings[j]);
            }
            free(pStrings);
            return false;
        }
    }
    va_end(ap);

    CALL_ONREQUEST(request, pStrings, countStrings * sizeof(char *), pRI, slotId);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
            memsetAndFreeStrings(1, pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }
    return true;
}

bool dispatchStrings(int serial, int slotId, int request, const hidl_vec<hidl_string>& data) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    int countStrings = data.size();
    char **pStrings;
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }

    for (int i = 0; i < countStrings; i++) {
        if (!copyHidlStringToRil(&pStrings[i], data[i], pRI)) {
            for (int j = 0; j < i; j++) {
                memsetAndFreeStrings(1, pStrings[j]);
            }
            free(pStrings);
            return false;
        }
    }

    CALL_ONREQUEST(request, pStrings, countStrings * sizeof(char *), pRI, slotId);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
            memsetAndFreeStrings(1, pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }
    return true;
}

bool dispatchInts(int serial, int slotId, int request, int countInts, ...) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    int *pInts = (int *)calloc(countInts, sizeof(int));

    if (pInts == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    va_list ap;
    va_start(ap, countInts);
    for (int i = 0; i < countInts; i++) {
        pInts[i] = va_arg(ap, int);
    }
    va_end(ap);

    CALL_ONREQUEST(request, pInts, countInts * sizeof(int), pRI, slotId);

    if (pInts != NULL) {
#ifdef MEMSET_FREED
        memset(pInts, 0, countInts * sizeof(int));
#endif
        free(pInts);
    }
    return true;
}

bool dispatchCallForwardStatus(int serial, int slotId, int request,
                              const CallForwardInfo& callInfo) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_CallForwardInfo cf;
    cf.status = (int) callInfo.status;
    cf.reason = callInfo.reason;
    cf.serviceClass = callInfo.serviceClass;
    cf.toa = callInfo.toa;
    cf.timeSeconds = callInfo.timeSeconds;

    if (!copyHidlStringToRil(&cf.number, callInfo.number, pRI)) {
        return false;
    }

    CALL_ONREQUEST(request, &cf, sizeof(cf), pRI, slotId);

    memsetAndFreeStrings(1, cf.number);

    return true;
}

bool dispatchCallForwardInTimeSlotStatus(int serial, int slotId, int request,
                              const CallForwardInfoEx& callInfoEx) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_CallForwardInfoEx cfEx;
    cfEx.status = (int) callInfoEx.status;
    cfEx.reason = callInfoEx.reason;
    cfEx.serviceClass = callInfoEx.serviceClass;
    cfEx.toa = callInfoEx.toa;
    cfEx.timeSeconds = callInfoEx.timeSeconds;

    if (!copyHidlStringToRil(&cfEx.number, callInfoEx.number, pRI)) {
        return false;
    }

    if (!copyHidlStringToRil(&cfEx.timeSlotBegin, callInfoEx.timeSlotBegin, pRI)) {
        memsetAndFreeStrings(1, cfEx.number);
        return false;
    }

    if (!copyHidlStringToRil(&cfEx.timeSlotEnd, callInfoEx.timeSlotEnd, pRI)) {
        memsetAndFreeStrings(2, cfEx.number, cfEx.timeSlotBegin);
        return false;
    }

    s_vendorFunctions->onRequest(request, &cfEx, sizeof(cfEx), pRI,
            pRI->socket_id);

    memsetAndFreeStrings(3, cfEx.number, cfEx.timeSlotBegin, cfEx.timeSlotEnd);

    return true;
}

bool dispatchRaw(int serial, int slotId, int request, const hidl_vec<uint8_t>& rawBytes) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    const uint8_t *uData = rawBytes.data();

    CALL_ONREQUEST(request, (void *) uData, rawBytes.size(), pRI, slotId);

    return true;
}

bool dispatchIccApdu(int serial, int slotId, int request, const SimApdu& message) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_SIM_APDU apdu = {};

    apdu.sessionid = message.sessionId;
    apdu.cla = message.cla;
    apdu.instruction = message.instruction;
    apdu.p1 = message.p1;
    apdu.p2 = message.p2;
    apdu.p3 = message.p3;

    if (!copyHidlStringToRil(&apdu.data, message.data, pRI)) {
        return false;
    }

    CALL_ONREQUEST(request, &apdu, sizeof(apdu), pRI, slotId);

    memsetAndFreeStrings(1, apdu.data);

    return true;
}

void checkReturnStatus(int32_t slotId, Return<void>& ret) {
    if (ret.isOk() == false) {
        RLOGE("checkReturnStatus: unable to call response/indication callback");
        // Remote process hosting the callbacks must be dead. Reset the callback objects;
        // there's no other recovery to be done here. When the client process is back up, it will
        // call setResponseFunctions()

        // Caller should already hold rdlock, release that first
        // note the current counter to avoid overwriting updates made by another thread before
        // write lock is acquired.
        int counter = mCounterRadio[slotId];
        pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(slotId);
        int ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // acquire wrlock
        ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // make sure the counter value has not changed
        if (counter == mCounterRadio[slotId]) {
            if(isImsSlot(slotId)) {
                radioService[slotId]->mRadioResponseIms = NULL;
                radioService[slotId]->mRadioIndicationIms = NULL;
            }
            else {
                radioService[slotId]->mRadioResponse = NULL;
                radioService[slotId]->mRadioIndication = NULL;
                radioService[slotId]->mRadioResponseV1_1 = NULL;
                radioService[slotId]->mRadioIndicationV1_1 = NULL;
            }
            mCounterRadio[slotId]++;
        } else {
            RLOGE("checkReturnStatus: not resetting responseFunctions as they likely "
                    "got updated on another thread");
        }

        // release wrlock
        ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // Reacquire rdlock
        ret = pthread_rwlock_rdlock(radioServiceRwlockPtr);
        assert(ret == 0);
    }
}

void RadioImpl::checkReturnStatus(Return<void>& ret) {
    ::checkReturnStatus(mSlotId, ret);
}

Return<void> RadioImpl::setResponseFunctions(
        const ::android::sp<AOSP_V1_0::IRadioResponse>& radioResponseParam,
        const ::android::sp<AOSP_V1_0::IRadioIndication>& radioIndicationParam) {
    RLOGI("setResponseFunctions, slotId = %d", mSlotId);

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(mSlotId);
    int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
    assert(ret == 0);

    mRadioResponse = radioResponseParam;
    mRadioIndication = radioIndicationParam;
    mRadioResponseV1_1 = AOSP_V1_1::IRadioResponse::castFrom(mRadioResponse).withDefault(nullptr);
    mRadioIndicationV1_1 = AOSP_V1_1::IRadioIndication::castFrom(mRadioIndication).withDefault(nullptr);
    if (mRadioResponseV1_1 == nullptr || mRadioIndicationV1_1 == nullptr) {
        mRadioResponseV1_1 = nullptr;
        mRadioIndicationV1_1 = nullptr;
    }

    mCounterRadio[mSlotId]++;

    ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(ret == 0);

    // client is connected. Send initial indications.
    android::onNewCommandConnect((RIL_SOCKET_ID) mSlotId);

    return Void();
}

Return<void> RadioImpl::setResponseFunctionsMtk(
        const ::android::sp<VENDOR_V2_0::IRadioResponse>& radioResponseParam,
        const ::android::sp<VENDOR_V2_0::IRadioIndication>& radioIndicationParam) {
    RLOGI("setResponseFunctionsMtk, slotId = %d", mSlotId);

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(mSlotId);
    int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
    assert(ret == 0);

    mRadioResponseMtk = radioResponseParam;
    mRadioIndicationMtk = radioIndicationParam;

    mRadioResponseMtkV2_1 = VENDOR_V2_1::IRadioResponse::castFrom(mRadioResponseMtk).withDefault(nullptr);
    mRadioIndicationMtkV2_1 = VENDOR_V2_1::IRadioIndication::castFrom(mRadioIndicationMtk).withDefault(nullptr);
    if (mRadioResponseMtkV2_1 == nullptr || mRadioIndicationMtkV2_1 == nullptr) {
        mRadioResponseMtkV2_1 = nullptr;
        mRadioIndicationMtkV2_1 = nullptr;
    }

    mRadioResponseMtkV2_2 = VENDOR_V2_2::IRadioResponse::castFrom(mRadioResponseMtk).withDefault(nullptr);
    mRadioIndicationMtkV2_2 = VENDOR_V2_2::IRadioIndication::castFrom(mRadioIndicationMtk).withDefault(nullptr);
    if (mRadioResponseMtkV2_2 == nullptr || mRadioIndicationMtkV2_2 == nullptr) {
        mRadioResponseMtkV2_2 = nullptr;
        mRadioIndicationMtkV2_2 = nullptr;
    }

    mRadioResponseMtkV2_3 = VENDOR_V2_3::IRadioResponse::castFrom(mRadioResponseMtk).withDefault(nullptr);
    mRadioIndicationMtkV2_3 = VENDOR_V2_3::IRadioIndication::castFrom(mRadioIndicationMtk).withDefault(nullptr);
    if (mRadioResponseMtkV2_3 == nullptr || mRadioIndicationMtkV2_3 == nullptr) {
        mRadioResponseMtkV2_3 = nullptr;
        mRadioIndicationMtkV2_3 = nullptr;
    }


    ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(ret == 0);

    return Void();
}

Return<void> RadioImpl::setResponseFunctionsIms(
        const ::android::sp<IImsRadioResponse>& radioResponseParam,
        const ::android::sp<IImsRadioIndication>& radioIndicationParam) {
    RLOGD("setResponseFunctionsIms");

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(mSlotId);
    int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
    assert(ret == 0);

    mRadioResponseIms = radioResponseParam;
    mRadioIndicationIms = radioIndicationParam;

    ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(ret == 0);

    return Void();
}

Return<void> RadioImpl::getIccCardStatus(int32_t serial) {
    RLOGD("getIccCardStatus: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SIM_STATUS);
    return Void();
}

Return<void> RadioImpl::supplyIccPinForApp(int32_t serial, const hidl_string& pin,
        const hidl_string& aid) {
    RLOGD("supplyIccPinForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN,
            2, pin.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::supplyIccPukForApp(int32_t serial, const hidl_string& puk,
                                           const hidl_string& pin, const hidl_string& aid) {
    RLOGD("supplyIccPukForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK,
            3, puk.c_str(), pin.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::supplyIccPin2ForApp(int32_t serial, const hidl_string& pin2,
                                            const hidl_string& aid) {
    RLOGD("supplyIccPin2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN2,
            2, pin2.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::supplyIccPuk2ForApp(int32_t serial, const hidl_string& puk2,
                                            const hidl_string& pin2, const hidl_string& aid) {
    RLOGD("supplyIccPuk2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK2,
            3, puk2.c_str(), pin2.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::changeIccPinForApp(int32_t serial, const hidl_string& oldPin,
                                           const hidl_string& newPin, const hidl_string& aid) {
    RLOGD("changeIccPinForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN,
            3, oldPin.c_str(), newPin.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::changeIccPin2ForApp(int32_t serial, const hidl_string& oldPin2,
                                            const hidl_string& newPin2, const hidl_string& aid) {
    RLOGD("changeIccPin2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN2,
            3, oldPin2.c_str(), newPin2.c_str(), aid.c_str());
    return Void();
}

Return<void> RadioImpl::supplyNetworkDepersonalization(int32_t serial,
                                                       const hidl_string& netPin) {
    RLOGD("supplyNetworkDepersonalization: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION,
            1, netPin.c_str());
    return Void();
}

Return<void> RadioImpl::getCurrentCalls(int32_t serial) {
    RLOGD("getCurrentCalls: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CURRENT_CALLS);
    return Void();
}

Return<void> RadioImpl::dial(int32_t serial, const Dial& dialInfo) {
    RLOGD("dial: serial %d", serial);

    int requestId = RIL_REQUEST_DIAL;
    if(isImsSlot(mSlotId)) {
        requestId = RIL_REQUEST_IMS_DIAL;
    }

    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, requestId);
    if (pRI == NULL) {
        return Void();
    }
    RIL_Dial dial = {};
    RIL_UUS_Info uusInfo = {};
    int32_t sizeOfDial = sizeof(dial);

    if (!copyHidlStringToRil(&dial.address, dialInfo.address, pRI)) {
        return Void();
    }
    dial.clir = (int) dialInfo.clir;

    if (dialInfo.uusInfo.size() != 0) {
        uusInfo.uusType = (RIL_UUS_Type) dialInfo.uusInfo[0].uusType;
        uusInfo.uusDcs = (RIL_UUS_DCS) dialInfo.uusInfo[0].uusDcs;

        if (dialInfo.uusInfo[0].uusData.size() == 0) {
            uusInfo.uusData = NULL;
            uusInfo.uusLength = 0;
        } else {
            if (!copyHidlStringToRil(&uusInfo.uusData, dialInfo.uusInfo[0].uusData, pRI)) {
                memsetAndFreeStrings(1, dial.address);
                return Void();
            }
            uusInfo.uusLength = dialInfo.uusInfo[0].uusData.size();
        }

        dial.uusInfo = &uusInfo;
    }

    CALL_ONREQUEST(requestId, &dial, sizeOfDial, pRI, mSlotId);

    memsetAndFreeStrings(2, dial.address, uusInfo.uusData);

    return Void();
}

Return<void> RadioImpl::getImsiForApp(int32_t serial, const hidl_string& aid) {
    RLOGD("getImsiForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_GET_IMSI,
            1, aid.c_str());
    return Void();
}

Return<void> RadioImpl::hangup(int32_t serial, int32_t gsmIndex) {
    RLOGD("hangup: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_HANGUP, 1, gsmIndex);
    return Void();
}

Return<void> RadioImpl::hangupWaitingOrBackground(int32_t serial) {
    RLOGD("hangupWaitingOrBackground: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND);
    return Void();
}

Return<void> RadioImpl::hangupForegroundResumeBackground(int32_t serial) {
    RLOGD("hangupForegroundResumeBackground: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND);
    return Void();
}

Return<void> RadioImpl::switchWaitingOrHoldingAndActive(int32_t serial) {
    RLOGD("switchWaitingOrHoldingAndActive: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE);
    return Void();
}

Return<void> RadioImpl::conference(int32_t serial) {
    RLOGD("conference: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CONFERENCE);
    return Void();
}

Return<void> RadioImpl::rejectCall(int32_t serial) {
    RLOGD("rejectCall: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_UDUB);
    return Void();
}

Return<void> RadioImpl::getLastCallFailCause(int32_t serial) {
    RLOGD("getLastCallFailCause: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_LAST_CALL_FAIL_CAUSE);
    return Void();
}

Return<void> RadioImpl::getSignalStrength(int32_t serial) {
    RLOGD("getSignalStrength: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SIGNAL_STRENGTH);
    return Void();
}

Return<void> RadioImpl::getVoiceRegistrationState(int32_t serial) {
    RLOGD("getVoiceRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_VOICE_REGISTRATION_STATE);
    return Void();
}

Return<void> RadioImpl::getDataRegistrationState(int32_t serial) {
    RLOGD("getDataRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DATA_REGISTRATION_STATE);
    return Void();
}

Return<void> RadioImpl::getOperator(int32_t serial) {
    RLOGD("getOperator: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_OPERATOR);
    return Void();
}

Return<void> RadioImpl::setRadioPower(int32_t serial, bool on) {
    RLOGD("setRadioPower: serial %d on %d", serial, on);
    dispatchInts(serial, mSlotId, RIL_REQUEST_RADIO_POWER, 1, BOOL_TO_INT(on));
    return Void();
}

Return<void> RadioImpl::sendDtmf(int32_t serial, const hidl_string& s) {
    RLOGD("sendDtmf: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_DTMF, s.c_str());
    return Void();
}

Return<void> RadioImpl::sendSms(int32_t serial, const GsmSmsMessage& message) {
    RLOGD("sendSms: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SEND_SMS,
            2, message.smscPdu.c_str(), message.pdu.c_str());
    return Void();
}

Return<void> RadioImpl::sendSMSExpectMore(int32_t serial, const GsmSmsMessage& message) {
    RLOGD("sendSMSExpectMore: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SEND_SMS_EXPECT_MORE,
            2, message.smscPdu.c_str(), message.pdu.c_str());
    return Void();
}

static bool convertMvnoTypeToString(MvnoType type, char *&str) {
    switch (type) {
        case MvnoType::IMSI:
            str = (char *)"imsi";
            return true;
        case MvnoType::GID:
            str = (char *)"gid";
            return true;
        case MvnoType::SPN:
            str = (char *)"spn";
            return true;
        case MvnoType::NONE:
            str = (char *)"";
            return true;
    }
    return false;
}

Return<void> RadioImpl::setupDataCall(int32_t serial, RadioTechnology radioTechnology,
                                      const DataProfileInfo& dataProfileInfo, bool modemCognitive,
                                      bool roamingAllowed, bool isRoaming) {

    RLOGD("setupDataCall: serial %d", serial);

    if (s_vendorFunctions->version >= 4 && s_vendorFunctions->version <= 14) {
        const hidl_string &protocol =
                (isRoaming ? dataProfileInfo.roamingProtocol : dataProfileInfo.protocol);
        dispatchStrings(serial, mSlotId, RIL_REQUEST_SETUP_DATA_CALL, 7,
            std::to_string((int) radioTechnology + 2).c_str(),
            std::to_string((int) dataProfileInfo.profileId).c_str(),
            dataProfileInfo.apn.c_str(),
            dataProfileInfo.user.c_str(),
            dataProfileInfo.password.c_str(),
            std::to_string((int) dataProfileInfo.authType).c_str(),
            protocol.c_str());
    } else if (s_vendorFunctions->version >= 15) {
        char *mvnoTypeStr = NULL;
        if (!convertMvnoTypeToString(dataProfileInfo.mvnoType, mvnoTypeStr)) {
            RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                    RIL_REQUEST_SETUP_DATA_CALL);
            if (pRI != NULL) {
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            }
            return Void();
        }
        dispatchStrings(serial, mSlotId, RIL_REQUEST_SETUP_DATA_CALL, 15,
            std::to_string((int) radioTechnology + 2).c_str(),
            std::to_string((int) dataProfileInfo.profileId).c_str(),
            dataProfileInfo.apn.c_str(),
            dataProfileInfo.user.c_str(),
            dataProfileInfo.password.c_str(),
            std::to_string((int) dataProfileInfo.authType).c_str(),
            dataProfileInfo.protocol.c_str(),
            dataProfileInfo.roamingProtocol.c_str(),
            std::to_string(dataProfileInfo.supportedApnTypesBitmap).c_str(),
            std::to_string(dataProfileInfo.bearerBitmap).c_str(),
            modemCognitive ? "1" : "0",
            std::to_string(dataProfileInfo.mtu).c_str(),
            mvnoTypeStr,
            dataProfileInfo.mvnoMatchData.c_str(),
            roamingAllowed ? "1" : "0");
    } else {
        RLOGE("Unsupported RIL version %d, min version expected 4", s_vendorFunctions->version);
        RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                RIL_REQUEST_SETUP_DATA_CALL);
        if (pRI != NULL) {
            sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        }
    }
    return Void();
}

Return<void> RadioImpl::iccIOForApp(int32_t serial, const IccIo& iccIo) {
    RLOGD("iccIOForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SIM_IO);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SIM_IO_v6 rilIccIo = {};
    rilIccIo.command = iccIo.command;
    rilIccIo.fileid = iccIo.fileId;
    if (!copyHidlStringToRil(&rilIccIo.path, iccIo.path, pRI)) {
        return Void();
    }

    rilIccIo.p1 = iccIo.p1;
    rilIccIo.p2 = iccIo.p2;
    rilIccIo.p3 = iccIo.p3;

    if (!copyHidlStringToRil(&rilIccIo.data, iccIo.data, pRI)) {
        memsetAndFreeStrings(1, rilIccIo.path);
        return Void();
    }

    if (!copyHidlStringToRil(&rilIccIo.pin2, iccIo.pin2, pRI)) {
        memsetAndFreeStrings(2, rilIccIo.path, rilIccIo.data);
        return Void();
    }

    if (!copyHidlStringToRil(&rilIccIo.aidPtr, iccIo.aid, pRI)) {
        memsetAndFreeStrings(3, rilIccIo.path, rilIccIo.data, rilIccIo.pin2);
        return Void();
    }

    CALL_ONREQUEST(RIL_REQUEST_SIM_IO, &rilIccIo, sizeof(rilIccIo), pRI, mSlotId);

    memsetAndFreeStrings(4, rilIccIo.path, rilIccIo.data, rilIccIo.pin2, rilIccIo.aidPtr);

    return Void();
}

Return<void> RadioImpl::sendUssd(int32_t serial, const hidl_string& ussd) {
    RLOGD("sendUssd: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SEND_USSD, ussd.c_str());
    return Void();
}

Return<void> RadioImpl::cancelPendingUssd(int32_t serial) {
    RLOGD("cancelPendingUssd: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CANCEL_USSD);
    return Void();
}

Return<void> RadioImpl::getClir(int32_t serial) {
    RLOGD("getClir: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CLIR);
    return Void();
}

Return<void> RadioImpl::setClir(int32_t serial, int32_t status) {
    RLOGD("setClir: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CLIR, 1, status);
    return Void();
}

Return<void> RadioImpl::getCallForwardStatus(int32_t serial, const CallForwardInfo& callInfo) {
    RLOGD("getCallForwardStatus: serial %d", serial);
    dispatchCallForwardStatus(serial, mSlotId, RIL_REQUEST_QUERY_CALL_FORWARD_STATUS,
            callInfo);
    return Void();
}

Return<void> RadioImpl::setCallForward(int32_t serial, const CallForwardInfo& callInfo) {
    RLOGD("setCallForward: serial %d", serial);
    dispatchCallForwardStatus(serial, mSlotId, RIL_REQUEST_SET_CALL_FORWARD,
            callInfo);
    return Void();
}

Return<void> RadioImpl::setColp(int32_t serial, int32_t status) {
    RLOGD("setColp: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_COLP, 1, status);
    return Void();
}

Return<void> RadioImpl::setColr(int32_t serial, int32_t status) {
    RLOGD("setColr: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_COLR, 1, status);
    return Void();
}

Return<void> RadioImpl::queryCallForwardInTimeSlotStatus(int32_t serial,
                                                         const CallForwardInfoEx& callInfoEx) {
    RLOGD("queryCallForwardInTimeSlotStatus: serial %d", serial);
    dispatchCallForwardInTimeSlotStatus(serial, mSlotId, RIL_REQUEST_QUERY_CALL_FORWARD_IN_TIME_SLOT,
            callInfoEx);
    return Void();
}

Return<void> RadioImpl::setCallForwardInTimeSlot(int32_t serial,
                                                 const CallForwardInfoEx& callInfoEx) {
    RLOGD("setCallForwardInTimeSlot: serial %d", serial);
    dispatchCallForwardInTimeSlotStatus(serial, mSlotId, RIL_REQUEST_SET_CALL_FORWARD_IN_TIME_SLOT,
            callInfoEx);
    return Void();
}

Return<void> RadioImpl::runGbaAuthentication(int32_t serial,
            const hidl_string& nafFqdn, const hidl_string& nafSecureProtocolId,
            bool forceRun, int32_t netId) {
    RLOGD("runGbaAuthentication: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_RUN_GBA, 4,
                    nafFqdn.c_str(), nafSecureProtocolId.c_str(),
                    forceRun ? "1" : "0", (std::to_string(netId)).c_str());
    return Void();
}

Return<void> RadioImpl::getCallWaiting(int32_t serial, int32_t serviceClass) {
    RLOGD("getCallWaiting: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_QUERY_CALL_WAITING, 1, serviceClass);
    return Void();
}

Return<void> RadioImpl::setCallWaiting(int32_t serial, bool enable, int32_t serviceClass) {
    RLOGD("setCallWaiting: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CALL_WAITING, 2, BOOL_TO_INT(enable),
            serviceClass);
    return Void();
}

Return<void> RadioImpl::acknowledgeLastIncomingGsmSms(int32_t serial,
                                                      bool success, SmsAcknowledgeFailCause cause) {
    RLOGD("acknowledgeLastIncomingGsmSms: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SMS_ACKNOWLEDGE, 2, BOOL_TO_INT(success),
            cause);
    return Void();
}

Return<void> RadioImpl::acceptCall(int32_t serial) {
    RLOGD("acceptCall: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_ANSWER);
    return Void();
}

Return<void> RadioImpl::deactivateDataCall(int32_t serial,
                                           int32_t cid, bool reasonRadioShutDown) {
    RLOGD("deactivateDataCall: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_DEACTIVATE_DATA_CALL,
            2, (const char *) (std::to_string(cid)).c_str(), reasonRadioShutDown ? "1" : "0");
    return Void();
}

Return<void> RadioImpl::getFacilityLockForApp(int32_t serial, const hidl_string& facility,
                                              const hidl_string& password, int32_t serviceClass,
                                              const hidl_string& appId) {
    RLOGD("getFacilityLockForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_QUERY_FACILITY_LOCK,
            4, facility.c_str(), password.c_str(),
            (std::to_string(serviceClass)).c_str(), appId.c_str());
    return Void();
}

Return<void> RadioImpl::setFacilityLockForApp(int32_t serial, const hidl_string& facility,
                                              bool lockState, const hidl_string& password,
                                              int32_t serviceClass, const hidl_string& appId) {
    RLOGD("setFacilityLockForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_FACILITY_LOCK,
            5, facility.c_str(), lockState ? "1" : "0", password.c_str(),
            (std::to_string(serviceClass)).c_str(), appId.c_str() );
    return Void();
}

Return<void> RadioImpl::setBarringPassword(int32_t serial, const hidl_string& facility,
                                           const hidl_string& oldPassword,
                                           const hidl_string& newPassword) {
    RLOGD("setBarringPassword: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_BARRING_PASSWORD,
            3, facility.c_str(), oldPassword.c_str(), newPassword.c_str());
    return Void();
}

Return<void> RadioImpl::getNetworkSelectionMode(int32_t serial) {
    RLOGD("getNetworkSelectionMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE);
    return Void();
}

Return<void> RadioImpl::setNetworkSelectionModeAutomatic(int32_t serial) {
    RLOGD("setNetworkSelectionModeAutomatic: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC);
    return Void();
}

Return<void> RadioImpl::setNetworkSelectionModeManual(int32_t serial,
                                                      const hidl_string& operatorNumeric) {
    RLOGD("setNetworkSelectionModeManual: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL,
            operatorNumeric.c_str());
    return Void();
}

Return<void> RadioImpl::setNetworkSelectionModeManualWithAct(int32_t serial,
                                                      const hidl_string& operatorNumeric,
                                                      const hidl_string& act,
                                                      const hidl_string& mode) {
    RLOGD("setNetworkSelectionModeManualWithAct: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL_WITH_ACT, 3,
            operatorNumeric.c_str(), act.c_str(), mode.c_str());
    return Void();
}

Return<void> RadioImpl::getAvailableNetworks(int32_t serial) {
    RLOGD("getAvailableNetworks: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_AVAILABLE_NETWORKS);
    return Void();
}

Return<void> RadioImpl::getAvailableNetworksWithAct(int32_t serial) {
    RLOGD("getAvailableNetworksWithAct: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_AVAILABLE_NETWORKS_WITH_ACT);
    return Void();
}

Return<void> RadioImpl::cancelAvailableNetworks(int32_t serial) {
    RLOGD("cancelAvailableNetworks: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_ABORT_QUERY_AVAILABLE_NETWORKS);
    return Void();
}

Return<void> RadioImpl::startNetworkScan(int32_t serial, const AOSP_V1_1::NetworkScanRequest& request) {
    RLOGD("startNetworkScan: serial %d", serial);

    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_START_NETWORK_SCAN);
    if (pRI == NULL) {
        return Void();
    }

    if (request.specifiers.size() > MAX_RADIO_ACCESS_NETWORKS) {
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return Void();
    }

    RIL_NetworkScanRequest scan_request = {};

    scan_request.type = (RIL_ScanType) request.type;
    scan_request.interval = request.interval;
    scan_request.specifiers_length = request.specifiers.size();
    for (size_t i = 0; i < request.specifiers.size(); ++i) {
        if (request.specifiers[i].geranBands.size() > MAX_BANDS ||
            request.specifiers[i].utranBands.size() > MAX_BANDS ||
            request.specifiers[i].eutranBands.size() > MAX_BANDS ||
            request.specifiers[i].channels.size() > MAX_CHANNELS) {
            sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            return Void();
        }
        const AOSP_V1_1::RadioAccessSpecifier& ras_from =
                request.specifiers[i];
        RIL_RadioAccessSpecifier& ras_to = scan_request.specifiers[i];

        ras_to.radio_access_network = (RIL_RadioAccessNetworks) ras_from.radioAccessNetwork;
        ras_to.channels_length = ras_from.channels.size();

        std::copy(ras_from.channels.begin(), ras_from.channels.end(), ras_to.channels);
        const std::vector<uint32_t> * bands = nullptr;
        switch (request.specifiers[i].radioAccessNetwork) {
            case AOSP_V1_1::RadioAccessNetworks::GERAN:
                ras_to.bands_length = ras_from.geranBands.size();
                bands = (std::vector<uint32_t> *) &ras_from.geranBands;
                break;
            case AOSP_V1_1::RadioAccessNetworks::UTRAN:
                ras_to.bands_length = ras_from.utranBands.size();
                bands = (std::vector<uint32_t> *) &ras_from.utranBands;
                break;
            case AOSP_V1_1::RadioAccessNetworks::EUTRAN:
                ras_to.bands_length = ras_from.eutranBands.size();
                bands = (std::vector<uint32_t> *) &ras_from.eutranBands;
                break;
            default:
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                return Void();
        }
        // safe to copy to geran_bands because it's a union member
        for (size_t idx = 0; idx < ras_to.bands_length; ++idx) {
            ras_to.bands.geran_bands[idx] = (RIL_GeranBands) (*bands)[idx];
        }
    }

    CALL_ONREQUEST(RIL_REQUEST_START_NETWORK_SCAN, &scan_request, sizeof(scan_request), pRI,
            mSlotId);

    return Void();
}

Return<void> RadioImpl::stopNetworkScan(int32_t serial) {
    RLOGD("stopNetworkScan: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_STOP_NETWORK_SCAN);
    return Void();
}

Return<void> RadioImpl::startDtmf(int32_t serial, const hidl_string& s) {
    RLOGD("startDtmf: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_DTMF_START,
            s.c_str());
    return Void();
}

Return<void> RadioImpl::stopDtmf(int32_t serial) {
    RLOGD("stopDtmf: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DTMF_STOP);
    return Void();
}

Return<void> RadioImpl::getBasebandVersion(int32_t serial) {
    RLOGD("getBasebandVersion: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_BASEBAND_VERSION);
    return Void();
}

Return<void> RadioImpl::separateConnection(int32_t serial, int32_t gsmIndex) {
    RLOGD("separateConnection: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SEPARATE_CONNECTION, 1, gsmIndex);
    return Void();
}

Return<void> RadioImpl::setMute(int32_t serial, bool enable) {
    RLOGD("setMute: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_MUTE, 1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::getMute(int32_t serial) {
    RLOGD("getMute: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_MUTE);
    return Void();
}

Return<void> RadioImpl::setBarringPasswordCheckedByNW(int32_t serial, const hidl_string& facility,
                                           const hidl_string& oldPassword,
                                           const hidl_string& newPassword,
                                           const hidl_string& cfmPassword) {
    RLOGD("setBarringPasswordCheckedByNW: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_BARRING_PASSWORD,
            4, facility.c_str(), oldPassword.c_str(), newPassword.c_str(), cfmPassword.c_str());
    return Void();
}

Return<void> RadioImpl::getClip(int32_t serial) {
    RLOGD("getClip: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_CLIP);
    return Void();
}

Return<void> RadioImpl::setClip(int32_t serial, int32_t clipEnable) {
    RLOGD("setClip: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CLIP, 1, clipEnable);
    return Void();
}

Return<void> RadioImpl::getColp(int32_t serial) {
    RLOGD("getColp: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_COLP);
    return Void();
}

Return<void> RadioImpl::getColr(int32_t serial) {
    RLOGD("getColr: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_COLR);
    return Void();
}

Return<void> RadioImpl::sendCnap(int32_t serial, const hidl_string& cnapMessage) {
    RLOGD("sendCnap: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SEND_CNAP, cnapMessage.c_str());
    return Void();
}

Return<void> RadioImpl::getDataCallList(int32_t serial) {
    RLOGD("getDataCallList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DATA_CALL_LIST);
    return Void();
}

Return<void> RadioImpl::setSuppServiceNotifications(int32_t serial, bool enable) {
    RLOGD("setSuppServiceNotifications: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION, 1,
            BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::writeSmsToSim(int32_t serial, const SmsWriteArgs& smsWriteArgs) {
    RLOGD("writeSmsToSim: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_WRITE_SMS_TO_SIM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SMS_WriteArgs args;
    args.status = (int) smsWriteArgs.status;

    if (!copyHidlStringToRil(&args.pdu, smsWriteArgs.pdu, pRI)) {
        return Void();
    }

    if (!copyHidlStringToRil(&args.smsc, smsWriteArgs.smsc, pRI)) {
        memsetAndFreeStrings(1, args.pdu);
        return Void();
    }

    CALL_ONREQUEST(RIL_REQUEST_WRITE_SMS_TO_SIM, &args, sizeof(args), pRI, mSlotId);

    memsetAndFreeStrings(2, args.smsc, args.pdu);

    return Void();
}

Return<void> RadioImpl::deleteSmsOnSim(int32_t serial, int32_t index) {
    RLOGD("deleteSmsOnSim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_DELETE_SMS_ON_SIM, 1, index);
    return Void();
}

Return<void> RadioImpl::setBandMode(int32_t serial, RadioBandMode mode) {
    RLOGD("setBandMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_BAND_MODE, 1, mode);
    return Void();
}

Return<void> RadioImpl::getAvailableBandModes(int32_t serial) {
    RLOGD("getAvailableBandModes: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE);
    return Void();
}

Return<void> RadioImpl::sendEnvelope(int32_t serial, const hidl_string& command) {
    RLOGD("sendEnvelope: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND,
            command.c_str());
    return Void();
}

Return<void> RadioImpl::sendTerminalResponseToSim(int32_t serial,
                                                  const hidl_string& commandResponse) {
    RLOGD("sendTerminalResponseToSim: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE,
            commandResponse.c_str());
    return Void();
}

Return<void> RadioImpl::handleStkCallSetupRequestFromSim(int32_t serial, bool accept) {
    RLOGD("handleStkCallSetupRequestFromSim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM,
            1, BOOL_TO_INT(accept));
    return Void();
}

Return<void> RadioImpl::handleStkCallSetupRequestFromSimWithResCode(int32_t serial,
        int32_t resultCode) {
    RLOGD("handleStkCallSetupRequestFromSimwithResCode: serial %d", serial);
    dispatchInts(serial, mSlotId,
            RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM_WITH_RESULT_CODE,
            1, resultCode);
    return Void();
}

Return<void> RadioImpl::explicitCallTransfer(int32_t serial) {
    RLOGD("explicitCallTransfer: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_EXPLICIT_CALL_TRANSFER);
    return Void();
}

Return<void> RadioImpl::setPreferredNetworkType(int32_t serial, PreferredNetworkType nwType) {
    RLOGD("setPreferredNetworkType: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE, 1, nwType);
    return Void();
}

Return<void> RadioImpl::getPreferredNetworkType(int32_t serial) {
    RLOGD("getPreferredNetworkType: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE);
    return Void();
}

Return<void> RadioImpl::getNeighboringCids(int32_t serial) {
    RLOGD("getNeighboringCids: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_NEIGHBORING_CELL_IDS);
    return Void();
}

Return<void> RadioImpl::setLocationUpdates(int32_t serial, bool enable) {
    RLOGD("setLocationUpdates: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_LOCATION_UPDATES, 1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::setCdmaSubscriptionSource(int32_t serial, CdmaSubscriptionSource cdmaSub) {
    RLOGD("setCdmaSubscriptionSource: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE, 1, cdmaSub);
    return Void();
}

Return<void> RadioImpl::setCdmaRoamingPreference(int32_t serial, CdmaRoamingType type) {
    RLOGD("setCdmaRoamingPreference: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE, 1, type);
    return Void();
}

Return<void> RadioImpl::getCdmaRoamingPreference(int32_t serial) {
    RLOGD("getCdmaRoamingPreference: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE);
    return Void();
}

Return<void> RadioImpl::setTTYMode(int32_t serial, TtyMode mode) {
    RLOGD("setTTYMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_TTY_MODE, 1, mode);
    return Void();
}

Return<void> RadioImpl::getTTYMode(int32_t serial) {
    RLOGD("getTTYMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_TTY_MODE);
    return Void();
}

Return<void> RadioImpl::setPreferredVoicePrivacy(int32_t serial, bool enable) {
    RLOGD("setPreferredVoicePrivacy: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE,
            1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::getPreferredVoicePrivacy(int32_t serial) {
    RLOGD("getPreferredVoicePrivacy: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE);
    return Void();
}

Return<void> RadioImpl::sendCDMAFeatureCode(int32_t serial, const hidl_string& featureCode) {
    RLOGD("sendCDMAFeatureCode: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_CDMA_FLASH,
            featureCode.c_str());
    return Void();
}

Return<void> RadioImpl::sendBurstDtmf(int32_t serial, const hidl_string& dtmf, int32_t on,
                                      int32_t off) {
    RLOGD("sendBurstDtmf: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CDMA_BURST_DTMF,
            3, dtmf.c_str(), (std::to_string(on)).c_str(),
            (const char *) (std::to_string(off)).c_str());
    return Void();
}

void constructCdmaSms(RIL_CDMA_SMS_Message &rcsm, const CdmaSmsMessage& sms) {
    rcsm.uTeleserviceID = sms.teleserviceId;
    rcsm.bIsServicePresent = BOOL_TO_INT(sms.isServicePresent);
    rcsm.uServicecategory = sms.serviceCategory;
    rcsm.sAddress.digit_mode = (RIL_CDMA_SMS_DigitMode) sms.address.digitMode;
    rcsm.sAddress.number_mode = (RIL_CDMA_SMS_NumberMode) sms.address.numberMode;
    rcsm.sAddress.number_type = (RIL_CDMA_SMS_NumberType) sms.address.numberType;
    rcsm.sAddress.number_plan = (RIL_CDMA_SMS_NumberPlan) sms.address.numberPlan;

    rcsm.sAddress.number_of_digits = sms.address.digits.size();
    int digitLimit= MIN((rcsm.sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.sAddress.digits[i] = sms.address.digits[i];
    }

    rcsm.sSubAddress.subaddressType = (RIL_CDMA_SMS_SubaddressType) sms.subAddress.subaddressType;
    rcsm.sSubAddress.odd = BOOL_TO_INT(sms.subAddress.odd);

    rcsm.sSubAddress.number_of_digits = sms.subAddress.digits.size();
    digitLimit= MIN((rcsm.sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.sSubAddress.digits[i] = sms.subAddress.digits[i];
    }

    rcsm.uBearerDataLen = sms.bearerData.size();
    digitLimit= MIN((rcsm.uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.aBearerData[i] = sms.bearerData[i];
    }
}

Return<void> RadioImpl::sendCdmaSms(int32_t serial, const CdmaSmsMessage& sms) {
    RLOGD("sendCdmaSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CDMA_SEND_SMS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_Message rcsm = {};
    constructCdmaSms(rcsm, sms);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &rcsm, sizeof(rcsm), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::acknowledgeLastIncomingCdmaSms(int32_t serial, const CdmaSmsAck& smsAck) {
    RLOGD("acknowledgeLastIncomingCdmaSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_Ack rcsa = {};

    rcsa.uErrorClass = (RIL_CDMA_SMS_ErrorClass) smsAck.errorClass;
    rcsa.uSMSCauseCode = smsAck.smsCauseCode;

    CALL_ONREQUEST(pRI->pCI->requestNumber, &rcsa, sizeof(rcsa), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::getGsmBroadcastConfig(int32_t serial) {
    RLOGD("getGsmBroadcastConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG);
    return Void();
}

Return<void> RadioImpl::setGsmBroadcastConfig(int32_t serial,
                                              const hidl_vec<GsmBroadcastSmsConfigInfo>&
                                              configInfo) {
    RLOGD("setGsmBroadcastConfig: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG);
    if (pRI == NULL) {
        return Void();
    }

    int num = configInfo.size();
    RIL_GSM_BroadcastSmsConfigInfo gsmBci[num];
    RIL_GSM_BroadcastSmsConfigInfo *gsmBciPtrs[num];

    for (int i = 0 ; i < num ; i++ ) {
        gsmBciPtrs[i] = &gsmBci[i];
        gsmBci[i].fromServiceId = configInfo[i].fromServiceId;
        gsmBci[i].toServiceId = configInfo[i].toServiceId;
        gsmBci[i].fromCodeScheme = configInfo[i].fromCodeScheme;
        gsmBci[i].toCodeScheme = configInfo[i].toCodeScheme;
        gsmBci[i].selected = BOOL_TO_INT(configInfo[i].selected);
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, gsmBciPtrs,
            num * sizeof(RIL_GSM_BroadcastSmsConfigInfo *), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::setGsmBroadcastActivation(int32_t serial, bool activate) {
    RLOGD("setGsmBroadcastActivation: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION,
            1, BOOL_TO_INT(!activate));
    return Void();
}

Return<void> RadioImpl::getCdmaBroadcastConfig(int32_t serial) {
    RLOGD("getCdmaBroadcastConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG);
    return Void();
}

Return<void> RadioImpl::setCdmaBroadcastConfig(int32_t serial,
                                               const hidl_vec<CdmaBroadcastSmsConfigInfo>&
                                               configInfo) {
    RLOGD("setCdmaBroadcastConfig: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG);
    if (pRI == NULL) {
        return Void();
    }

    int num = configInfo.size();
    RIL_CDMA_BroadcastSmsConfigInfo cdmaBci[num];
    RIL_CDMA_BroadcastSmsConfigInfo *cdmaBciPtrs[num];

    for (int i = 0 ; i < num ; i++ ) {
        cdmaBciPtrs[i] = &cdmaBci[i];
        cdmaBci[i].service_category = configInfo[i].serviceCategory;
        cdmaBci[i].language = configInfo[i].language;
        cdmaBci[i].selected = BOOL_TO_INT(configInfo[i].selected);
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, cdmaBciPtrs,
            num * sizeof(RIL_CDMA_BroadcastSmsConfigInfo *), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::setCdmaBroadcastActivation(int32_t serial, bool activate) {
    RLOGD("setCdmaBroadcastActivation: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION,
            1, BOOL_TO_INT(!activate));
    return Void();
}

Return<void> RadioImpl::getCDMASubscription(int32_t serial) {
    RLOGD("getCDMASubscription: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_SUBSCRIPTION);
    return Void();
}

Return<void> RadioImpl::writeSmsToRuim(int32_t serial, const CdmaSmsWriteArgs& cdmaSms) {
    RLOGD("writeSmsToRuim: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_WriteArgs rcsw = {};
    rcsw.status = (int) cdmaSms.status;
    constructCdmaSms(rcsw.message, cdmaSms.message);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &rcsw, sizeof(rcsw), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::deleteSmsOnRuim(int32_t serial, int32_t index) {
    RLOGD("deleteSmsOnRuim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM, 1, index);
    return Void();
}

Return<void> RadioImpl::getDeviceIdentity(int32_t serial) {
    RLOGD("getDeviceIdentity: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DEVICE_IDENTITY);
    return Void();
}

Return<void> RadioImpl::exitEmergencyCallbackMode(int32_t serial) {
    RLOGD("exitEmergencyCallbackMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE);
    return Void();
}

Return<void> RadioImpl::getSmscAddress(int32_t serial) {
    RLOGD("getSmscAddress: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SMSC_ADDRESS);
    return Void();
}

Return<void> RadioImpl::setSmscAddress(int32_t serial, const hidl_string& smsc) {
    RLOGD("setSmscAddress: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SET_SMSC_ADDRESS,
            smsc.c_str());
    return Void();
}

Return<void> RadioImpl::reportSmsMemoryStatus(int32_t serial, bool available) {
    RLOGD("reportSmsMemoryStatus: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_REPORT_SMS_MEMORY_STATUS, 1,
            BOOL_TO_INT(available));
    return Void();
}

Return<void> RadioImpl::reportStkServiceIsRunning(int32_t serial) {
    RLOGD("reportStkServiceIsRunning: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING);
    return Void();
}

Return<void> RadioImpl::getCdmaSubscriptionSource(int32_t serial) {
    RLOGD("getCdmaSubscriptionSource: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE);
    return Void();
}

Return<void> RadioImpl::requestIsimAuthentication(int32_t serial, const hidl_string& challenge) {
    RLOGD("requestIsimAuthentication: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_ISIM_AUTHENTICATION,
            challenge.c_str());
    return Void();
}

Return<void> RadioImpl::acknowledgeIncomingGsmSmsWithPdu(int32_t serial, bool success,
                                                         const hidl_string& ackPdu) {
    RLOGD("acknowledgeIncomingGsmSmsWithPdu: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU,
            2, success ? "1" : "0", ackPdu.c_str());
    return Void();
}

Return<void> RadioImpl::sendEnvelopeWithStatus(int32_t serial, const hidl_string& contents) {
    RLOGD("sendEnvelopeWithStatus: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS,
            contents.c_str());
    return Void();
}

Return<void> RadioImpl::getVoiceRadioTechnology(int32_t serial) {
    RLOGD("getVoiceRadioTechnology: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_VOICE_RADIO_TECH);
    return Void();
}

Return<void> RadioImpl::getCellInfoList(int32_t serial) {
    RLOGD("getCellInfoList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CELL_INFO_LIST);
    return Void();
}

Return<void> RadioImpl::setCellInfoListRate(int32_t serial, int32_t rate) {
    RLOGD("setCellInfoListRate: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE, 1, rate);
    return Void();
}

Return<void> RadioImpl::setInitialAttachApn(int32_t serial, const DataProfileInfo& dataProfileInfo,
                                            bool modemCognitive, bool isRoaming) {
    RLOGD("setInitialAttachApn: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_INITIAL_ATTACH_APN);
    if (pRI == NULL) {
        return Void();
    }

    if (s_vendorFunctions->version <= 14) {
        RIL_InitialAttachApn iaa = {};

        if (dataProfileInfo.apn.size() == 0) {
            iaa.apn = (char *) calloc(1, sizeof(char));
            if (iaa.apn == NULL) {
                RLOGE("Memory allocation failed for request %s",
                        requestToString(pRI->pCI->requestNumber));
                sendErrorResponse(pRI, RIL_E_NO_MEMORY);
                return Void();
            }
            iaa.apn[0] = '\0';
        } else {
            if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
                return Void();
            }
        }

        const hidl_string &protocol =
                (isRoaming ? dataProfileInfo.roamingProtocol : dataProfileInfo.protocol);

        if (!copyHidlStringToRil(&iaa.protocol, protocol, pRI)) {
            memsetAndFreeStrings(1, iaa.apn);
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            memsetAndFreeStrings(2, iaa.apn, iaa.protocol);
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            memsetAndFreeStrings(3, iaa.apn, iaa.protocol, iaa.username);
            return Void();
        }

        CALL_ONREQUEST(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI, mSlotId);

        memsetAndFreeStrings(4, iaa.apn, iaa.protocol, iaa.username, iaa.password);
    } else {
        RIL_InitialAttachApn_v15 iaa = {};

        if (dataProfileInfo.apn.size() == 0) {
            iaa.apn = (char *) calloc(1, sizeof(char));
            if (iaa.apn == NULL) {
                RLOGE("Memory allocation failed for request %s",
                        requestToString(pRI->pCI->requestNumber));
                sendErrorResponse(pRI, RIL_E_NO_MEMORY);
                return Void();
            }
            iaa.apn[0] = '\0';
        } else {
            if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
                return Void();
            }
        }

        if (!copyHidlStringToRil(&iaa.protocol, dataProfileInfo.protocol, pRI)) {
            memsetAndFreeStrings(1, iaa.apn);
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.roamingProtocol, dataProfileInfo.roamingProtocol, pRI)) {
            memsetAndFreeStrings(2, iaa.apn, iaa.protocol);
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            memsetAndFreeStrings(3, iaa.apn, iaa.protocol, iaa.roamingProtocol);
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            memsetAndFreeStrings(4, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username);
            return Void();
        }
        iaa.supportedTypesBitmask = dataProfileInfo.supportedApnTypesBitmap;
        iaa.bearerBitmask = dataProfileInfo.bearerBitmap;
        iaa.modemCognitive = BOOL_TO_INT(modemCognitive);
        iaa.mtu = dataProfileInfo.mtu;

        if (!convertMvnoTypeToString(dataProfileInfo.mvnoType, iaa.mvnoType)) {
            sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            memsetAndFreeStrings(5, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username,
                    iaa.password);
            return Void();
        }

        if (!copyHidlStringToRil(&iaa.mvnoMatchData, dataProfileInfo.mvnoMatchData, pRI)) {
            memsetAndFreeStrings(5, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username,
                    iaa.password);
            return Void();
        }

        CALL_ONREQUEST(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI, mSlotId);

        memsetAndFreeStrings(6, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username,
                iaa.password, iaa.mvnoMatchData);
    }

    return Void();
}

Return<void> RadioImpl::setInitialAttachApnEx(int32_t serial,
        const DataProfileInfo& dataProfileInfo, bool modemCognitive, bool isRoaming,
        bool canHandleIms) {
    RLOGD("setInitialAttachApnEx: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_INITIAL_ATTACH_APN);
    if (pRI == NULL) {
        return Void();
    }

    if (s_vendorFunctions->version <= 14) {
        RIL_InitialAttachApn iaa = {};

        if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
            return Void();
        }

        const hidl_string &protocol =
                (isRoaming ? dataProfileInfo.roamingProtocol : dataProfileInfo.protocol);

        if (!copyHidlStringToRil(&iaa.protocol, protocol, pRI)) {
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            return Void();
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI,
                pRI->socket_id);

        memsetAndFreeStrings(4, iaa.apn, iaa.protocol, iaa.username, iaa.password);
    } else {
        RIL_InitialAttachApn_v15 iaa = {};

        if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.protocol, dataProfileInfo.protocol, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.roamingProtocol, dataProfileInfo.roamingProtocol, pRI)) {
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            return Void();
        }
        iaa.supportedTypesBitmask = dataProfileInfo.supportedApnTypesBitmap;
        iaa.bearerBitmask = dataProfileInfo.bearerBitmap;
        iaa.modemCognitive = BOOL_TO_INT(modemCognitive);
        iaa.mtu = dataProfileInfo.mtu;

        if (!convertMvnoTypeToString(dataProfileInfo.mvnoType, iaa.mvnoType)) {
            sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            return Void();
        }

        if (!copyHidlStringToRil(&iaa.mvnoMatchData, dataProfileInfo.mvnoMatchData, pRI)) {
            return Void();
        }
        iaa.canHandleIms = BOOL_TO_INT(canHandleIms);
        s_vendorFunctions->onRequest(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI,
                pRI->socket_id);

        memsetAndFreeStrings(6, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username,
                iaa.password, iaa.mvnoMatchData);
    }

    return Void();
}

Return<void> RadioImpl::getImsRegistrationState(int32_t serial) {
    RLOGD("getImsRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_IMS_REGISTRATION_STATE);
    return Void();
}

bool dispatchImsGsmSms(const ImsSmsMessage& message, RequestInfo *pRI) {
    RIL_IMS_SMS_Message rism = {};
    char **pStrings;
    int countStrings = 2;
    int dataLen = sizeof(char *) * countStrings;

    rism.tech = RADIO_TECH_3GPP;
    rism.retry = BOOL_TO_INT(message.retry);
    rism.messageRef = message.messageRef;

    if (message.gsmMessage.size() != 1) {
        RLOGE("dispatchImsGsmSms: Invalid len %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return false;
    }

    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("dispatchImsGsmSms: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }

    if (!copyHidlStringToRil(&pStrings[0], message.gsmMessage[0].smscPdu, pRI)) {
#ifdef MEMSET_FREED
        memset(pStrings, 0, dataLen);
#endif
        free(pStrings);
        return false;
    }

    if (!copyHidlStringToRil(&pStrings[1], message.gsmMessage[0].pdu, pRI)) {
        memsetAndFreeStrings(1, pStrings[0]);
#ifdef MEMSET_FREED
        memset(pStrings, 0, dataLen);
#endif
        free(pStrings);
        return false;
    }

    rism.message.gsmMessage = pStrings;
    CALL_ONREQUEST(pRI->pCI->requestNumber, &rism, sizeof(RIL_RadioTechnologyFamily) +
            sizeof(uint8_t) + sizeof(int32_t) + dataLen, pRI, pRI->socket_id);

    for (int i = 0 ; i < countStrings ; i++) {
        memsetAndFreeStrings(1, pStrings[i]);
    }

#ifdef MEMSET_FREED
    memset(pStrings, 0, dataLen);
#endif
    free(pStrings);

    return true;
}

struct ImsCdmaSms {
    RIL_IMS_SMS_Message imsSms;
    RIL_CDMA_SMS_Message cdmaSms;
};

bool dispatchImsCdmaSms(const ImsSmsMessage& message, RequestInfo *pRI) {
    ImsCdmaSms temp = {};

    if (message.cdmaMessage.size() != 1) {
        RLOGE("dispatchImsCdmaSms: Invalid len %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return false;
    }

    temp.imsSms.tech = RADIO_TECH_3GPP2;
    temp.imsSms.retry = BOOL_TO_INT(message.retry);
    temp.imsSms.messageRef = message.messageRef;
    temp.imsSms.message.cdmaMessage = &temp.cdmaSms;

    constructCdmaSms(temp.cdmaSms, message.cdmaMessage[0]);

    // Vendor code expects payload length to include actual msg payload
    // (sizeof(RIL_CDMA_SMS_Message)) instead of (RIL_CDMA_SMS_Message *) + size of other fields in
    // RIL_IMS_SMS_Message
    int payloadLen = sizeof(RIL_RadioTechnologyFamily) + sizeof(uint8_t) + sizeof(int32_t)
            + sizeof(RIL_CDMA_SMS_Message);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &temp.imsSms, payloadLen, pRI, pRI->socket_id);

    return true;
}

Return<void> RadioImpl::sendImsSms(int32_t serial, const ImsSmsMessage& message) {
    RLOGD("sendImsSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_IMS_SEND_SMS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_RadioTechnologyFamily format = (RIL_RadioTechnologyFamily) message.tech;

    if (RADIO_TECH_3GPP == format) {
        dispatchImsGsmSms(message, pRI);
    } else if (RADIO_TECH_3GPP2 == format) {
        dispatchImsCdmaSms(message, pRI);
    } else {
        RLOGE("sendImsSms: Invalid radio tech %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
    }
    return Void();
}

Return<void> RadioImpl::iccTransmitApduBasicChannel(int32_t serial, const SimApdu& message) {
    RLOGD("iccTransmitApduBasicChannel: serial %d", serial);
    dispatchIccApdu(serial, mSlotId, RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC, message);
    return Void();
}

Return<void> RadioImpl::iccOpenLogicalChannel(int32_t serial, const hidl_string& aid, int32_t p2) {
    RLOGD("iccOpenLogicalChannel: serial %d", serial);
    if (s_vendorFunctions->version < 15) {
        dispatchString(serial, mSlotId, RIL_REQUEST_SIM_OPEN_CHANNEL, aid.c_str());
    } else {
        RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SIM_OPEN_CHANNEL);
        if (pRI == NULL) {
            return Void();
        }

        RIL_OpenChannelParams params = {};

        params.p2 = p2;

        if (!copyHidlStringToRil(&params.aidPtr, aid, pRI)) {
            return Void();
        }

        CALL_ONREQUEST(pRI->pCI->requestNumber, &params, sizeof(params), pRI, mSlotId);

        memsetAndFreeStrings(1, params.aidPtr);
    }
    return Void();
}

Return<void> RadioImpl::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {
    RLOGD("iccCloseLogicalChannel: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SIM_CLOSE_CHANNEL, 1, channelId);
    return Void();
}

Return<void> RadioImpl::iccTransmitApduLogicalChannel(int32_t serial, const SimApdu& message) {
    RLOGD("iccTransmitApduLogicalChannel: serial %d", serial);
    dispatchIccApdu(serial, mSlotId, RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL, message);
    return Void();
}

Return<void> RadioImpl::nvReadItem(int32_t serial, NvItem itemId) {
    RLOGD("nvReadItem: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_NV_READ_ITEM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_NV_ReadItem nvri = {};
    nvri.itemID = (RIL_NV_Item) itemId;

    CALL_ONREQUEST(pRI->pCI->requestNumber, &nvri, sizeof(nvri), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::nvWriteItem(int32_t serial, const NvWriteItem& item) {
    RLOGD("nvWriteItem: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_NV_WRITE_ITEM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_NV_WriteItem nvwi = {};

    nvwi.itemID = (RIL_NV_Item) item.itemId;

    if (!copyHidlStringToRil(&nvwi.value, item.value, pRI)) {
        return Void();
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, &nvwi, sizeof(nvwi), pRI, mSlotId);

    memsetAndFreeStrings(1, nvwi.value);
    return Void();
}

Return<void> RadioImpl::nvWriteCdmaPrl(int32_t serial, const hidl_vec<uint8_t>& prl) {
    RLOGD("nvWriteCdmaPrl: serial %d", serial);
    dispatchRaw(serial, mSlotId, RIL_REQUEST_NV_WRITE_CDMA_PRL, prl);
    return Void();
}

Return<void> RadioImpl::nvResetConfig(int32_t serial, ResetNvType resetType) {
    int rilResetType = -1;
    RLOGD("nvResetConfig: serial %d", serial);
    /* Convert ResetNvType to RIL.h values
     * RIL_REQUEST_NV_RESET_CONFIG
     * 1 - reload all NV items
     * 2 - erase NV reset (SCRTN)
     * 3 - factory reset (RTN)
     */
    switch(resetType) {
      case ResetNvType::RELOAD:
        rilResetType = 1;
        break;
      case ResetNvType::ERASE:
        rilResetType = 2;
        break;
      case ResetNvType::FACTORY_RESET:
        rilResetType = 3;
        break;
    }
    dispatchInts(serial, mSlotId, RIL_REQUEST_NV_RESET_CONFIG, 1, (int) resetType);
    return Void();
}

Return<void> RadioImpl::setUiccSubscription(int32_t serial, const SelectUiccSub& uiccSub) {
    RLOGD("setUiccSubscription: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_UICC_SUBSCRIPTION);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SelectUiccSub rilUiccSub = {};

    rilUiccSub.slot = uiccSub.slot;
    rilUiccSub.app_index = uiccSub.appIndex;
    rilUiccSub.sub_type = (RIL_SubscriptionType) uiccSub.subType;
    rilUiccSub.act_status = (RIL_UiccSubActStatus) uiccSub.actStatus;

    CALL_ONREQUEST(pRI->pCI->requestNumber, &rilUiccSub, sizeof(rilUiccSub), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::setDataAllowed(int32_t serial, bool allow) {
    RLOGD("setDataAllowed: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_ALLOW_DATA, 1, BOOL_TO_INT(allow));
    return Void();
}

Return<void> RadioImpl::getHardwareConfig(int32_t serial) {
    RLOGD("getHardwareConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_HARDWARE_CONFIG);
    return Void();
}

Return<void> RadioImpl::requestIccSimAuthentication(int32_t serial, int32_t authContext,
        const hidl_string& authData, const hidl_string& aid) {
    RLOGD("requestIccSimAuthentication: serial %d, %d", serial, authContext);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SIM_AUTHENTICATION);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SimAuthentication pf = {};

    pf.authContext = authContext;

    if (!copyHidlStringToRil(&pf.authData, authData, pRI)) {
        return Void();
    }

    if (!copyHidlStringToRil(&pf.aid, aid, pRI)) {
        memsetAndFreeStrings(1, pf.authData);
        return Void();
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, &pf, sizeof(pf), pRI, mSlotId);

    memsetAndFreeStrings(2, pf.authData, pf.aid);
    return Void();
}

/**
 * @param numProfiles number of data profile
 * @param dataProfiles the pointer to the actual data profiles. The acceptable type is
          RIL_DataProfileInfo or RIL_DataProfileInfo_v15.
 * @param dataProfilePtrs the pointer to the pointers that point to each data profile structure
 * @param numfields number of string-type member in the data profile structure
 * @param ... the variadic parameters are pointers to each string-type member
 **/
template <typename T>
void freeSetDataProfileData(int numProfiles, T *dataProfiles, T **dataProfilePtrs,
                            int numfields, ...) {
    va_list args;
    va_start(args, numfields);

    // Iterate through each string-type field that need to be free.
    for (int i = 0; i < numfields; i++) {
        // Iterate through each data profile and free that specific string-type field.
        // The type 'char *T::*' is a type of pointer to a 'char *' member inside T structure.
        char *T::*ptr = va_arg(args, char *T::*);
        for (int j = 0; j < numProfiles; j++) {
            memsetAndFreeStrings(1, dataProfiles[j].*ptr);
        }
    }

    va_end(args);

#ifdef MEMSET_FREED
    memset(dataProfiles, 0, numProfiles * sizeof(T));
    memset(dataProfilePtrs, 0, numProfiles * sizeof(T *));
#endif
    free(dataProfiles);
    free(dataProfilePtrs);
}

Return<void> RadioImpl::setDataProfile(int32_t serial, const hidl_vec<DataProfileInfo>& profiles,
                                       bool isRoaming) {
    RLOGD("setDataProfile: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SET_DATA_PROFILE);
    if (pRI == NULL) {
        return Void();
    }

    size_t num = profiles.size();
    bool success = false;

    if (s_vendorFunctions->version <= 14) {

        RIL_DataProfileInfo *dataProfiles =
            (RIL_DataProfileInfo *) calloc(num, sizeof(RIL_DataProfileInfo));

        if (dataProfiles == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        RIL_DataProfileInfo **dataProfilePtrs =
            (RIL_DataProfileInfo **) calloc(num, sizeof(RIL_DataProfileInfo *));
        if (dataProfilePtrs == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            free(dataProfiles);
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        for (size_t i = 0; i < num; i++) {
            dataProfilePtrs[i] = &dataProfiles[i];

            success = copyHidlStringToRil(&dataProfiles[i].apn, profiles[i].apn, pRI);

            const hidl_string &protocol =
                    (isRoaming ? profiles[i].roamingProtocol : profiles[i].protocol);

            if (success && !copyHidlStringToRil(&dataProfiles[i].protocol, protocol, pRI)) {
                success = false;
            }

            if (success && !copyHidlStringToRil(&dataProfiles[i].user, profiles[i].user, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].password, profiles[i].password,
                    pRI)) {
                success = false;
            }

            if (!success) {
                freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 4,
                    &RIL_DataProfileInfo::apn, &RIL_DataProfileInfo::protocol,
                    &RIL_DataProfileInfo::user, &RIL_DataProfileInfo::password);
                return Void();
            }

            dataProfiles[i].profileId = (RIL_DataProfile) profiles[i].profileId;
            dataProfiles[i].authType = (int) profiles[i].authType;
            dataProfiles[i].type = (int) profiles[i].type;
            dataProfiles[i].maxConnsTime = profiles[i].maxConnsTime;
            dataProfiles[i].maxConns = profiles[i].maxConns;
            dataProfiles[i].waitTime = profiles[i].waitTime;
            dataProfiles[i].enabled = BOOL_TO_INT(profiles[i].enabled);
        }

        CALL_ONREQUEST(RIL_REQUEST_SET_DATA_PROFILE, dataProfilePtrs,
                num * sizeof(RIL_DataProfileInfo *), pRI, mSlotId);

        freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 4,
                &RIL_DataProfileInfo::apn, &RIL_DataProfileInfo::protocol,
                &RIL_DataProfileInfo::user, &RIL_DataProfileInfo::password);
    } else {
        // M: use data profile to sync apn tables to modem
        // remark AOSP
        //RIL_DataProfileInfo_v15 *dataProfiles =
        //    (RIL_DataProfileInfo_v15 *) calloc(num, sizeof(RIL_DataProfileInfo_v15));
        RIL_MtkDataProfileInfo *dataProfiles =
            (RIL_MtkDataProfileInfo *) calloc(num, sizeof(RIL_MtkDataProfileInfo));

        if (dataProfiles == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        // M: use data profile to sync apn tables to modem
        // remark AOSP
        //RIL_DataProfileInfo_v15 **dataProfilePtrs =
        //    (RIL_DataProfileInfo_v15 **) calloc(num, sizeof(RIL_DataProfileInfo_v15 *));
        RIL_MtkDataProfileInfo **dataProfilePtrs =
            (RIL_MtkDataProfileInfo **) calloc(num, sizeof(RIL_MtkDataProfileInfo *));

        if (dataProfilePtrs == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            free(dataProfiles);
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        for (size_t i = 0; i < num; i++) {
            dataProfilePtrs[i] = &dataProfiles[i];

            success = copyHidlStringToRil(&dataProfiles[i].apn, profiles[i].apn, pRI);
            if (success && !copyHidlStringToRil(&dataProfiles[i].protocol, profiles[i].protocol,
                    pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].roamingProtocol,
                    profiles[i].roamingProtocol, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].user, profiles[i].user, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].password, profiles[i].password,
                    pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].mvnoMatchData,
                    profiles[i].mvnoMatchData, pRI)) {
                success = false;
            }

            if (success && !convertMvnoTypeToString(profiles[i].mvnoType,
                    dataProfiles[i].mvnoType)) {
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                success = false;
            }

            if (!success) {
                // M: use data profile to sync apn tables to modem
                // remark AOSP
                //freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                //    &RIL_DataProfileInfo_v15::apn, &RIL_DataProfileInfo_v15::protocol,
                //    &RIL_DataProfileInfo_v15::roamingProtocol, &RIL_DataProfileInfo_v15::user,
                //    &RIL_DataProfileInfo_v15::password, &RIL_DataProfileInfo_v15::mvnoMatchData);
                freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                    &RIL_MtkDataProfileInfo::apn, &RIL_MtkDataProfileInfo::protocol,
                    &RIL_MtkDataProfileInfo::roamingProtocol, &RIL_MtkDataProfileInfo::user,
                    &RIL_MtkDataProfileInfo::password, &RIL_MtkDataProfileInfo::mvnoMatchData);
                return Void();
            }

            dataProfiles[i].profileId = (RIL_DataProfile) profiles[i].profileId;
            dataProfiles[i].authType = (int) profiles[i].authType;
            dataProfiles[i].type = (int) profiles[i].type;
            dataProfiles[i].maxConnsTime = profiles[i].maxConnsTime;
            dataProfiles[i].maxConns = profiles[i].maxConns;
            dataProfiles[i].waitTime = profiles[i].waitTime;
            dataProfiles[i].enabled = BOOL_TO_INT(profiles[i].enabled);
            dataProfiles[i].supportedTypesBitmask = profiles[i].supportedApnTypesBitmap;
            dataProfiles[i].bearerBitmask = profiles[i].bearerBitmap;
            dataProfiles[i].mtu = profiles[i].mtu;

            // M: use data profile to sync apn tables to modem
            // set default value for inactiveTimer
            dataProfiles[i].inactiveTimer = 0;

        }

        CALL_ONREQUEST(RIL_REQUEST_SET_DATA_PROFILE, dataProfilePtrs,
                num * sizeof(RIL_MtkDataProfileInfo *), pRI, mSlotId);

        // M: use data profile to sync apn tables to modem
        // remark AOSP
        //freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
        //        &RIL_DataProfileInfo_v15::apn, &RIL_DataProfileInfo_v15::protocol,
        //        &RIL_DataProfileInfo_v15::roamingProtocol, &RIL_DataProfileInfo_v15::user,
        //        &RIL_DataProfileInfo_v15::password, &RIL_DataProfileInfo_v15::mvnoMatchData);
        freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                &RIL_MtkDataProfileInfo::apn, &RIL_MtkDataProfileInfo::protocol,
                &RIL_MtkDataProfileInfo::roamingProtocol, &RIL_MtkDataProfileInfo::user,
                &RIL_MtkDataProfileInfo::password, &RIL_MtkDataProfileInfo::mvnoMatchData);
    }

    return Void();
}

Return<void> RadioImpl::setDataProfileEx(int32_t serial, const hidl_vec<MtkDataProfileInfo>& profiles,
                                       bool isRoaming) {
    RLOGD("setDataProfileEx: serial %d, version: %d", serial, s_vendorFunctions->version);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SET_DATA_PROFILE);
    if (pRI == NULL) {
        RLOGD("setDataProfileEx: pRI == NULL");
        return Void();
    }

    size_t num = profiles.size();
    bool success = false;

    if (s_vendorFunctions->version >= 15) {
        RIL_MtkDataProfileInfo *dataProfiles =
            (RIL_MtkDataProfileInfo *) calloc(num, sizeof(RIL_MtkDataProfileInfo));

        if (dataProfiles == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        RIL_MtkDataProfileInfo **dataProfilePtrs =
            (RIL_MtkDataProfileInfo **) calloc(num, sizeof(RIL_MtkDataProfileInfo *));
        if (dataProfilePtrs == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            free(dataProfiles);
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        for (size_t i = 0; i < num; i++) {
            dataProfilePtrs[i] = &dataProfiles[i];

            success = copyHidlStringToRil(&dataProfiles[i].apn, profiles[i].dpi.apn, pRI);
            if (success && !copyHidlStringToRil(&dataProfiles[i].protocol, profiles[i].dpi.protocol,
                    pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].roamingProtocol,
                    profiles[i].dpi.roamingProtocol, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].user, profiles[i].dpi.user, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].password, profiles[i].dpi.password,
                    pRI)) {
                success = false;
            }

            if (success && !copyHidlStringToRil(&dataProfiles[i].mvnoMatchData,
                    profiles[i].dpi.mvnoMatchData, pRI)) {
                success = false;
            }

            if (success && !convertMvnoTypeToString(profiles[i].dpi.mvnoType,
                    dataProfiles[i].mvnoType)) {
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                success = false;
            }

            if (!success) {
                freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                    &RIL_MtkDataProfileInfo::apn, &RIL_MtkDataProfileInfo::protocol,
                    &RIL_MtkDataProfileInfo::roamingProtocol, &RIL_MtkDataProfileInfo::user,
                    &RIL_MtkDataProfileInfo::password, &RIL_MtkDataProfileInfo::mvnoMatchData);
                return Void();
            }

            dataProfiles[i].profileId = (RIL_DataProfile) profiles[i].dpi.profileId;
            dataProfiles[i].authType = (int) profiles[i].dpi.authType;
            dataProfiles[i].type = (int) profiles[i].dpi.type;
            dataProfiles[i].maxConnsTime = profiles[i].dpi.maxConnsTime;
            dataProfiles[i].maxConns = profiles[i].dpi.maxConns;
            dataProfiles[i].waitTime = profiles[i].dpi.waitTime;
            dataProfiles[i].enabled = BOOL_TO_INT(profiles[i].dpi.enabled);
            dataProfiles[i].supportedTypesBitmask = profiles[i].dpi.supportedApnTypesBitmap;
            dataProfiles[i].bearerBitmask = profiles[i].dpi.bearerBitmap;
            dataProfiles[i].mtu = profiles[i].dpi.mtu;
            dataProfiles[i].inactiveTimer = profiles[i].inactiveTimer;
            RLOGD("setDataProfileEx: inactiveTimer: %d", dataProfiles[i].inactiveTimer);
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_DATA_PROFILE, dataProfilePtrs,
                num * sizeof(RIL_MtkDataProfileInfo *), pRI, pRI->socket_id);
        freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                &RIL_MtkDataProfileInfo::apn, &RIL_MtkDataProfileInfo::protocol,
                &RIL_MtkDataProfileInfo::roamingProtocol, &RIL_MtkDataProfileInfo::user,
                &RIL_MtkDataProfileInfo::password, &RIL_MtkDataProfileInfo::mvnoMatchData);
    }
    return Void();
}

Return<void> RadioImpl::requestShutdown(int32_t serial) {
    RLOGD("requestShutdown: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SHUTDOWN);
    return Void();
}

Return<void> RadioImpl::getRadioCapability(int32_t serial) {
    RLOGD("getRadioCapability: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_RADIO_CAPABILITY);
    return Void();
}

Return<void> RadioImpl::setRadioCapability(int32_t serial, const RadioCapability& rc) {
    RLOGD("setRadioCapability: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SET_RADIO_CAPABILITY);
    if (pRI == NULL) {
        return Void();
    }

    RIL_RadioCapability rilRc = {};

    // TODO : set rilRc.version using HIDL version ?
    rilRc.session = rc.session;
    rilRc.phase = (int) rc.phase;
    rilRc.rat = (int) rc.raf;
    rilRc.status = (int) rc.status;
    strncpy(rilRc.logicalModemUuid, rc.logicalModemUuid.c_str(), MAX_UUID_LENGTH);

    CALL_ONREQUEST(pRI->pCI->requestNumber, &rilRc, sizeof(rilRc), pRI, mSlotId);

    return Void();
}

Return<void> RadioImpl::startLceService(int32_t serial, int32_t reportInterval, bool pullMode) {
    RLOGD("startLceService: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_START_LCE, 2, reportInterval,
            BOOL_TO_INT(pullMode));
    return Void();
}

Return<void> RadioImpl::stopLceService(int32_t serial) {
    RLOGD("stopLceService: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_STOP_LCE);
    return Void();
}

Return<void> RadioImpl::pullLceData(int32_t serial) {
    RLOGD("pullLceData: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_PULL_LCEDATA);
    return Void();
}

Return<void> RadioImpl::getModemActivityInfo(int32_t serial) {
    RLOGD("getModemActivityInfo: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_ACTIVITY_INFO);
    return Void();
}

Return<void> RadioImpl::setAllowedCarriers(int32_t serial, bool allAllowed,
                                           const CarrierRestrictions& carriers) {
    RLOGD("setAllowedCarriers: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_CARRIER_RESTRICTIONS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CarrierRestrictions cr = {};
    RIL_Carrier *allowedCarriers = NULL;
    RIL_Carrier *excludedCarriers = NULL;

    cr.len_allowed_carriers = carriers.allowedCarriers.size();
    allowedCarriers = (RIL_Carrier *)calloc(cr.len_allowed_carriers, sizeof(RIL_Carrier));
    if (allowedCarriers == NULL) {
        RLOGE("setAllowedCarriers: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return Void();
    }
    cr.allowed_carriers = allowedCarriers;

    cr.len_excluded_carriers = carriers.excludedCarriers.size();
    excludedCarriers = (RIL_Carrier *)calloc(cr.len_excluded_carriers, sizeof(RIL_Carrier));
    if (excludedCarriers == NULL) {
        RLOGE("setAllowedCarriers: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
#ifdef MEMSET_FREED
        memset(allowedCarriers, 0, cr.len_allowed_carriers * sizeof(RIL_Carrier));
#endif
        free(allowedCarriers);
        return Void();
    }
    cr.excluded_carriers = excludedCarriers;

    for (int i = 0; i < cr.len_allowed_carriers; i++) {
        allowedCarriers[i].mcc = carriers.allowedCarriers[i].mcc.c_str();
        allowedCarriers[i].mnc = carriers.allowedCarriers[i].mnc.c_str();
        allowedCarriers[i].match_type = (RIL_CarrierMatchType) carriers.allowedCarriers[i].matchType;
        allowedCarriers[i].match_data = carriers.allowedCarriers[i].matchData.c_str();
    }

    for (int i = 0; i < cr.len_excluded_carriers; i++) {
        excludedCarriers[i].mcc = carriers.excludedCarriers[i].mcc.c_str();
        excludedCarriers[i].mnc = carriers.excludedCarriers[i].mnc.c_str();
        excludedCarriers[i].match_type =
                (RIL_CarrierMatchType) carriers.excludedCarriers[i].matchType;
        excludedCarriers[i].match_data = carriers.excludedCarriers[i].matchData.c_str();
    }

    CALL_ONREQUEST(pRI->pCI->requestNumber, &cr, sizeof(RIL_CarrierRestrictions), pRI, mSlotId);

#ifdef MEMSET_FREED
    memset(allowedCarriers, 0, cr.len_allowed_carriers * sizeof(RIL_Carrier));
    memset(excludedCarriers, 0, cr.len_excluded_carriers * sizeof(RIL_Carrier));
#endif
    free(allowedCarriers);
    free(excludedCarriers);
    return Void();
}

Return<void> RadioImpl::getAllowedCarriers(int32_t serial) {
    RLOGD("getAllowedCarriers: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_GET_CARRIER_RESTRICTIONS);
    sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
    //dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CARRIER_RESTRICTIONS);
    return Void();
}

Return<void> RadioImpl::sendDeviceState(int32_t serial, DeviceStateType deviceStateType,
                                        bool state) {
    RLOGD("sendDeviceState: serial %d", serial);
    if (s_vendorFunctions->version < 15) {
        if (deviceStateType ==  DeviceStateType::LOW_DATA_EXPECTED) {
            RLOGD("sendDeviceState: calling screen state %d", BOOL_TO_INT(!state));
            dispatchInts(serial, mSlotId, RIL_REQUEST_SCREEN_STATE, 1, BOOL_TO_INT(!state));
        } else {
            RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                    RIL_REQUEST_SEND_DEVICE_STATE);
            sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        }
        return Void();
    }
    dispatchInts(serial, mSlotId, RIL_REQUEST_SEND_DEVICE_STATE, 2, (int) deviceStateType,
            BOOL_TO_INT(state));
    return Void();
}

Return<void> RadioImpl::setIndicationFilter(int32_t serial, int32_t indicationFilter) {
    RLOGD("setIndicationFilter: serial %d", serial);
    if (s_vendorFunctions->version < 15) {
        RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER);
        sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        return Void();
    }
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER, 1, indicationFilter);
    return Void();
}

Return<void> RadioImpl::setSimCardPower(int32_t serial, bool powerUp) {
    RLOGD("setSimCardPower: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SIM_CARD_POWER, 1, BOOL_TO_INT(powerUp));
    return Void();
}

Return<void> RadioImpl::setSimCardPower_1_1(int32_t serial, const AOSP_V1_1::CardPowerState state) {
#if VDBG
    RLOGD("setSimCardPower_1_1: serial %d state %d", serial, state);
#endif
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SIM_CARD_POWER, 1, state);
    return Void();
}

Return<void> RadioImpl::setCarrierInfoForImsiEncryption(int32_t serial,
        const AOSP_V1_1::ImsiEncryptionInfo& data) {
    RLOGD("setCarrierInfoForImsiEncryption: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(
            serial, mSlotId, RIL_REQUEST_SET_CARRIER_INFO_IMSI_ENCRYPTION);
    if (pRI == NULL) {
        RLOGE("setCarrierInfoForImsiEncryption: pRI == NULL");
        return Void();
    }

    RIL_CarrierInfoForImsiEncryption imsiEncryption = {};

    if (!copyHidlStringToRil(&imsiEncryption.mnc, data.mnc, pRI)) {
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return Void();
    }
    if (!copyHidlStringToRil(&imsiEncryption.mcc, data.mcc, pRI)) {
        memsetAndFreeStrings(1, imsiEncryption.mnc);
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return Void();
    }
    if (!copyHidlStringToRil(&imsiEncryption.keyIdentifier, data.keyIdentifier, pRI)) {
        memsetAndFreeStrings(2, imsiEncryption.mnc, imsiEncryption.mcc);
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return Void();
    }
    int32_t lSize = data.carrierKey.size();
    imsiEncryption.carrierKey = new uint8_t[lSize];
    memcpy(imsiEncryption.carrierKey, data.carrierKey.data(), lSize);
    imsiEncryption.expirationTime = data.expirationTime;
    CALL_ONREQUEST(pRI->pCI->requestNumber, &imsiEncryption,
            sizeof(RIL_CarrierInfoForImsiEncryption), pRI, mSlotId);
    delete(imsiEncryption.carrierKey);
    return Void();
}

Return<void> RadioImpl::startKeepalive(int32_t serial, const AOSP_V1_1::KeepaliveRequest& keepalive) {
#if VDBG
    RLOGD("%s(): %d", __FUNCTION__, serial);
#endif
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_START_KEEPALIVE);

    // M @{ we don't support
    if (pRI != NULL) {
        sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
    }
    return Void();
    // M @}

    RIL_KeepaliveRequest kaReq = {};

    kaReq.type = static_cast<RIL_KeepaliveType>(keepalive.type);
    switch(kaReq.type) {
        case NATT_IPV4:
            if (keepalive.sourceAddress.size() != 4 ||
                    keepalive.destinationAddress.size() != 4) {
                RLOGE("Invalid address for keepalive!");
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                return Void();
            }
            break;
        case NATT_IPV6:
            if (keepalive.sourceAddress.size() != 16 ||
                    keepalive.destinationAddress.size() != 16) {
                RLOGE("Invalid address for keepalive!");
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                return Void();
            }
            break;
        default:
            RLOGE("Unknown packet keepalive type!");
            sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            return Void();
    }

    ::memcpy(kaReq.sourceAddress, keepalive.sourceAddress.data(), keepalive.sourceAddress.size());
    kaReq.sourcePort = keepalive.sourcePort;

    ::memcpy(kaReq.destinationAddress,
            keepalive.destinationAddress.data(), keepalive.destinationAddress.size());
    kaReq.destinationPort = keepalive.destinationPort;

    kaReq.maxKeepaliveIntervalMillis = keepalive.maxKeepaliveIntervalMillis;
    kaReq.cid = keepalive.cid; // This is the context ID of the data call

    CALL_ONREQUEST(pRI->pCI->requestNumber, &kaReq, sizeof(RIL_KeepaliveRequest), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::stopKeepalive(int32_t serial, int32_t sessionHandle) {
#if VDBG
    RLOGD("%s(): %d", __FUNCTION__, serial);
#endif
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_STOP_KEEPALIVE);

    // M @{ we don't support
    if (pRI != NULL) {
        sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
    }
    return Void();
    // M @}

    CALL_ONREQUEST(pRI->pCI->requestNumber, &sessionHandle, sizeof(uint32_t), pRI, mSlotId);
    return Void();
}

Return<void> RadioImpl::responseAcknowledgement() {
    android::releaseWakeLock();
    return Void();
}

Return<void> RadioImpl::setTrm(int32_t serial, int32_t mode) {
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_TRM, 1, mode);
    return Void();
}

// MTK-START: SIM
Return<void> RadioImpl::getATR(int32_t serial) {
    RLOGD("getATR: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SIM_GET_ATR);
    return Void();
}

Return<void> RadioImpl::setSimPower(int32_t serial, int32_t mode) {
    RLOGD("setSimPower: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SIM_POWER, 1, mode);
    return Void();
}
// MTK-END

// MTK-START: NW
// Femtocell feature
Return<void> RadioImpl::getFemtocellList(int32_t serial) {
    RLOGD("getFemtocellList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_FEMTOCELL_LIST);
    return Void();
}

// MTK-START: SIM ME LOCK
Return<void> RadioImpl::queryNetworkLock(int32_t serial, int32_t category) {
    RLOGD("queryNetworkLock: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_QUERY_SIM_NETWORK_LOCK, 1, category);
    return Void();
}

Return<void> RadioImpl::setNetworkLock(int32_t serial, int32_t category,
        int32_t lockop, const ::android::hardware::hidl_string& password,
        const ::android::hardware::hidl_string& data_imsi,
        const ::android::hardware::hidl_string& gid1,
        const ::android::hardware::hidl_string& gid2) {
    RLOGD("setNetworkLock: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_SIM_NETWORK_LOCK,
            6,
            std::to_string((int) category).c_str(),
            std::to_string((int) lockop).c_str(),
            password.c_str(),
            data_imsi.c_str(),
            gid1.c_str(),
            gid2.c_str());
    return Void();
}
// MTK-END

Return<void> RadioImpl::abortFemtocellList(int32_t serial) {
    RLOGD("abortFemtocellList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_ABORT_FEMTOCELL_LIST);
    return Void();
}

Return<void> RadioImpl::selectFemtocell(int32_t serial,
                                                const hidl_string& operatorNumeric,
                                                const hidl_string& act,
                                                const hidl_string& csgId) {
    RLOGD("selectFemtocell: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SELECT_FEMTOCELL, 3,
            operatorNumeric.c_str(), act.c_str(), csgId.c_str());
    return Void();
}

Return<void> RadioImpl::queryFemtoCellSystemSelectionMode(int32_t serial) {
    RLOGD("queryFemtoCellSystemSelectionMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_FEMTOCELL_SYSTEM_SELECTION_MODE);
    return Void();
}

Return<void> RadioImpl::setFemtoCellSystemSelectionMode(int32_t serial, int32_t mode) {
    RLOGD("setFemtoCellSystemSelectionMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_FEMTOCELL_SYSTEM_SELECTION_MODE, 1, mode);
    return Void();
}
// MTK-END: NW

// MTK-START: SIM GBA
bool dispatchSimGeneralAuth(int serial, int slotId, int request, const SimAuthStructure& simAuth) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_SimAuthStructure rilSimAuth;
    rilSimAuth.sessionId= simAuth.sessionId;
    rilSimAuth.mode= simAuth.mode;

    if (!copyHidlStringToRil(&rilSimAuth.param1, simAuth.param1, pRI)) {
        return false;
    }

    if (!copyHidlStringToRil(&rilSimAuth.param2, simAuth.param2, pRI)) {
        return false;
    }

    rilSimAuth.tag = simAuth.tag;

    s_vendorFunctions->onRequest(request, &rilSimAuth, sizeof(rilSimAuth), pRI,
            pRI->socket_id);

    memsetAndFreeStrings(1, rilSimAuth.param1);
    memsetAndFreeStrings(1, rilSimAuth.param2);

    return true;
}

Return<void> RadioImpl::doGeneralSimAuthentication(int32_t serial,
        const SimAuthStructure& simAuth) {
    RLOGD("doGeneralSimAuthentication: serial %d", serial);
    dispatchSimGeneralAuth(serial, mSlotId, RIL_REQUEST_GENERAL_SIM_AUTH, simAuth);
    return Void();
}
// MTK-END

/// M: CC: MTK proprietary call control ([IMS] common flow) @{
Return<void> RadioImpl::hangupAll(int32_t serial) {
    RLOGD("hangupAll: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_HANGUP_ALL);
    return Void();
}

Return<void> RadioImpl::setCallIndication(int32_t serial, int32_t mode, int32_t callId, int32_t seqNumber) {
    RLOGD("setCallIndication: mode %d, callId %d, seqNumber %d", mode, callId, seqNumber);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CALL_INDICATION, 3, mode, callId, seqNumber);
    return Void();
}

Return<void> RadioImpl::emergencyDial(int32_t serial, const Dial& dialInfo) {
    RLOGD("emergencyDial: serial %d", serial);

    int requestId = RIL_REQUEST_EMERGENCY_DIAL;
    if(isImsSlot(mSlotId)) {
        requestId = RIL_REQUEST_IMS_EMERGENCY_DIAL;
    }

    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, requestId);
    if (pRI == NULL) {
        return Void();
    }
    RIL_Dial dial = {};
    RIL_UUS_Info uusInfo = {};
    int32_t sizeOfDial = sizeof(dial);

    if (!copyHidlStringToRil(&dial.address, dialInfo.address, pRI)) {
        return Void();
    }
    dial.clir = (int) dialInfo.clir;

    if (dialInfo.uusInfo.size() != 0) {
        uusInfo.uusType = (RIL_UUS_Type) dialInfo.uusInfo[0].uusType;
        uusInfo.uusDcs = (RIL_UUS_DCS) dialInfo.uusInfo[0].uusDcs;

        if (dialInfo.uusInfo[0].uusData.size() == 0) {
            uusInfo.uusData = NULL;
            uusInfo.uusLength = 0;
        } else {
            if (!copyHidlStringToRil(&uusInfo.uusData, dialInfo.uusInfo[0].uusData, pRI)) {
                memsetAndFreeStrings(1, dial.address);
                return Void();
            }
            uusInfo.uusLength = dialInfo.uusInfo[0].uusData.size();
        }

        dial.uusInfo = &uusInfo;
    }

    s_vendorFunctions->onRequest(requestId,
                                 &dial, sizeOfDial, pRI,pRI->socket_id);

    memsetAndFreeStrings(2, dial.address, uusInfo.uusData);
    return Void();
}

Return<void> RadioImpl::setEccServiceCategory(int32_t serial, int32_t serviceCategory) {
    RLOGD("setEccServiceCategory: serial %d, serviceCategory %d", serial, serviceCategory);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_ECC_SERVICE_CATEGORY, 1, serviceCategory);
    return Void();
}

// M: Set ECC ist to MD
Return<void> RadioImpl::setEccList(int32_t serial, const hidl_string& list1,
        const hidl_string& list2) {
    RLOGD("setEccList: ecclist %s, %s", list1.c_str(), list2.c_str());
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_ECC_LIST, 2,
            list1.c_str(), list2.c_str());
    return Void();
}
/// M: CC: @}

// M: [LTE][Low Power][UL traffic shaping] @{
Return<void> RadioImpl::setLteAccessStratumReport(int32_t serial, int32_t enable) {
    RLOGD("setLteAccessStratumReport: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_LTE_ACCESS_STRATUM_REPORT, 1, enable);
    return Void();
}

Return<void> RadioImpl::setLteUplinkDataTransfer(int32_t serial, int32_t state, int32_t interfaceId) {
    RLOGD("setLteUplinkDataTransfer: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_LTE_UPLINK_DATA_TRANSFER, 2, state, interfaceId);
    return Void();
}
// M: [LTE][Low Power][UL traffic shaping] @}

/// M: CC: For 3G VT only @{
Return<void> RadioImpl::vtDial(int32_t serial, const Dial& dialInfo) {
    RLOGD("vtDial: serial %d", serial);

    int requestId = RIL_REQUEST_VT_DIAL;
    if(isImsSlot(mSlotId)) {
        requestId = RIL_REQUEST_IMS_VT_DIAL;
    }

    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, requestId);
    if (pRI == NULL) {
        return Void();
    }
    RIL_Dial dial = {};
    RIL_UUS_Info uusInfo = {};
    int32_t sizeOfDial = sizeof(dial);

    if (!copyHidlStringToRil(&dial.address, dialInfo.address, pRI)) {
        return Void();
    }
    dial.clir = (int) dialInfo.clir;

    if (dialInfo.uusInfo.size() != 0) {
        uusInfo.uusType = (RIL_UUS_Type) dialInfo.uusInfo[0].uusType;
        uusInfo.uusDcs = (RIL_UUS_DCS) dialInfo.uusInfo[0].uusDcs;

        if (dialInfo.uusInfo[0].uusData.size() == 0) {
            uusInfo.uusData = NULL;
            uusInfo.uusLength = 0;
        } else {
            if (!copyHidlStringToRil(&uusInfo.uusData,
                                     dialInfo.uusInfo[0].uusData, pRI)) {
                memsetAndFreeStrings(1, dial.address);
                return Void();
            }
            uusInfo.uusLength = dialInfo.uusInfo[0].uusData.size();
        }

        dial.uusInfo = &uusInfo;
    }

    s_vendorFunctions->onRequest(requestId,
                                 &dial, sizeOfDial, pRI,pRI->socket_id);

    memsetAndFreeStrings(2, dial.address, uusInfo.uusData);

    return Void();
}

Return<void> RadioImpl::voiceAccept(int32_t serial, int32_t callId) {
    RLOGD("voiceAccept: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_VOICE_ACCEPT, 1, callId);

    return Void();
}

Return<void> RadioImpl::replaceVtCall(int32_t serial, int32_t index) {
    RLOGD("replaceVtCall: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_REPLACE_VT_CALL, 1, index);

    return Void();
}
/// M: CC: @}

/// M: CC: E911 request current status.
Return<void> RadioImpl::currentStatus(int32_t serial, int32_t airplaneMode,
        int32_t imsReg) {
    RLOGD("currentStatus: serial %d, airplaneMode %d, imsReg %d", serial,
            airplaneMode, imsReg);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CURRENT_STATUS, 2, airplaneMode, imsReg);
    return Void();
}

/// M: CC: E911 request set ECC preferred RAT.
Return<void> RadioImpl::eccPreferredRat(int32_t serial, int32_t phoneType) {
    RLOGD("eccPreferredRat: serial %d, phoneType %d", serial, phoneType);
    return Void();
}

// MTK-START: RIL_REQUEST_SWITCH_MODE_FOR_ECC
Return<void> RadioImpl::triggerModeSwitchByEcc(int32_t serial, int32_t mode) {
    dispatchInts(serial, mSlotId, RIL_REQUEST_SWITCH_MODE_FOR_ECC, 1, mode);
    return Void();
}
// MTK-END

Return<void> OemHookImpl::setResponseFunctions(
        const ::android::sp<IOemHookResponse>& oemHookResponseParam,
        const ::android::sp<IOemHookIndication>& oemHookIndicationParam) {
    RLOGD("OemHookImpl::setResponseFunctions-donothing");

    return Void();
}

Return<void> OemHookImpl::sendRequestRaw(int32_t serial, const hidl_vec<uint8_t>& data) {
    RLOGD("OemHookImpl::sendRequestRaw: serial %d, donothing", serial);
    return Void();
}

Return<void> RadioImpl::sendRequestRaw(int32_t serial, const hidl_vec<uint8_t>& data) {
    RLOGD("RadioImpl::sendRequestRaw: serial %d", serial);
    dispatchRaw(serial, mSlotId, RIL_REQUEST_OEM_HOOK_RAW, data);
    return Void();
}

/////////////////////////////////////////////////////////////////////////////////////////
/// [IMS] -------------- IMS Request Function ------------------------------------------
/////////////////////////////////////////////////////////////////////////////////////////
Return<void> RadioImpl::videoCallAccept(int32_t serial,
                                        int32_t videoMode, int32_t callId) {
    RLOGD("videoCallAccept: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_VIDEO_CALL_ACCEPT, 2, videoMode, callId);
    return Void();
}

Return<void> RadioImpl::imsEctCommand(int32_t serial, const hidl_string& number,
        int32_t type) {
    RLOGD("imsEctCommand: serial %d", serial);
    std::string strType = std::to_string(type);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_IMS_ECT,
            2, (const char *)number.c_str(), (const char *)strType.c_str());
    return Void();
}

Return<void> RadioImpl::holdCall(int32_t serial, int32_t callId) {
    RLOGD("holdCall: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_HOLD_CALL, 1, callId);
    return Void();
}

Return<void> RadioImpl::resumeCall(int32_t serial, int32_t callId) {
    RLOGD("resumeCall: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_RESUME_CALL, 1, callId);
    return Void();
}

Return<void> RadioImpl::imsDeregNotification(int32_t serial, int32_t cause) {
    RLOGD("imsDeregNotification: serial %d, cause %d", serial, cause);
    dispatchInts(serial, mSlotId, RIL_REQUEST_IMS_DEREG_NOTIFICATION, 1, cause);
    return Void();
}

Return<void> RadioImpl::setImsEnable(int32_t serial, bool isOn) {
    RLOGD("setImsEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_IMS_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setVolteEnable(int32_t serial, bool isOn) {
    RLOGD("setVolteEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_VOLTE_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setWfcEnable(int32_t serial, bool isOn) {
    RLOGD("setWfcEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_WFC_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setVilteEnable(int32_t serial, bool isOn) {
    RLOGD("setVilteEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_VILTE_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setViWifiEnable(int32_t serial, bool isOn) {
    RLOGD("setViWifiEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_VIWIFI_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setImsVoiceEnable(int32_t serial, bool isOn) {
    RLOGD("setImsVoiceEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_IMS_VOICE_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setImsVideoEnable(int32_t serial, bool isOn) {
    RLOGD("setImsVideoEnable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_IMS_VIDEO_ENABLE, 1, isOn ? 1:0);
    return Void();
}

Return<void> RadioImpl::setImscfg(int32_t serial, bool volteEnable,
        bool vilteEnable, bool vowifiEnable, bool viwifiEnable,
        bool smsEnable, bool eimsEnable) {
    int params[6] = {0};
    params[0] = volteEnable ? 1:0;
    params[1] = vilteEnable ? 1:0;
    params[2] = vowifiEnable ? 1:0;
    params[3] = viwifiEnable ? 1:0;
    params[4] = smsEnable ? 1:0;
    params[5] = eimsEnable ? 1:0;

    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_IMSCFG, 6, params);
    return Void();
}

Return<void> RadioImpl::setModemImsCfg(int32_t serial, const hidl_string& keys,
        const hidl_string& values, int32_t type) {
   LOGD("setModemImsCfg: serial %d", serial);
   dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_MD_IMSCFG, 4, keys.c_str(), values.c_str(),
           std::to_string(type).c_str());
   return Void();
}

Return<void> RadioImpl::getProvisionValue(int32_t serial,
        const hidl_string& provisionstring) {
    RLOGD("getProvisionValue: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_GET_PROVISION_VALUE, provisionstring.c_str());
    return Void();
}

Return<void> RadioImpl::setProvisionValue(int32_t serial,
        const hidl_string& provisionstring, const hidl_string& provisionValue) {
    RLOGD("setProvisionValue: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_PROVISION_VALUE,
            2, provisionstring.c_str(), provisionValue.c_str());
    return Void();
}

Return<void> RadioImpl::addImsConferenceCallMember(int32_t serial, int32_t confCallId,
        const hidl_string& address, int32_t callToAdd) {
    RLOGD("addImsConferenceCallMember: serial %d", serial);

    dispatchStrings(serial, mSlotId, RIL_REQUEST_ADD_IMS_CONFERENCE_CALL_MEMBER, 3,
            std::to_string(confCallId).c_str(), address.c_str(),
            std::to_string(callToAdd).c_str());

    return Void();
}

Return<void> RadioImpl::removeImsConferenceCallMember(int32_t serial, int32_t confCallId,
        const hidl_string& address, int32_t callToRemove) {
    RLOGD("removeImsConferenceCallMember: serial %d", serial);

    dispatchStrings(serial, mSlotId, RIL_REQUEST_REMOVE_IMS_CONFERENCE_CALL_MEMBER, 3,
            std::to_string(confCallId).c_str(), address.c_str(),
            std::to_string(callToRemove).c_str());

    return Void();
}


Return<void> RadioImpl::setWfcProfile(int32_t serial, int32_t wfcPreference) {
    RLOGD("setWfcProfile: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_WFC_PROFILE, 1, wfcPreference);
    return Void();
}

Return<void> RadioImpl::conferenceDial(int32_t serial, const ConferenceDial& dialInfo) {
    RLOGD("conferenceDial: serial %d", serial);

    int request = RIL_REQUEST_CONFERENCE_DIAL;
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, request);
    if (pRI == NULL) {
        return Void();
    }

    int countStrings = dialInfo.dialNumbers.size() + 3;
    char **pStrings;
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));

        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return Void();
    }

    if(!copyHidlStringToRil(&pStrings[0], dialInfo.isVideoCall ? "1":"0", pRI)) {
        free(pStrings);
        return Void();
    }

    if(!copyHidlStringToRil(&pStrings[1],
                            std::to_string(dialInfo.dialNumbers.size()).c_str(), pRI)) {
        memsetAndFreeStrings(1, pStrings[0]);
        free(pStrings);
        return Void();
    }

    int i = 0;
    for (i = 0; i < (int) dialInfo.dialNumbers.size(); i++) {
        if (!copyHidlStringToRil(&pStrings[i + 2], dialInfo.dialNumbers[i], pRI)) {
            for (int j = 0; j < i + 2; j++) {
                memsetAndFreeStrings(1, pStrings[j]);
            }

            free(pStrings);
            return Void();
        }
    }

    if(!copyHidlStringToRil(&pStrings[i + 2],
        std::to_string((int)dialInfo.clir).c_str(), pRI)) {
        for (int j = 0; j < (int) dialInfo.dialNumbers.size() + 2; j++) {
            memsetAndFreeStrings(1, pStrings[j]);
        }

        free(pStrings);
        return Void();
    }

    s_vendorFunctions->onRequest(request, pStrings, countStrings * sizeof(char *), pRI,
            pRI->socket_id);

    if (pStrings != NULL) {
        for (int j = 0 ; j < countStrings ; j++) {
            memsetAndFreeStrings(1, pStrings[j]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }

    return Void();
}

Return<void> RadioImpl::vtDialWithSipUri(int32_t serial, const hidl_string& address) {
    RLOGD("vtDialWithSipUri: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_VT_DIAL_WITH_SIP_URI, address.c_str());

    return Void();
}

Return<void> RadioImpl::dialWithSipUri(int32_t serial, const hidl_string& address) {
    RLOGD("dialWithSipUri: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_DIAL_WITH_SIP_URI, address.c_str());

    return Void();
}

Return<void> RadioImpl::sendUssi(int32_t serial, int32_t action,
        const hidl_string& ussiString) {
    RLOGD("sendUssi: serial %d", serial);

    hidl_string strAction = std::to_string(action);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SEND_USSI, 2,
                    strAction.c_str(), ussiString.c_str());

    return Void();
}

Return<void> RadioImpl::cancelUssi(int32_t serial) {
    RLOGD("cancelPendingUssi: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CANCEL_USSI);

    return Void();
}

Return<void> RadioImpl::forceReleaseCall(int32_t serial, int32_t callId) {
    RLOGD("forceHangup: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_FORCE_RELEASE_CALL, 1,
                 callId);

    return Void();
}

Return<void> RadioImpl::imsBearerActivationDone(int32_t serial, int32_t aid,
       int32_t status) {

    RLOGD("responseBearerActivationDone: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_IMS_BEARER_ACTIVATION_DONE, 2,
                 aid, status);

    return Void();
}

Return<void> RadioImpl::imsBearerDeactivationDone(int32_t serial, int32_t aid,
       int32_t status) {

    RLOGD("responseBearerDeactivationDone: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_IMS_BEARER_DEACTIVATION_DONE, 2,
                 aid, status);

    return Void();
}
Return<void> RadioImpl::setImsRtpReport(int32_t serial, int32_t pdnId,
       int32_t networkId, int32_t timer) {
    RLOGD("setImsRtpReport: serial %d", serial);

    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_IMS_RTP_REPORT, 3,
                 pdnId, networkId, timer);

    return Void();
}

Return<void> RadioImpl::pullCall(int32_t serial,
                                 const hidl_string& target,
                                 bool isVideoCall) {
    RLOGD("pullCall: serial %d", serial);

    dispatchStrings(serial, mSlotId, RIL_REQUEST_PULL_CALL, 2, target.c_str(),
                 isVideoCall ? "1":"0");

    return Void();
}
Return<void> RadioImpl::setImsRegistrationReport(int32_t serial) {
    RLOGD("setImsRegistrationReport: serial %d", serial);

    dispatchVoid(serial, mSlotId, RIL_REQUEST_SET_IMS_REGISTRATION_REPORT);
    return Void();
}

Return<void> RadioImpl::sendRequestStrings(int32_t serial,
        const hidl_vec<hidl_string>& data) {
    RLOGD("RadioImpl::sendRequestStrings: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_OEM_HOOK_STRINGS, data);
    return Void();
}

/// M: eMBMS feature
Return<void> RadioImpl::sendEmbmsAtCommand(int32_t serial, const hidl_string& s) {
    RLOGD("sendEmbmsAtCommand: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_EMBMS_AT_CMD, s.c_str());
    return Void();
}
/// M: eMBMS end

Return<void> OemHookImpl::sendRequestStrings(int32_t serial,
        const hidl_vec<hidl_string>& data) {
    RLOGD("OemHookImpl::sendRequestStrings: serial %d, donothing", serial);
    return Void();
}

// ATCI Start
Return<void> RadioImpl::setResponseFunctionsForAtci(
        const ::android::sp<IAtciResponse>& atciResponseParam,
        const ::android::sp<IAtciIndication>& atciIndicationParam) {
    RLOGD("RadioImpl::setResponseFunctionsForAtci");
    mAtciResponse = atciResponseParam;
    mAtciIndication = atciIndicationParam;
    return Void();
}

Return<void> RadioImpl::sendAtciRequest(int32_t serial, const hidl_vec<uint8_t>& data) {
    RLOGD("RadioImpl::sendAtciRequest: serial %d", serial);
    dispatchRaw(serial, mSlotId, RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL, data);
    return Void();
}
// ATCI End

// worldmode {
Return<void> RadioImpl::setResumeRegistration(int32_t serial, int32_t sessionId) {
    RLOGD("RadioImpl::setResumeRegistration: serial %d sessionId %d", serial, sessionId);
    dispatchInts(serial, mSlotId, RIL_REQUEST_RESUME_REGISTRATION, 1, sessionId);
    return Void();
}

Return<void> RadioImpl::storeModemType(int32_t serial, int32_t modemType) {
    RLOGD("RadioImpl::storeModemType: serial %d modemType %d", serial, modemType);
    dispatchInts(serial, mSlotId, RIL_REQUEST_STORE_MODEM_TYPE, 1, modemType);
    return Void();
}

Return<void> RadioImpl::reloadModemType(int32_t serial, int32_t modemType) {
    RLOGD("RadioImpl::reloadModemType: serial %d modemType %d", serial, modemType);
    dispatchInts(serial, mSlotId, RIL_REQUEST_RELOAD_MODEM_TYPE, 1, modemType);
    return Void();
}
// worldmode }
// PHB START
Return<void> RadioImpl::queryPhbStorageInfo(int32_t serial, int32_t type) {
    RLOGD("queryPhbStorageInfo: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_QUERY_PHB_STORAGE_INFO,
            1, type);
    return Void();
}

bool dispatchPhbEntry(int serial, int slotId, int request,
                              const PhbEntryStructure& phbEntry) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_PhbEntryStructure pbe;
    pbe.type = phbEntry.type;
    pbe.index = phbEntry.index;

    if (!copyHidlStringToRil(&pbe.number, phbEntry.number, pRI)) {
        return false;
    }
    pbe.ton = phbEntry.ton;

    if (!copyHidlStringToRil(&pbe.alphaId, phbEntry.alphaId, pRI)) {
        return false;
    }

    s_vendorFunctions->onRequest(request, &pbe, sizeof(pbe), pRI,
            pRI->socket_id);

    memsetAndFreeStrings(1, pbe.number);
    memsetAndFreeStrings(1, pbe.alphaId);

    return true;
}

Return<void> RadioImpl::writePhbEntry(int32_t serial, const PhbEntryStructure& phbEntry) {
    RLOGD("writePhbEntry: serial %d", serial);
    dispatchPhbEntry(serial, mSlotId, RIL_REQUEST_WRITE_PHB_ENTRY, phbEntry);
    return Void();
}

Return<void> RadioImpl::readPhbEntry(int32_t serial, int32_t type, int32_t bIndex, int32_t eIndex) {
    RLOGD("readPhbEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_PHB_ENTRY,
            3, type, bIndex, eIndex);
    return Void();
}

Return<void> RadioImpl::queryUPBCapability(int32_t serial) {
    RLOGD("queryUPBCapability: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_UPB_CAPABILITY);
    return Void();
}

Return<void> RadioImpl::editUPBEntry(int32_t serial, const hidl_vec<hidl_string>& data) {
    RLOGD("editUPBEntry: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_EDIT_UPB_ENTRY, data);
    return Void();
}

Return<void> RadioImpl::deleteUPBEntry(int32_t serial, int32_t entryType, int32_t adnIndex, int32_t entryIndex) {
    RLOGD("deleteUPBEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_DELETE_UPB_ENTRY,
           3, entryType, adnIndex, entryIndex);
    return Void();
}

Return<void> RadioImpl::readUPBGasList(int32_t serial, int32_t startIndex, int32_t endIndex) {
    RLOGD("readUPBGasList: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_UPB_GAS_LIST,
            2, startIndex, endIndex);
    return Void();
}

Return<void> RadioImpl::readUPBGrpEntry(int32_t serial, int32_t adnIndex) {
    RLOGD("readUPBGrpEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_UPB_GRP,
            1, adnIndex);
    return Void();
}

bool dispatchGrpEntry(int serial, int slotId, int request, int adnIndex, const hidl_vec<int32_t>& grpId) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    int countInts = grpId.size() + 1;
    int *pInts = (int *)calloc(countInts, sizeof(int));

    if (pInts == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    pInts[0] = adnIndex;
    for (int i = 1; i < countInts; i++) {
        pInts[i] = grpId[i-1];
    }

    s_vendorFunctions->onRequest(request, pInts, countInts * sizeof(int), pRI,
            pRI->socket_id);

    if (pInts != NULL) {
#ifdef MEMSET_FREED
        memset(pInts, 0, countInts * sizeof(int));
#endif
        free(pInts);
    }
    return true;
}

Return<void> RadioImpl::writeUPBGrpEntry(int32_t serial, int32_t adnIndex, const hidl_vec<int32_t>& grpIds) {
    RLOGD("writeUPBGrpEntry: serial %d", serial);
    dispatchGrpEntry(serial, mSlotId, RIL_REQUEST_WRITE_UPB_GRP, adnIndex, grpIds);
    return Void();
}

Return<void> RadioImpl::getPhoneBookStringsLength(int32_t serial) {
    RLOGD("getPhoneBookStringsLength: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_PHB_STRING_LENGTH);
    return Void();
}

Return<void> RadioImpl::getPhoneBookMemStorage(int32_t serial) {
    RLOGD("getPhoneBookMemStorage: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_PHB_MEM_STORAGE);
    return Void();
}

Return<void> RadioImpl::setPhoneBookMemStorage(int32_t serial,
        const hidl_string& storage, const hidl_string& password) {
    RLOGD("setPhoneBookMemStorage: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_PHB_MEM_STORAGE,
            2, storage.c_str(), password.c_str());
    return Void();
}

Return<void> RadioImpl::readPhoneBookEntryExt(int32_t serial, int32_t index1, int32_t index2) {
    RLOGD("readPhoneBookEntryExt: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_PHB_ENTRY_EXT,
            2, index1, index2);
    return Void();
}

bool dispatchPhbEntryExt(int serial, int slotId, int request,
                              const PhbEntryExt& phbEntryExt) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_PHB_ENTRY pbe;
    pbe.index = phbEntryExt.index;

    if (!copyHidlStringToRil(&pbe.number, phbEntryExt.number, pRI)) {
        return false;
    }
    pbe.type = phbEntryExt.type;

    if (!copyHidlStringToRil(&pbe.text, phbEntryExt.text, pRI)) {
        return false;
    }

    pbe.hidden = phbEntryExt.hidden;

    if (!copyHidlStringToRil(&pbe.group, phbEntryExt.group, pRI)) {
        return false;
    }

    if (!copyHidlStringToRil(&pbe.adnumber, phbEntryExt.adnumber, pRI)) {
        return false;
    }
    pbe.adtype = phbEntryExt.adtype;

    if (!copyHidlStringToRil(&pbe.secondtext, phbEntryExt.secondtext, pRI)) {
        return false;
    }

    if (!copyHidlStringToRil(&pbe.email, phbEntryExt.email, pRI)) {
        return false;
    }

    s_vendorFunctions->onRequest(request, &pbe, sizeof(pbe), pRI,
            pRI->socket_id);

    memsetAndFreeStrings(1, pbe.number);
    memsetAndFreeStrings(1, pbe.text);
    memsetAndFreeStrings(1, pbe.group);
    memsetAndFreeStrings(1, pbe.adnumber);
    memsetAndFreeStrings(1, pbe.secondtext);
    memsetAndFreeStrings(1, pbe.email);

    return true;
}

Return<void> RadioImpl::writePhoneBookEntryExt(int32_t serial, const PhbEntryExt& phbEntryExt) {
    RLOGD("writePhoneBookEntryExt: serial %d", serial);
    dispatchPhbEntryExt(serial, mSlotId, RIL_REQUEST_WRITE_PHB_ENTRY_EXT,
            phbEntryExt);
    return Void();
}

Return<void> RadioImpl::queryUPBAvailable(int32_t serial, int32_t eftype, int32_t fileIndex) {
    RLOGD("queryUPBAvailable: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_QUERY_UPB_AVAILABLE,
            2, eftype, fileIndex);
    return Void();
}

Return<void> RadioImpl::readUPBEmailEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex) {
    RLOGD("readUPBEmailEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_EMAIL_ENTRY,
            2, adnIndex, fileIndex);
    return Void();
}

Return<void> RadioImpl::readUPBSneEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex) {
    RLOGD("readUPBSneEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_SNE_ENTRY,
            2, adnIndex, fileIndex);
    return Void();
}

Return<void> RadioImpl::readUPBAnrEntry(int32_t serial, int32_t adnIndex, int32_t fileIndex) {
    RLOGD("readUPBAnrEntry: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_ANR_ENTRY,
            2, adnIndex, fileIndex);
    return Void();
}

Return<void> RadioImpl::readUPBAasList(int32_t serial, int32_t startIndex, int32_t endIndex) {
    RLOGD("readUPBAasList: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_READ_UPB_AAS_LIST,
            2, startIndex, endIndex);
    return Void();
}
// PHB END

Return<void> RadioImpl::setRxTestConfig(int32_t serial, int32_t antType) {
    RLOGD("setRxTestConfig: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_VSS_ANTENNA_CONF, 1, antType);
    return Void();
}
Return<void> RadioImpl::getRxTestResult(int32_t serial, int32_t mode) {
    RLOGD("getRxTestResult: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_VSS_ANTENNA_INFO, 1, mode);
    return Void();
}

Return<void> RadioImpl::getPOLCapability(int32_t serial) {
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_POL_CAPABILITY);
    return Void();
}

Return<void> RadioImpl::getCurrentPOLList(int32_t serial) {
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_POL_LIST);
    return Void();
}

Return<void> RadioImpl::setPOLEntry(int32_t serial, int32_t index,
        const ::android::hardware::hidl_string& numeric,
        int32_t nAct) {
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_POL_ENTRY,
            3,
            std::to_string((int) index).c_str(),
            numeric.c_str(),
            std::to_string((int) nAct).c_str());
    return Void();
}

/// M: [Network][C2K] Sprint roaming control @{
Return<void> RadioImpl::setRoamingEnable(int32_t serial, const hidl_vec<int32_t>& config) {
    RLOGD("setRoamingEnable: serial: %d", serial);
    if (config.size() == 6) {
        dispatchInts(serial, mSlotId, RIL_REQUEST_SET_ROAMING_ENABLE, 6,
                config[0], config[1], config[2], config[3], config[4], config[5]);
    } else {
        RLOGE("setRoamingEnable: param error, num: %d (should be 6)", (int) config.size());
    }
    return Void();
}

Return<void> RadioImpl::getRoamingEnable(int32_t serial, int32_t phoneId) {
    dispatchInts(serial, mSlotId, RIL_REQUEST_GET_ROAMING_ENABLE, 1, phoneId);
    return Void();
}
/// @}

/***************************************************************************************************
 * RESPONSE FUNCTIONS
 * Functions above are used for requests going from framework to vendor code. The ones below are
 * responses for those requests coming back from the vendor code.
 **************************************************************************************************/

void radio::acknowledgeRequest(int slotId, int serial) {
    if (radioService[slotId]->mRadioResponse != NULL) {
        Return<void> retStatus = radioService[slotId]->mRadioResponse->acknowledgeRequest(serial);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acknowledgeRequest: radioService[%d]->mRadioResponse == NULL", slotId);
    }
}

void populateResponseInfo(RadioResponseInfo& responseInfo, int serial, int responseType,
                         RIL_Errno e) {
    responseInfo.serial = serial;
    switch (responseType) {
        case RESPONSE_SOLICITED:
            responseInfo.type = RadioResponseType::SOLICITED;
            break;
        case RESPONSE_SOLICITED_ACK_EXP:
            responseInfo.type = RadioResponseType::SOLICITED_ACK_EXP;
            break;
    }
    responseInfo.error = (RadioError) e;
}

int responseIntOrEmpty(RadioResponseInfo& responseInfo, int serial, int responseType, RIL_Errno e,
               void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    int ret = -1;

    if (response == NULL && responseLen == 0) {
        // Earlier RILs did not send a response for some cases although the interface
        // expected an integer as response. Do not return error if response is empty. Instead
        // Return -1 in those cases to maintain backward compatibility.
    } else if (response == NULL || responseLen != sizeof(int)) {
        RLOGE("responseIntOrEmpty: Invalid response");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        int *p_int = (int *) response;
        ret = p_int[0];
    }
    return ret;
}

int responseInt(RadioResponseInfo& responseInfo, int serial, int responseType, RIL_Errno e,
               void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    int ret = -1;

    if (response == NULL || responseLen != sizeof(int)) {
        RLOGE("responseInt: Invalid response");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        int *p_int = (int *) response;
        ret = p_int[0];
    }
    return ret;
}

int radio::getIccCardStatusResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        CardStatus cardStatus = {};
        if (response == NULL || responseLen != sizeof(RIL_CardStatus_v6)) {
            RLOGE("getIccCardStatusResponse: Invalid response");
            cardStatus.cardState = CardState::ERROR;
            cardStatus.universalPinState = PinState::UNKNOWN;
            cardStatus.gsmUmtsSubscriptionAppIndex = -1;
            cardStatus.cdmaSubscriptionAppIndex = -1;
            cardStatus.imsSubscriptionAppIndex = -1;
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_CardStatus_v6 *p_cur = ((RIL_CardStatus_v6 *) response);
            cardStatus.cardState = (CardState) p_cur->card_state;
            cardStatus.universalPinState = (PinState) p_cur->universal_pin_state;
            cardStatus.gsmUmtsSubscriptionAppIndex = p_cur->gsm_umts_subscription_app_index;
            cardStatus.cdmaSubscriptionAppIndex = p_cur->cdma_subscription_app_index;
            cardStatus.imsSubscriptionAppIndex = p_cur->ims_subscription_app_index;

            RIL_AppStatus *rilAppStatus = p_cur->applications;
            cardStatus.applications.resize(p_cur->num_applications);
            AppStatus *appStatus = cardStatus.applications.data();
            RLOGD("getIccCardStatusResponse: num_applications %d", p_cur->num_applications);
            for (int i = 0; i < p_cur->num_applications; i++) {
                appStatus[i].appType = (AppType) rilAppStatus[i].app_type;
                appStatus[i].appState = (AppState) rilAppStatus[i].app_state;
                appStatus[i].persoSubstate = (PersoSubstate) rilAppStatus[i].perso_substate;
                appStatus[i].aidPtr = convertCharPtrToHidlString(rilAppStatus[i].aid_ptr);
                appStatus[i].appLabelPtr = convertCharPtrToHidlString(
                        rilAppStatus[i].app_label_ptr);
                appStatus[i].pin1Replaced = rilAppStatus[i].pin1_replaced;
                appStatus[i].pin1 = (PinState) rilAppStatus[i].pin1;
                appStatus[i].pin2 = (PinState) rilAppStatus[i].pin2;
            }
        }

        // M @{
        s_cardStatus[slotId].cardState = cardStatus.cardState;
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                getIccCardStatusResponse(responseInfo, cardStatus);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getIccCardStatusResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::supplyIccPinForAppResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("supplyIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPinForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("supplyIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPukForAppResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("supplyIccPukForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->supplyIccPukForAppResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("supplyIccPukForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPin2ForAppResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("supplyIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPin2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("supplyIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPuk2ForAppResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("supplyIccPuk2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPuk2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("supplyIccPuk2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPinForAppResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("changeIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                changeIccPinForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("changeIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPin2ForAppResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("changeIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                changeIccPin2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("changeIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyNetworkDepersonalizationResponse(int slotId,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("supplyNetworkDepersonalizationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyNetworkDepersonalizationResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("supplyNetworkDepersonalizationResponse: radioService[%d]->mRadioResponse == "
                "NULL", slotId);
    }

    return 0;
}

int radio::getCurrentCallsResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("getCurrentCallsResponse: serial %d", serial);

    if (radioService[slotId] != NULL && (radioService[slotId]->mRadioResponseMtk != NULL
            || radioService[slotId]->mRadioResponse != NULL)) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<Call> calls;
        if (response == NULL || (responseLen % sizeof(RIL_Call *)) != 0) {
            RLOGE("getCurrentCallsResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_Call *);
            calls.resize(num);

            for (int i = 0 ; i < num ; i++) {
                RIL_Call *p_cur = ((RIL_Call **) response)[i];
                /* each call info */
                calls[i].state = (CallState) p_cur->state;
                calls[i].index = p_cur->index;
                calls[i].toa = p_cur->toa;
                calls[i].isMpty = p_cur->isMpty;
                calls[i].isMT = p_cur->isMT;
                calls[i].als = p_cur->als;
                calls[i].isVoice = p_cur->isVoice;
                calls[i].isVoicePrivacy = p_cur->isVoicePrivacy;
                calls[i].number = convertCharPtrToHidlString(p_cur->number);
                calls[i].numberPresentation = (CallPresentation) p_cur->numberPresentation;
                calls[i].name = convertCharPtrToHidlString(p_cur->name);
                calls[i].namePresentation = (CallPresentation) p_cur->namePresentation;
                if (p_cur->uusInfo != NULL && p_cur->uusInfo->uusData != NULL) {
                    RIL_UUS_Info *uusInfo = p_cur->uusInfo;
                    calls[i].uusInfo[0].uusType = (UusType) uusInfo->uusType;
                    calls[i].uusInfo[0].uusDcs = (UusDcs) uusInfo->uusDcs;
                    // convert uusInfo->uusData to a null-terminated string
                    char *nullTermStr = strndup(uusInfo->uusData, uusInfo->uusLength);
                    calls[i].uusInfo[0].uusData = nullTermStr;
                    free(nullTermStr);
                }
            }
        }

        Return<void> retStatus = (radioService[slotId]->mRadioResponseMtk != NULL ?
                radioService[slotId]->mRadioResponseMtk->getCurrentCallsResponse(
                        responseInfo, calls) :
                radioService[slotId]->mRadioResponse->getCurrentCallsResponse(
                        responseInfo, calls));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCurrentCallsResponse: radioService[%d] or mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::dialResponse(int slotId,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responseLen) {
    RLOGD("dialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->dialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("dialResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getIMSIForAppResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responseLen) {
    RLOGD("getIMSIForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getIMSIForAppResponse(
                responseInfo, convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getIMSIForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupConnectionResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("hangupConnectionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->hangupConnectionResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("hangupConnectionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupWaitingOrBackgroundResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("hangupWaitingOrBackgroundResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->hangupWaitingOrBackgroundResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("hangupWaitingOrBackgroundResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupForegroundResumeBackgroundResponse(int slotId, int responseType, int serial,
                                                    RIL_Errno e, void *response,
                                                    size_t responseLen) {
    RLOGD("hangupWaitingOrBackgroundResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->hangupWaitingOrBackgroundResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("hangupWaitingOrBackgroundResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::switchWaitingOrHoldingAndActiveResponse(int slotId, int responseType, int serial,
                                                   RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("switchWaitingOrHoldingAndActiveResponse: serial %d", serial);

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                    mRadioResponseIms->switchWaitingOrHoldingAndActiveResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("switchWaitingOrHoldingAndActiveResponse: radioService[%d]->mRadioResponseIms"
                    " == NULL", slotId);
        }
    } else if (radioService[slotId]->mRadioResponseMtk != NULL
            || radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = (radioService[slotId]->mRadioResponseMtk != NULL ?
                radioService[slotId]->mRadioResponseMtk->switchWaitingOrHoldingAndActiveResponse(
                        responseInfo) :
                radioService[slotId]->mRadioResponse->switchWaitingOrHoldingAndActiveResponse(
                        responseInfo));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("switchWaitingOrHoldingAndActiveResponse: radioService[%d] or mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::conferenceResponse(int slotId, int responseType,
                             int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("conferenceResponse: serial %d", serial);

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                    mRadioResponseIms->conferenceResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("conferenceResponse: radioService[%d]->mRadioResponseIms"
                    " == NULL", slotId);
        }
    } else if (radioService[slotId]->mRadioResponseMtk != NULL
            || radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = (radioService[slotId]->mRadioResponseMtk != NULL ?
                radioService[slotId]->mRadioResponseMtk->conferenceResponse(responseInfo) :
                radioService[slotId]->mRadioResponse->conferenceResponse(responseInfo));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("conferenceResponse: radioService[%d] or mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::rejectCallResponse(int slotId, int responseType,
                             int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("rejectCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->rejectCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("rejectCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getLastCallFailCauseResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e, void *response,
                                       size_t responseLen) {
    RLOGD("getLastCallFailCauseResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        LastCallFailCauseInfo info = {};
        info.vendorCause = hidl_string();
        if (response == NULL) {
            RLOGE("getCurrentCallsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (responseLen == sizeof(int)) {
            int *pInt = (int *) response;
            info.causeCode = (LastCallFailCause) pInt[0];
        } else if (responseLen == sizeof(RIL_LastCallFailCauseInfo))  {
            RIL_LastCallFailCauseInfo *pFailCauseInfo = (RIL_LastCallFailCauseInfo *) response;
            info.causeCode = (LastCallFailCause) pFailCauseInfo->cause_code;
            info.vendorCause = convertCharPtrToHidlString(pFailCauseInfo->vendor_cause);
        } else {
            RLOGE("getCurrentCallsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getLastCallFailCauseResponse(
                responseInfo, info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getLastCallFailCauseResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getSignalStrengthResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("getSignalStrengthResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        SignalStrength signalStrength = {};
        if (response == NULL || responseLen != sizeof(RIL_SignalStrength_v10)) {
            RLOGE("getSignalStrengthResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilSignalStrengthToHal(response, responseLen, signalStrength);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getSignalStrengthResponse(
                responseInfo, signalStrength);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getSignalStrengthResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

RIL_CellInfoType getCellInfoTypeRadioTechnology(char *rat) {
    if (rat == NULL) {
        return RIL_CELL_INFO_TYPE_NONE;
    }

    int radioTech = atoi(rat);

    switch(radioTech) {

        case RADIO_TECH_GPRS:
        case RADIO_TECH_EDGE:
        case RADIO_TECH_GSM: {
            return RIL_CELL_INFO_TYPE_GSM;
        }

        case RADIO_TECH_UMTS:
        case RADIO_TECH_HSDPA:
        case RADIO_TECH_HSUPA:
        case RADIO_TECH_HSPA:
        case RADIO_TECH_HSPAP: {
            return RIL_CELL_INFO_TYPE_WCDMA;
        }

        case RADIO_TECH_IS95A:
        case RADIO_TECH_IS95B:
        case RADIO_TECH_1xRTT:
        case RADIO_TECH_EVDO_0:
        case RADIO_TECH_EVDO_A:
        case RADIO_TECH_EVDO_B:
        case RADIO_TECH_EHRPD: {
            return RIL_CELL_INFO_TYPE_CDMA;
        }

        case RADIO_TECH_LTE:
        case RADIO_TECH_LTE_CA: {
            return RIL_CELL_INFO_TYPE_LTE;
        }

        case RADIO_TECH_TD_SCDMA: {
            return RIL_CELL_INFO_TYPE_TD_SCDMA;
        }

        default: {
            break;
        }
    }

    return RIL_CELL_INFO_TYPE_NONE;

}

void fillCellIdentityResponse(CellIdentity &cellIdentity, RIL_CellIdentity_v16 &rilCellIdentity) {

    cellIdentity.cellIdentityGsm.resize(0);
    cellIdentity.cellIdentityWcdma.resize(0);
    cellIdentity.cellIdentityCdma.resize(0);
    cellIdentity.cellIdentityTdscdma.resize(0);
    cellIdentity.cellIdentityLte.resize(0);
    cellIdentity.cellInfoType = (CellInfoType)rilCellIdentity.cellInfoType;
    switch(rilCellIdentity.cellInfoType) {

        case RIL_CELL_INFO_TYPE_GSM: {
            cellIdentity.cellIdentityGsm.resize(1);
            cellIdentity.cellIdentityGsm[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityGsm.mcc);
            cellIdentity.cellIdentityGsm[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityGsm.mnc);
            cellIdentity.cellIdentityGsm[0].lac = rilCellIdentity.cellIdentityGsm.lac;
            cellIdentity.cellIdentityGsm[0].cid = rilCellIdentity.cellIdentityGsm.cid;
            cellIdentity.cellIdentityGsm[0].arfcn = rilCellIdentity.cellIdentityGsm.arfcn;
            cellIdentity.cellIdentityGsm[0].bsic = rilCellIdentity.cellIdentityGsm.bsic;
            break;
        }

        case RIL_CELL_INFO_TYPE_WCDMA: {
            cellIdentity.cellIdentityWcdma.resize(1);
            cellIdentity.cellIdentityWcdma[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityWcdma.mcc);
            cellIdentity.cellIdentityWcdma[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityWcdma.mnc);
            cellIdentity.cellIdentityWcdma[0].lac = rilCellIdentity.cellIdentityWcdma.lac;
            cellIdentity.cellIdentityWcdma[0].cid = rilCellIdentity.cellIdentityWcdma.cid;
            cellIdentity.cellIdentityWcdma[0].psc = rilCellIdentity.cellIdentityWcdma.psc;
            cellIdentity.cellIdentityWcdma[0].uarfcn = rilCellIdentity.cellIdentityWcdma.uarfcn;
            break;
        }

        case RIL_CELL_INFO_TYPE_CDMA: {
            cellIdentity.cellIdentityCdma.resize(1);
            cellIdentity.cellIdentityCdma[0].networkId = rilCellIdentity.cellIdentityCdma.networkId;
            cellIdentity.cellIdentityCdma[0].systemId = rilCellIdentity.cellIdentityCdma.systemId;
            cellIdentity.cellIdentityCdma[0].baseStationId =
                    rilCellIdentity.cellIdentityCdma.basestationId;
            cellIdentity.cellIdentityCdma[0].longitude = rilCellIdentity.cellIdentityCdma.longitude;
            cellIdentity.cellIdentityCdma[0].latitude = rilCellIdentity.cellIdentityCdma.latitude;
            break;
        }

        case RIL_CELL_INFO_TYPE_LTE: {
            cellIdentity.cellIdentityLte.resize(1);
            cellIdentity.cellIdentityLte[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityLte.mcc);
            cellIdentity.cellIdentityLte[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityLte.mnc);
            cellIdentity.cellIdentityLte[0].ci = rilCellIdentity.cellIdentityLte.ci;
            cellIdentity.cellIdentityLte[0].pci = rilCellIdentity.cellIdentityLte.pci;
            cellIdentity.cellIdentityLte[0].tac = rilCellIdentity.cellIdentityLte.tac;
            cellIdentity.cellIdentityLte[0].earfcn = rilCellIdentity.cellIdentityLte.earfcn;
            break;
        }

        case RIL_CELL_INFO_TYPE_TD_SCDMA: {
            cellIdentity.cellIdentityTdscdma.resize(1);
            cellIdentity.cellIdentityTdscdma[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityTdscdma.mcc);
            cellIdentity.cellIdentityTdscdma[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityTdscdma.mnc);
            cellIdentity.cellIdentityTdscdma[0].lac = rilCellIdentity.cellIdentityTdscdma.lac;
            cellIdentity.cellIdentityTdscdma[0].cid = rilCellIdentity.cellIdentityTdscdma.cid;
            cellIdentity.cellIdentityTdscdma[0].cpid = rilCellIdentity.cellIdentityTdscdma.cpid;
            break;
        }

        default: {
            break;
        }
    }
}

int convertResponseStringEntryToInt(char **response, int index, int numStrings) {
    if ((response != NULL) &&  (numStrings > index) && (response[index] != NULL)) {
        return atoi(response[index]);
    }

    return -1;
}

void fillCellIdentityFromVoiceRegStateResponseString(CellIdentity &cellIdentity,
        int numStrings, char** response) {

    RIL_CellIdentity_v16 rilCellIdentity;
    memset(&rilCellIdentity, -1, sizeof(RIL_CellIdentity_v16));

    rilCellIdentity.cellInfoType = getCellInfoTypeRadioTechnology(response[3]);
    switch(rilCellIdentity.cellInfoType) {

        case RIL_CELL_INFO_TYPE_GSM: {
            rilCellIdentity.cellIdentityGsm.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityGsm.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_WCDMA: {
            rilCellIdentity.cellIdentityWcdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityWcdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            rilCellIdentity.cellIdentityWcdma.psc =
                    convertResponseStringEntryToInt(response, 14, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_TD_SCDMA:{
            rilCellIdentity.cellIdentityTdscdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityTdscdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_CDMA:{
            rilCellIdentity.cellIdentityCdma.basestationId =
                    convertResponseStringEntryToInt(response, 4, numStrings);
            rilCellIdentity.cellIdentityCdma.longitude =
                    convertResponseStringEntryToInt(response, 5, numStrings);
            rilCellIdentity.cellIdentityCdma.latitude =
                    convertResponseStringEntryToInt(response, 6, numStrings);
            rilCellIdentity.cellIdentityCdma.systemId =
                    convertResponseStringEntryToInt(response, 8, numStrings);
            rilCellIdentity.cellIdentityCdma.networkId =
                    convertResponseStringEntryToInt(response, 9, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_LTE:{
            rilCellIdentity.cellIdentityLte.tac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityLte.ci =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }

        default: {
            break;
        }
    }

    fillCellIdentityResponse(cellIdentity, rilCellIdentity);
}

void fillCellIdentityFromDataRegStateResponseString(CellIdentity &cellIdentity,
        int numStrings, char** response) {

    RIL_CellIdentity_v16 rilCellIdentity;
    memset(&rilCellIdentity, -1, sizeof(RIL_CellIdentity_v16));

    rilCellIdentity.cellInfoType = getCellInfoTypeRadioTechnology(response[3]);
    switch(rilCellIdentity.cellInfoType) {
        case RIL_CELL_INFO_TYPE_GSM: {
            rilCellIdentity.cellIdentityGsm.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityGsm.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_WCDMA: {
            rilCellIdentity.cellIdentityWcdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityWcdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_TD_SCDMA:{
            rilCellIdentity.cellIdentityTdscdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityTdscdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_LTE: {
            rilCellIdentity.cellIdentityLte.tac =
                    convertResponseStringEntryToInt(response, 6, numStrings);
            rilCellIdentity.cellIdentityLte.pci =
                    convertResponseStringEntryToInt(response, 7, numStrings);
            rilCellIdentity.cellIdentityLte.ci =
                    convertResponseStringEntryToInt(response, 8, numStrings);
            break;
        }
        default: {
            break;
        }
    }

    fillCellIdentityResponse(cellIdentity, rilCellIdentity);
}

int radio::getVoiceRegistrationStateResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("getVoiceRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        VoiceRegStateResult voiceRegResponse = {};
        int numStrings = responseLen / sizeof(char *);
        if (response == NULL) {
               RLOGE("getVoiceRegistrationStateResponse Invalid response: NULL");
               if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (s_vendorFunctions->version <= 14) {
            if (numStrings != 15) {
                RLOGE("getVoiceRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **resp = (char **) response;
                voiceRegResponse.regState = (RegState) ATOI_NULL_HANDLED_DEF(resp[0], 4);
                voiceRegResponse.rat = ATOI_NULL_HANDLED(resp[3]);
                voiceRegResponse.cssSupported = ATOI_NULL_HANDLED_DEF(resp[7], 0);
                voiceRegResponse.roamingIndicator = ATOI_NULL_HANDLED(resp[10]);
                voiceRegResponse.systemIsInPrl = ATOI_NULL_HANDLED_DEF(resp[11], 0);
                voiceRegResponse.defaultRoamingIndicator = ATOI_NULL_HANDLED_DEF(resp[12], 0);
                voiceRegResponse.reasonForDenial = ATOI_NULL_HANDLED_DEF(resp[13], 0);
                fillCellIdentityFromVoiceRegStateResponseString(voiceRegResponse.cellIdentity,
                        numStrings, resp);
            }
        } else {
            RIL_VoiceRegistrationStateResponse *voiceRegState =
                    (RIL_VoiceRegistrationStateResponse *)response;

            if (responseLen != sizeof(RIL_VoiceRegistrationStateResponse)) {
                RLOGE("getVoiceRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                voiceRegResponse.regState = (RegState) voiceRegState->regState;
                voiceRegResponse.rat = voiceRegState->rat;;
                voiceRegResponse.cssSupported = voiceRegState->cssSupported;
                voiceRegResponse.roamingIndicator = voiceRegState->roamingIndicator;
                voiceRegResponse.systemIsInPrl = voiceRegState->systemIsInPrl;
                voiceRegResponse.defaultRoamingIndicator = voiceRegState->defaultRoamingIndicator;
                voiceRegResponse.reasonForDenial = voiceRegState->reasonForDenial;
                fillCellIdentityResponse(voiceRegResponse.cellIdentity,
                        voiceRegState->cellIdentity);
            }
        }

        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->getVoiceRegistrationStateResponse(
                responseInfo, voiceRegResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getVoiceRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getDataRegistrationStateResponse(int slotId,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("getDataRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        DataRegStateResult dataRegResponse = {};
        if (response == NULL) {
            RLOGE("getDataRegistrationStateResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (s_vendorFunctions->version <= 14) {
            int numStrings = responseLen / sizeof(char *);
            if ((numStrings != 6) && (numStrings != 11)) {
                RLOGE("getDataRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **resp = (char **) response;
                dataRegResponse.regState = (RegState) ATOI_NULL_HANDLED_DEF(resp[0], 4);
                dataRegResponse.rat =  ATOI_NULL_HANDLED_DEF(resp[3], 0);
                dataRegResponse.reasonDataDenied =  ATOI_NULL_HANDLED(resp[4]);
                dataRegResponse.maxDataCalls =  ATOI_NULL_HANDLED_DEF(resp[5], 1);
                fillCellIdentityFromDataRegStateResponseString(dataRegResponse.cellIdentity,
                        numStrings, resp);
            }
        } else {
            RIL_DataRegistrationStateResponse *dataRegState =
                    (RIL_DataRegistrationStateResponse *)response;

            if (responseLen != sizeof(RIL_DataRegistrationStateResponse)) {
                RLOGE("getDataRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                dataRegResponse.regState = (RegState) dataRegState->regState;
                dataRegResponse.rat = dataRegState->rat;;
                dataRegResponse.reasonDataDenied = dataRegState->reasonDataDenied;
                dataRegResponse.maxDataCalls = dataRegState->maxDataCalls;
                fillCellIdentityResponse(dataRegResponse.cellIdentity, dataRegState->cellIdentity);
            }
        }

        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->getDataRegistrationStateResponse(responseInfo,
                dataRegResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getDataRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getOperatorResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("getOperatorResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_string longName;
        hidl_string shortName;
        hidl_string numeric;
        int numStrings = responseLen / sizeof(char *);
        if (response == NULL || numStrings != 3) {
            RLOGE("getOperatorResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;

        } else {
            char **resp = (char **) response;
            longName = convertCharPtrToHidlString(resp[0]);
            shortName = convertCharPtrToHidlString(resp[1]);
            numeric = convertCharPtrToHidlString(resp[2]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getOperatorResponse(
                responseInfo, longName, shortName, numeric);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getOperatorResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setRadioPowerResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responseLen) {
    RLOGD("setRadioPowerResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setRadioPowerResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setRadioPowerResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendDtmfResponse(int slotId,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responseLen) {
    RLOGD("sendDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendDtmfResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendDtmfResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

SendSmsResult makeSendSmsResult(RadioResponseInfo& responseInfo, int serial, int responseType,
                                RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    SendSmsResult result = {};

    if (response == NULL || responseLen != sizeof(RIL_SMS_Response)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        result.ackPDU = hidl_string();
    } else {
        RIL_SMS_Response *resp = (RIL_SMS_Response *) response;
        result.messageRef = resp->messageRef;
        result.ackPDU = convertCharPtrToHidlString(resp->ackPDU);
        result.errorCode = resp->errorCode;
    }
    return result;
}

int radio::sendSmsResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("sendSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendSmsResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendSMSExpectMoreResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("sendSMSExpectMoreResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendSMSExpectMoreResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendSMSExpectMoreResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setupDataCallResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("setupDataCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        MtkSetupDataCallResult result = {};
        if (response == NULL || responseLen != sizeof(RIL_Data_Call_Response_v11)) {
            RLOGE("setupDataCallResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            result.dcr.status = DataCallFailCause::ERROR_UNSPECIFIED;
            result.dcr.type = hidl_string();
            result.dcr.ifname = hidl_string();
            result.dcr.addresses = hidl_string();
            result.dcr.dnses = hidl_string();
            result.dcr.gateways = hidl_string();
            result.dcr.pcscf = hidl_string();
        } else {
            convertRilDataCallToHalEx((RIL_Data_Call_Response_v11 *) response, result);
        }

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setupDataCallResponseEx(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        SetupDataCallResult result = {};
        if (response == NULL || responseLen != sizeof(RIL_Data_Call_Response_v11)) {
            RLOGE("setupDataCallResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            result.status = DataCallFailCause::ERROR_UNSPECIFIED;
            result.type = hidl_string();
            result.ifname = hidl_string();
            result.addresses = hidl_string();
            result.dnses = hidl_string();
            result.gateways = hidl_string();
            result.pcscf = hidl_string();
        } else {
            convertRilDataCallToHal((RIL_Data_Call_Response_v11 *) response, result);
        }

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponse->setupDataCallResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setupDataCallResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

IccIoResult responseIccIo(RadioResponseInfo& responseInfo, int serial, int responseType,
                           RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    IccIoResult result = {};

    if (response == NULL || responseLen != sizeof(RIL_SIM_IO_Response)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        result.simResponse = hidl_string();
    } else {
        RIL_SIM_IO_Response *resp = (RIL_SIM_IO_Response *) response;
        result.sw1 = resp->sw1;
        result.sw2 = resp->sw2;
        result.simResponse = convertCharPtrToHidlString(resp->simResponse);
    }
    return result;
}

int radio::iccIOForAppResponse(int slotId,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responseLen) {
    RLOGD("iccIOForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->iccIOForAppResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("iccIOForAppResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendUssdResponse(int slotId,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responseLen) {
    RLOGD("sendUssdResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendUssdResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendUssdResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::cancelPendingUssdResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("cancelPendingUssdResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->cancelPendingUssdResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cancelPendingUssdResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getClirResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("getClirResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int n = -1, m = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("getClirResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            n = pInt[0];
            m = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getClirResponse(responseInfo,
                n, m);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getClirResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setClirResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("setClirResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setClirResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setClirResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getCallForwardStatusResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("getCallForwardStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<CallForwardInfo> callForwardInfos;

        if (response == NULL || responseLen % sizeof(RIL_CallForwardInfo *) != 0) {
            RLOGE("getCallForwardStatusResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_CallForwardInfo *);
            callForwardInfos.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_CallForwardInfo *resp = ((RIL_CallForwardInfo **) response)[i];
                callForwardInfos[i].status = (CallForwardInfoStatus) resp->status;
                callForwardInfos[i].reason = resp->reason;
                callForwardInfos[i].serviceClass = resp->serviceClass;
                callForwardInfos[i].toa = resp->toa;
                callForwardInfos[i].number = convertCharPtrToHidlString(resp->number);
                callForwardInfos[i].timeSeconds = resp->timeSeconds;
            }
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getCallForwardStatusResponse(
                responseInfo, callForwardInfos);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCallForwardStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCallForwardResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("setCallForwardResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setCallForwardResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCallForwardResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getCallWaitingResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("getCallWaitingResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int serviceClass = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("getCallWaitingResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
            serviceClass = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getCallWaitingResponse(
                responseInfo, enable, serviceClass);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCallWaitingResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setCallWaitingResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("setCallWaitingResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setCallWaitingResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCallWaitingResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::acknowledgeLastIncomingGsmSmsResponse(int slotId,
                                                int responseType, int serial, RIL_Errno e,
                                                void *response, size_t responseLen) {
    RLOGD("acknowledgeLastIncomingGsmSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->acknowledgeLastIncomingGsmSmsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acknowledgeLastIncomingGsmSmsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::acceptCallResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("acceptCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->acceptCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acceptCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::deactivateDataCallResponse(int slotId,
                                                int responseType, int serial, RIL_Errno e,
                                                void *response, size_t responseLen) {
    RLOGD("deactivateDataCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponse->deactivateDataCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("deactivateDataCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getFacilityLockForAppResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("getFacilityLockForAppResponse: serial %d", serial);

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
            Return<void> retStatus = radioService[slotId]->mRadioResponseIms->
                    getFacilityLockForAppResponse(responseInfo, ret);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("getFacilityLockForAppResponse: radioService[%d]->mRadioResponseIms == NULL",
                    slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponse != NULL) {
            RadioResponseInfo responseInfo = {};
            int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
            Return<void> retStatus = radioService[slotId]->mRadioResponse->
                    getFacilityLockForAppResponse(responseInfo, ret);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("getFacilityLockForAppResponse: radioService[%d]->mRadioResponse == NULL",
                    slotId);
        }
    }
    return 0;
}

int radio::setFacilityLockForAppResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("setFacilityLockForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseIntOrEmpty(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setFacilityLockForAppResponse(responseInfo,
                ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setFacilityLockForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setBarringPasswordResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("setBarringPasswordResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setBarringPasswordResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setBarringPasswordResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getNetworkSelectionModeResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e, void *response,
                                          size_t responseLen) {
    RLOGD("getNetworkSelectionModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool manual = false;
        int serviceClass;
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("getNetworkSelectionModeResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            manual = pInt[0] == 1 ? true : false;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getNetworkSelectionModeResponse(
                responseInfo,
                manual);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getNetworkSelectionModeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setNetworkSelectionModeAutomaticResponse(int slotId, int responseType, int serial,
                                                    RIL_Errno e, void *response,
                                                    size_t responseLen) {
    RLOGD("setNetworkSelectionModeAutomaticResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setNetworkSelectionModeAutomaticResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setNetworkSelectionModeAutomaticResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setNetworkSelectionModeManualResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("setNetworkSelectionModeManualResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setNetworkSelectionModeManualResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acceptCallResponse: radioService[%d]->setNetworkSelectionModeManualResponse "
                "== NULL", slotId);
    }

    return 0;
}

// MTK-START: SIM
int radio::getATRResponse(int slotId,
        int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("getATRResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->getATRResponse(
                responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvReadItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setSimPowerResponse(int slotId,
        int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setSimPowerResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setSimPowerResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvReadItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}
// MTK-END

// MTK-START: SIM ME LOCK
int radio::queryNetworkLockResponse(int slotId,
        int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("queryNetworkLockResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        RIL_SimMeLockCatInfo *pLockCatInfo = (RIL_SimMeLockCatInfo *) response;
        int category = -1, state = -1, retry_cnt = -1, autolock_cnt = -1,
                num_set = -1, total_set = -1, key_state = -1;

        if (response == NULL || responseLen % sizeof(RIL_SimMeLockCatInfo) != 0) {
            RLOGE("queryNetworkLockResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            category = pLockCatInfo->catagory;
            state = pLockCatInfo->state;
            retry_cnt = pLockCatInfo->retry_cnt;
            autolock_cnt = pLockCatInfo->autolock_cnt;
            num_set = pLockCatInfo->num_set;
            total_set = pLockCatInfo->total_set;
            key_state = pLockCatInfo->key_state;
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->queryNetworkLockResponse(
                responseInfo, category, state, retry_cnt, autolock_cnt, num_set, total_set,
                key_state);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("queryNetworkLockResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setNetworkLockResponse(int slotId,
        int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setNetworkLockResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setNetworkLockResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setNetworkLockResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}
// MTK-END

int convertOperatorStatusToInt(const char *str) {
    if (strncmp("unknown", str, 9) == 0) {
        return (int) OperatorStatus::UNKNOWN;
    } else if (strncmp("available", str, 9) == 0) {
        return (int) OperatorStatus::AVAILABLE;
    } else if (strncmp("current", str, 9) == 0) {
        return (int) OperatorStatus::CURRENT;
    } else if (strncmp("forbidden", str, 9) == 0) {
        return (int) OperatorStatus::FORBIDDEN;
    } else {
        return -1;
    }
}

int radio::getAvailableNetworksResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("getAvailableNetworksResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<OperatorInfo> networks;
        if (response == NULL || responseLen % (4 * sizeof(char *))!= 0) {
            RLOGE("getAvailableNetworksResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            networks.resize(numStrings/4);
            for (int i = 0, j = 0; i < numStrings; i = i + 4, j++) {
                networks[j].alphaLong = convertCharPtrToHidlString(resp[i]);
                networks[j].alphaShort = convertCharPtrToHidlString(resp[i + 1]);
                networks[j].operatorNumeric = convertCharPtrToHidlString(resp[i + 2]);
                int status = convertOperatorStatusToInt(resp[i + 3]);
                if (status == -1) {
                    if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
                } else {
                    networks[j].status = (OperatorStatus) status;
                }
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAvailableNetworksResponse(responseInfo,
                networks);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getAvailableNetworksResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

// MTK-START: NW
int radio::setNetworkSelectionModeManualWithActResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("setNetworkSelectionModeManualWithActResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setNetworkSelectionModeManualWithActResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acceptCallResponse: radioService[%d]->setNetworkSelectionModeManualWithActResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::getAvailableNetworksWithActResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("getAvailableNetworksWithActResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<OperatorInfoWithAct> networks;
        if (response == NULL || responseLen % (6 * sizeof(char *))!= 0) {
            RLOGE("getAvailableNetworksWithActResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            networks.resize(numStrings/6);
            for (int i = 0, j = 0; i < numStrings; i = i + 6, j++) {
                networks[j].base.alphaLong = convertCharPtrToHidlString(resp[i]);
                networks[j].base.alphaShort = convertCharPtrToHidlString(resp[i + 1]);
                networks[j].base.operatorNumeric = convertCharPtrToHidlString(resp[i + 2]);
                int status = convertOperatorStatusToInt(resp[i + 3]);
                if (status == -1) {
                    if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
                } else {
                    networks[j].base.status = (OperatorStatus) status;
                }
                networks[j].lac = convertCharPtrToHidlString(resp[i + 4]);
                networks[j].act = convertCharPtrToHidlString(resp[i + 5]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getAvailableNetworksWithActResponse(responseInfo,
                networks);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getAvailableNetworksWithActResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::cancelAvailableNetworksResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("cancelAvailableNetworksResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->cancelAvailableNetworksResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acceptCallResponse: radioService[%d]->cancelAvailableNetworksResponse "
                "== NULL", slotId);
    }

    return 0;
}

// Femtocell feature
int radio::getFemtocellListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("getFemtocellListResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> femtoList;
        if (response == NULL) {
            RLOGE("getFemtocellListResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            femtoList.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                femtoList[i] = convertCharPtrToHidlString(resp[i]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getFemtocellListResponse(
                responseInfo, femtoList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getFemtocellListResponse: radioService[%d]->mRadioIndicationMtk "
                "== NULL", slotId);
    }

    return 0;
}

int radio::abortFemtocellListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("abortFemtocellListResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->abortFemtocellListResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("abortFemtocellListResponse: radioService[%d]->mRadioResponseMtk "
                "== NULL", slotId);
    }

    return 0;
}

int radio::selectFemtocellResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("selectFemtocellResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->selectFemtocellResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("selectFemtocellResponse: radioService[%d]->mRadioResponseMtk "
                "== NULL", slotId);
    }

    return 0;
}

int radio::queryFemtoCellSystemSelectionModeResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("queryFemtoCellSystemSelectionModeResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int mode = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->queryFemtoCellSystemSelectionModeResponse(
                responseInfo, mode);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("queryFemtoCellSystemSelectionModeResponse: radioService[%d]->mRadioResponseMtk "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setFemtoCellSystemSelectionModeResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("setFemtoCellSystemSelectionModeResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setFemtoCellSystemSelectionModeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setFemtoCellSystemSelectionModeResponse: radioService[%d]->mRadioResponseMtkmRadioResponseMtk "
                "== NULL", slotId);
    }

    return 0;
}
// MTK-END: NW

int radio::startDtmfResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen) {
    RLOGD("startDtmfResponse: serial %d", serial);

    if(radioService[slotId] == NULL) {
        RLOGE("startDtmfResponse: radioService[%d] == NULL", slotId);
        return 0;
    }

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseIms->startDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("startDtmfResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    }
    else {

        if(radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseMtk->startDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
        else if(radioService[slotId]->mRadioResponse != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->startDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    }

    return 0;
}

int radio::stopDtmfResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("stopDtmfResponse: serial %d", serial);

    if(radioService[slotId] == NULL) {
        RLOGE("stopDtmfResponse: radioService[%d] == NULL", slotId);
        return 0;
    }

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseIms->stopDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("stopDtmfResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    }
    else {

        if(radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseMtk->stopDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
        else if(radioService[slotId]->mRadioResponse != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->stopDtmfResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    }

    return 0;
}

int radio::getBasebandVersionResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("getBasebandVersionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getBasebandVersionResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getBasebandVersionResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::separateConnectionResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("separateConnectionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL
            || radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = (radioService[slotId]->mRadioResponseMtk != NULL ?
                radioService[slotId]->mRadioResponseMtk->separateConnectionResponse(responseInfo) :
                radioService[slotId]->mRadioResponse->separateConnectionResponse(responseInfo));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("separateConnectionResponse: radioService[%d] or mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setMuteResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen) {
    RLOGD("setMuteResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setMuteResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setMuteResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getMuteResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("getMuteResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int serviceClass;
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("getMuteResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getMuteResponse(responseInfo,
                enable);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getMuteResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getClipResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen) {
    RLOGD("getClipResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getClipResponse(responseInfo,
                (ClipStatus) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getClipResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setClipResponse(int slotId, int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("setClipResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setClipResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setClipResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::getColpResponse(int slotId, int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("getColpResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int n = -1, m = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("getColpResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            n = pInt[0];
            m = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->getColpResponse(responseInfo,
                n, m);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getColpResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::getColrResponse(int slotId, int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("getColrResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        int n = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                getColrResponse(responseInfo, n);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getColrResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::sendCnapResponse(int slotId, int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen) {
    RLOGD("sendCnapResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int n = -1, m = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("sendCnapResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            n = pInt[0];
            m = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                sendCnapResponse(responseInfo, n, m);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendCnapResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::setColpResponse(int slotId, int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("setColpResponse: serial %d", serial);

    if (isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseIms->setColpResponse(
                    responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setColpResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setColpResponse(
                    responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setColpResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
        }
    }

    return 0;
}

int radio::setColrResponse(int slotId, int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("setColrResponse: serial %d", serial);

    if (isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseIms->setColrResponse(
                    responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setColrResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setColrResponse(
                    responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setColrResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
        }
    }

    return 0;
}

int radio::queryCallForwardInTimeSlotStatusResponse(int slotId, int responseType, int serial,
                                                    RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("queryCallForwardInTimeSlotStatusResponse: serial %d", serial);

    if (isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            hidl_vec<CallForwardInfoEx> callForwardInfoExs;

            if ((response == NULL && responseLen != 0)
                    || responseLen % sizeof(RIL_CallForwardInfoEx *) != 0) {
                RLOGE("queryCallForwardInTimeSlotStatusResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                int num = responseLen / sizeof(RIL_CallForwardInfoEx *);
                callForwardInfoExs.resize(num);
                for (int i = 0 ; i < num; i++) {
                    RIL_CallForwardInfoEx *resp = ((RIL_CallForwardInfoEx **) response)[i];
                    callForwardInfoExs[i].status = (CallForwardInfoStatus) resp->status;
                    callForwardInfoExs[i].reason = resp->reason;
                    callForwardInfoExs[i].serviceClass = resp->serviceClass;
                    callForwardInfoExs[i].toa = resp->toa;
                    callForwardInfoExs[i].number = convertCharPtrToHidlString(resp->number);
                    callForwardInfoExs[i].timeSeconds = resp->timeSeconds;
                    callForwardInfoExs[i].timeSlotBegin = convertCharPtrToHidlString(resp->timeSlotBegin);
                    callForwardInfoExs[i].timeSlotEnd = convertCharPtrToHidlString(resp->timeSlotEnd);
                }
            }

            Return<void> retStatus = radioService[slotId]->mRadioResponseIms->
                    queryCallForwardInTimeSlotStatusResponse(responseInfo, callForwardInfoExs);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("queryCallForwardInTimeSlotStatusResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            hidl_vec<CallForwardInfoEx> callForwardInfoExs;

            if ((response == NULL && responseLen != 0)
                    || responseLen % sizeof(RIL_CallForwardInfoEx *) != 0) {
                RLOGE("queryCallForwardInTimeSlotStatusResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                int num = responseLen / sizeof(RIL_CallForwardInfoEx *);
                callForwardInfoExs.resize(num);
                for (int i = 0 ; i < num; i++) {
                    RIL_CallForwardInfoEx *resp = ((RIL_CallForwardInfoEx **) response)[i];
                    callForwardInfoExs[i].status = (CallForwardInfoStatus) resp->status;
                    callForwardInfoExs[i].reason = resp->reason;
                    callForwardInfoExs[i].serviceClass = resp->serviceClass;
                    callForwardInfoExs[i].toa = resp->toa;
                    callForwardInfoExs[i].number = convertCharPtrToHidlString(resp->number);
                    callForwardInfoExs[i].timeSeconds = resp->timeSeconds;
                    callForwardInfoExs[i].timeSlotBegin = convertCharPtrToHidlString(resp->timeSlotBegin);
                    callForwardInfoExs[i].timeSlotEnd = convertCharPtrToHidlString(resp->timeSlotEnd);
                }
            }

            Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                    queryCallForwardInTimeSlotStatusResponse(responseInfo, callForwardInfoExs);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("queryCallForwardInTimeSlotStatusResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
        }
    }

    return 0;
}

int radio::setCallForwardInTimeSlotResponse(int slotId, int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen) {
    RLOGD("setCallForwardInTimeSlotResponse: serial %d", serial);

    if (isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseIms->
                    setCallForwardInTimeSlotResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setCallForwardInTimeSlotResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                    setCallForwardInTimeSlotResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setCallForwardInTimeSlotResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
        }
    }

    return 0;
}

int radio::runGbaAuthenticationResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("runGbaAuthenticationResponse: serial %d", serial);

    if (isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            hidl_vec<hidl_string> resList;
            int numStrings = responseLen / sizeof(char*);
            if (response == NULL || responseLen % sizeof(char *) != 0) {
                RLOGE("runGbaAuthenticationResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **pString = (char **) response;
                resList.resize(numStrings);
                for (int i = 0; i < numStrings; i++) {
                    resList[i] = convertCharPtrToHidlString(pString[i]);
                }
            }
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseIms->runGbaAuthenticationResponse(responseInfo,
                    resList);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("runGbaAuthenticationResponse: radioService[%d]->mRadioResponseIms == NULL",
                    slotId);
        }
    } else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            hidl_vec<hidl_string> resList;
            int numStrings = responseLen / sizeof(char*);
            if (response == NULL || responseLen % sizeof(char *) != 0) {
                RLOGE("runGbaAuthenticationResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **pString = (char **) response;
                resList.resize(numStrings);
                for (int i = 0; i < numStrings; i++) {
                    resList[i] = convertCharPtrToHidlString(pString[i]);
                }
            }
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseMtk->runGbaAuthenticationResponse(responseInfo,
                    resList);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("runGbaAuthenticationResponse: radioService[%d]->mRadioResponseMtk == NULL",
                    slotId);
        }
    }

    return 0;
}

int radio::getDataCallListResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("getDataCallListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<MtkSetupDataCallResult> ret;
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("getDataCallListResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilDataCallListToHalEx(response, responseLen, ret);
        }

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->getDataCallListResponseEx(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<SetupDataCallResult> ret;
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("getDataCallListResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilDataCallListToHal(response, responseLen, ret);
        }

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getDataCallListResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getDataCallListResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setSuppServiceNotificationsResponse(int slotId,
                                              int responseType, int serial, RIL_Errno e,
                                              void *response, size_t responseLen) {
    RLOGD("setSuppServiceNotificationsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setSuppServiceNotificationsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setSuppServiceNotificationsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::deleteSmsOnSimResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("deleteSmsOnSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->deleteSmsOnSimResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("deleteSmsOnSimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setBandModeResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen) {
    RLOGD("setBandModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setBandModeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setBandModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::writeSmsToSimResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen) {
    RLOGD("writeSmsToSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->writeSmsToSimResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("writeSmsToSimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getAvailableBandModesResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e, void *response,
                                        size_t responseLen) {
    RLOGD("getAvailableBandModesResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<RadioBandMode> modes;
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("getAvailableBandModesResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            modes.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                modes[i] = (RadioBandMode) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAvailableBandModesResponse(responseInfo,
                modes);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getAvailableBandModesResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendEnvelopeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("sendEnvelopeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendEnvelopeResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendEnvelopeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendTerminalResponseToSimResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("sendTerminalResponseToSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendTerminalResponseToSimResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendTerminalResponseToSimResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::handleStkCallSetupRequestFromSimResponse(int slotId,
                                                   int responseType, int serial,
                                                   RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("handleStkCallSetupRequestFromSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->handleStkCallSetupRequestFromSimResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("handleStkCallSetupRequestFromSimResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::handleStkCallSetupRequestFromSimWithResCodeResponse(int slotId,
                                                   int responseType, int serial,
                                                   RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("handleStkCallSetupRequestFromSimWithResCodeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->handleStkCallSetupRequestFromSimWithResCodeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("handleStkCallSetupRequestFromSimWithResCodeResponse: radioService[%d]->mRadioResponseMtk "
                "== NULL", slotId);
    }

    return 0;
}

int radio::explicitCallTransferResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("explicitCallTransferResponse: serial %d", serial);

    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponseIms
                                          ->explicitCallTransferResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("explicitCallTransferResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                                      slotId);
        }
    }
    else {
        if (radioService[slotId]->mRadioResponseMtk != NULL
                || radioService[slotId]->mRadioResponse != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus =
                    (radioService[slotId]->mRadioResponseMtk != NULL ?
                            radioService[slotId]->mRadioResponseMtk->explicitCallTransferResponse(
                                    responseInfo) :
                            radioService[slotId]->mRadioResponse->explicitCallTransferResponse(
                                    responseInfo));
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("explicitCallTransferResponse: radioService[%d] or mRadioResponse == NULL",
                                                                                      slotId);
        }
    }

    return 0;
}

int radio::setPreferredNetworkTypeResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("setPreferredNetworkTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setPreferredNetworkTypeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setPreferredNetworkTypeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}


int radio::getPreferredNetworkTypeResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("getPreferredNetworkTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getPreferredNetworkTypeResponse(
                responseInfo, (PreferredNetworkType) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPreferredNetworkTypeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getNeighboringCidsResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("getNeighboringCidsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<NeighboringCell> cells;

        if (response == NULL || responseLen % sizeof(RIL_NeighboringCell *) != 0) {
            RLOGE("getNeighboringCidsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_NeighboringCell *);
            cells.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_NeighboringCell *resp = ((RIL_NeighboringCell **) response)[i];
                cells[i].cid = convertCharPtrToHidlString(resp->cid);
                cells[i].rssi = resp->rssi;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getNeighboringCidsResponse(responseInfo,
                cells);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getNeighboringCidsResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setLocationUpdatesResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("setLocationUpdatesResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setLocationUpdatesResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setLocationUpdatesResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaSubscriptionSourceResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("setCdmaSubscriptionSourceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaSubscriptionSourceResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCdmaSubscriptionSourceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaRoamingPreferenceResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("setCdmaRoamingPreferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaRoamingPreferenceResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCdmaRoamingPreferenceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaRoamingPreferenceResponse(int slotId,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("getCdmaRoamingPreferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaRoamingPreferenceResponse(
                responseInfo, (CdmaRoamingType) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCdmaRoamingPreferenceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setTTYModeResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("setTTYModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setTTYModeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setTTYModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getTTYModeResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("getTTYModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getTTYModeResponse(responseInfo,
                (TtyMode) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getTTYModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setPreferredVoicePrivacyResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("setPreferredVoicePrivacyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setPreferredVoicePrivacyResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setPreferredVoicePrivacyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getPreferredVoicePrivacyResponse(int slotId,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("getPreferredVoicePrivacyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 1) {
            RLOGE("getPreferredVoicePrivacyResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
        }
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getPreferredVoicePrivacyResponse(
                responseInfo, enable);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPreferredVoicePrivacyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendCDMAFeatureCodeResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("sendCDMAFeatureCodeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendCDMAFeatureCodeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendCDMAFeatureCodeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendBurstDtmfResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("sendBurstDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendBurstDtmfResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendBurstDtmfResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendCdmaSmsResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("sendCdmaSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendCdmaSmsResponse(responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendCdmaSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::acknowledgeLastIncomingCdmaSmsResponse(int slotId,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("acknowledgeLastIncomingCdmaSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->acknowledgeLastIncomingCdmaSmsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acknowledgeLastIncomingCdmaSmsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::getGsmBroadcastConfigResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("getGsmBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<GsmBroadcastSmsConfigInfo> configs;

        if (response == NULL || responseLen % sizeof(RIL_GSM_BroadcastSmsConfigInfo *) != 0) {
            RLOGE("getGsmBroadcastConfigResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);
            configs.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_GSM_BroadcastSmsConfigInfo *resp =
                        ((RIL_GSM_BroadcastSmsConfigInfo **) response)[i];
                configs[i].fromServiceId = resp->fromServiceId;
                configs[i].toServiceId = resp->toServiceId;
                configs[i].fromCodeScheme = resp->fromCodeScheme;
                configs[i].toCodeScheme = resp->toCodeScheme;
                configs[i].selected = resp->selected == 1 ? true : false;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getGsmBroadcastConfigResponse(responseInfo,
                configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getGsmBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setGsmBroadcastConfigResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("setGsmBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setGsmBroadcastConfigResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setGsmBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setGsmBroadcastActivationResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("setGsmBroadcastActivationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setGsmBroadcastActivationResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setGsmBroadcastActivationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaBroadcastConfigResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responseLen) {
    RLOGD("getCdmaBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<CdmaBroadcastSmsConfigInfo> configs;

        if (response == NULL || responseLen % sizeof(RIL_CDMA_BroadcastSmsConfigInfo *) != 0) {
            RLOGE("getCdmaBroadcastConfigResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_CDMA_BroadcastSmsConfigInfo *);
            configs.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_CDMA_BroadcastSmsConfigInfo *resp =
                        ((RIL_CDMA_BroadcastSmsConfigInfo **) response)[i];
                configs[i].serviceCategory = resp->service_category;
                configs[i].language = resp->language;
                configs[i].selected = resp->selected == 1 ? true : false;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaBroadcastConfigResponse(responseInfo,
                configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCdmaBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaBroadcastConfigResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responseLen) {
    RLOGD("setCdmaBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaBroadcastConfigResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCdmaBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaBroadcastActivationResponse(int slotId,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("setCdmaBroadcastActivationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaBroadcastActivationResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCdmaBroadcastActivationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCDMASubscriptionResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e, void *response,
                                      size_t responseLen) {
    RLOGD("getCDMASubscriptionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        int numStrings = responseLen / sizeof(char *);
        hidl_string emptyString;
        if (response == NULL || numStrings != 5) {
            RLOGE("getOperatorResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getCDMASubscriptionResponse(
                    responseInfo, emptyString, emptyString, emptyString, emptyString, emptyString);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            char **resp = (char **) response;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getCDMASubscriptionResponse(
                    responseInfo,
                    convertCharPtrToHidlString(resp[0]),
                    convertCharPtrToHidlString(resp[1]),
                    convertCharPtrToHidlString(resp[2]),
                    convertCharPtrToHidlString(resp[3]),
                    convertCharPtrToHidlString(resp[4]));
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("getCDMASubscriptionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::writeSmsToRuimResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("writeSmsToRuimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->writeSmsToRuimResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("writeSmsToRuimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::deleteSmsOnRuimResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("deleteSmsOnRuimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->deleteSmsOnRuimResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("deleteSmsOnRuimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }
    return 0;
}

int radio::getDeviceIdentityResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("getDeviceIdentityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        int numStrings = responseLen / sizeof(char *);
        hidl_string emptyString;
        if (response == NULL || numStrings != 4) {
            RLOGE("getDeviceIdentityResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getDeviceIdentityResponse(responseInfo,
                    emptyString, emptyString, emptyString, emptyString);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            char **resp = (char **) response;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getDeviceIdentityResponse(responseInfo,
                    convertCharPtrToHidlString(resp[0]),
                    convertCharPtrToHidlString(resp[1]),
                    convertCharPtrToHidlString(resp[2]),
                    convertCharPtrToHidlString(resp[3]));
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("getDeviceIdentityResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::exitEmergencyCallbackModeResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("exitEmergencyCallbackModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->exitEmergencyCallbackModeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("exitEmergencyCallbackModeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getSmscAddressResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("getSmscAddressResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getSmscAddressResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getSmscAddressResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setEccListResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen) {
    RLOGD("setEccListResponse: slotId %d, serial %d, e %d", slotId, serial, e);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setEccListResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setEccListResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}
/// M: CC: @}

int radio::setSmscAddressResponse(int slotId,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("setSmscAddressResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setSmscAddressResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setSmscAddressResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::reportSmsMemoryStatusResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("reportSmsMemoryStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->reportSmsMemoryStatusResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("reportSmsMemoryStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::reportStkServiceIsRunningResponse(int slotId,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("reportStkServiceIsRunningResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                reportStkServiceIsRunningResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("reportStkServiceIsRunningResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaSubscriptionSourceResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("getCdmaSubscriptionSourceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaSubscriptionSourceResponse(
                responseInfo, (CdmaSubscriptionSource) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCdmaSubscriptionSourceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::requestIsimAuthenticationResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("requestIsimAuthenticationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestIsimAuthenticationResponse(
                responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("requestIsimAuthenticationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::acknowledgeIncomingGsmSmsWithPduResponse(int slotId,
                                                   int responseType,
                                                   int serial, RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("acknowledgeIncomingGsmSmsWithPduResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->acknowledgeIncomingGsmSmsWithPduResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("acknowledgeIncomingGsmSmsWithPduResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::sendEnvelopeWithStatusResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responseLen) {
    RLOGD("sendEnvelopeWithStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e,
                response, responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendEnvelopeWithStatusResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendEnvelopeWithStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getVoiceRadioTechnologyResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("getVoiceRadioTechnologyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getVoiceRadioTechnologyResponse(
                responseInfo, (RadioTechnology) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getVoiceRadioTechnologyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCellInfoListResponse(int slotId,
                                   int responseType,
                                   int serial, RIL_Errno e, void *response,
                                   size_t responseLen) {
    RLOGD("getCellInfoListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<CellInfo> ret;
        if (response == NULL || responseLen % sizeof(RIL_CellInfo_v12) != 0) {
            RLOGE("getCellInfoListResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilCellInfoListToHal(response, responseLen, ret);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getCellInfoListResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getCellInfoListResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setCellInfoListRateResponse(int slotId,
                                       int responseType,
                                       int serial, RIL_Errno e, void *response,
                                       size_t responseLen) {
    RLOGD("setCellInfoListRateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCellInfoListRateResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCellInfoListRateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setInitialAttachApnResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("setInitialAttachApnResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setInitialAttachApnResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setInitialAttachApnResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getImsRegistrationStateResponse(int slotId,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("getImsRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool isRegistered = false;
        int ratFamily = 0;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("getImsRegistrationStateResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            isRegistered = pInt[0] == 1 ? true : false;
            ratFamily = pInt[1];
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getImsRegistrationStateResponse(
                responseInfo, isRegistered, (RadioTechnologyFamily) ratFamily);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getImsRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendImsSmsResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("sendImsSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendImsSmsResponse(responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::iccTransmitApduBasicChannelResponse(int slotId,
                                               int responseType, int serial, RIL_Errno e,
                                               void *response, size_t responseLen) {
    RLOGD("iccTransmitApduBasicChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccTransmitApduBasicChannelResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("iccTransmitApduBasicChannelResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::iccOpenLogicalChannelResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responseLen) {
    RLOGD("iccOpenLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int channelId = -1;
        hidl_vec<int8_t> selectResponse;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("iccOpenLogicalChannelResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            channelId = pInt[0];
            selectResponse.resize(numInts - 1);
            for (int i = 1; i < numInts; i++) {
                selectResponse[i - 1] = (int8_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccOpenLogicalChannelResponse(responseInfo,
                channelId, selectResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("iccOpenLogicalChannelResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::iccCloseLogicalChannelResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("iccCloseLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccCloseLogicalChannelResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("iccCloseLogicalChannelResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::iccTransmitApduLogicalChannelResponse(int slotId,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("iccTransmitApduLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccTransmitApduLogicalChannelResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("iccTransmitApduLogicalChannelResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::nvReadItemResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen) {
    RLOGD("nvReadItemResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->nvReadItemResponse(
                responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvReadItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvWriteItemResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("nvWriteItemResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvWriteItemResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvWriteItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvWriteCdmaPrlResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("nvWriteCdmaPrlResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvWriteCdmaPrlResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvWriteCdmaPrlResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvResetConfigResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("nvResetConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvResetConfigResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nvResetConfigResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setUiccSubscriptionResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("setUiccSubscriptionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setUiccSubscriptionResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setUiccSubscriptionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setDataAllowedResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("setDataAllowedResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setDataAllowedResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setDataAllowedResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getHardwareConfigResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("getHardwareConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<HardwareConfig> result;
        if (response == NULL || responseLen % sizeof(RIL_HardwareConfig) != 0) {
            RLOGE("hardwareConfigChangedInd: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilHardwareConfigListToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getHardwareConfigResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getHardwareConfigResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::requestIccSimAuthenticationResponse(int slotId,
                                               int responseType, int serial, RIL_Errno e,
                                               void *response, size_t responseLen) {
    RLOGD("requestIccSimAuthenticationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestIccSimAuthenticationResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("requestIccSimAuthenticationResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setDataProfileResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("setDataProfileResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setDataProfileResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setDataProfileResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::requestShutdownResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("requestShutdownResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestShutdownResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("requestShutdownResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

void responseRadioCapability(RadioResponseInfo& responseInfo, int serial,
        int responseType, RIL_Errno e, void *response, size_t responseLen, RadioCapability& rc) {
    populateResponseInfo(responseInfo, serial, responseType, e);

    if (response == NULL || responseLen != sizeof(RIL_RadioCapability)) {
        RLOGE("responseRadioCapability: Invalid response");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        rc.logicalModemUuid = hidl_string();
    } else {
        convertRilRadioCapabilityToHal(response, responseLen, rc);
    }
}

int radio::getRadioCapabilityResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("getRadioCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        RadioCapability result = {};
        responseRadioCapability(responseInfo, serial, responseType, e, response, responseLen,
                result);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getRadioCapabilityResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getRadioCapabilityResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setRadioCapabilityResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("setRadioCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        RadioCapability result = {};
        responseRadioCapability(responseInfo, serial, responseType, e, response, responseLen,
                result);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setRadioCapabilityResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setRadioCapabilityResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

LceStatusInfo responseLceStatusInfo(RadioResponseInfo& responseInfo, int serial, int responseType,
                                    RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    LceStatusInfo result = {};

    if (response == NULL || responseLen != sizeof(RIL_LceStatusInfo)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        RIL_LceStatusInfo *resp = (RIL_LceStatusInfo *) response;
        result.lceStatus = (LceStatus) resp->lce_status;
        result.actualIntervalMs = (uint8_t) resp->actual_interval_ms;
    }

    return result;
}

int radio::startLceServiceResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("startLceServiceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        LceStatusInfo result = responseLceStatusInfo(responseInfo, serial, responseType, e,
                response, responseLen);

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::SIM_ABSENT;
        }
        // M @}

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->startLceServiceResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("startLceServiceResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::stopLceServiceResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("stopLceServiceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        LceStatusInfo result = responseLceStatusInfo(responseInfo, serial, responseType, e,
                response, responseLen);

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->stopLceServiceResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stopLceServiceResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::pullLceDataResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("pullLceDataResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        LceDataInfo result = {};
        if (response == NULL || responseLen != sizeof(RIL_LceDataInfo)) {
            RLOGE("pullLceDataResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilLceDataInfoToHal(response, responseLen, result);
        }

        // M @{
        if (s_cardStatus[slotId].cardState == CardState::ABSENT) {
            responseInfo.error = RadioError::NONE;
        }
        // M @}

        Return<void> retStatus = radioService[slotId]->mRadioResponse->pullLceDataResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("pullLceDataResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getModemActivityInfoResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("getModemActivityInfoResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        ActivityStatsInfo info;
        if (response == NULL || responseLen != sizeof(RIL_ActivityStatsInfo)) {
            RLOGE("getModemActivityInfoResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_ActivityStatsInfo *resp = (RIL_ActivityStatsInfo *)response;
            info.sleepModeTimeMs = resp->sleep_mode_time_ms;
            info.idleModeTimeMs = resp->idle_mode_time_ms;
            for(int i = 0; i < RIL_NUM_TX_POWER_LEVELS; i++) {
                info.txmModetimeMs[i] = resp->tx_mode_time_ms[i];
            }
            info.rxModeTimeMs = resp->rx_mode_time_ms;
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getModemActivityInfoResponse(responseInfo,
                info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getModemActivityInfoResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setAllowedCarriersResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("setAllowedCarriersResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setAllowedCarriersResponse(responseInfo,
                ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setAllowedCarriersResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getAllowedCarriersResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("getAllowedCarriersResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        CarrierRestrictions carrierInfo = {};
        bool allAllowed = true;
        if (response == NULL || responseLen != sizeof(RIL_CarrierRestrictions)) {
            RLOGE("getAllowedCarriersResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_CarrierRestrictions *pCr = (RIL_CarrierRestrictions *)response;
            if (pCr->len_allowed_carriers > 0 || pCr->len_excluded_carriers > 0) {
                allAllowed = false;
            }

            carrierInfo.allowedCarriers.resize(pCr->len_allowed_carriers);
            for(int i = 0; i < pCr->len_allowed_carriers; i++) {
                RIL_Carrier *carrier = pCr->allowed_carriers + i;
                carrierInfo.allowedCarriers[i].mcc = convertCharPtrToHidlString(carrier->mcc);
                carrierInfo.allowedCarriers[i].mnc = convertCharPtrToHidlString(carrier->mnc);
                carrierInfo.allowedCarriers[i].matchType = (CarrierMatchType) carrier->match_type;
                carrierInfo.allowedCarriers[i].matchData =
                        convertCharPtrToHidlString(carrier->match_data);
            }

            carrierInfo.excludedCarriers.resize(pCr->len_excluded_carriers);
            for(int i = 0; i < pCr->len_excluded_carriers; i++) {
                RIL_Carrier *carrier = pCr->excluded_carriers + i;
                carrierInfo.excludedCarriers[i].mcc = convertCharPtrToHidlString(carrier->mcc);
                carrierInfo.excludedCarriers[i].mnc = convertCharPtrToHidlString(carrier->mnc);
                carrierInfo.excludedCarriers[i].matchType = (CarrierMatchType) carrier->match_type;
                carrierInfo.excludedCarriers[i].matchData =
                        convertCharPtrToHidlString(carrier->match_data);
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAllowedCarriersResponse(responseInfo,
                allAllowed, carrierInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getAllowedCarriersResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendDeviceStateResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen) {
    RLOGD("sendDeviceStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendDeviceStateResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendDeviceStateResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setCarrierInfoForImsiEncryptionResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("setCarrierInfoForImsiEncryptionResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseV1_1 != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseV1_1->
                setCarrierInfoForImsiEncryptionResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setCarrierInfoForImsiEncryptionResponse: radioService[%d]->mRadioResponseV1_1 == "
                "NULL", slotId);
    }
    return 0;
}

int radio::setIndicationFilterResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen) {
    RLOGD("setIndicationFilterResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setIndicationFilterResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setIndicationFilterResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setSimCardPowerResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
#if VDBG
    RLOGD("setSimCardPowerResponse: serial %d", serial);
#endif

    if (radioService[slotId]->mRadioResponse != NULL
            || radioService[slotId]->mRadioResponseV1_1 != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        if (radioService[slotId]->mRadioResponseV1_1 != NULL) {
            Return<void> retStatus = radioService[slotId]->mRadioResponseV1_1->
                    setSimCardPowerResponse_1_1(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGD("setSimCardPowerResponse: radioService[%d]->mRadioResponseV1_1 == NULL",
                    slotId);
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->setSimCardPowerResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("setSimCardPowerResponse: radioService[%d]->mRadioResponse == NULL && "
                "radioService[%d]->mRadioResponseV1_1 == NULL", slotId, slotId);
    }
    return 0;
}

int radio::startNetworkScanResponse(int slotId, int responseType, int serial, RIL_Errno e,
                                    void *response, size_t responseLen) {
#if VDBG
    RLOGD("startNetworkScanResponse: serial %d", serial);
#endif

    if (radioService[slotId]->mRadioResponseV1_1 != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseV1_1->startNetworkScanResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("startNetworkScanResponse: radioService[%d]->mRadioResponseV1_1 == NULL", slotId);
    }

    return 0;
}

int radio::stopNetworkScanResponse(int slotId, int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
#if VDBG
    RLOGD("stopNetworkScanResponse: serial %d", serial);
#endif

    if (radioService[slotId]->mRadioResponseV1_1 != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseV1_1->stopNetworkScanResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stopNetworkScanResponse: radioService[%d]->mRadioResponseV1_1 == NULL", slotId);
    }

    return 0;
}

void convertRilKeepaliveStatusToHal(const RIL_KeepaliveStatus *rilStatus,
        AOSP_V1_1::KeepaliveStatus& halStatus) {
    halStatus.sessionHandle = rilStatus->sessionHandle;
    halStatus.code = static_cast<AOSP_V1_1::KeepaliveStatusCode>(rilStatus->code);
}

int radio::startKeepaliveResponse(int slotId, int responseType, int serial, RIL_Errno e,
                                    void *response, size_t responseLen) {
#if VDBG
    RLOGD("%s(): %d", __FUNCTION__, serial);
#endif
    RadioResponseInfo responseInfo = {};
    populateResponseInfo(responseInfo, serial, responseType, e);

    // If we don't have a radio service, there's nothing we can do
    if (radioService[slotId]->mRadioResponseV1_1 == NULL) {
        RLOGE("%s: radioService[%d]->mRadioResponseV1_1 == NULL", __FUNCTION__, slotId);
        return 0;
    }

    AOSP_V1_1::KeepaliveStatus ks = {};
    if (response == NULL || responseLen != sizeof(AOSP_V1_1::KeepaliveStatus)) {
        RLOGE("%s: invalid response - %d", __FUNCTION__, static_cast<int>(e));
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        convertRilKeepaliveStatusToHal(static_cast<RIL_KeepaliveStatus*>(response), ks);
    }

    Return<void> retStatus =
            radioService[slotId]->mRadioResponseV1_1->startKeepaliveResponse(responseInfo, ks);
    radioService[slotId]->checkReturnStatus(retStatus);
    return 0;
}

int radio::stopKeepaliveResponse(int slotId, int responseType, int serial, RIL_Errno e,
                                    void *response, size_t responseLen) {
#if VDBG
    RLOGD("%s(): %d", __FUNCTION__, serial);
#endif
    RadioResponseInfo responseInfo = {};
    populateResponseInfo(responseInfo, serial, responseType, e);

    // If we don't have a radio service, there's nothing we can do
    if (radioService[slotId]->mRadioResponseV1_1 == NULL) {
        RLOGE("%s: radioService[%d]->mRadioResponseV1_1 == NULL", __FUNCTION__, slotId);
        return 0;
    }

    Return<void> retStatus =
            radioService[slotId]->mRadioResponseV1_1->stopKeepaliveResponse(responseInfo);
    radioService[slotId]->checkReturnStatus(retStatus);
    return 0;
}

int radio::sendRequestRawResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
   RLOGD("sendRequestRawResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<uint8_t> data;

        if (response == NULL) {
            RLOGE("sendRequestRawResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            data.setToExternal((uint8_t *) response, responseLen);
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                sendRequestRawResponse(responseInfo, data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("sendRequestRawResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::sendRequestStringsResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("sendRequestStringsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> data;

        if ((response == NULL && responseLen != 0) || responseLen % sizeof(char *) != 0) {
            RLOGE("sendRequestStringsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            data.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                data[i] = convertCharPtrToHidlString(resp[i]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->sendRequestStringsResponse(
                responseInfo, data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("sendRequestStringsResponse: radioService[%d]->mRadioResponseMtk == "
                "NULL", slotId);
    }

    return 0;
}

// ATCI Start
int radio::sendAtciResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("sendAtciResponse: serial %d", serial);
    if (radioService[slotId] != NULL && radioService[slotId]->mAtciResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<uint8_t> data;

        if (response == NULL) {
            RLOGE("sendAtciResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            data.setToExternal((uint8_t *) response, responseLen);
        }
        Return<void> retStatus = radioService[slotId]->mAtciResponse->
                sendAtciResponse(responseInfo, data);
        if (!retStatus.isOk()) {
            RLOGE("sendAtciResponse: unable to call response callback");
            radioService[slotId]->mAtciResponse = NULL;
            radioService[slotId]->mAtciIndication = NULL;
        }
    } else {
        RLOGE("sendAtciResponse: radioService[%d]->mAtciResponse == NULL", slotId);
    }

    return 0;
}
// ATCI End

/// M: CC: call control @{
int radio::hangupAllResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responselen) {
    RLOGD("hangupAllResponse: serial %d", serial);
    if(isImsSlot(slotId)) {

        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseIms->hangupAllResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("hangupAllResponse: radioService[%d]->mRadioResponseIms == NULL",
                    slotId);
        }
    }
    else {

        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseMtk->hangupAllResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("hangupAllResponse: radioService[%d]->mRadioResponseMtk == NULL",
                    slotId);
        }
    }

    return 0;
}

int radio::setCallIndicationResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responselen) {
    RLOGD("setCallIndicationResponse: serial %d", serial);
    if(isImsSlot(slotId)) {

        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseIms->
                                     setCallIndicationResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setCallIndicationResponse: radioService[%d]->mRadioResponseIms == NULL",
                    slotId);
        }
    }
    else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseMtk->
                                     setCallIndicationResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setCallIndicationResponse: radioService[%d]->mRadioResponseMtk == NULL",
                    slotId);
        }
    }

    return 0;
}

int radio::emergencyDialResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responselen) {
    RLOGD("emergencyDialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->emergencyDialResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("emergencyDialResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::setEccServiceCategoryResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responselen) {
    RLOGD("setEccServiceCategoryResponse: serial %d", serial);
    if(isImsSlot(slotId)) {

        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseIms->
                                     setEccServiceCategoryResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setEccServiceCategoryResponse: radioService[%d]->mRadioResponseIms == NULL",
                    slotId);
        }
    }
    else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseMtk->
                                     setEccServiceCategoryResponse(responseInfo);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("setEccServiceCategoryResponse: radioService[%d]->mRadioResponseMtk == NULL",
                    slotId);
        }
    }

    return 0;
}
/// M: CC: @}

int radio::vtDialResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("vtDialResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseMtk->
                                 vtDialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("vtDialResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::voiceAcceptResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("voiceAcceptResponse: serial %d", serial);
    if(isImsSlot(slotId)) {
        if (radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]
                                     ->mRadioResponseIms
                                     ->voiceAcceptResponse(responseInfo);

            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("voiceAcceptResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                             slotId);
        }
    }
    else {
        if (radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]
                                     ->mRadioResponseMtk
                                     ->voiceAcceptResponse(responseInfo);

            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("voiceAcceptResponse: radioService[%d]->mRadioResponseMtk == NULL",
                                                                             slotId);
        }
    }

    return 0;
}

int radio::replaceVtCallResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("replaceVtCallResponse: serial %d", serial);
    if(isImsSlot(slotId)) {
        if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseIms != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseIms->
                                     replaceVtCallResponse(responseInfo);

            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("replaceVtCallResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
        }
    }
    else {
        if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
            RadioResponseInfo responseInfo = {};
            populateResponseInfo(responseInfo, serial, responseType, e);
            Return<void> retStatus = radioService[slotId]->
                                     mRadioResponseMtk->
                                     replaceVtCallResponse(responseInfo);

            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            RLOGE("replaceVtCallResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
        }
    }

    return 0;
}

/// M: CC: E911 request current status response
int radio::currentStatusResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("currentStatusResponse: serial %d", serial);
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseMtk->
                                 currentStatusResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("currentStatusResponse: radioService[%d]->mRadioResponseMtk == NULL",
                                                                           slotId);
    }

    return 0;
}

/// M: CC: E911 request set ECC preferred RAT.
int radio::eccPreferredRatResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("eccPreferredRatResponse: serial %d", serial);
    return 0;
}

// PHB START , interface only currently.
int radio::queryPhbStorageInfoResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("queryPhbStorageInfoResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> storageInfo;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("queryPhbStorageInfoResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            storageInfo.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                storageInfo[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->queryPhbStorageInfoResponse(responseInfo,
                storageInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("queryPhbStorageInfoResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::writePhbEntryResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("writePhbEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->writePhbEntryResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("writePhbEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

void convertRilPhbEntryStructureToHal(void *response, size_t responseLen,
        hidl_vec<PhbEntryStructure>& resultList) {
    int num = responseLen / sizeof(RIL_PhbEntryStructure*);
    RIL_PhbEntryStructure **phbEntryResponse = (RIL_PhbEntryStructure **) response;
    resultList.resize(num);
    for (int i = 0; i < num; i++) {
        resultList[i].type = phbEntryResponse[i]->type;
        resultList[i].index = phbEntryResponse[i]->index;
        resultList[i].number = convertCharPtrToHidlString(phbEntryResponse[i]->number);
        resultList[i].ton = phbEntryResponse[i]->ton;
        resultList[i].alphaId = convertCharPtrToHidlString(phbEntryResponse[i]->alphaId);
    }
}

int radio::readPhbEntryResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen) {
    RLOGD("readPhbEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<PhbEntryStructure> result;
        if (response == NULL || responseLen % sizeof(RIL_PhbEntryStructure*) != 0) {
            RLOGE("readPhbEntryResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilPhbEntryStructureToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->readPhbEntryResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readPhbEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::queryUPBCapabilityResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("queryUPBCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> upbCapability;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("queryUPBCapabilityResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            upbCapability.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                upbCapability[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->queryUPBCapabilityResponse(responseInfo,
                upbCapability);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("queryUPBCapabilityResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::editUPBEntryResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("editUPBEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->editUPBEntryResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("editUPBEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::deleteUPBEntryResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("deleteUPBEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->deleteUPBEntryResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("deleteUPBEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::readUPBGasListResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("readUPBGasListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> gasList;
        int numStrings = responseLen / sizeof(char*);
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("readUPBGasListResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **pString = (char **) response;
            gasList.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                gasList[i] = convertCharPtrToHidlString(pString[i]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBGasListResponse(responseInfo,
                gasList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBGasListResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::readUPBGrpEntryResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("readUPBGrpEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> grpEntries;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("readUPBGrpEntryResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            grpEntries.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                grpEntries[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBGrpEntryResponse(responseInfo,
                grpEntries);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBGrpEntryResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::writeUPBGrpEntryResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("writeUPBGrpEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->writeUPBGrpEntryResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("writeUPBGrpEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::getPhoneBookStringsLengthResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("getPhoneBookStringsLengthResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> stringLengthInfo;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("getPhoneBookStringsLengthResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            stringLengthInfo.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                stringLengthInfo[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBGrpEntryResponse(responseInfo,
                stringLengthInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPhoneBookStringsLengthResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::getPhoneBookMemStorageResponse(int slotId,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("getPhoneBookMemStorageResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        PhbMemStorageResponse phbMemStorage;
        if (response == NULL || responseLen != sizeof(RIL_PHB_MEM_STORAGE_RESPONSE)) {
            RLOGE("getPhoneBookMemStorageResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_PHB_MEM_STORAGE_RESPONSE *resp = (RIL_PHB_MEM_STORAGE_RESPONSE *)response;
            phbMemStorage.storage = convertCharPtrToHidlString(resp->storage);
            phbMemStorage.used = resp->used;
            phbMemStorage.total = resp->total;
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getPhoneBookMemStorageResponse(responseInfo,
                phbMemStorage);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPhoneBookMemStorageResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::setPhoneBookMemStorageResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("setPhoneBookMemStorageResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setPhoneBookMemStorageResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setPhoneBookMemStorageResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

void convertRilPhbEntryExtStrucutreToHal(void *response, size_t responseLen,
        hidl_vec<PhbEntryExt>& resultList) {
    int num = responseLen / sizeof(RIL_PHB_ENTRY *);

    RIL_PHB_ENTRY **phbEntryExtResponse = (RIL_PHB_ENTRY **) response;
    resultList.resize(num);
    for (int i = 0; i < num; i++) {
        resultList[i].index = phbEntryExtResponse[i]->index;
        resultList[i].number = convertCharPtrToHidlString(phbEntryExtResponse[i]->number);
        resultList[i].type = phbEntryExtResponse[i]->type;
        resultList[i].text = convertCharPtrToHidlString(phbEntryExtResponse[i]->text);
        resultList[i].hidden = phbEntryExtResponse[i]->hidden;
        resultList[i].group = convertCharPtrToHidlString(phbEntryExtResponse[i]->group);
        resultList[i].adnumber = convertCharPtrToHidlString(phbEntryExtResponse[i]->adnumber);
        resultList[i].adtype = phbEntryExtResponse[i]->adtype;
        resultList[i].secondtext = convertCharPtrToHidlString(phbEntryExtResponse[i]->secondtext);
        resultList[i].email = convertCharPtrToHidlString(phbEntryExtResponse[i]->email);
    }
}

int radio::readPhoneBookEntryExtResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen) {
    RLOGD("readPhoneBookEntryExtResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<PhbEntryExt> result;
        if (response == NULL || responseLen % sizeof(RIL_PHB_ENTRY *) != 0) {
            RLOGE("readPhoneBookEntryExtResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilPhbEntryExtStrucutreToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->readPhoneBookEntryExtResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readPhoneBookEntryExtResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::writePhoneBookEntryExtResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("writePhoneBookEntryExtResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->writePhoneBookEntryExtResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("writePhoneBookEntryExtResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::queryUPBAvailableResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("queryUPBAvailableResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> upbAvailable;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("queryUPBAvailableResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            upbAvailable.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                upbAvailable[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->queryUPBAvailableResponse(responseInfo,
                upbAvailable);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("queryUPBAvailableResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::readUPBEmailEntryResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("readUPBEmailEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBEmailEntryResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBEmailEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::readUPBSneEntryResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("readUPBSneEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBSneEntryResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBSneEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::readUPBAnrEntryResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen) {
    RLOGD("readUPBAnrEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<PhbEntryStructure> result;
        if (response == NULL || responseLen % sizeof(RIL_PhbEntryStructure*) != 0) {
            RLOGE("readPhbEntryResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilPhbEntryStructureToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->readUPBAnrEntryResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBAnrEntryResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::readUPBAasListResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("readUPBAasListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> aasList;
        int numStrings = responseLen / sizeof(char *);
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("readUPBAasListResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **pString = (char **) response;
            aasList.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                aasList[i] = convertCharPtrToHidlString(pString[i]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->readUPBAasListResponse(responseInfo,
                aasList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("readUPBAasListResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}
// PHB END

/// [IMS] IMS Response Start
int radio::imsEmergencyDialResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e,
                                    void *response, size_t responselen) {
    RLOGD("imsEmergencyDialResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 emergencyDialResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEmergencyDialResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                              slotId);
    }

    return 0;
}

int radio::imsDialResponse(int slotId,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responseLen) {
    RLOGD("imsDialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 dialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsDialResponse: radioService[%d]->mRadioResponseIms == NULL", slotId);
    }

    return 0;
}

int radio::imsVtDialResponse(int slotId, int responseType,
                             int serial, RIL_Errno e, void *response,
                             size_t responselen) {
    RLOGD("imsVtDialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 vtDialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsVtDialResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                       slotId);
    }

    return 0;
}
int radio::videoCallAcceptResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("acceptVideoCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 videoCallAcceptResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEctCommandResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                           slotId);
    }
    return 0;
}
int radio::imsEctCommandResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("imsEctCommandResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 imsEctCommandResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEctCommandResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                           slotId);
    }
    return 0;
}
int radio::holdCallResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("holdResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 holdCallResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("holdCallResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                      slotId);
    }
    return 0;
}
int radio::resumeCallResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("resumeResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 resumeCallResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("resumeCallResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                        slotId);
    }
    return 0;
}
int radio::imsDeregNotificationResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("imsDeregNotificationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 imsDeregNotificationResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsDeregNotificationResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                                  slotId);
    }

    return 0;
}

int radio::setImsEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setImsEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setImsEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                slotId);
    }
    return 0;
}

int radio::setVolteEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setVolteEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setVolteEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setVolteEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                slotId);
    }
    return 0;
}

int radio::setWfcEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setWfcEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setWfcEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setWfcEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                          slotId);
    }
    return 0;
}
int radio::setVilteEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setVilteEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setVilteEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setVilteEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                            slotId);
    }

    return 0;
}
int radio::setViWifiEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setViWifiEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setViWifiEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setViWifiEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                             slotId);
    }

    return 0;
}
int radio::setImsVoiceEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("setImsVoiceEnableResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setImsVoiceEnableResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsVoiceEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                               slotId);
    }

    return 0;
}

int radio::setImsVideoEnableResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setImsVideoEnableResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setImsVideoEnableResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsVideoEnableResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                               slotId);
    }

    return 0;
}

int radio::setImscfgResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setImscfgResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setImscfgResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImscfgResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                       slotId);
    }

    return 0;
}

int radio::setModemImsCfgResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setModemImsCfgResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setModemImsCfgResponse(responseInfo,
                                 convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsModemCfgResponse: radioService[%d]->mRadioResponseIms == NULL",
                slotId);
    }

    return 0;
}


int radio::getProvisionValueResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("getProvisionValueResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 getProvisionValueResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getProvisionValueResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                               slotId);
    }

    return 0;
}

int radio::setProvisionValueResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setProvisionValueResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setProvisionValueResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setProvisionValueResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                               slotId);
    }

    return 0;
}

int radio::addImsConferenceCallMemberResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("addImsConfCallMemberRsp: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 addImsConferenceCallMemberResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("addImsConfCallMemberRsp: radioService[%d]->mRadioResponseIms == NULL",
                                                                             slotId);
    }

    return 0;
}

int radio::removeImsConferenceCallMemberResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("removeImsConfCallMemberRsp: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        Return<void> retStatus = radioService[slotId]->
                               mRadioResponseIms->
                               removeImsConferenceCallMemberResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("removeImsConfCallMemberRsp: radioService[%d]->mRadioResponseIms == NULL",
                                                                                slotId);
    }

    return 0;
}
int radio::setWfcProfileResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("setWfcProfileResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setWfcProfileResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setWfcProfileResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                           slotId);
    }

    return 0;
}
int radio::conferenceDialResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("conferenceDialResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 conferenceDialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("conferenceDialResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                            slotId);
    }

    return 0;
}
int radio::vtDialWithSipUriResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("vtDialWithSipUriResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 vtDialWithSipUriResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("vtDialWithSipUriResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                              slotId);
    }

    return 0;
}
int radio::dialWithSipUriResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("dialWithSipUriResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 dialWithSipUriResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("dialWithSipUriResponse: radioService[%d]->mRadioResponseIms == NULL",
                slotId);
    }

    return 0;
}
int radio::sendUssiResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("sendUssiResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]
                                 ->mRadioResponseIms
                                 ->sendUssiResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendUssiResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                      slotId);
    }

    return 0;
}

int radio::cancelUssiResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {
    RLOGD("cancelUssiResponse: serial %d", serial);
    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]
                                 ->mRadioResponseIms
                                 ->cancelUssiResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cancelUssiResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                        slotId);
    }

    return 0;
}
int radio::forceReleaseCallResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("forceReleaseCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 forceReleaseCallResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("forceReleaseCallResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                              slotId);
    }
    return 0;
}

int radio::setImsRtpReportResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response,
                            size_t responselen) {

    RLOGD("setImsRtpReportResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setImsRtpReportResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsRtpReportResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                             slotId);
    }

    return 0;
}

int radio::imsBearerActivationDoneResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("imsBearerActiveResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 imsBearerActivationDoneResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsBearerActiveResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                             slotId);
    }

    return 0;
}

int radio::imsBearerDeactivationDoneResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen) {

    RLOGD("imsBearerDeactiveResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 imsBearerDeactivationDoneResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsBearerDeactiveResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                               slotId);
    }

    return 0;
}

int radio::pullCallResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responselen) {
    RLOGD("pullCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseIms
                                                     ->pullCallResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("pullCallResponse: radioService[%d]->mRadioResponseIms == NULL",
                                                                      slotId);
    }
    return 0;
}

int radio::setImsRegistrationReportResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responselen) {
    RLOGD("setImsRegistrationReportRsp: serial %d", serial);


    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseIms
                                                     ->setImsRegistrationReportResponse(
                                                       responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setImsRegistrationReportRsp: radioService[%d]->mRadioResponseIms == NULL",
                                                                                 slotId);
    }
    return 0;
}

// World Phone {
int radio::setResumeRegistrationResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("setResumeRegistrationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setResumeRegistrationResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setResumeRegistrationResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::storeModemTypeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("storeModemTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->storeModemTypeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("storeModemTypeResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::reloadModemTypeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("reloadModemTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->reloadModemTypeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("reloadModemTypeResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

// World Phone }

// Radio Indication functions

RadioIndicationType convertIntToRadioIndicationType(int indicationType) {
    return indicationType == RESPONSE_UNSOLICITED ? (RadioIndicationType::UNSOLICITED) :
            (RadioIndicationType::UNSOLICITED_ACK_EXP);
}

int radio::radioStateChangedInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responseLen) {

    if(s_vendorFunctions == NULL) {
        RLOGE("radioStateChangedInd: service is not ready");
        return 0;
    }

    // Retrive Radio State
    RadioState radioState = (RadioState) s_vendorFunctions->
                                         onStateRequest((RIL_SOCKET_ID)slotId);
    RLOGD("radioStateChangedInd: radioState %d, slot = %d", radioState, slotId);

    // Send to RILJ
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->radioStateChanged(
                convertIntToRadioIndicationType(indicationType), radioState);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        Return<void> retStatus = radioService[slotId]->mRadioIndication->radioStateChanged(
                convertIntToRadioIndicationType(indicationType), radioState);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radioStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    // Send to IMS
    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL && radioService[imsSlotId]->mRadioIndication != NULL) {

        Return<void> retStatus = radioService[imsSlotId]->mRadioIndication->radioStateChanged(
                convertIntToRadioIndicationType(indicationType), radioState);
        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radioStateChangedInd: radioService[%d]->mRadioIndication == NULL", imsSlotId);
    }

    return 0;
}

int radio::callStateChangedInd(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("callStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->callStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("callStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::networkStateChangedInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("networkStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->networkStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("networkStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::responseFemtocellInfo(int slotId,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("responseFemtocellInfo: invalid indication");
            return 0;
        }

        hidl_vec<hidl_string> info;
        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        info.resize(numStrings);
        for (int i = 0; i < numStrings; i++) {
            info[i] = convertCharPtrToHidlString(resp[i]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->responseFemtocellInfo(
                convertIntToRadioIndicationType(indicationType), info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responseFemtocellInfo: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

uint8_t hexCharToInt(uint8_t c) {
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);

    return INVALID_HEX_CHAR;
}

uint8_t * convertHexStringToBytes(void *response, size_t responseLen) {
    if (responseLen % 2 != 0) {
        return NULL;
    }

    uint8_t *bytes = (uint8_t *)calloc(responseLen/2, sizeof(uint8_t));
    if (bytes == NULL) {
        RLOGE("convertHexStringToBytes: cannot allocate memory for bytes string");
        return NULL;
    }
    uint8_t *hexString = (uint8_t *)response;

    for (size_t i = 0; i < responseLen; i += 2) {
        uint8_t hexChar1 = hexCharToInt(hexString[i]);
        uint8_t hexChar2 = hexCharToInt(hexString[i + 1]);

        if (hexChar1 == INVALID_HEX_CHAR || hexChar2 == INVALID_HEX_CHAR) {
            RLOGE("convertHexStringToBytes: invalid hex char %d %d",
                    hexString[i], hexString[i + 1]);
            free(bytes);
            return NULL;
        }
        bytes[i/2] = ((hexChar1 << 4) | hexChar2);
    }

    return bytes;
}

int radio::newSmsInd(int slotId, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("newSmsInd: invalid response");
            return 0;
        }

        uint8_t *bytes = convertHexStringToBytes(response, responseLen);
        if (bytes == NULL) {
            RLOGE("newSmsInd: convertHexStringToBytes failed");
            return 0;
        }

        hidl_vec<uint8_t> pdu;
        pdu.setToExternal(bytes, responseLen/2);
        RLOGD("newSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSms(
                convertIntToRadioIndicationType(indicationType), pdu);
        radioService[slotId]->checkReturnStatus(retStatus);
        free(bytes);
    } else {
        RLOGE("newSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsStatusReportInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("newSmsStatusReportInd: invalid response");
            return 0;
        }

        uint8_t *bytes = convertHexStringToBytes(response, responseLen);
        if (bytes == NULL) {
            RLOGE("newSmsStatusReportInd: convertHexStringToBytes failed");
            return 0;
        }

        hidl_vec<uint8_t> pdu;
        pdu.setToExternal(bytes, responseLen/2);
        RLOGD("newSmsStatusReportInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSmsStatusReport(
                convertIntToRadioIndicationType(indicationType), pdu);
        radioService[slotId]->checkReturnStatus(retStatus);
        free(bytes);
    } else {
        RLOGE("newSmsStatusReportInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsOnSimInd(int slotId, int indicationType,
                          int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("newSmsOnSimInd: invalid response");
            return 0;
        }
        int32_t recordNumber = ((int32_t *) response)[0];
        RLOGD("newSmsOnSimInd: slotIndex %d", recordNumber);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSmsOnSim(
                convertIntToRadioIndicationType(indicationType), recordNumber);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("newSmsOnSimInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::onUssdInd(int slotId, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != 2 * sizeof(char *)) {
            RLOGE("onUssdInd: invalid response");
            return 0;
        }
        char **strings = (char **) response;
        char *mode = strings[0];
        hidl_string msg = convertCharPtrToHidlString(strings[1]);
        UssdModeType modeType = (UssdModeType) atoi(mode);
        RLOGD("onUssdInd: mode %s", mode);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->onUssd(
                convertIntToRadioIndicationType(indicationType), modeType, msg);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onUssdInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::nitzTimeReceivedInd(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("nitzTimeReceivedInd: invalid response");
            return 0;
        }
        hidl_string nitzTime = convertCharPtrToHidlString((char *) response);
        int64_t timeReceived = android::elapsedRealtime();
        RLOGD("nitzTimeReceivedInd: nitzTime %s receivedTime %" PRId64, nitzTime.c_str(),
                timeReceived);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->nitzTimeReceived(
                convertIntToRadioIndicationType(indicationType), nitzTime, timeReceived);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("nitzTimeReceivedInd: radioService[%d]->mRadioIndication == NULL", slotId);
        return -1;
    }

    return 0;
}

void convertRilSignalStrengthToHal(void *response, size_t responseLen,
        SignalStrength& signalStrength) {
    RIL_SignalStrength_v10 *rilSignalStrength = (RIL_SignalStrength_v10 *) response;

    // Fixup LTE for backwards compatibility
    // signalStrength: -1 -> 99
    if (rilSignalStrength->LTE_SignalStrength.signalStrength == -1) {
        rilSignalStrength->LTE_SignalStrength.signalStrength = 99;
    }
    // rsrp: -1 -> INT_MAX all other negative value to positive.
    // So remap here
    if (rilSignalStrength->LTE_SignalStrength.rsrp == -1) {
        rilSignalStrength->LTE_SignalStrength.rsrp = INT_MAX;
    } else if (rilSignalStrength->LTE_SignalStrength.rsrp < -1) {
        rilSignalStrength->LTE_SignalStrength.rsrp = -rilSignalStrength->LTE_SignalStrength.rsrp;
    }
    // rsrq: -1 -> INT_MAX
    if (rilSignalStrength->LTE_SignalStrength.rsrq == -1) {
        rilSignalStrength->LTE_SignalStrength.rsrq = INT_MAX;
    }
    // Not remapping rssnr is already using INT_MAX
    // cqi: -1 -> INT_MAX
    if (rilSignalStrength->LTE_SignalStrength.cqi == -1) {
        rilSignalStrength->LTE_SignalStrength.cqi = INT_MAX;
    }

    signalStrength.gw.signalStrength = rilSignalStrength->GW_SignalStrength.signalStrength;
    signalStrength.gw.bitErrorRate = rilSignalStrength->GW_SignalStrength.bitErrorRate;
    signalStrength.cdma.dbm = rilSignalStrength->CDMA_SignalStrength.dbm;
    signalStrength.cdma.ecio = rilSignalStrength->CDMA_SignalStrength.ecio;
    signalStrength.evdo.dbm = rilSignalStrength->EVDO_SignalStrength.dbm;
    signalStrength.evdo.ecio = rilSignalStrength->EVDO_SignalStrength.ecio;
    signalStrength.evdo.signalNoiseRatio =
            rilSignalStrength->EVDO_SignalStrength.signalNoiseRatio;
    signalStrength.lte.signalStrength = rilSignalStrength->LTE_SignalStrength.signalStrength;
    signalStrength.lte.rsrp = rilSignalStrength->LTE_SignalStrength.rsrp;
    signalStrength.lte.rsrq = rilSignalStrength->LTE_SignalStrength.rsrq;
    signalStrength.lte.rssnr = rilSignalStrength->LTE_SignalStrength.rssnr;
    signalStrength.lte.cqi = rilSignalStrength->LTE_SignalStrength.cqi;
    signalStrength.lte.timingAdvance = rilSignalStrength->LTE_SignalStrength.timingAdvance;
    signalStrength.tdScdma.rscp = rilSignalStrength->TD_SCDMA_SignalStrength.rscp;
}

int radio::currentSignalStrengthInd(int slotId,
                                    int indicationType, int token, RIL_Errno e,
                                    void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SignalStrength_v10)) {
            RLOGE("currentSignalStrengthInd: invalid response");
            return 0;
        }

        SignalStrength signalStrength = {};
        convertRilSignalStrengthToHal(response, responseLen, signalStrength);

        RLOGD("currentSignalStrengthInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->currentSignalStrength(
                convertIntToRadioIndicationType(indicationType), signalStrength);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("currentSignalStrengthInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilDataCallToHal(RIL_Data_Call_Response_v11 *dcResponse,
        SetupDataCallResult& dcResult) {
    dcResult.status = (DataCallFailCause) dcResponse->status;
    dcResult.suggestedRetryTime = dcResponse->suggestedRetryTime;
    dcResult.cid = dcResponse->cid;
    dcResult.active = dcResponse->active;
    dcResult.type = convertCharPtrToHidlString(dcResponse->type);
    dcResult.ifname = convertCharPtrToHidlString(dcResponse->ifname);
    dcResult.addresses = convertCharPtrToHidlString(dcResponse->addresses);
    dcResult.dnses = convertCharPtrToHidlString(dcResponse->dnses);
    dcResult.gateways = convertCharPtrToHidlString(dcResponse->gateways);
    dcResult.pcscf = convertCharPtrToHidlString(dcResponse->pcscf);
    dcResult.mtu = dcResponse->mtu;
}

void convertRilDataCallToHalEx(RIL_Data_Call_Response_v11 *dcResponse,
        MtkSetupDataCallResult& dcResult) {
    dcResult.dcr.status = (DataCallFailCause) dcResponse->status;
    dcResult.dcr.suggestedRetryTime = dcResponse->suggestedRetryTime;
    dcResult.dcr.cid = dcResponse->cid;
    dcResult.dcr.active = dcResponse->active;
    dcResult.dcr.type = convertCharPtrToHidlString(dcResponse->type);
    dcResult.dcr.ifname = convertCharPtrToHidlString(dcResponse->ifname);
    dcResult.dcr.addresses = convertCharPtrToHidlString(dcResponse->addresses);
    dcResult.dcr.dnses = convertCharPtrToHidlString(dcResponse->dnses);
    dcResult.dcr.gateways = convertCharPtrToHidlString(dcResponse->gateways);
    dcResult.dcr.pcscf = convertCharPtrToHidlString(dcResponse->pcscf);
    dcResult.dcr.mtu = dcResponse->mtu;
    // M: [OD over ePDG] start
    dcResult.rat = dcResponse->eran_type;
    // M: [OD over ePDG] end
}

void convertRilDataCallListToHal(void *response, size_t responseLen,
        hidl_vec<SetupDataCallResult>& dcResultList) {
    int num = responseLen / sizeof(RIL_Data_Call_Response_v11);

    RIL_Data_Call_Response_v11 *dcResponse = (RIL_Data_Call_Response_v11 *) response;
    dcResultList.resize(num);
    for (int i = 0; i < num; i++) {
        convertRilDataCallToHal(&dcResponse[i], dcResultList[i]);
    }
}

void convertRilDataCallListToHalEx(void *response, size_t responseLen,
        hidl_vec<MtkSetupDataCallResult>& dcResultList) {
    int num = responseLen / sizeof(RIL_Data_Call_Response_v11);

    RIL_Data_Call_Response_v11 *dcResponse = (RIL_Data_Call_Response_v11 *) response;
    dcResultList.resize(num);
    for (int i = 0; i < num; i++) {
        convertRilDataCallToHalEx(&dcResponse[i], dcResultList[i]);
    }
}

int radio::dataCallListChangedInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("dataCallListChangedInd: invalid response");
            return 0;
        }
        hidl_vec<MtkSetupDataCallResult> dcList;
        convertRilDataCallListToHalEx(response, responseLen, dcList);
        RLOGD("dataCallListChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->dataCallListChangedEx(
                convertIntToRadioIndicationType(indicationType), dcList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("dataCallListChangedInd: invalid response");
            return 0;
        }
        hidl_vec<SetupDataCallResult> dcList;
        convertRilDataCallListToHal(response, responseLen, dcList);
        RLOGD("dataCallListChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->dataCallListChanged(
                convertIntToRadioIndicationType(indicationType), dcList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("dataCallListChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::suppSvcNotifyInd(int slotId, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SuppSvcNotification)) {
            RLOGE("suppSvcNotifyInd: invalid response");
            return 0;
        }

        SuppSvcNotification suppSvc = {};
        RIL_SuppSvcNotification *ssn = (RIL_SuppSvcNotification *) response;
        suppSvc.isMT = ssn->notificationType;
        suppSvc.code = ssn->code;
        suppSvc.index = ssn->index;
        suppSvc.type = ssn->type;
        suppSvc.number = convertCharPtrToHidlString(ssn->number);

        RLOGD("suppSvcNotifyInd: isMT %d code %d index %d type %d",
                suppSvc.isMT, suppSvc.code, suppSvc.index, suppSvc.type);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->suppSvcNotify(
                convertIntToRadioIndicationType(indicationType), suppSvc);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("suppSvcNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cfuStatusNotifyInd(int slotId, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != 2 * sizeof(int *)) {
            RLOGE("cfuStatusNotifyInd: invalid response");
            return 0;
        }

        CfuStatusNotification cfuStatus = {};
        int *csn = (int *) response;
        cfuStatus.status = csn[0];
        cfuStatus.lineId = csn[1];

        RLOGD("cfuStatusNotifyInd: status = %d, line = %d", cfuStatus.status, cfuStatus.lineId);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->cfuStatusNotify(
                convertIntToRadioIndicationType(indicationType), cfuStatus);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cfuStatusNotifyInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

/// M: CC: call control ([IMS] common flow) @{
int radio::incomingCallIndicationInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {

    if(response == NULL) {
        RLOGE("incomingCallIndicationInd: response is NULL");
        return 0;
    }

    char **resp = (char **) response;
    int numStrings = responseLen / sizeof(char *);
    if(numStrings < 7) {
        RLOGE("incomingCallIndicationInd: items length is invalid, slot = %d", slotId);
        return 0;
    }

    int mode = atoi(resp[3]);
    if(mode >= 20) {   // Code Mode >= 20, IMS's Call Info Indication, otherwise CS's
        int imsSlot = toImsSlot(slotId);
        if (radioService[imsSlot] != NULL &&
            radioService[imsSlot]->mRadioIndicationIms != NULL) {

            IncomingCallNotification inCallNotify = {};
            // EAIC: <callId>, <number>, <type>, <call mode>, <seq no>
            inCallNotify.callId = convertCharPtrToHidlString(resp[0]);
            inCallNotify.number = convertCharPtrToHidlString(resp[1]);
            inCallNotify.type = convertCharPtrToHidlString(resp[2]);
            inCallNotify.callMode = convertCharPtrToHidlString(resp[3]);
            inCallNotify.seqNo = convertCharPtrToHidlString(resp[4]);
            inCallNotify.redirectNumber = convertCharPtrToHidlString(resp[5]);

            RLOGD("incomingCallIndicationInd: %s, %s, %s, %s, %s, %s, slot = %d",
                    resp[0], resp[1], resp[2], resp[3], resp[4], resp[5], imsSlot);

            Return<void> retStatus = radioService[imsSlot]->
                                     mRadioIndicationIms->incomingCallIndication(
                                     convertIntToRadioIndicationType(indicationType),
                                     inCallNotify);
            radioService[imsSlot]->checkReturnStatus(retStatus);
        }
        else {
            RLOGE("incomingCallIndicationInd: service[%d]->mRadioIndicationIms == NULL",
                                                                                imsSlot);
        }

        return 0;
    }

    //      case RIL_UNSOL_INCOMING_CALL_INDICATION: ret = responseStrings(p); break;
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != 7 * sizeof(char *)) {
            RLOGE("incomingCallIndicationInd: invalid response");
            return 0;
        }
        IncomingCallNotification inCallNotify = {};
        char **strings = (char **) response;
        // EAIC: <callId>, <number>, <type>, <call mode>, <seq no>
        inCallNotify.callId = convertCharPtrToHidlString(strings[0]);
        inCallNotify.number = convertCharPtrToHidlString(strings[1]);
        inCallNotify.type = convertCharPtrToHidlString(strings[2]);
        inCallNotify.callMode = convertCharPtrToHidlString(strings[3]);
        inCallNotify.seqNo = convertCharPtrToHidlString(strings[4]);
        inCallNotify.redirectNumber = convertCharPtrToHidlString(strings[5]);
        // string[6] is used by ims. no need here.
        RLOGD("incomingCallIndicationInd: %s, %s, %s, %s, %s, %s",
                strings[0], strings[1], strings[2],
                strings[3], strings[4], strings[5]);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->incomingCallIndication(
                convertIntToRadioIndicationType(indicationType), inCallNotify);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("incomingCallIndicationInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }


    return 0;
}

/// M: CC: GSM 02.07 B.1.26 Ciphering Indicator support
int radio::cipherIndicationInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != 4 * sizeof(char *)) {
            RLOGE("cipherInd: invalid response");
            return 0;
        }
        CipherNotification cipherNotify = {};
        char **strings = (char **) response;
        //+ECIPH:  <sim_cipher_ind>,<mm_connection>,<cs_cipher_on>,<ps_cipher_on>
        cipherNotify.simCipherStatus = convertCharPtrToHidlString(strings[0]);
        cipherNotify.sessionStatus = convertCharPtrToHidlString(strings[1]);
        cipherNotify.csStatus = convertCharPtrToHidlString(strings[2]);
        cipherNotify.psStatus = convertCharPtrToHidlString(strings[3]);
        RLOGD("cipherInd: %s, %s, %s, %s", strings[0], strings[1], strings[2], strings[3]);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->cipherIndication(
                convertIntToRadioIndicationType(indicationType), cipherNotify);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cipherInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

/// M: CC: call control @{
int radio::crssNotifyInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CrssNotification)) {
            RLOGE("crssNotifyInd: invalid response");
            return 0;
        }
        CrssNotification crssNotify = {};
        RIL_CrssNotification *crss = (RIL_CrssNotification *) response;
        crssNotify.code = crss->code;
        crssNotify.type = crss->type;
        crssNotify.number = convertCharPtrToHidlString(crss->number);
        crssNotify.alphaid = convertCharPtrToHidlString(crss->alphaid);
        crssNotify.cli_validity = crss->cli_validity;

        RLOGD("crssNotifyInd: code %d type %d cli_validity %d",
                crssNotify.code, crssNotify.type, crssNotify.cli_validity);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->crssIndication(
                convertIntToRadioIndicationType(indicationType), crssNotify);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("crssNotifyInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

/// M: CC: For 3G VT only
int radio::vtStatusInfoInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("vtStatusInfoInd: invalid response");
            return 0;
        }
        int32_t info = ((int32_t *) response)[0];
        RLOGD("vtStatusInfoInd: %d", info);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->vtStatusInfoIndication(
                convertIntToRadioIndicationType(indicationType), info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("vtStatusInfoInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

/// M: CC: GSA HD Voice for 2/3G network support
int radio::speechCodecInfoInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("speechCodecInfoInd: invalid response");
            return 0;
        }
        int32_t info = ((int32_t *) response)[0];
        RLOGD("speechCodecInfoInd: %d", info);
        Return<void> retStatus =
                radioService[slotId]->mRadioIndicationMtk->speechCodecInfoIndication(
                convertIntToRadioIndicationType(indicationType), info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("speechCodecInfoInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

/// M: CC: @}

/// M: CC: CDMA call accepted indication @{
int radio::cdmaCallAcceptedInd(int slotId, int indicationType,
                               int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("cdmaCallAcceptedInd: slotId=%d", slotId);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->cdmaCallAccepted(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaCallAcceptedInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
/// @}

int radio::stkSessionEndInd(int slotId, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("stkSessionEndInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkSessionEnd(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stkSessionEndInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkProactiveCommandInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("stkProactiveCommandInd: invalid response");
            return 0;
        }
        RLOGD("stkProactiveCommandInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkProactiveCommand(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stkProactiveCommandInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkEventNotifyInd(int slotId, int indicationType,
                             int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("stkEventNotifyInd: invalid response");
            return 0;
        }
        RLOGD("stkEventNotifyInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkEventNotify(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stkEventNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkCallSetupInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("stkCallSetupInd: invalid response");
            return 0;
        }
        int32_t timeout = ((int32_t *) response)[0];
        RLOGD("stkCallSetupInd: timeout %d", timeout);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkCallSetup(
                convertIntToRadioIndicationType(indicationType), timeout);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stkCallSetupInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simSmsStorageFullInd(int slotId,
                                int indicationType, int token, RIL_Errno e, void *response,
                                size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("simSmsStorageFullInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("simSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simRefreshInd(int slotId, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SimRefreshResponse_v7)) {
            RLOGE("simRefreshInd: invalid response");
            return 0;
        }

        SimRefreshResult refreshResult = {};
        RIL_SimRefreshResponse_v7 *simRefreshResponse = ((RIL_SimRefreshResponse_v7 *) response);
        refreshResult.type =
                (android::hardware::radio::V1_0::SimRefreshType) simRefreshResponse->result;
        refreshResult.efId = simRefreshResponse->ef_id;
        refreshResult.aid = convertCharPtrToHidlString(simRefreshResponse->aid);

        RLOGD("simRefreshInd: type %d efId %d", refreshResult.type, refreshResult.efId);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simRefresh(
                convertIntToRadioIndicationType(indicationType), refreshResult);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("simRefreshInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilCdmaSignalInfoRecordToHal(RIL_CDMA_SignalInfoRecord *signalInfoRecord,
        CdmaSignalInfoRecord& record) {
    record.isPresent = signalInfoRecord->isPresent;
    record.signalType = signalInfoRecord->signalType;
    record.alertPitch = signalInfoRecord->alertPitch;
    record.signal = signalInfoRecord->signal;
}

int radio::callRingInd(int slotId, int indicationType,
                       int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        bool isGsm;
        CdmaSignalInfoRecord record = {};
        if (response == NULL || responseLen == 0) {
            isGsm = true;
        } else {
            isGsm = false;
            if (responseLen != sizeof (RIL_CDMA_SignalInfoRecord)) {
                RLOGE("callRingInd: invalid response");
                return 0;
            }
            convertRilCdmaSignalInfoRecordToHal((RIL_CDMA_SignalInfoRecord *) response, record);
        }

        RLOGD("callRingInd: isGsm %d", isGsm);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->callRing(
                convertIntToRadioIndicationType(indicationType), isGsm, record);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("callRingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simStatusChangedInd(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("simStatusChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simStatusChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("simStatusChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaNewSmsInd(int slotId, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_SMS_Message)) {
            RLOGE("cdmaNewSmsInd: invalid response");
            return 0;
        }

        CdmaSmsMessage msg = {};
        RIL_CDMA_SMS_Message *rilMsg = (RIL_CDMA_SMS_Message *) response;
        msg.teleserviceId = rilMsg->uTeleserviceID;
        msg.isServicePresent = rilMsg->bIsServicePresent;
        msg.serviceCategory = rilMsg->uServicecategory;
        msg.address.digitMode =
                (android::hardware::radio::V1_0::CdmaSmsDigitMode) rilMsg->sAddress.digit_mode;
        msg.address.numberMode =
                (android::hardware::radio::V1_0::CdmaSmsNumberMode) rilMsg->sAddress.number_mode;
        msg.address.numberType =
                (android::hardware::radio::V1_0::CdmaSmsNumberType) rilMsg->sAddress.number_type;
        msg.address.numberPlan =
                (android::hardware::radio::V1_0::CdmaSmsNumberPlan) rilMsg->sAddress.number_plan;

        int digitLimit = MIN((rilMsg->sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
        msg.address.digits.setToExternal(rilMsg->sAddress.digits, digitLimit);

        msg.subAddress.subaddressType = (android::hardware::radio::V1_0::CdmaSmsSubaddressType)
                rilMsg->sSubAddress.subaddressType;
        msg.subAddress.odd = rilMsg->sSubAddress.odd;

        digitLimit= MIN((rilMsg->sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
        msg.subAddress.digits.setToExternal(rilMsg->sSubAddress.digits, digitLimit);

        digitLimit = MIN((rilMsg->uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
        msg.bearerData.setToExternal(rilMsg->aBearerData, digitLimit);

        RLOGD("cdmaNewSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaNewSms(
                convertIntToRadioIndicationType(indicationType), msg);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaNewSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newBroadcastSmsInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("newBroadcastSmsInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("newBroadcastSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newBroadcastSms(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("newBroadcastSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaRuimSmsStorageFullInd(int slotId,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("cdmaRuimSmsStorageFullInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaRuimSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaRuimSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::restrictedStateChangedInd(int slotId,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("restrictedStateChangedInd: invalid response");
            return 0;
        }
        int32_t state = ((int32_t *) response)[0];
        RLOGD("restrictedStateChangedInd: state %d", state);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->restrictedStateChanged(
                convertIntToRadioIndicationType(indicationType), (PhoneRestrictedState) state);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("restrictedStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::enterEmergencyCallbackModeInd(int slotId,
                                         int indicationType, int token, RIL_Errno e, void *response,
                                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("enterEmergencyCallbackModeInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->enterEmergencyCallbackMode(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("enterEmergencyCallbackModeInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaCallWaitingInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_CallWaiting_v6)) {
            RLOGE("cdmaCallWaitingInd: invalid response");
            return 0;
        }

        CdmaCallWaiting callWaitingRecord = {};
        RIL_CDMA_CallWaiting_v6 *callWaitingRil = ((RIL_CDMA_CallWaiting_v6 *) response);
        callWaitingRecord.number = convertCharPtrToHidlString(callWaitingRil->number);
        callWaitingRecord.numberPresentation =
                (CdmaCallWaitingNumberPresentation) callWaitingRil->numberPresentation;
        callWaitingRecord.name = convertCharPtrToHidlString(callWaitingRil->name);
        convertRilCdmaSignalInfoRecordToHal(&callWaitingRil->signalInfoRecord,
                callWaitingRecord.signalInfoRecord);
        callWaitingRecord.numberType = (CdmaCallWaitingNumberType) callWaitingRil->number_type;
        callWaitingRecord.numberPlan = (CdmaCallWaitingNumberPlan) callWaitingRil->number_plan;

        RLOGD("cdmaCallWaitingInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaCallWaiting(
                convertIntToRadioIndicationType(indicationType), callWaitingRecord);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaCallWaitingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaOtaProvisionStatusInd(int slotId,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("cdmaOtaProvisionStatusInd: invalid response");
            return 0;
        }
        int32_t status = ((int32_t *) response)[0];
        RLOGD("cdmaOtaProvisionStatusInd: status %d", status);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaOtaProvisionStatus(
                convertIntToRadioIndicationType(indicationType), (CdmaOtaProvisionStatus) status);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaOtaProvisionStatusInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaInfoRecInd(int slotId,
                          int indicationType, int token, RIL_Errno e, void *response,
                          size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_InformationRecords)) {
            RLOGE("cdmaInfoRecInd: invalid response");
            return 0;
        }

        CdmaInformationRecords records = {};
        RIL_CDMA_InformationRecords *recordsRil = (RIL_CDMA_InformationRecords *) response;

        char* string8 = NULL;
        int num = MIN(recordsRil->numberOfInfoRecs, RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        if (recordsRil->numberOfInfoRecs > RIL_CDMA_MAX_NUMBER_OF_INFO_RECS) {
            RLOGE("cdmaInfoRecInd: received %d recs which is more than %d, dropping "
                    "additional ones", recordsRil->numberOfInfoRecs,
                    RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        }
        records.infoRec.resize(num);
        for (int i = 0 ; i < num ; i++) {
            CdmaInformationRecord *record = &records.infoRec[i];
            RIL_CDMA_InformationRecord *infoRec = &recordsRil->infoRec[i];
            record->name = (CdmaInfoRecName) infoRec->name;
            // All vectors should be size 0 except one which will be size 1. Set everything to
            // size 0 initially.
            record->display.resize(0);
            record->number.resize(0);
            record->signal.resize(0);
            record->redir.resize(0);
            record->lineCtrl.resize(0);
            record->clir.resize(0);
            record->audioCtrl.resize(0);
            switch (infoRec->name) {
                case RIL_CDMA_DISPLAY_INFO_REC:
                case RIL_CDMA_EXTENDED_DISPLAY_INFO_REC: {
                    if (infoRec->rec.display.alpha_len > CDMA_ALPHA_INFO_BUFFER_LENGTH) {
                        RLOGE("cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d", (int) infoRec->rec.display.alpha_len,
                                CDMA_ALPHA_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.display.alpha_len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.display.alpha_buf, infoRec->rec.display.alpha_len);
                    string8[(int)infoRec->rec.display.alpha_len] = '\0';

                    record->display.resize(1);
                    record->display[0].alphaBuf = string8;
                    free(string8);
                    string8 = NULL;
                    break;
                }

                case RIL_CDMA_CALLED_PARTY_NUMBER_INFO_REC:
                case RIL_CDMA_CALLING_PARTY_NUMBER_INFO_REC:
                case RIL_CDMA_CONNECTED_NUMBER_INFO_REC: {
                    if (infoRec->rec.number.len > CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                        RLOGE("cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d", (int) infoRec->rec.number.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.number.len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.number.buf, infoRec->rec.number.len);
                    string8[(int)infoRec->rec.number.len] = '\0';

                    record->number.resize(1);
                    record->number[0].number = string8;
                    free(string8);
                    string8 = NULL;
                    record->number[0].numberType = infoRec->rec.number.number_type;
                    record->number[0].numberPlan = infoRec->rec.number.number_plan;
                    record->number[0].pi = infoRec->rec.number.pi;
                    record->number[0].si = infoRec->rec.number.si;
                    break;
                }

                case RIL_CDMA_SIGNAL_INFO_REC: {
                    record->signal.resize(1);
                    record->signal[0].isPresent = infoRec->rec.signal.isPresent;
                    record->signal[0].signalType = infoRec->rec.signal.signalType;
                    record->signal[0].alertPitch = infoRec->rec.signal.alertPitch;
                    record->signal[0].signal = infoRec->rec.signal.signal;
                    break;
                }

                case RIL_CDMA_REDIRECTING_NUMBER_INFO_REC: {
                    if (infoRec->rec.redir.redirectingNumber.len >
                                                  CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                        RLOGE("cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d\n",
                                (int)infoRec->rec.redir.redirectingNumber.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.redir.redirectingNumber.len + 1) *
                            sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.redir.redirectingNumber.buf,
                            infoRec->rec.redir.redirectingNumber.len);
                    string8[(int)infoRec->rec.redir.redirectingNumber.len] = '\0';

                    record->redir.resize(1);
                    record->redir[0].redirectingNumber.number = string8;
                    free(string8);
                    string8 = NULL;
                    record->redir[0].redirectingNumber.numberType =
                            infoRec->rec.redir.redirectingNumber.number_type;
                    record->redir[0].redirectingNumber.numberPlan =
                            infoRec->rec.redir.redirectingNumber.number_plan;
                    record->redir[0].redirectingNumber.pi = infoRec->rec.redir.redirectingNumber.pi;
                    record->redir[0].redirectingNumber.si = infoRec->rec.redir.redirectingNumber.si;
                    record->redir[0].redirectingReason =
                            (CdmaRedirectingReason) infoRec->rec.redir.redirectingReason;
                    break;
                }

                case RIL_CDMA_LINE_CONTROL_INFO_REC: {
                    record->lineCtrl.resize(1);
                    record->lineCtrl[0].lineCtrlPolarityIncluded =
                            infoRec->rec.lineCtrl.lineCtrlPolarityIncluded;
                    record->lineCtrl[0].lineCtrlToggle = infoRec->rec.lineCtrl.lineCtrlToggle;
                    record->lineCtrl[0].lineCtrlReverse = infoRec->rec.lineCtrl.lineCtrlReverse;
                    record->lineCtrl[0].lineCtrlPowerDenial =
                            infoRec->rec.lineCtrl.lineCtrlPowerDenial;
                    break;
                }

                case RIL_CDMA_T53_CLIR_INFO_REC: {
                    record->clir.resize(1);
                    record->clir[0].cause = infoRec->rec.clir.cause;
                    break;
                }

                case RIL_CDMA_T53_AUDIO_CONTROL_INFO_REC: {
                    record->audioCtrl.resize(1);
                    record->audioCtrl[0].upLink = infoRec->rec.audioCtrl.upLink;
                    record->audioCtrl[0].downLink = infoRec->rec.audioCtrl.downLink;
                    break;
                }

                case RIL_CDMA_T53_RELEASE_INFO_REC:
                    RLOGE("cdmaInfoRecInd: RIL_CDMA_T53_RELEASE_INFO_REC: INVALID");
                    return 0;

                default:
                    RLOGE("cdmaInfoRecInd: Incorrect name value");
                    return 0;
            }
        }

        RLOGD("cdmaInfoRecInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaInfoRec(
                convertIntToRadioIndicationType(indicationType), records);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaInfoRecInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::indicateRingbackToneInd(int slotId,
                                   int indicationType, int token, RIL_Errno e, void *response,
                                   size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("indicateRingbackToneInd: invalid response");
            return 0;
        }
        bool start = ((int32_t *) response)[0];
        RLOGD("indicateRingbackToneInd: start %d", start);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->indicateRingbackTone(
                convertIntToRadioIndicationType(indicationType), start);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("indicateRingbackToneInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::resendIncallMuteInd(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("resendIncallMuteInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->resendIncallMute(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("resendIncallMuteInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaSubscriptionSourceChangedInd(int slotId,
                                            int indicationType, int token, RIL_Errno e,
                                            void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("cdmaSubscriptionSourceChangedInd: invalid response");
            return 0;
        }
        int32_t cdmaSource = ((int32_t *) response)[0];
        RLOGD("cdmaSubscriptionSourceChangedInd: cdmaSource %d", cdmaSource);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->
                cdmaSubscriptionSourceChanged(convertIntToRadioIndicationType(indicationType),
                (CdmaSubscriptionSource) cdmaSource);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaSubscriptionSourceChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaPrlChangedInd(int slotId,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("cdmaPrlChangedInd: invalid response");
            return 0;
        }
        int32_t version = ((int32_t *) response)[0];
        RLOGD("cdmaPrlChangedInd: version %d", version);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaPrlChanged(
                convertIntToRadioIndicationType(indicationType), version);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cdmaPrlChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::exitEmergencyCallbackModeInd(int slotId,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("exitEmergencyCallbackModeInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->exitEmergencyCallbackMode(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("exitEmergencyCallbackModeInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::rilConnectedInd(int slotId,
                           int indicationType, int token, RIL_Errno e, void *response,
                           size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("rilConnectedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->rilConnected(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("rilConnectedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    // hangup all if the Phone process disconnected to sync the AP / MD state
    if (radioService[slotId] != NULL) {
        radioService[slotId]->hangupAll(0);
    }
    return 0;
}

int radio::voiceRadioTechChangedInd(int slotId,
                                    int indicationType, int token, RIL_Errno e, void *response,
                                    size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("voiceRadioTechChangedInd: invalid response");
            return 0;
        }
        int32_t rat = ((int32_t *) response)[0];
        RLOGD("voiceRadioTechChangedInd: rat %d", rat);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->voiceRadioTechChanged(
                convertIntToRadioIndicationType(indicationType), (RadioTechnology) rat);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("voiceRadioTechChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilCellInfoListToHal(void *response, size_t responseLen, hidl_vec<CellInfo>& records) {
    int num = responseLen / sizeof(RIL_CellInfo_v12);
    records.resize(num);

    RIL_CellInfo_v12 *rillCellInfo = (RIL_CellInfo_v12 *) response;
    for (int i = 0; i < num; i++) {
        records[i].cellInfoType = (CellInfoType) rillCellInfo->cellInfoType;
        records[i].registered = rillCellInfo->registered;
        records[i].timeStampType = (TimeStampType) rillCellInfo->timeStampType;
        records[i].timeStamp = rillCellInfo->timeStamp;
        // All vectors should be size 0 except one which will be size 1. Set everything to
        // size 0 initially.
        records[i].gsm.resize(0);
        records[i].wcdma.resize(0);
        records[i].cdma.resize(0);
        records[i].lte.resize(0);
        records[i].tdscdma.resize(0);
        switch(rillCellInfo->cellInfoType) {
            case RIL_CELL_INFO_TYPE_GSM: {
                records[i].gsm.resize(1);
                CellInfoGsm *cellInfoGsm = &records[i].gsm[0];
                cellInfoGsm->cellIdentityGsm.mcc =
                        std::to_string(rillCellInfo->CellInfo.gsm.cellIdentityGsm.mcc);
                cellInfoGsm->cellIdentityGsm.mnc =
                        std::to_string(rillCellInfo->CellInfo.gsm.cellIdentityGsm.mnc);
                cellInfoGsm->cellIdentityGsm.lac =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.lac;
                cellInfoGsm->cellIdentityGsm.cid =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.cid;
                cellInfoGsm->cellIdentityGsm.arfcn =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.arfcn;
                cellInfoGsm->cellIdentityGsm.bsic =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.bsic;
                cellInfoGsm->signalStrengthGsm.signalStrength =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.signalStrength;
                cellInfoGsm->signalStrengthGsm.bitErrorRate =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.bitErrorRate;
                cellInfoGsm->signalStrengthGsm.timingAdvance =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.timingAdvance;
                break;
            }

            case RIL_CELL_INFO_TYPE_WCDMA: {
                records[i].wcdma.resize(1);
                CellInfoWcdma *cellInfoWcdma = &records[i].wcdma[0];
                cellInfoWcdma->cellIdentityWcdma.mcc =
                        std::to_string(rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.mcc);
                cellInfoWcdma->cellIdentityWcdma.mnc =
                        std::to_string(rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.mnc);
                cellInfoWcdma->cellIdentityWcdma.lac =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.lac;
                cellInfoWcdma->cellIdentityWcdma.cid =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.cid;
                cellInfoWcdma->cellIdentityWcdma.psc =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.psc;
                cellInfoWcdma->cellIdentityWcdma.uarfcn =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.uarfcn;
                cellInfoWcdma->signalStrengthWcdma.signalStrength =
                        rillCellInfo->CellInfo.wcdma.signalStrengthWcdma.signalStrength;
                cellInfoWcdma->signalStrengthWcdma.bitErrorRate =
                        rillCellInfo->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate;
                break;
            }

            case RIL_CELL_INFO_TYPE_CDMA: {
                records[i].cdma.resize(1);
                CellInfoCdma *cellInfoCdma = &records[i].cdma[0];
                cellInfoCdma->cellIdentityCdma.networkId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.networkId;
                cellInfoCdma->cellIdentityCdma.systemId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.systemId;
                cellInfoCdma->cellIdentityCdma.baseStationId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.basestationId;
                cellInfoCdma->cellIdentityCdma.longitude =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.longitude;
                cellInfoCdma->cellIdentityCdma.latitude =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.latitude;
                cellInfoCdma->signalStrengthCdma.dbm =
                        rillCellInfo->CellInfo.cdma.signalStrengthCdma.dbm;
                cellInfoCdma->signalStrengthCdma.ecio =
                        rillCellInfo->CellInfo.cdma.signalStrengthCdma.ecio;
                cellInfoCdma->signalStrengthEvdo.dbm =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.dbm;
                cellInfoCdma->signalStrengthEvdo.ecio =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.ecio;
                cellInfoCdma->signalStrengthEvdo.signalNoiseRatio =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.signalNoiseRatio;
                break;
            }

            case RIL_CELL_INFO_TYPE_LTE: {
                records[i].lte.resize(1);
                CellInfoLte *cellInfoLte = &records[i].lte[0];
                cellInfoLte->cellIdentityLte.mcc =
                        std::to_string(rillCellInfo->CellInfo.lte.cellIdentityLte.mcc);
                cellInfoLte->cellIdentityLte.mnc =
                        std::to_string(rillCellInfo->CellInfo.lte.cellIdentityLte.mnc);
                cellInfoLte->cellIdentityLte.ci =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.ci;
                cellInfoLte->cellIdentityLte.pci =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.pci;
                cellInfoLte->cellIdentityLte.tac =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.tac;
                cellInfoLte->cellIdentityLte.earfcn =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.earfcn;
                cellInfoLte->signalStrengthLte.signalStrength =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.signalStrength;
                cellInfoLte->signalStrengthLte.rsrp =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rsrp;
                cellInfoLte->signalStrengthLte.rsrq =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rsrq;
                cellInfoLte->signalStrengthLte.rssnr =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rssnr;
                cellInfoLte->signalStrengthLte.cqi =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.cqi;
                cellInfoLte->signalStrengthLte.timingAdvance =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.timingAdvance;
                break;
            }

            case RIL_CELL_INFO_TYPE_TD_SCDMA: {
                records[i].tdscdma.resize(1);
                CellInfoTdscdma *cellInfoTdscdma = &records[i].tdscdma[0];
                cellInfoTdscdma->cellIdentityTdscdma.mcc =
                        std::to_string(rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.mcc);
                cellInfoTdscdma->cellIdentityTdscdma.mnc =
                        std::to_string(rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.mnc);
                cellInfoTdscdma->cellIdentityTdscdma.lac =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.lac;
                cellInfoTdscdma->cellIdentityTdscdma.cid =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.cid;
                cellInfoTdscdma->cellIdentityTdscdma.cpid =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.cpid;
                cellInfoTdscdma->signalStrengthTdscdma.rscp =
                        rillCellInfo->CellInfo.tdscdma.signalStrengthTdscdma.rscp;
                break;
            }
            default: {
                break;
            }
        }
        rillCellInfo += 1;
    }
}

int radio::cellInfoListInd(int slotId,
                           int indicationType, int token, RIL_Errno e, void *response,
                           size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_CellInfo_v12) != 0) {
            RLOGE("cellInfoListInd: invalid response");
            return 0;
        }

        hidl_vec<CellInfo> records;
        convertRilCellInfoListToHal(response, responseLen, records);

        RLOGD("cellInfoListInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cellInfoList(
                convertIntToRadioIndicationType(indicationType), records);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("cellInfoListInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::imsNetworkStateChangedInd(int slotId,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("imsNetworkStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->imsNetworkStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsNetworkStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::subscriptionStatusChangedInd(int slotId,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("subscriptionStatusChangedInd: invalid response");
            return 0;
        }
        bool activate = ((int32_t *) response)[0];
        RLOGD("subscriptionStatusChangedInd: activate %d", activate);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->subscriptionStatusChanged(
                convertIntToRadioIndicationType(indicationType), activate);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("subscriptionStatusChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::srvccStateNotifyInd(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("srvccStateNotifyInd: invalid response");
            return 0;
        }
        int32_t state = ((int32_t *) response)[0];
        RLOGD("srvccStateNotifyInd: rat %d", state);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->srvccStateNotify(
                convertIntToRadioIndicationType(indicationType), (SrvccState) state);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("srvccStateNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilHardwareConfigListToHal(void *response, size_t responseLen,
        hidl_vec<HardwareConfig>& records) {
    int num = responseLen / sizeof(RIL_HardwareConfig);
    records.resize(num);

    RIL_HardwareConfig *rilHardwareConfig = (RIL_HardwareConfig *) response;
    for (int i = 0; i < num; i++) {
        records[i].type = (HardwareConfigType) rilHardwareConfig[i].type;
        records[i].uuid = convertCharPtrToHidlString(rilHardwareConfig[i].uuid);
        records[i].state = (HardwareConfigState) rilHardwareConfig[i].state;
        switch (rilHardwareConfig[i].type) {
            case RIL_HARDWARE_CONFIG_MODEM: {
                records[i].modem.resize(1);
                records[i].sim.resize(0);
                HardwareConfigModem *hwConfigModem = &records[i].modem[0];
                hwConfigModem->rat = rilHardwareConfig[i].cfg.modem.rat;
                hwConfigModem->maxVoice = rilHardwareConfig[i].cfg.modem.maxVoice;
                hwConfigModem->maxData = rilHardwareConfig[i].cfg.modem.maxData;
                hwConfigModem->maxStandby = rilHardwareConfig[i].cfg.modem.maxStandby;
                break;
            }

            case RIL_HARDWARE_CONFIG_SIM: {
                records[i].sim.resize(1);
                records[i].modem.resize(0);
                records[i].sim[0].modemUuid =
                        convertCharPtrToHidlString(rilHardwareConfig[i].cfg.sim.modemUuid);
                break;
            }
        }
    }
}

int radio::hardwareConfigChangedInd(int slotId,
                                    int indicationType, int token, RIL_Errno e, void *response,
                                    size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_HardwareConfig) != 0) {
            RLOGE("hardwareConfigChangedInd: invalid response");
            return 0;
        }

        hidl_vec<HardwareConfig> configs;
        convertRilHardwareConfigListToHal(response, responseLen, configs);

        RLOGD("hardwareConfigChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->hardwareConfigChanged(
                convertIntToRadioIndicationType(indicationType), configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("hardwareConfigChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilRadioCapabilityToHal(void *response, size_t responseLen, RadioCapability& rc) {
    RIL_RadioCapability *rilRadioCapability = (RIL_RadioCapability *) response;
    rc.session = rilRadioCapability->session;
    rc.phase = (android::hardware::radio::V1_0::RadioCapabilityPhase) rilRadioCapability->phase;
    rc.raf = rilRadioCapability->rat;
    rc.logicalModemUuid = convertCharPtrToHidlString(rilRadioCapability->logicalModemUuid);
    rc.status = (android::hardware::radio::V1_0::RadioCapabilityStatus) rilRadioCapability->status;
}

int radio::radioCapabilityIndicationInd(int slotId,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_RadioCapability)) {
            RLOGE("radioCapabilityIndicationInd: invalid response");
            return 0;
        }

        RadioCapability rc = {};
        convertRilRadioCapabilityToHal(response, responseLen, rc);

        RLOGD("radioCapabilityIndicationInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->radioCapabilityIndication(
                convertIntToRadioIndicationType(indicationType), rc);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radioCapabilityIndicationInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

bool isServiceTypeCfQuery(RIL_SsServiceType serType, RIL_SsRequestType reqType) {
    if ((reqType == SS_INTERROGATION) &&
        (serType == SS_CFU ||
         serType == SS_CF_BUSY ||
         serType == SS_CF_NO_REPLY ||
         serType == SS_CF_NOT_REACHABLE ||
         serType == SS_CF_ALL ||
         serType == SS_CF_ALL_CONDITIONAL)) {
        return true;
    }
    return false;
}

int radio::onSupplementaryServiceIndicationInd(int slotId,
                                               int indicationType, int token, RIL_Errno e,
                                               void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_StkCcUnsolSsResponse)) {
            RLOGE("onSupplementaryServiceIndicationInd: invalid response");
            return 0;
        }

        RIL_StkCcUnsolSsResponse *rilSsResponse = (RIL_StkCcUnsolSsResponse *) response;
        StkCcUnsolSsResult ss = {};
        ss.serviceType = (SsServiceType) rilSsResponse->serviceType;
        ss.requestType = (SsRequestType) rilSsResponse->requestType;
        ss.teleserviceType = (SsTeleserviceType) rilSsResponse->teleserviceType;
        ss.serviceClass = rilSsResponse->serviceClass;
        ss.result = (RadioError) rilSsResponse->result;

        if (isServiceTypeCfQuery(rilSsResponse->serviceType, rilSsResponse->requestType)) {
            RLOGD("onSupplementaryServiceIndicationInd CF type, num of Cf elements %d",
                    rilSsResponse->cfData.numValidIndexes);
            if (rilSsResponse->cfData.numValidIndexes > NUM_SERVICE_CLASSES) {
                RLOGE("onSupplementaryServiceIndicationInd numValidIndexes is greater than "
                        "max value %d, truncating it to max value", NUM_SERVICE_CLASSES);
                rilSsResponse->cfData.numValidIndexes = NUM_SERVICE_CLASSES;
            }

            ss.cfData.resize(1);
            ss.ssInfo.resize(0);

            /* number of call info's */
            ss.cfData[0].cfInfo.resize(rilSsResponse->cfData.numValidIndexes);

            for (int i = 0; i < rilSsResponse->cfData.numValidIndexes; i++) {
                 RIL_CallForwardInfo cf = rilSsResponse->cfData.cfInfo[i];
                 CallForwardInfo *cfInfo = &ss.cfData[0].cfInfo[i];

                 cfInfo->status = (CallForwardInfoStatus) cf.status;
                 cfInfo->reason = cf.reason;
                 cfInfo->serviceClass = cf.serviceClass;
                 cfInfo->toa = cf.toa;
                 cfInfo->number = convertCharPtrToHidlString(cf.number);
                 cfInfo->timeSeconds = cf.timeSeconds;
                 RLOGD("onSupplementaryServiceIndicationInd: "
                        "Data: %d,reason=%d,cls=%d,toa=%d,num=%s,tout=%d],", cf.status,
                        cf.reason, cf.serviceClass, cf.toa, (char*)cf.number, cf.timeSeconds);
            }
        } else {
            ss.ssInfo.resize(1);
            ss.cfData.resize(0);

            /* each int */
            ss.ssInfo[0].ssInfo.resize(SS_INFO_MAX);
            for (int i = 0; i < SS_INFO_MAX; i++) {
                 RLOGD("onSupplementaryServiceIndicationInd: Data: %d",
                        rilSsResponse->ssInfo[i]);
                 ss.ssInfo[0].ssInfo[i] = rilSsResponse->ssInfo[i];
            }
        }

        RLOGD("onSupplementaryServiceIndicationInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->
                onSupplementaryServiceIndication(convertIntToRadioIndicationType(indicationType),
                ss);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSupplementaryServiceIndicationInd: "
                "radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkCallControlAlphaNotifyInd(int slotId,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("stkCallControlAlphaNotifyInd: invalid response");
            return 0;
        }
        RLOGD("stkCallControlAlphaNotifyInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkCallControlAlphaNotify(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("stkCallControlAlphaNotifyInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilLceDataInfoToHal(void *response, size_t responseLen, LceDataInfo& lce) {
    RIL_LceDataInfo *rilLceDataInfo = (RIL_LceDataInfo *)response;
    lce.lastHopCapacityKbps = rilLceDataInfo->last_hop_capacity_kbps;
    lce.confidenceLevel = rilLceDataInfo->confidence_level;
    lce.lceSuspended = rilLceDataInfo->lce_suspended;
}

int radio::lceDataInd(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_LceDataInfo)) {
            RLOGE("lceDataInd: invalid response");
            return 0;
        }

        LceDataInfo lce = {};
        convertRilLceDataInfoToHal(response, responseLen, lce);
        RLOGD("lceDataInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->lceData(
                convertIntToRadioIndicationType(indicationType), lce);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("lceDataInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::pcoDataInd(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_PCO_Data)) {
            RLOGE("pcoDataInd: invalid response");
            return 0;
        }

        PcoDataInfo pco = {};
        RIL_PCO_Data *rilPcoData = (RIL_PCO_Data *)response;
        pco.cid = rilPcoData->cid;
        pco.bearerProto = convertCharPtrToHidlString(rilPcoData->bearer_proto);
        pco.pcoId = rilPcoData->pco_id;
        pco.contents.setToExternal((uint8_t *) rilPcoData->contents, rilPcoData->contents_length);

        RLOGD("pcoDataInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->pcoData(
                convertIntToRadioIndicationType(indicationType), pco);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("pcoDataInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

// M: [VzW] Data Framework @{
int radio::pcoDataAfterAttachedInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_PCO_Data_attached)) {
            RLOGE("pcoDataAfterAttachedInd: invalid response");
            return 0;
        }

        PcoDataAttachedInfo pco = {};
        RIL_PCO_Data_attached *rilPcoData = (RIL_PCO_Data_attached *)response;
        pco.cid = rilPcoData->cid;
        pco.apnName = convertCharPtrToHidlString(rilPcoData->apn_name);
        pco.bearerProto = convertCharPtrToHidlString(rilPcoData->bearer_proto);
        pco.pcoId = rilPcoData->pco_id;
        pco.contents.setToExternal((uint8_t *) rilPcoData->contents, rilPcoData->contents_length);

        RLOGD("pcoDataAfterAttachedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->pcoDataAfterAttached(
                convertIntToRadioIndicationType(indicationType), pco);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("pcoDataAfterAttachedInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

// M: [VzW] Data Framework @{
int radio::volteLteConnectionStatusInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("volteLteConnectionStatusInd Invalid response");
            return 0;
        }
        RLOGD("volteLteConnectionStatusInd");
        hidl_vec<int32_t> data;
        int *pInt = (int *) response;
        int numInts = responseLen / sizeof(int);
        data.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            data[i] = (int32_t) pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->volteLteConnectionStatus(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("volteLteConnectionStatusInd: radioService[%d]->volteLteConnectionStatusInd == NULL", slotId);
    }
    return 0;
}
// M: [VzW] Data Framework @}

int radio::modemResetInd(int slotId,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("modemResetInd: invalid response");
            return 0;
        }
        RLOGD("modemResetInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->modemReset(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("modemResetInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::networkScanResultInd(int slotId,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationV1_1 != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("networkScanResultInd: invalid response");
            return 0;
        }
        RLOGD("networkScanResultInd");
        RIL_NetworkScanResult *networkScanResult = (RIL_NetworkScanResult *) response;

        AOSP_V1_1::NetworkScanResult result;
        result.status = (AOSP_V1_1::ScanStatus) networkScanResult->status;
        result.error = (RadioError) e;
        convertRilCellInfoListToHal(
                networkScanResult->network_infos,
                networkScanResult->network_infos_length * sizeof(RIL_CellInfo_v12),
                result.networkInfos);

        Return<void> retStatus = radioService[slotId]->mRadioIndicationV1_1->networkScanResult(
                convertIntToRadioIndicationType(indicationType), result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("networkScanResultInd: radioService[%d]->mRadioIndicationV1_1 == NULL", slotId);
    }
    return 0;
}

int radio::oemHookRawInd(int slotId,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("oemHookRawInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("oemHookRawInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->oemHookRaw(
                convertIntToRadioIndicationType(indicationType), data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("oemHookRawInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

// ATCI Start
int radio::atciInd(int slotId,
                   int indicationType, int token, RIL_Errno e, void *response,
                   size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mAtciIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("atciInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("atciInd");
        Return<void> retStatus = radioService[slotId]->mAtciIndication->atciInd(
                convertIntToRadioIndicationType(indicationType), data);
        if (!retStatus.isOk()) {
            RLOGE("sendAtciResponse: unable to call indication callback");
            radioService[slotId]->mAtciResponse = NULL;
            radioService[slotId]->mAtciIndication = NULL;
        }
    } else {
        RLOGE("atciInd: radioService[%d]->mAtciIndication == NULL", slotId);
    }

    return 0;
}
// ATCI End

/// [IMS] Indication ////////////////////////////////////////////////////////////////////
int radio::callInfoIndicationInd(int slotId,
                                 int indicationType, int token, RIL_Errno e,
                                 void *response, size_t responseLen) {

    char **resp = (char **) response;
    int numStrings = responseLen / sizeof(char *);
    if(numStrings < 5) {
        RLOGE("callInfoIndicationInd: items length is invalid, slot = %d", slotId);
        return 0;
    }

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        hidl_vec<hidl_string> data;
        data.resize(numStrings);
        for (int i = 0; i < numStrings; i++) {
            data[i] = convertCharPtrToHidlString(resp[i]);
            RLOGD("callInfoIndicationInd:: %d: %s", i, resp[i]);
        }

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->callInfoIndication(
                                 convertIntToRadioIndicationType(indicationType),
                                 data);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("callInfoIndicationInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                          imsSlotId);
    }

    return 0;
}

int radio::econfResultIndicationInd(int slotId,
                                    int indicationType, int token, RIL_Errno e,
                                    void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        if(numStrings < 5) {
            RLOGE("econfResultIndicationInd: items length invalid, slotId = %d",
                                                                     imsSlotId);
            return 0;
        }

        hidl_string confCallId = convertCharPtrToHidlString(resp[0]);
        hidl_string op = convertCharPtrToHidlString(resp[1]);
        hidl_string num = convertCharPtrToHidlString(resp[2]);
        hidl_string result = convertCharPtrToHidlString(resp[3]);
        hidl_string cause = convertCharPtrToHidlString(resp[4]);
        hidl_string joinedCallId;
        if(numStrings > 5) {
            joinedCallId = convertCharPtrToHidlString(resp[5]);
        }

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->econfResultIndication(
                                 convertIntToRadioIndicationType(indicationType),
                                 confCallId, op, num, result, cause, joinedCallId);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    }
    else {
        RLOGE("econfResultIndicationInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                             imsSlotId);
    }

    return 0;
}

int radio::sipCallProgressIndicatorInd(int slotId,
                                       int indicationType, int token, RIL_Errno e,
                                       void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        if(numStrings < 5) {
            RLOGE("sipCallProgressIndicatorInd: items length invalid, slotId = %d",
                                                                        imsSlotId);
            return 0;
        }

        hidl_string callId = convertCharPtrToHidlString(resp[0]);
        hidl_string dir = convertCharPtrToHidlString(resp[1]);
        hidl_string sipMsgType = convertCharPtrToHidlString(resp[2]);
        hidl_string method = convertCharPtrToHidlString(resp[3]);
        hidl_string responseCode = convertCharPtrToHidlString(resp[4]);
        hidl_string reasonText;
        if(numStrings > 5) {
            reasonText = convertCharPtrToHidlString(resp[5]);
        }

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->sipCallProgressIndicator(
                                 convertIntToRadioIndicationType(indicationType),
                                 callId, dir, sipMsgType, method, responseCode, reasonText);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    }
    else {
        RLOGE("sipCallProgressIndicatorInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                                imsSlotId);
    }

    return 0;
}

int radio::callmodChangeIndicatorInd(int slotId,
                                    int indicationType, int token, RIL_Errno e,
                                    void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        if(numStrings < 5) {
            RLOGE("callmodChangeIndicatorInd: items length invalid, slotId = %d",
                                                                       imsSlotId);
            return 0;
        }

        hidl_string callId = convertCharPtrToHidlString(resp[0]);
        hidl_string callMode = convertCharPtrToHidlString(resp[1]);
        hidl_string videoState = convertCharPtrToHidlString(resp[2]);
        hidl_string autoDirection = convertCharPtrToHidlString(resp[3]);
        hidl_string pau = convertCharPtrToHidlString(resp[4]);

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->callmodChangeIndicator(
                                 convertIntToRadioIndicationType(indicationType),
                                 callId, callMode, videoState, autoDirection, pau);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    }
    else {
        RLOGE("callmodChangeIndicatorInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                              imsSlotId);
    }

    return 0;
}

int radio::videoCapabilityIndicatorInd(int slotId,
                                       int indicationType, int token, RIL_Errno e,
                                       void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        if(numStrings < 3) {
            RLOGE("videoCapabilityIndicatorInd: items length invalid, slotId = %d",
                                                                        imsSlotId);
            return 0;
        }

        hidl_string callId = convertCharPtrToHidlString(resp[0]);
        hidl_string localVideoCaoability = convertCharPtrToHidlString(resp[1]);
        hidl_string remoteVideoCaoability = convertCharPtrToHidlString(resp[2]);

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->videoCapabilityIndicator(
                                 convertIntToRadioIndicationType(indicationType),
                                 callId, localVideoCaoability, remoteVideoCaoability);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    }
    else {
        RLOGE("videoCapabilityIndicatorInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                                imsSlotId);
    }

    return 0;
}

int radio::onUssiInd(int slotId,
                     int indicationType, int token, RIL_Errno e, void *response,
                     size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        if(numStrings < 7) {
            RLOGE("onUssiInd: items length invalid, slotId = %d",
                                                      imsSlotId);
            return 0;
        }

        hidl_string clazz = convertCharPtrToHidlString(resp[0]);
        hidl_string status = convertCharPtrToHidlString(resp[1]);
        hidl_string str = convertCharPtrToHidlString(resp[2]);
        hidl_string lang = convertCharPtrToHidlString(resp[3]);
        hidl_string errorCode = convertCharPtrToHidlString(resp[4]);
        hidl_string alertingPattern = convertCharPtrToHidlString(resp[5]);
        hidl_string sipCause = convertCharPtrToHidlString(resp[6]);

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->onUssi(
                                 convertIntToRadioIndicationType(indicationType),
                                 clazz, status, str, lang, errorCode,
                                 alertingPattern, sipCause);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    }
    else {
        RLOGE("onUssiInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                              imsSlotId);
    }

    return 0;
}

int radio::getProvisionDoneInd(int slotId,
                               int indicationType, int token, RIL_Errno e,
                               void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        hidl_string result1;
        hidl_string result2;
        int numStrings = responseLen / sizeof(char *);

        if (response == NULL || numStrings < 2) {
            RLOGE("getProvisionDone Invalid response: NULL");
            return 0;
        } else {
            char **resp = (char **) response;
            result1 = convertCharPtrToHidlString(resp[0]);
            result2 = convertCharPtrToHidlString(resp[1]);
        }

        RLOGD("getProvisionDoneInd");
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->getProvisionDone(
                                 convertIntToRadioIndicationType(indicationType),
                                 result1, result2);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getProvisionDoneInd: radioService[%d]->mRadioIndicationMtk == NULL",
                                                                         imsSlotId);
    }

    return 0;
}

int radio::imsRtpInfoInd(int slotId,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        hidl_string pdnId;
        hidl_string networkId;
        hidl_string timer;
        hidl_string sendPktLost;
        hidl_string recvPktLost;
        int numStrings = responseLen / sizeof(char *);

        if (response == NULL || numStrings < 5) {
            RLOGE("imsRtpInfoInd Invalid response: NULL");
            return 0;
        } else {
            char **resp = (char **) response;
            pdnId = convertCharPtrToHidlString(resp[0]);
            networkId = convertCharPtrToHidlString(resp[1]);
            timer = convertCharPtrToHidlString(resp[2]);
            sendPktLost = convertCharPtrToHidlString(resp[3]);
            recvPktLost = convertCharPtrToHidlString(resp[4]);
        }

        RLOGD("imsRtpInfoInd");
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->imsRtpInfo(
                                 convertIntToRadioIndicationType(indicationType),
                                 pdnId, networkId, timer, sendPktLost, recvPktLost);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsRtpInfoInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                   imsSlotId);
    }

    return 0;
}

int radio::onXuiInd(int slotId,
                    int indicationType, int token, RIL_Errno e, void *response,
                    size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        hidl_string accountId;
        hidl_string broadcastFlag;
        hidl_string xuiInfo;
        int numStrings = responseLen / sizeof(char *);

        if (response == NULL || numStrings < 3) {
            RLOGE("onXuiInd Invalid response: NULL");
            return 0;
        } else {
            char **resp = (char **) response;
            accountId = convertCharPtrToHidlString(resp[0]);
            broadcastFlag = convertCharPtrToHidlString(resp[1]);
            xuiInfo = convertCharPtrToHidlString(resp[2]);
        }

        RLOGD("onXuiInd");
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->onXui(
                                 convertIntToRadioIndicationType(indicationType),
                                 accountId, broadcastFlag, xuiInfo);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onXuiInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                             imsSlotId);
    }

    return 0;
}

int radio::imsEventPackageIndicationInd(int slotId,
                                        int indicationType, int token, RIL_Errno e,
                                        void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        hidl_string callId;
        hidl_string pType;
        hidl_string urcIdx;
        hidl_string totalUrcCount;
        hidl_string rawData;
        int numStrings = responseLen / sizeof(char *);

        if (response == NULL || numStrings < 5) {
            RLOGE("imsEventPackageIndication Invalid response: NULL");
            return 0;
        } else {
            char **resp = (char **) response;
            callId = convertCharPtrToHidlString(resp[0]);
            pType = convertCharPtrToHidlString(resp[1]);
            urcIdx = convertCharPtrToHidlString(resp[2]);
            totalUrcCount = convertCharPtrToHidlString(resp[3]);
            rawData = convertCharPtrToHidlString(resp[4]);
        }

        RLOGD("imsEventPackageIndication");
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->imsEventPackageIndication(
                                 convertIntToRadioIndicationType(indicationType),
                                 callId, pType, urcIdx, totalUrcCount, rawData);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEventPackageIndication: radioService[%d]->mRadioIndicationIms == NULL",
                                                                              imsSlotId);
    }

    return 0;
}

int radio::imsRegistrationInfoInd(int slotId,
                              int indicationType, int token, RIL_Errno e,
                              void *response, size_t responseLen) {
    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL &&
            radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen < (sizeof(int) * 2)) {
            RLOGE("imsRegistrationInfoInd: invalid response");
            return 0;
        }

        int status = ((int32_t *) response)[0];
        int capacity = ((int32_t *) response)[1];

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 imsRegistrationInfo(
                                 convertIntToRadioIndicationType(indicationType),
                                 status, capacity);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsRegistrationInfoInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                            imsSlotId);
    }

    return 0;
}

int radio::imsEnableDoneInd(int slotId,
                            int indicationType, int token, RIL_Errno e,
                            void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        RLOGD("imsEnableDoneInd");
        Return<void> retStatus =
                radioService[imsSlotId]->mRadioIndicationIms->imsEnableDone(
                convertIntToRadioIndicationType(indicationType));

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEnableDoneInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                     imsSlotId);
    }

    return 0;
}

int radio::imsDisableDoneInd(int slotId,
                             int indicationType, int token, RIL_Errno e,
                             void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        RLOGD("imsDisableDoneInd");
        Return<void> retStatus =
                radioService[imsSlotId]->mRadioIndicationIms->imsDisableDone(
                convertIntToRadioIndicationType(indicationType));

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsDisableDoneInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                      imsSlotId);
    }

    return 0;
}

int radio::imsEnableStartInd(int slotId,
                             int type, int token, RIL_Errno e,
                             void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        RLOGD("imsEnableStartInd, slotId = %d, IMS slotId = %d", slotId, imsSlotId);
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 imsEnableStart(convertIntToRadioIndicationType(type));

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsEnableStartInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                      imsSlotId);
    }

    return 0;
}

int radio::imsDisableStartInd(int slotId,
                              int type, int token, RIL_Errno e,
                              void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        RLOGD("imsDisableStartInd, slotId = %d, IMS slotId = %d", slotId, imsSlotId);
        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 imsDisableStart(convertIntToRadioIndicationType(type));

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsDisableStartInd: radioService[%d]->mRadioIndicationMtk == NULL", imsSlotId);
    }

    return 0;
}

int radio::ectIndicationInd(int slotId,
                            int indicationType, int token, RIL_Errno e,
                            void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL &&
            radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen < (sizeof(int) * 3)) {
            RLOGE("ectIndicationInd: invalid response");
            return 0;
        }

        int callId = ((int32_t *) response)[0];
        int ectResult = ((int32_t *) response)[1];
        int cause = ((int32_t *) response)[2];

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 ectIndication(
                                 convertIntToRadioIndicationType(indicationType),
                                 callId, ectResult, cause);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("ectIndicationInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                     imsSlotId);
    }

    return 0;
}

int radio::volteSettingInd(int slotId,
                           int indicationType, int token, RIL_Errno e,
                           void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL &&
            radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen < (sizeof(int))) {
            RLOGE("volteSettingInd: invalid response");
            return 0;
        }

        int status = ((int32_t *) response)[0];
        bool isEnable = (status == 1) ? true : false;

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 volteSetting(
                                 convertIntToRadioIndicationType(indicationType),
                                 isEnable);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("volteSettingInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                    imsSlotId);
    }

    return 0;
}

int radio::imsBearerActivationInd(int slotId,
                                  int indicationType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {

    RLOGD("imsBearerActivationInd: serial %d", serial);

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen != sizeof(RIL_IMS_BearerNotification)) {
            RLOGE("imsBearerActivationInd: invalid response");
            return 0;
        }

        RIL_IMS_BearerNotification *p_cur = (RIL_IMS_BearerNotification *) response;
        int aid = p_cur->aid;
        hidl_string type = convertCharPtrToHidlString(p_cur->type);

        Return<void> retStatus =
                radioService[imsSlotId]->
                mRadioIndicationIms->
                imsBearerActivation(convertIntToRadioIndicationType(indicationType),
                                    aid, type);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsBearerActivationInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                            imsSlotId);
    }

    return 0;
}

int radio::imsBearerDeactivationInd(int slotId,
                                    int indicationType, int serial, RIL_Errno e,
                                    void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    RLOGD("imsBearerDeactivationInd: serial %d", serial);

    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen != sizeof(RIL_IMS_BearerNotification)) {
            RLOGE("imsBearerDeactivationInd: invalid response");
            return 0;
        }

        RIL_IMS_BearerNotification *p_cur = (RIL_IMS_BearerNotification *) response;
        int aid = p_cur->aid;
        hidl_string type = convertCharPtrToHidlString(p_cur->type);

        Return<void> retStatus =
                radioService[imsSlotId]->
                mRadioIndicationIms->
                imsBearerDeactivation(convertIntToRadioIndicationType(indicationType),
                                      aid, type);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsBearerDeactivationInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                              imsSlotId);
    }

    return 0;
}

int radio::imsBearerInitInd(int slotId,
                            int indicationType, int serial, RIL_Errno e,
                            void *response, size_t responseLen) {
    int imsSlotId = toImsSlot(slotId);
    RLOGD("imsBearerInitInd: serial %d", serial);
    if (radioService[imsSlotId] != NULL
            && radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        Return<void> retStatus =
                radioService[imsSlotId]->
                mRadioIndicationIms->
                imsBearerInit(convertIntToRadioIndicationType(indicationType));

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsBearerInitInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                     imsSlotId);
    }

    return 0;
}

int radio::imsDeregDoneInd(int slotId,
                           int indicationType, int token, RIL_Errno e,
                           void *response, size_t responseLen) {

    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL &&
            radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 imsDeregDone(
                                 convertIntToRadioIndicationType(indicationType));

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsDeregDoneInd: radioService[%d]->mRadioIndicationIms = NULL",
                                                                   imsSlotId);
    }
    return 0;
}

int radio::confSRVCCInd(int slotId,
                        int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL &&
            radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("confSRVCCInd: invalid response");
            return 0;
        }
        hidl_vec<int32_t> data;
        int numInts = responseLen / sizeof(int);
        data.resize(numInts);
        int *pInt = (int *) response;

        for (int i = 0; i < numInts; i++) {
            data[i] = (int32_t) pInt[i];
        }

        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->confSRVCC(
                                 convertIntToRadioIndicationType(indicationType), data);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("confSRVCCInd: radioService[%d]->mRadioIndicationMtk = NULL", slotId);
    }

    return 0;
}

int radio::multiImsCountInd(int slotId,
          int indicationType, int token, RIL_Errno e,
          void *response, size_t responseLen) {
    RLOGE("multiImsCountInd: should not enter here");
    return 0;
}

int radio::imsSupportEccInd(int slotId,
                     int indicationType, int token, RIL_Errno e,
                     void *response, size_t responseLen) {
    int imsSlotId = toImsSlot(slotId);
    if (radioService[imsSlotId] != NULL &&
            radioService[imsSlotId]->mRadioIndicationIms != NULL) {

        if (response == NULL || responseLen < sizeof(int)) {
            RLOGE("imsSupportEccInd: invalid response");
            return 0;
        }

        int status = ((int32_t *) response)[0];

        Return<void> retStatus = radioService[imsSlotId]->
                                 mRadioIndicationIms->
                                 imsSupportEcc(
                                 convertIntToRadioIndicationType(indicationType),
                                 status);

        radioService[imsSlotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("imsSupportEccInd: radioService[%d]->mRadioIndicationIms == NULL",
                                                                            imsSlotId);
    }

    return 0;
}

void radio::registerService(RIL_RadioFunctions *callbacks, CommandInfo *commands) {
    using namespace android::hardware;
    int simCount = 1;
    const char *serviceNames[] = {
            android::RIL_getServiceName()
            , RIL2_SERVICE_NAME
            , RIL3_SERVICE_NAME
            , RIL4_SERVICE_NAME
            };

    const char *imsServiceNames[] = {
            IMS_RIL1_SERVICE_NAME
            , IMS_RIL2_SERVICE_NAME
            , IMS_RIL3_SERVICE_NAME
            , IMS_RIL4_SERVICE_NAME
            };

    /* [ALPS03590595]Set s_vendorFunctions and s_commands before registering service to
        null exception timing issue. */
    s_vendorFunctions = callbacks;
    s_commands = commands;

    char tempstr[PROPERTY_VALUE_MAX] = {0};

    property_get("ro.radio.simcount", tempstr, "0");
    simCount = atoi(tempstr);
    if (simCount == 0) {
        simCount = getSimCount();
    }
    configureRpcThreadpool(1, true /* callerWillJoin */);
    for (int i = 0; i < simCount; i++) {
        pthread_rwlock_t *radioServiceRwlockPtr = getRadioServiceRwlock(i);
        int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
        assert(ret == 0);

        radioService[i] = new RadioImpl;
        radioService[i]->mSlotId = i;
        oemHookService[i] = new OemHookImpl;
        oemHookService[i]->mSlotId = i;
        RLOGI("registerService: starting IRadio %s", serviceNames[i]);
        android::status_t status = radioService[i]->registerAsService(serviceNames[i]);
        status = oemHookService[i]->registerAsService(serviceNames[i]);
        int imsSlot = (i % simCount) + (DIVISION_IMS * simCount);
        radioService[imsSlot] = new RadioImpl;
        radioService[imsSlot]->mSlotId = imsSlot;
        RLOGD("radio::registerService: starting IMS IRadio %s, slot = %d, realSlot = %d",
              imsServiceNames[i], radioService[imsSlot]->mSlotId, imsSlot);

        // Register IMS Radio Stub
        status = radioService[imsSlot]->registerAsService(imsServiceNames[i]);
        RLOGD("radio::registerService IRadio for IMS status:%d", status);

        ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);
    }
}

void rilc_thread_pool() {
    joinRpcThreadpool();
}

pthread_rwlock_t * radio::getRadioServiceRwlock(int slotId) {
    pthread_rwlock_t *radioServiceRwlockPtr = &(radioServiceRwlocks[toRealSlot(slotId)]);
    return radioServiceRwlockPtr;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// [Telephony] -------------- Telephony Extension Indication Function ------------------
/////////////////////////////////////////////////////////////////////////////////////////
int radio::resetAttachApnInd(int slotId, int indicationType, int token, RIL_Errno e,
                             void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        Return<void> retStatus
                = radioService[slotId]->mRadioIndicationMtk->resetAttachApnInd(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("resetAttachApnInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

int radio::mdChangeApnInd(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_vec<int32_t> data;

        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("mdChangeApnInd Invalid response: NULL");
            return 0;
        } else {
            int *pInt = (int *) response;
            data.resize(numInts);
            for (int i=0; i<numInts; i++) {
                data[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioIndicationMtk->mdChangedApnInd(
                convertIntToRadioIndicationType(indicationType), data[0]);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("mdChangeApnInd: radioService[%d]->mRadioIndicationMtk == NULL",
                slotId);
    }
    return 0;
}

// MTK-START: SIM
int radio::onVirtualSimOn(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onVirtualSimOn");
        int32_t simInserted = ((int32_t *) response)[0];
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onVirtualSimOn(
                convertIntToRadioIndicationType(indicationType), simInserted);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onVirtualSimOn: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onVirtualSimOff(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onVirtualSimOff");
        int32_t simInserted = ((int32_t *) response)[0];
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onVirtualSimOff(
                convertIntToRadioIndicationType(indicationType), simInserted);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onVirtualSimOff: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onImeiLock(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onImeiLock");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onImeiLock(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onImeiLock: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onImsiRefreshDone(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onImsiRefreshDone");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onImsiRefreshDone(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onImsiRefreshDone: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onCardDetectedInd(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    return 0;
}

Return<void> RadioImpl::setModemPower(int32_t serial, bool isOn) {
    RLOGD("setModemPower: serial: %d, isOn: %d", serial, isOn);
    if (isOn) {
        dispatchVoid(serial, mSlotId, RIL_REQUEST_MODEM_POWERON);
    } else {
        dispatchVoid(serial, mSlotId, RIL_REQUEST_MODEM_POWEROFF);
    }
    return Void();
}

int radio::setModemPowerResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setModemPowerResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setModemPowerResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setModemPowerResponse: radioService[%d]->setModemPowerResponse "
                "== NULL", slotId);
    }
    return 0;
}

int radio::setTrmResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("setTrmResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponseMtk->setTrmResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setTrmResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }
    return 0;
}

// SMS-START
Return<void> RadioImpl::getSmsParameters(int32_t serial) {
    RLOGD("getSmsParameters: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SMS_PARAMS);
    return Void();
}

bool dispatchSmsParametrs(int serial, int slotId, int request, const SmsParams& message) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_SmsParams params;
    memset (&params, 0, sizeof(RIL_SmsParams));

    params.dcs = message.dcs;
    params.format = message.format;
    params.pid = message.pid;
    params.vp = message.vp;

    s_vendorFunctions->onRequest(request, &params, sizeof(params), pRI, pRI->socket_id);

    return true;
}


Return<void> RadioImpl::setSmsParameters(int32_t serial, const SmsParams& message) {
    RLOGD("setSmsParameters: serial %d", serial);
    dispatchSmsParametrs(serial, mSlotId, RIL_REQUEST_SET_SMS_PARAMS, message);
    return Void();
}

Return<void> RadioImpl::setEtws(int32_t serial, int32_t mode) {
    RLOGD("setEtws: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_ETWS, 1, mode);
    return Void();
}

Return<void> RadioImpl::removeCbMsg(int32_t serial, int32_t channelId, int32_t serialId) {
    RLOGD("removeCbMsg: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_REMOVE_CB_MESSAGE, 2, channelId, serialId);
    return Void();
}

Return<void> RadioImpl::getSmsMemStatus(int32_t serial) {
    RLOGD("getSmsMemStatus: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SMS_SIM_MEM_STATUS);
    return Void();
}

Return<void> RadioImpl::setGsmBroadcastLangs(int32_t serial, const hidl_string& langs) {
    RLOGD("setGsmBroadcastLangs: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_GSM_SET_BROADCAST_LANGUAGE, langs.c_str());
    return Void();
}

Return<void> RadioImpl::getGsmBroadcastLangs(int32_t serial) {
    RLOGD("getGsmBroadcastLangs: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GSM_GET_BROADCAST_LANGUAGE);
    return Void();
}

Return<void> RadioImpl::getGsmBroadcastActivation(int32_t serial) {
    RLOGD("getGsmBroadcastActivation: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_GSM_SMS_BROADCAST_ACTIVATION);
    return Void();
}

int radio::getSmsParametersResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        SmsParams params = {};
        if (response == NULL || responseLen != sizeof(RIL_SmsParams)) {
            RLOGE("getSmsParametersResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_SmsParams *p_cur = ((RIL_SmsParams *) response);
            params.format = p_cur->format;
            params.dcs = p_cur->dcs;
            params.vp = p_cur->vp;
            params.pid = p_cur->pid;
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                getSmsParametersResponse(responseInfo, params);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getSmsParametersResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::setSmsParametersResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("setSmsParametersResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setSmsParametersResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setSmsParametersResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::setEtwsResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("setEtwsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setEtwsResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setEtwsResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::removeCbMsgResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("removeCbMsgResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->removeCbMsgResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("removeCbMsgResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::newEtwsInd(int slotId,
        int indicationType, int token, RIL_Errno e, void *response, size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen == 0) {
            RLOGE("newEtwsInd: invalid response");
            return 0;
        }

        EtwsNotification etws = {};
        RIL_CBEtwsNotification *pEtws = (RIL_CBEtwsNotification *)response;
        etws.messageId = pEtws->messageId;
        etws.serialNumber = pEtws->serialNumber;
        etws.warningType = pEtws->warningType;
        etws.plmnId = convertCharPtrToHidlString(pEtws->plmnId);
        etws.securityInfo = convertCharPtrToHidlString(pEtws->securityInfo);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->newEtwsInd(
                convertIntToRadioIndicationType(indicationType), etws);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("newEtwsInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

int radio::getSmsMemStatusResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        SmsMemStatus params = {};
        if (response == NULL || responseLen != sizeof(RIL_SMS_Memory_Status)) {
            RLOGE("getSmsMemStatusResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_SMS_Memory_Status *p_cur = ((RIL_SMS_Memory_Status *) response);
            params.used = p_cur->used;
            params.total = p_cur->total;
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                getSmsMemStatusResponse(responseInfo, params);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getSmsMemStatusResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::setGsmBroadcastLangsResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("setGsmBroadcastLangsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setGsmBroadcastLangsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setGsmBroadcastLangsResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::getGsmBroadcastLangsResponse(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                getGsmBroadcastLangsResponse(
                responseInfo, convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getGsmBroadcastLangsResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::getGsmBroadcastActivationRsp(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int *pInt = (int *) response;
        int activation = pInt[0];
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                getGsmBroadcastActivationRsp(responseInfo, activation);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getGsmBroadcastActivationRsp: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::meSmsStorageFullInd(int slotId,
        int indicationType, int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("meSmsStorageFullInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->meSmsStorageFullInd(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("meSmsStorageFullInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::smsReadyInd(int slotId,
        int indicationType, int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("smsReadyInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->smsReadyInd(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("smsReadyInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;

}
// SMS-END

int radio::responsePsNetworkStateChangeInd(int slotId,
                                           int indicationType, int token, RIL_Errno e,
                                           void *response, size_t responseLen) {

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_vec<int32_t> data;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("responsePsNetworkStateChangeInd Invalid response: NULL");
            return 0;
        } else {
            int *pInt = (int *) response;
            data.resize(numInts);
            for (int i=0; i<numInts; i++) {
                data[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioIndicationMtk->responsePsNetworkStateChangeInd(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responsePsNetworkStateChangeInd: radioService[%d]->responsePsNetworkStateChangeInd == NULL",
                slotId);
    }
    return 0;
}

// MTK-START: RIL_REQUEST_SWITCH_MODE_FOR_ECC
int radio::triggerModeSwitchByEccResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("triggerModeSwitchByEccResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                triggerModeSwitchByEccResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("triggerModeSwitchByEccResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }
    return 0;
}
// MTK-END

int radio::responseCsNetworkStateChangeInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("responseCsNetworkStateChangeInd Invalid response: NULL");
            return 0;
        }
        RLOGD("responseCsNetworkStateChangeInd");
        hidl_vec<hidl_string> data;
        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        data.resize(numStrings);
        for (int i = 0; i < numStrings; i++) {
            data[i] = convertCharPtrToHidlString(resp[i]);
            RLOGD("responseCsNetworkStateChangeInd:: %d: %s", i, resp[i]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->responseCsNetworkStateChangeInd(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responseCsNetworkStateChangeInd: radioService[%d]->responseCsNetworkStateChangeInd == NULL", slotId);
    }
    return 0;
}

int radio::responseInvalidSimInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("responseInvalidSimInd Invalid response: NULL");
            return 0;
        }
        RLOGD("responseInvalidSimInd");
        hidl_vec<hidl_string> data;
        char **resp = (char **) response;
        int numStrings = responseLen / sizeof(char *);
        data.resize(numStrings);
        for (int i = 0; i < numStrings; i++) {
            data[i] = convertCharPtrToHidlString(resp[i]);
            RLOGD("responseInvalidSimInd: %d: %s", i, resp[i]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->responseInvalidSimInd(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responseInvalidSimInd: radioService[%d]->responseInvalidSimInd == NULL", slotId);
    }
    return 0;
}

int radio::responseNetworkEventInd(int slotId,
                                           int indicationType, int token, RIL_Errno e,
                                           void *response, size_t responseLen) {

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("responseNetworkEventInd Invalid response: NULL");
            return 0;
        }
        RLOGD("responseNetworkEventInd");
        hidl_vec<int32_t> data;
        int *pInt = (int *) response;
        int numInts = responseLen / sizeof(int);
        data.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            data[i] = (int32_t) pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->responseNetworkEventInd(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responseNetworkEventInd: radioService[%d]->responseNetworkEventInd == NULL", slotId);
    }
    return 0;
}

int radio::responseModulationInfoInd(int slotId,
                                           int indicationType, int token, RIL_Errno e,
                                           void *response, size_t responseLen) {

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("responseModulationInfoInd Invalid response: NULL");
            return 0;
        }
        RLOGD("responseModulationInfoInd");
        hidl_vec<int32_t> data;
        int *pInt = (int *) response;
        int numInts = responseLen / sizeof(int);
        data.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            data[i] = (int32_t) pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->responseModulationInfoInd(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("responseModulationInfoInd: radioService[%d]->responseModulationInfoInd == NULL", slotId);
    }
    return 0;
}

int radio::dataAllowedNotificationInd(int slotId, int indicationType,
        int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("dataAllowedNotificationInd: invalid response");
            return 0;
        }
        int32_t allowed = ((int32_t *) response)[0];
        RLOGD("dataAllowedNotificationInd[%d]: %d", slotId, allowed);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->dataAllowedNotification(
                convertIntToRadioIndicationType(indicationType), allowed);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("dataAllowedNotificationInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

Return<void> RadioImpl::setApcMode(int32_t serial, int32_t mode,
        int32_t reportMode, int32_t interval) {
    RLOGD("setApcMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_PSEUDO_CELL_MODE, 3,
            mode, reportMode, interval);
    return Void();
}

Return<void> RadioImpl::getApcInfo(int32_t serial) {
    RLOGD("getApcInfo: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_PSEUDO_CELL_INFO);
    return Void();
}

int radio::setApcModeResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setApcModeResponse: serial %d", serial);
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
            = radioService[slotId]->mRadioResponseMtk->setApcModeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setApcModeResponse: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

int radio::getApcInfoResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("getApcInfoResponse: serial %d", serial);
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> pseudoCellInfo;
        if (response == NULL) {
            RLOGE("getApcInfoResponse Invalid response: NULL");
            return 0;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            pseudoCellInfo.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                pseudoCellInfo[i] = (int32_t)(pInt[i]);
            }
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->getApcInfoResponse(
                responseInfo, pseudoCellInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getApcInfoResponse: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

int radio::onPseudoCellInfoInd(int slotId, int indicationType, int token, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("onPseudoCellInfoInd");
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_vec<int32_t> pseudoCellInfo;
        if (response == NULL) {
            RLOGE("onPseudoCellInfoInd Invalid response: NULL");
            return 0;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            pseudoCellInfo.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                pseudoCellInfo[i] = (int32_t)(pInt[i]);
            }
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onPseudoCellInfoInd(
                convertIntToRadioIndicationType(indicationType), pseudoCellInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onPseudoCellInfoInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

Return<void> RadioImpl::getSmsRuimMemoryStatus(int32_t serial) {
    RLOGD("getSmsRuimMemoryStatus: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SMS_RUIM_MEM_STATUS);
    return Void();
}

int radio::getSmsRuimMemoryStatusResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("getSmsRuimMemoryStatusResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        SmsMemStatus status = {};
        if (response == NULL || responseLen != sizeof (RIL_SMS_Memory_Status)) {
            RLOGE("getSmsRuimMemoryStatusResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_SMS_Memory_Status *mem_status = (RIL_SMS_Memory_Status*)response;
            status.used = mem_status->used;
            status.total = mem_status->total;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getSmsRuimMemoryStatusResponse(
                responseInfo, status);
        radioService[slotId]->checkReturnStatus(retStatus);

    } else {
        RLOGE("getSmsRuimMemoryStatusResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }
    return 0;
}

// FastDormancy Begin
bool dispatchFdMode(int serial, int slotId, int request, int mode, int param1, int param2) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_FdModeStructure args;
    args.mode = mode;

    /* AT+EFD=<mode>[,<param1>[,<param2>]] */
    /* For all modes: but mode 0 & 1 only has one argument */
    if (mode == 0 || mode == 1) {
        args.paramNumber = 1;
    }

    if (mode == 2) {
        args.paramNumber = 3;
        args.parameter1 = param1;
        args.parameter2 = param2;
    }

    if (mode == 3) {
        args.paramNumber = 2;
        args.parameter1 = param1;
    }

    s_vendorFunctions->onRequest(request, &args, sizeof(RIL_FdModeStructure), pRI, pRI->socket_id);

    return true;
}

Return<void> RadioImpl::setFdMode(int32_t serial, int mode, int param1, int param2) {
    RLOGD("setFdMode: serial %d mode %d para1 %d para2 %d", serial, mode, param1, param2);
    dispatchFdMode(serial, mSlotId, RIL_REQUEST_SET_FD_MODE, mode, param1, param2);
    return Void();
}

int radio::setFdModeResponse(int slotId, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->setFdModeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setFdModeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}
// FastDormancy End
/// M: eMBMS feature
int radio::sendEmbmsAtCommandResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("sendEmbmsAtCommandResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->sendEmbmsAtCommandResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("sendEmbmsAtCommandResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::embmsAtInfoInd(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    //dbg
    RLOGD("embmsAtInfoInd: slotId:%d", slotId);
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_string info;
        if (response == NULL) {
            RLOGE("embmsAtInfoInd: invalid response");
            return 0;
        } else {
            RLOGD("embmsAtInfoInd[%d]: %s", slotId, (char*)response);
            info = convertCharPtrToHidlString((char *)response);
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->eMBMSAtInfoIndication(
                convertIntToRadioIndicationType(indicationType), info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("embmsAtInfoInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::embmsSessionStatusInd(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen == 0) {
            RLOGE("embmsSessionStatusInd: invalid response");
            return 0;
        }

        int32_t status = ((int32_t *) response)[0];
        RLOGD("embmsSessionStatusInd[%d]: %d", slotId, status);
        Return<void> retStatus =
            radioService[slotId]->mRadioIndicationMtk->eMBMSSessionStatusIndication(
                convertIntToRadioIndicationType(indicationType), status);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("embmsSessionStatusInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
/// M: eMBMS end

// World Phone {
int radio::plmnChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen % sizeof(char *) != 0) {
            RLOGE("plmnChangedIndication: invalid response");
            return 0;
        }
        hidl_vec<hidl_string> plmn;
        char **resp = (char **) response;
        int numStrings = responselen / sizeof(char *);
        plmn.resize(numStrings);
        for (int i = 0; i < numStrings; i++) {
            plmn[i] = convertCharPtrToHidlString(resp[i]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->plmnChangedIndication(
                convertIntToRadioIndicationType(indicationType), plmn);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("plmnChangedIndication: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::registrationSuspendedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen % sizeof(int) != 0) {
            RLOGE("registrationSuspendedIndication: invalid response");
            return 0;
        }
        hidl_vec<int32_t> sessionId;
        int numInts = responselen / sizeof(int);
        int *pInt = (int *) response;
        sessionId.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            sessionId[i] = pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->registrationSuspendedIndication(
                convertIntToRadioIndicationType(indicationType), sessionId);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("registrationSuspendedIndication: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::gmssRatChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen % sizeof(int) != 0) {
            RLOGE("gmssRatChangedIndication: invalid response");
            return 0;
        }
        hidl_vec<int32_t> gmss;
        int numInts = responselen / sizeof(int);
        int *pInt = (int *) response;
        gmss.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            gmss[i] = pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->gmssRatChangedIndication(
                convertIntToRadioIndicationType(indicationType), gmss);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("gmssRatChangedIndication: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::worldModeChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen % sizeof(int) != 0) {
            RLOGE("worldModeChangedIndication: invalid response");
            return 0;
        }
        hidl_vec<int32_t> mode;
        int numInts = responselen / sizeof(int);
        int *pInt = (int *) response;
        mode.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            mode[i] = pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->worldModeChangedIndication(
                convertIntToRadioIndicationType(indicationType), mode);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("worldModeChangedIndication: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// World Phone }

int radio::esnMeidChangeInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("esnMeidChangeInd: invalid response");
            return 0;
        }
        hidl_string esnMeid((const char*)response, responseLen);
        RLOGD("esnMeidChangeInd (0x%s - %d)", esnMeid.c_str(), responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->esnMeidChangeInd(
                convertIntToRadioIndicationType(indicationType),
                esnMeid);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("esnMeidChangeInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}

// PHB START
int radio::phbReadyNotificationInd(int slotId,
                                   int indicationType, int token, RIL_Errno e, void *response,
                                   size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("phbReadyNotificationInd: invalid response");
            return 0;
        }
        RLOGD("phbReadyNotificationInd");
        int32_t isPhbReady = ((int32_t *) response)[0];
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->phbReadyNotification(
                convertIntToRadioIndicationType(indicationType), isPhbReady);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("phbReadyNotificationInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

     return 0;
}  // PHB END

Return<void> RadioImpl::resetRadio(int32_t serial) {
    RLOGD("resetRadio: serial: %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_RESET_RADIO);
    return Void();
}

int radio::resetRadioResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->resetRadioResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("resetRadioResponse: radioService[%d]->resetRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

// / M: BIP {
int radio::bipProactiveCommandInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("bipProactiveCommandInd: invalid response");
            return 0;
        }
        RLOGD("bipProactiveCommandInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->bipProactiveCommand(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("bipProactiveCommandInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// / M: BIP }
// / M: OTASP {
int radio::triggerOtaSPInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("triggerOtaSPInd: invalid response");
            return 0;
        }
        RLOGD("triggerOtaSPInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->triggerOtaSP(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("triggerOtaSPInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// / M: OTASP }

// / M: STK {
int radio::onStkMenuResetInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("onStkMenuResetInd: invalid response");
            return 0;
        }
        RLOGD("onStkMenuResetInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onStkMenuReset(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onStkMenuResetInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// / M: STK }

// M: Data Framework - common part enhancement @{
Return<void> RadioImpl::syncDataSettingsToMd(int32_t serial, const hidl_vec<int32_t>& settings) {
    RLOGD("syncDataSettingsToMd: serial: %d", serial);
    if (settings.size() == 3) {
        dispatchInts(serial, mSlotId, RIL_REQUEST_SYNC_DATA_SETTINGS_TO_MD, 3,
                settings[0], settings[1], settings[2]);
    } else {
        RLOGE("syncDataSettingsToMd: param error, num: %d (should be 3)", (int) settings.size());
    }
    return Void();
}

int radio::syncDataSettingsToMdResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->syncDataSettingsToMdResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("syncDataSettingsToMdResponse: radioService[%d]->resetRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}
// M: Data Framework - common part enhancement @}

// M: Data Framework - CC 33 @{
Return<void> RadioImpl::setRemoveRestrictEutranMode(int32_t serial, int32_t type) {
    RLOGD("setRemoveRestrictEutranMode: serial %d, type %d", serial, type);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_REMOVE_RESTRICT_EUTRAN_MODE, 1, type);
    return Void();
}
int radio::setRemoveRestrictEutranModeResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen){
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setRemoveRestrictEutranModeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setRemoveRestrictEutranModeResponse: radioService[%d]->resetRadioResponse "
                "== NULL", slotId);
    }
    return 0;
}
int radio::onRemoveRestrictEutran(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onRemoveRestrictEutran");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onRemoveRestrictEutran(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onRemoveRestrictEutran: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// M: Data Framework - CC 33 @}

// M: Data Framework - Data Retry enhancement @{
Return<void> RadioImpl::resetMdDataRetryCount(int32_t serial, const hidl_string& apn) {
    RLOGD("resetMdDataRetryCount: serial: %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_RESET_MD_DATA_RETRY_COUNT, apn.c_str());
    return Void();
}

int radio::resetMdDataRetryCountResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->resetMdDataRetryCountResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("resetMdDataRetryCountResponse: radioService[%d]->resetRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}
int radio::onMdDataRetryCountReset(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onMdDataRetryCountReset");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onMdDataRetryCountReset(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onMdDataRetryCountReset: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// M: Data Framework - Data Retry enhancement @}

int radio::onPcoStatus(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("onPcoStatus: invalid response");
            return 0;
        }
        hidl_vec<int32_t> pco;
        int numInts = responseLen / sizeof(int);
        int *pInt = (int *) response;
        pco.resize(numInts);
        for (int i = 0; i < numInts; i++) {
            pco[i] = pInt[i];
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onPcoStatus(
                convertIntToRadioIndicationType(indicationType), pco);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onPcoStatus: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

// M: [LTE][Low Power][UL traffic shaping] @{
int radio::setLteAccessStratumReportResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("setLteAccessStratumReportResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                setLteAccessStratumReportResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setLteAccessStratumReportResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::setLteUplinkDataTransferResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("setLteUplinkDataTransferResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                setLteUplinkDataTransferResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setLteUplinkDataTransferResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::onLteAccessStratumStateChanged(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_vec<int32_t> data;

        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("onLteAccessStratumStateChanged Invalid response: NULL");
            return 0;
        } else {
            int *pInt = (int *) response;
            data.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                data[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioIndicationMtk->onLteAccessStratumStateChanged(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("mdChangeApnInd: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }
    return 0;
}
// M: [LTE][Low Power][UL traffic shaping] @}

// MTK-START: SIM HOT SWAP
int radio::onSimPlugIn(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimPlugIn");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onSimPlugIn(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimPlugIn: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onSimPlugOut(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimPlugOut");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onSimPlugOut(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimPlugOut: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// MTK-END

// MTK-START: SIM MISSING/RECOVERY
int radio::onSimMissing(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimMissing");
        int32_t simInserted = ((int32_t *) response)[0];
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onSimMissing(
                convertIntToRadioIndicationType(indicationType), simInserted);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimMissing: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onSimRecovery(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimRecovery");
        int32_t simInserted = ((int32_t *) response)[0];
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onSimRecovery(
                convertIntToRadioIndicationType(indicationType), simInserted);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimRecovery: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// MTK-END

// MTK-START: SIM COMMON SLOT
int radio::onSimTrayPlugIn(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimTrayPlugIn");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onSimTrayPlugIn(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimTrayPlugIn: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}

int radio::onSimCommonSlotNoChanged(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        RLOGD("onSimCommonSlotNoChanged");
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->
                onSimCommonSlotNoChanged(convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onSimCommonSlotNoChanged: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// MTK-END

int radio::networkInfoInd(int slotId, int indicationType, int token, RIL_Errno e, void *response,
                                   size_t responseLen) {
    RLOGD("networkInfoInd");

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        hidl_vec<hidl_string> networkInfo;
        if (response == NULL) {
            RLOGE("networkInfoInd Invalid networkInfo: NULL");
            return 0;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            networkInfo.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                networkInfo[i] = convertCharPtrToHidlString(resp[i]);
            }
        }
        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->networkInfoInd(
                 convertIntToRadioIndicationType(indicationType), networkInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("networkInfoInd: radioService[%d]->mRadioIndication "
                 "== NULL", slotId);
    }
    return 0;
}

int radio::setRxTestConfigResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setRxTestConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> respAntConf;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("setRxTestConfigResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            respAntConf.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                respAntConf[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setRxTestConfigResponse(responseInfo,
                respAntConf);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setRxTestConfigResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::getRxTestResultResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("getRxTestResultResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> respAntInfo;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("getRxTestResultResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            respAntInfo.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                respAntInfo[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getRxTestResultResponse(responseInfo,
                respAntInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getRxTestResultResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

int radio::getPOLCapabilityResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("getPOLCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> polCapability;
        if (response == NULL) {
            RLOGE("getPOLCapabilityResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            polCapability.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                polCapability[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getPOLCapabilityResponse(
                responseInfo, polCapability);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPOLCapabilityResponse: radioService[%d]->getPOLCapabilityResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::getCurrentPOLListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("getCurrentPOLListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> polList;
        if (response == NULL) {
            RLOGE("getPOLCapabilityResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            polList.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                polList[i] = convertCharPtrToHidlString(resp[i]);
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getCurrentPOLListResponse(
                responseInfo, polList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getPOLCapabilityResponse: radioService[%d]->getPOLCapabilityResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setPOLEntryResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen) {
    RLOGD("setPOLEntryResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->setPOLEntryResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setPOLEntryResponse: radioService[%d]->setPOLEntryResponse "
                "== NULL", slotId);
    }

    return 0;

}

/// M: [Network][C2K] Sprint roaming control @{
int radio::setRoamingEnableResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("setRoamingEnableResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                setRoamingEnableResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setRoamingEnableResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }
    return 0;
}

int radio::getRoamingEnableResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("getRoamingEnableResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<int32_t> config;
        if (response == NULL) {
            RLOGE("getRoamingEnableResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            config.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                config[i] = (int32_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->getRoamingEnableResponse(
                responseInfo, config);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("getRoamingEnableResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }
    return 0;
}
/// @}

// External SIM [START]
bool dispatchVsimEvent(int serial, int slotId, int request,
        uint32_t transactionId, uint32_t eventId, uint32_t simType) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_VsimEvent args;
    args.transaction_id = transactionId;
    args.eventId = eventId;
    args.sim_type = simType;

    s_vendorFunctions->onRequest(request, &args, sizeof(args), pRI, pRI->socket_id);

    return true;
}

bool dispatchVsimOperationEvent(int serial, int slotId, int request,
        uint32_t transactionId, uint32_t eventId, int32_t result,
        int32_t dataLength, const hidl_vec<uint8_t>& data) {

    RLOGD("dispatchVsimOperationEvent: enter id=%d", eventId);

    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        RLOGD("dispatchVsimOperationEvent: pRI is NULL.");
        return false;
    }

    RIL_VsimOperationEvent args;

    memset (&args, 0, sizeof(args));

    // Transcation id
    args.transaction_id = transactionId;
    // Event id
    args.eventId = eventId;
    // Result
    args.result = result;
    // Data length
    args.data_length = dataLength;

    // Data array
    const uint8_t *uData = data.data();
    args.data= (char  *) calloc(1, (sizeof(char) * args.data_length * 2) + 1);
    memset(args.data, 0, ((sizeof(char) * args.data_length * 2) + 1));
    for (int i = 0; i < args.data_length; i++) {
        sprintf((args.data + (i*2)), "%02X", uData[i]);
    }

    //RLOGD("dispatchVsimOperationEvent: id=%d, data=%s", args.eventId, args.data);

    s_vendorFunctions->onRequest(request, &args, sizeof(args), pRI, pRI->socket_id);

    free(args.data);

    return true;
}

Return<void> RadioImpl::sendVsimNotification(int32_t serial, uint32_t transactionId,
        uint32_t eventId, uint32_t simType) {
    RLOGD("sendVsimNotification: serial %d", serial);
    dispatchVsimEvent(serial, mSlotId, RIL_REQUEST_VSIM_NOTIFICATION, transactionId, eventId, simType);
    return Void();
}

Return<void> RadioImpl::sendVsimOperation(int32_t serial, uint32_t transactionId,
        uint32_t eventId, int32_t result, int32_t dataLength, const hidl_vec<uint8_t>& data) {
    RLOGD("sendVsimOperation: serial %d", serial);
    dispatchVsimOperationEvent(serial, mSlotId, RIL_REQUEST_VSIM_OPERATION,
            transactionId, eventId, result, dataLength, data);
    return Void();
}

int radio::vsimNotificationResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {

    RLOGD("vsimNotificationResponse: serial %d, error: %d", serial, e);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        VsimEvent params = {};
        if (response == NULL || responseLen != sizeof(RIL_VsimEvent)) {
            RLOGE("vsimNotificationResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_VsimEvent *p_cur = ((RIL_VsimEvent *) response);
            params.transactionId = p_cur->transaction_id;
            params.eventId = p_cur->eventId;
            params.simType = p_cur->sim_type;
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk->
                vsimNotificationResponse(responseInfo, params);
        radioService[slotId]->checkReturnStatus(retStatus);

    } else {
        RLOGE("vsimNotificationResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::vsimOperationResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {

    RLOGD("vsimOperationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponseMtk->vsimOperationResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("vsimOperationResponse: radioService[%d]->mRadioResponseMtk == NULL", slotId);
    }

    return 0;
}

int radio::onVsimEventIndication(int slotId,
        int indicationType, int token, RIL_Errno e, void *response, size_t responselen) {

    RLOGD("onVsimEventIndication: indicationType %d", indicationType);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responselen == 0) {
            RLOGE("onVsimEventIndication: invalid response");
            return 0;
        }

        VsimOperationEvent event = {};
        RIL_VsimOperationEvent *response_data = (RIL_VsimOperationEvent *)response;
        event.transactionId = response_data->transaction_id;
        event.eventId = response_data->eventId;
        event.result = response_data->result;
        event.dataLength = response_data->data_length;
        event.data = convertCharPtrToHidlString(response_data->data);

        //RLOGD("onVsimEventIndication: id=%d, data_length=%d, data=%s", event.eventId, response_data->data_length, response_data->data);

        Return<void> retStatus = radioService[slotId]->mRadioIndicationMtk->onVsimEventIndication(
                convertIntToRadioIndicationType(indicationType), event);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("onVsimEventIndication: radioService[%d]->mRadioIndicationMtk == NULL", slotId);
    }

    return 0;
}
// External SIM [END]

Return<void> RadioImpl::setVoiceDomainPreference(int32_t serial, int32_t vdp){
    RLOGD("setVoiceDomainPreference: %d", vdp);

    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_VOICE_DOMAIN_PREFERENCE, 1, vdp);

    return Void();
}

int radio::setVoiceDomainPreferenceResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response,
                            size_t responselen) {

    RLOGD("setVoiceDomainPreferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponseIms != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->
                                 mRadioResponseIms->
                                 setVoiceDomainPreferenceResponse(responseInfo);

        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setVoiceDomainPreferenceResponse: radioService[%d]->mRadioResponseIms == NULL",
                slotId);
    }

    return 0;
}

/// M: Ims Data Framework
int radio::dedicatedBearerActivationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("dedicatedBearerActivationInd: serial %d", serial);

    DedicateDataCall ddcResult = {};
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Dedicate_Data_Call_Struct) != 0) {
            RLOGE("dedicatedBearerActivationInd: invalid response");
        } else {
            convertRilDedicatedBearerInfoToHal((RIL_Dedicate_Data_Call_Struct *)response, ddcResult);
            Return<void> retStatus
                    = radioService[slotId]->mRadioIndicationMtk->dedicatedBearerActivationInd(
                    convertIntToRadioIndicationType(indicationType), ddcResult);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("dedicatedBearerActivationInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::dedicatedBearerModificationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("dedicatedBearerModificationInd: serial %d", serial);

    DedicateDataCall ddcResult = {};
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Dedicate_Data_Call_Struct) != 0) {
            RLOGE("dedicatedBearerModificationInd: invalid response");
        } else {
            convertRilDedicatedBearerInfoToHal((RIL_Dedicate_Data_Call_Struct *)response, ddcResult);
            Return<void> retStatus
                    = radioService[slotId]->mRadioIndicationMtk->dedicatedBearerModificationInd(
                    convertIntToRadioIndicationType(indicationType), ddcResult);
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("dedicatedBearerModificationInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::dedicatedBearerDeactivationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("dedicatedBearerDeactivationInd: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndicationMtk != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("dedicatedBearerDeactivationInd: invalid response");
            return 0;
        }
        int32_t cid = ((int32_t *) response)[0];
        Return<void> retStatus
                = radioService[slotId]->mRadioIndicationMtk->dedicatedBearerDeactivationInd(
                convertIntToRadioIndicationType(indicationType), cid);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("dedicatedBearerDeactivationInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilDedicatedBearerInfoToHal(RIL_Dedicate_Data_Call_Struct* ddcResponse,
        DedicateDataCall& ddcResult) {

    RLOGD("convertRilDedicatedBearerInfoToHal");
    ddcResult.ddcId= ddcResponse->ddcId;
    ddcResult.interfaceId = ddcResponse->interfaceId;
    ddcResult.primaryCid = ddcResponse->primaryCid;
    ddcResult.cid = ddcResponse->cid;
    ddcResult.active = ddcResponse->active;
    ddcResult.signalingFlag = ddcResponse->signalingFlag;
    ddcResult.bearerId = ddcResponse->bearerId;

    if (ddcResponse->hasQos) {
        ddcResult.hasQos = 1;
        ddcResult.qos.qci = ddcResponse->qos.qci;
        ddcResult.qos.dlGbr = ddcResponse->qos.dlGbr;
        ddcResult.qos.ulGbr = ddcResponse->qos.ulGbr;
        ddcResult.qos.dlMbr = ddcResponse->qos.dlMbr;
        ddcResult.qos.ulMbr = ddcResponse->qos.ulMbr;
    }

    if (ddcResponse->hasTft) {
        ddcResult.hasTft = 1;
        ddcResult.tft.operation = ddcResponse->tft.operation;
        ddcResult.tft.pfNumber = ddcResponse->tft.pfNumber;
        ddcResult.tft.pfList.resize(ddcResponse->tft.pfNumber);
        for (int i = 0; i < ddcResult.tft.pfNumber; i ++) {
            ddcResult.tft.pfList[i].id = ddcResponse->tft.pfList[i].id;
            ddcResult.tft.pfList[i].precedence = ddcResponse->tft.pfList[i].precedence;
            ddcResult.tft.pfList[i].direction = ddcResponse->tft.pfList[i].direction;
            ddcResult.tft.pfList[i].networkPfIdentifier = ddcResponse->tft.pfList[i].networkPfIdentifier;
            ddcResult.tft.pfList[i].bitmap = ddcResponse->tft.pfList[i].bitmap;
            ddcResult.tft.pfList[i].address = convertCharPtrToHidlString(ddcResponse->tft.pfList[i].address);
            ddcResult.tft.pfList[i].mask = convertCharPtrToHidlString(ddcResponse->tft.pfList[i].mask);
            ddcResult.tft.pfList[i].protocolNextHeader = ddcResponse->tft.pfList[i].protocolNextHeader;
            ddcResult.tft.pfList[i].localPortLow = ddcResponse->tft.pfList[i].localPortLow;
            ddcResult.tft.pfList[i].localPortHigh = ddcResponse->tft.pfList[i].localPortHigh;
            ddcResult.tft.pfList[i].remotePortLow = ddcResponse->tft.pfList[i].remotePortLow;
            ddcResult.tft.pfList[i].remotePortHigh = ddcResponse->tft.pfList[i].remotePortHigh;
            ddcResult.tft.pfList[i].spi = ddcResponse->tft.pfList[i].spi;
            ddcResult.tft.pfList[i].tos = ddcResponse->tft.pfList[i].tos;
            ddcResult.tft.pfList[i].tosMask = ddcResponse->tft.pfList[i].tosMask;
            ddcResult.tft.pfList[i].flowLabel = ddcResponse->tft.pfList[i].flowLabel;
        }

        ddcResult.tft.tftParameter.linkedPfNumber = ddcResponse->tft.tftParameter.linkedPfNumber;
        int linkedPfNumber = ddcResult.tft.tftParameter.linkedPfNumber;
        ddcResult.tft.tftParameter.linkedPfList.resize(linkedPfNumber);
        for (int i = 0; i < linkedPfNumber; i++) {
            ddcResult.tft.tftParameter.linkedPfList[i] = ddcResponse->tft.tftParameter.linkedPfList[i];
        }
    }
    ddcResult.pcscf = convertCharPtrToHidlString(ddcResponse->pcscf);

    RLOGD("[PdnInfo] ddcId:%d, interfaceId:%d, primaryCid:%d, cid:%d, active:%d, signalingFlag:%d, bearerId:%d, pcscf:%s, hasQos:%d, hasTft:%d"
        , ddcResult.ddcId, ddcResult.interfaceId, ddcResult.primaryCid, ddcResult.cid
        , ddcResult.active, ddcResult.signalingFlag, ddcResult.bearerId, ddcResponse->pcscf
        , ddcResult.hasQos, ddcResult.hasTft);
    if (ddcResult.hasQos) {
        RLOGD("[QosInfo] qci:%d, dlGbr:%d, ulGbr:%d, dlMbr:%d, ulMbr:%d"
        , ddcResult.qos.qci, ddcResult.qos.dlGbr, ddcResult.qos.ulGbr, ddcResult.qos.dlMbr, ddcResult.qos.ulMbr);
    }

    if (ddcResult.hasTft) {
        RLOGD("[TftInfo] operation:%d, pfNumber:%d", ddcResult.tft.pfNumber, ddcResult.tft.pfNumber);
        for (int i = 0; i < ddcResult.tft.pfNumber; i ++) {
            RLOGD("[TftInfo] id:%d, precedence:%d, direction:%d, networkPfIdentifier:%d, bitmap:%d, address:%d, mask:%d, protocolNextHeader:%d",
                "localPortLow:%d, localPortHigh:%d, remotePortLow:%d, remotePortHigh:%d, spi:%d, tos:%d, tosMask:%d, flowLabel:%d"
                , ddcResult.tft.pfNumber, ddcResult.tft.pfNumber);
        }
    }
}
/// @}


Return<void> RadioImpl::setWifiSignalLevel(int32_t serial, int32_t phoneId,
            int32_t rssi, int32_t snr) {
    return Void();
}

Return<void> RadioImpl::setWifiEnabled(int32_t serial, int32_t phoneId,
    const hidl_string& ifName, int32_t isEnabled) {
    return Void();
}

Return<void> RadioImpl::setWifiFlightModeEnabled(int32_t serial, int32_t phoneId,
    const hidl_string& ifName, int32_t isWfiEnabled, int32_t isFlightModeOn) {
    return Void();
}

Return<void> RadioImpl::setWifiAssociated(int32_t serial, int32_t phoneId,
        const hidl_string& ifName, int32_t associated, const hidl_string& ssid,
        const hidl_string& apMac) {
    return Void();
}

Return<void> RadioImpl::setWifiIpAddress(int32_t serial, int32_t phoneId,
        const hidl_string& ifName, const hidl_string& ipv4Addr, const hidl_string& ipv6Addr) {
    return Void();
}

Return<void> RadioImpl::setLocationInfo(int32_t serial, int32_t phoneId,
        const hidl_string& accountId, const hidl_string& broadcastFlag, const hidl_string& latitude,
        const hidl_string& longitude, const hidl_string& accuracy, const hidl_string& method,
        const hidl_string& city, const hidl_string& state, const hidl_string& zip,
        const hidl_string& countryCode) {
    return Void();
}

Return<void> RadioImpl::setLocationInfoWlanMac(int32_t serial, int32_t phoneId,
        const hidl_string& accountId, const hidl_string& broadcastFlag, const hidl_string& latitude,
        const hidl_string& longitude, const hidl_string& accuracy, const hidl_string& method,
        const hidl_string& city, const hidl_string& state, const hidl_string& zip,
        const hidl_string& countryCode, const hidl_string& ueWlanMac) {
    return Void();
}

Return<void> RadioImpl::setEmergencyAddressId(int32_t serial,
        int32_t phoneId, const hidl_string& aid) {
    return Void();
}

Return<void> RadioImpl::setNattKeepAliveStatus(int32_t serial, int32_t phoneId,
        const hidl_string& ifName, bool enable,
        const hidl_string& srcIp, int32_t srcPort,
        const hidl_string& dstIp, int32_t dstPort) {
    return Void();
}

int radio::setWifiEnabledResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setWifiAssociatedResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setWifiSignalLevelResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setWifiIpAddressResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setLocationInfoResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setEmergencyAddressIdResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::setNattKeepAliveStatusResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}

int radio::onWifiMonitoringThreshouldChanged(int slotId, int indicationType,
        int token, RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onWifiPdnActivate(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onWfcPdnError(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onPdnHandover(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onWifiRoveout(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onLocationRequest(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onWfcPdnStateChanged(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

int radio::onNattKeepAliveChanged(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responselen) {
    return 0;
}

Return<void> RadioImpl::setE911State(int32_t serial, int32_t state) {
    RLOGD("setE911State: serial %d, state %d", serial, state);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_E911_STATE, 1, state);
    return Void();
}

int radio::setE911StateResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen) {
    RLOGD("setE911StateResponse: serial %d", serial);

    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
            = radioService[slotId]->mRadioResponseMtk->setE911StateResponse(
                    responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("setE911StateResponse: radioService[%d]->mRadioResponseMtk == NULL",
                slotId);
    }

    return 0;
}

Return<void> RadioImpl::setServiceStateToModem(int32_t serial, int32_t voiceRegState,
        int32_t dataRegState, int32_t voiceRoamingType, int32_t dataRoamingType,
        int32_t rilVoiceRegState, int32_t rilDataRegState) {
    RLOGD("setServiceStateToModem: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_SERVICE_STATE);
    if (pRI != NULL) {
        sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
    }
    return Void();
}

int radio::setServiceStateToModemResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioResponseMtk != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, err);
        Return<void> retStatus = radioService[slotId]->mRadioResponseMtk
                ->setServiceStateToModemResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("%s: radioService[%d]->mRadioResponseMtk == NULL", __func__, slotId);
    }
    return 0;
}

int radio::onTxPowerIndication(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen) {
    return 0;
}
