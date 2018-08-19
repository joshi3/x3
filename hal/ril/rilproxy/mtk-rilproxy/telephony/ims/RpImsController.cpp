/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2015. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

/*****************************************************************************
 * Include
 *****************************************************************************/

#include "RpImsController.h"
#include "RfxStatusDefs.h"
#include "RfxRootController.h"
#include "nw/RpNwStateController.h"
#include "util/RpFeatureOptionUtils.h"
#include <cutils/properties.h>

#define RFX_LOG_TAG "RP_IMS"
#define PROPERTY_3G_SIM "persist.radio.simswitch"
/// M: add for op09 default volte setting @{
#define PROPERTY_VOLTE_STATE    "persist.radio.volte_state"
#define PROPERTY_VOLTE_ENABLE    "persist.mtk.volte.enable"
#define PROPERTY_MULTI_IMS_SUPPORT    "persist.mtk_mims_support"
#define PROPERTY_MD_MIMS_SUPPORT      "ro.mtk_md_mims_support"
#define CARD_TYPE_INVALID          -1
#define CARD_TYPE_NONE             0
#define READ_ICCID_RETRY_TIME      500
#define READ_ICCID_MAX_RETRY_COUNT 12
static const char PROPERTY_ICCID_SIM[4][25] = {
    "ril.iccid.sim1",
    "ril.iccid.sim2",
    "ril.iccid.sim3",
    "ril.iccid.sim4"
};
static const char PROPERTY_LAST_ICCID_SIM[4][29] = {
    "persist.radio.lastsim1_iccid",
    "persist.radio.lastsim2_iccid",
    "persist.radio.lastsim3_iccid",
    "persist.radio.lastsim4_iccid"
};
bool RpImsController::sInitDone = false;
char RpImsController::sLastBootIccId[4][21] = {{0}, {0}, {0}, {0}};
int RpImsController::sLastBootVolteState = 0;
/// @}

/*****************************************************************************
 * Class RfxImsController
 * The class is created if the slot is single mode, LWG or C,
 * During class life time always communicate with one modem, gsm or c2k.
 *****************************************************************************/

RFX_IMPLEMENT_CLASS("RpImsController", RpImsController, RfxController);

RpImsController::RpImsController() :
    mIsInImsCall(false),
    mIsImsRegistered(false),
    mIsImsEnabled(false),
    mNoIccidTimerHandle(NULL),
    mNoIccidRetryCount(0),
    mMainSlotId(0),
    mIsBootUp(true),
    mIsSimSwitch(false) {
}

RpImsController::~RpImsController() {
}

void RpImsController::requestImsDeregister(int slotId, const sp<RfxAction>& action) {
    RFX_UNUSED(slotId);

    logD(RFX_LOG_TAG, "requestImsDeregister: ImsEnabled=%d InImsCall=%d ", mIsImsEnabled,
            mIsInImsCall);

    if (mIsImsEnabled == false) {
        if (action != NULL) {
            action->act();
        }
    } else {
        mDeregConfirmActionQueue.add(action);
        logD(RFX_LOG_TAG, "Current queued confirm action size=%zu",
                mDeregConfirmActionQueue.size());
        if (mIsInImsCall == true) {
            // just wait Ims call is terminated and then send ims dereg request
        } else {
            sendImsDeRegRequest();
        }
    }
}

void RpImsController::onInit() {
    RfxController::onInit();  // Required: invoke super class implementation

    logD(RFX_LOG_TAG, "onInit");

    getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_RADIO_STATE,
        RfxStatusChangeCallback(this, &RpImsController::onRadioStateChanged));
    getNonSlotScopeStatusManager()->registerStatusChanged(RFX_STATUS_KEY_MAIN_CAPABILITY_SLOT,
        RfxStatusChangeCallback(this, &RpImsController::onMainCapabilitySlotChanged));

    /// M: add for op09 default volte setting @{
    initOp09Ims();
    /// @}

    /// M: sync mims capability for legacy @{
    sync_mims_capa();
    /// @}

    const int request_id_list[] = {
            RIL_REQUEST_IMS_DEREG_NOTIFICATION,
            RIL_REQUEST_IMS_REGISTRATION_STATE,
        };
    const int urc_id_list[] = {
            RIL_UNSOL_IMS_ENABLE_START,      // +EIMS: 1
            RIL_UNSOL_IMS_DISABLE_DONE,      // +EIMCFLAG:0
            RIL_UNSOL_IMS_REGISTRATION_INFO, // +CIREGU:
            RIL_UNSOL_IMS_DEREG_DONE,        // +EIMSDEREG
            RIL_LOCAL_GSM_UNSOL_IMS_CALL_EXIST,
            /// M: add for op09 default volte setting @{
            RIL_UNSOL_VOLTE_SETTING,
            /// @}
        };

    // register request & URC id list
    // NOTE. one id can only be registered by one controller
    registerToHandleRequest(request_id_list,
            sizeof(request_id_list) / sizeof(int));
    registerToHandleUrc(urc_id_list, sizeof(urc_id_list) / sizeof(int));
}

void RpImsController::onDeinit() {
    logD(RFX_LOG_TAG, "onDeinit");
    getStatusManager()->unRegisterStatusChanged(RFX_STATUS_KEY_RADIO_STATE,
        RfxStatusChangeCallback(this, &RpImsController::onRadioStateChanged));
    getNonSlotScopeStatusManager()->unRegisterStatusChanged(RFX_STATUS_KEY_MAIN_CAPABILITY_SLOT,
        RfxStatusChangeCallback(this, &RpImsController::onMainCapabilitySlotChanged));
    /// M: add for op09 default volte setting @{
    deinitOp09Ims();
    /// @}
    RfxController::onDeinit();
}

bool RpImsController::onHandleRequest(const sp<RfxMessage>& message) {
    logD(RFX_LOG_TAG, "Handle request %s", message->toString().string());
    switch (message->getId()) {
    case RIL_REQUEST_IMS_DEREG_NOTIFICATION:
        handleImsDeRegRequest(message);
        break;
    case RIL_REQUEST_IMS_REGISTRATION_STATE:
        handleImsRegStateRequest(message);
        break;
    default:
        logD(RFX_LOG_TAG, "unknown request, ignore!");
        return false;
    }
    return true;
}

bool RpImsController::onHandleResponse(const sp<RfxMessage>& message) {
    logD(RFX_LOG_TAG, "Handle response %s.", message->toString().string());
    switch (message->getId()) {
    case RIL_REQUEST_IMS_DEREG_NOTIFICATION:
        handleImsDeRegResponse(message);
        break;
    case RIL_REQUEST_IMS_REGISTRATION_STATE:
        handleImsRegStateResponse(message);
        break;
    default:
        logD(RFX_LOG_TAG, "unknown request, ignore!");
        return false;
    }
    return true;
}

bool RpImsController::onHandleUrc(const sp<RfxMessage>& message) {

    logD(RFX_LOG_TAG, "Handle URC %s", message->toString().string());

    switch (message->getId()) {
    case RIL_UNSOL_IMS_ENABLE_START: {
        handleImsEnableStartUrc(message);
        break;
    }
    case RIL_UNSOL_IMS_DISABLE_DONE: {
        handleImsDisableDoneUrc(message);
        break;
    }
    case RIL_UNSOL_IMS_REGISTRATION_INFO: {
        handleImsRegistartionInfoUrc(message);
        break;
    }
    case RIL_UNSOL_IMS_DEREG_DONE: {
        handleImsDeRegDoneUrc(message);
        break;
    }
    case RIL_LOCAL_GSM_UNSOL_IMS_CALL_EXIST: {
        handleImsCallStatusChange(message);
        break;
    }
    /// M: add for op09 default volte setting @{
    case RIL_UNSOL_VOLTE_SETTING:
        break;
    /// @}
    default:
        logD(RFX_LOG_TAG, "unknown urc, ignore!");
        return false;
    }

    return true;
}

void RpImsController::handleImsDeRegRequest(const sp<RfxMessage>& message) {
    requestToRild(message);
}

void RpImsController::handleImsDeRegResponse(const sp<RfxMessage>& message) {
    responseToRilj(message);
}

void RpImsController::handleImsEnableStartUrc(const sp<RfxMessage>& message) {
    mIsImsEnabled = true;
    responseToRilj(message);
}
void RpImsController::handleImsRegStateRequest(const sp<RfxMessage>& message) {
    requestToRild(message);
}

void RpImsController::handleImsRegStateResponse(const sp<RfxMessage>& message) {
    Parcel *p = message->getParcel();
    int n = p->readInt32();
    int isImsReg = p->readInt32();
    int rilDataRadioTech = 0;
    PsRatFamily ratFamily = PS_RAT_FAMILY_UNKNOWN;

    // Check SMSoIMS debug flag first
    char smsformat[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.radio.smsformat.test", smsformat, "");

    if ((strlen(smsformat) == 4) && (strncmp(smsformat, "3gpp", 4) == 0)) {
        p->writeInt32(PS_RAT_FAMILY_GSM);
    } else if ((strlen(smsformat) == 5) &&  (strncmp(smsformat, "3gpp2", 5) == 0)) {
        p->writeInt32(PS_RAT_FAMILY_CDMA);
    } else {
        RfxNwServiceState serviceState = getStatusManager()
            ->getServiceStateValue(RFX_STATUS_KEY_SERVICE_STATE);
        rilDataRadioTech = serviceState.getRilDataRadioTech();

        RpNwStateController* nwStateController = (RpNwStateController *) findController(
                getSlotId(), RFX_OBJ_CLASS_INFO(RpNwStateController));
        ratFamily = nwStateController->getPsRatFamily(rilDataRadioTech);

        if (ratFamily == PS_RAT_FAMILY_IWLAN) {
            // treat IWLAN as 3GPP
            p->writeInt32(PS_RAT_FAMILY_GSM);
        } else {
            //PS_RAT_FAMILY_UNKNOWN = 0,
            //PS_RAT_FAMILY_GSM = 1,
            //PS_RAT_FAMILY_CDMA = 2,
            p->writeInt32(ratFamily);
        }
    }

    // read back for double check
    message->resetParcelDataStartPos();
    p->readInt32();
    p->readInt32();
    int format = p->readInt32();
    logD(RFX_LOG_TAG, "rilDataRadioTech=%d, smsformat=%s, ratFamily=%d, isImsReg=%d, format=%d",
            rilDataRadioTech, smsformat, ratFamily, isImsReg, format);

    responseToRilj(message);
}

void RpImsController::handleImsDisableDoneUrc(const sp<RfxMessage>& message) {
    mIsInImsCall = false;
    mIsImsRegistered = false;
    mIsImsEnabled = false;
    replyImsDeregConfirmAction();
    responseToRilj(message);
}

void RpImsController::handleImsRegistartionInfoUrc(const sp<RfxMessage>& message) {

    Parcel *p = message->getParcel();
    int numOfParameters = p->readInt32();
    int isImsReg = p->readInt32();
    logD(RFX_LOG_TAG, "handleImsRegistartionInfoUrc ImsReg=%d", isImsReg);

    if (isImsReg == 0) {
        mIsImsRegistered = false;
        mIsInImsCall = false;
        replyImsDeregConfirmAction();
    } else {
        mIsImsRegistered = true;
    }
    responseToRilj(message);
}

void RpImsController::handleImsDeRegDoneUrc(const sp<RfxMessage>& message) {
    mIsInImsCall = false;
    mIsImsRegistered = false;
    replyImsDeregConfirmAction();
    responseToRilj(message);
}

void RpImsController::handleImsCallStatusChange(const sp<RfxMessage>& message) {
    Parcel *p = message->getParcel();
    int numOfParameters = p->readInt32();
    int isImsCallExist = p->readInt32();
    int slotId = getSlotId();
    logD(RFX_LOG_TAG, "handleImsCallStatusChange InImsCall=%d", isImsCallExist);

    if (isImsCallExist == 0) {
        mIsInImsCall = false;
        if (mDeregConfirmActionQueue.isEmpty() == false) {
            if (mIsImsRegistered == false) {
                replyImsDeregConfirmAction();
            } else {
                sendImsDeRegRequest();
            }
        }
    } else {
        mIsInImsCall = true;
    }

    getStatusManager(slotId)->setBoolValue(RFX_STATUS_KEY_IMS_IN_CALL, mIsInImsCall);
}

void RpImsController::sendImsDeRegRequest() {
    // send AT+EIMSDEREG to GSM RILD
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    property_get(PROPERTY_3G_SIM, prop_value, "1");
    int capabilitySim = atoi(prop_value) - 1;
    logD(RFX_LOG_TAG, "sendImsDeRegRequest capabilitySim=%d", capabilitySim);

    sp<RfxMessage> request = RfxMessage::obtainRequest(capabilitySim,
            RADIO_TECH_GROUP_GSM, RIL_REQUEST_IMS_DEREG_NOTIFICATION);
    Parcel* parcel = request->getParcel();
    parcel->writeInt32(1);
    parcel->writeInt32(1);
    requestToRild(request);
}

void RpImsController::replyImsDeregConfirmAction() {
    logD(RFX_LOG_TAG, "replyImsDeregConfirmAction");

    sp<RfxAction> confirmAction;
    while (!mDeregConfirmActionQueue.isEmpty()) {
        confirmAction = mDeregConfirmActionQueue.itemAt(0);
        confirmAction->act();
        mDeregConfirmActionQueue.removeAt(0);
    }
}

void RpImsController::onRadioStateChanged(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant value) {
    RFX_UNUSED(key);
    RFX_UNUSED(old_value);

    RIL_RadioState radioState = (RIL_RadioState) value.asInt();
    logD(RFX_LOG_TAG, "radioState %d", radioState);

    if (radioState == RADIO_STATE_UNAVAILABLE) {
        mIsInImsCall = false;
        mIsImsRegistered = false;
        mIsImsEnabled = false;
        replyImsDeregConfirmAction();
    }
}

/// M: add for op09 volte setting @{
void RpImsController::initOp09Ims() {
    if (!RpFeatureOptionUtils::isCtVolteSupport()) {
        return;
    }
    getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_CARD_TYPE,
        RfxStatusChangeCallback(this, &RpImsController::onCardTypeChanged));

    char tempstr[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_3G_SIM, tempstr, "1");
    mMainSlotId = atoi(tempstr) - 1;

    if (sInitDone == false) {
        for (int slot = 0; slot < SIM_COUNT; slot++) {
            property_get(PROPERTY_LAST_ICCID_SIM[slot], sLastBootIccId[slot], "null");
        }
        char tempstr[PROPERTY_VALUE_MAX] = { 0 };
        property_get(PROPERTY_VOLTE_STATE, tempstr, "0");
        sLastBootVolteState = atoi(tempstr);
        logD(RFX_LOG_TAG, "initOp09Ims, sLastBootVolteState = %d", sLastBootVolteState);
        sInitDone = true;
    }
}

void RpImsController::deinitOp09Ims() {
    if (!RpFeatureOptionUtils::isCtVolteSupport()) {
        return;
    }
    getStatusManager()->unRegisterStatusChanged(RFX_STATUS_KEY_CARD_TYPE,
        RfxStatusChangeCallback(this, &RpImsController::onCardTypeChanged));
}

void RpImsController::sync_mims_capa() {
    char tempstr[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_MULTI_IMS_SUPPORT, tempstr, "-1");
    logD(RFX_LOG_TAG, "check mims key = %s", tempstr);
    if(strcmp("-1", tempstr) != 0){
        property_set(PROPERTY_MD_MIMS_SUPPORT, tempstr);
    }
    else{
        property_set(PROPERTY_MD_MIMS_SUPPORT, "1");
    }
}

void RpImsController::onMainCapabilitySlotChanged(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant new_value) {
    RFX_UNUSED(key);
    if (!RpFeatureOptionUtils::isCtVolteSupport()) {
        return;
    }
    int oldType = old_value.asInt();
    int newType = new_value.asInt();
    logD(RFX_LOG_TAG, "onMainCapabilitySlotChanged()  cap %d->%d, mMainSlotId:%d",
        oldType, newType, mMainSlotId);

    char tempstr[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_3G_SIM, tempstr, "1");
    int mainSlotId = atoi(tempstr) - 1;
    int curSlotId = getSlotId();

    if (RpFeatureOptionUtils::isMultipleImsSupport() == 0) {
        if (mMainSlotId != mainSlotId) {
            if (!RpFeatureOptionUtils::isOp09() && mainSlotId != curSlotId) {
                // For om project, only do the check for main capability sim.
                // Because om's design only need check CT card first insert or switch,
                // other operator sim will not set default volte setting.
                mMainSlotId = mainSlotId;
                return;
            }
            char newIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
            property_get(PROPERTY_ICCID_SIM[curSlotId], newIccIdStr, "");
            logD(RFX_LOG_TAG, "onMainCapabilitySlotChanged, newIccIdStr = %s",
                 givePrintableStr(newIccIdStr));
            if (strlen(newIccIdStr) == 0 || strcmp("N/A", newIccIdStr) == 0) {
                logD(RFX_LOG_TAG, "onMainCapabilitySlotChanged(), iccid not ready");
                // iccid change case, cover by onCardTypeChanged()
                // also set mMainSlotId in onCardTypeChanged()
            } else {
                mMainSlotId = mainSlotId;
                mIsSimSwitch = true;
                int card_type = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);
                setDefaultVolteState(curSlotId, newIccIdStr, card_type);
            }
        }
    }else {
        // Add for CT VoLTE v2.
        char curIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
        property_get(PROPERTY_ICCID_SIM[curSlotId], curIccIdStr, "");
        if (strlen(curIccIdStr) == 0 || strcmp("N/A", curIccIdStr) == 0) {
            logD(RFX_LOG_TAG, "onMainCapabilitySlotChanged(), iccid not ready");
            return;
        }
        mMainSlotId = mainSlotId;
        // Only update non-main slot
        if (mMainSlotId != curSlotId) {
            // CT+CT, subordinate 4g card shall set volte on
            if (strlen(curIccIdStr) != 0 &&
                isOp09SimCard(curSlotId, curIccIdStr, 0)) {
                char peerIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
                int peerSlotId = mMainSlotId;
                property_get(PROPERTY_ICCID_SIM[peerSlotId], peerIccIdStr, "");
                if (strlen(peerIccIdStr) != 0
                        && isOp09SimCard(peerSlotId, peerIccIdStr, 0)) {
                    // dual OP09 SIM, need update
                    setVolteStateProperty(curSlotId, 1);
                    setVolteStateProperty(peerSlotId, 0);
                    setVolteEnableProperty(curSlotId, 1);
                    setVolteEnableProperty(peerSlotId, 0);

                    // send volte state to IMS FW
                    sendDefaultVolteStateUrc(curSlotId, 1);
                    sendDefaultVolteStateUrc(peerSlotId, 0);
                    getStatusManager(curSlotId)
                        -> setIntValue(RFX_STATUS_KEY_VOLTE_STATE, 1);
                    getStatusManager(peerSlotId)
                        -> setIntValue(RFX_STATUS_KEY_VOLTE_STATE, 0);
                }
            }
        }
    }
}

void RpImsController::onCardTypeChanged(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant value) {
    RFX_UNUSED(key);
    // main slot status only update after switching finish, so must use property.
    char tempstr[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_3G_SIM, tempstr, "1");
    int mainSlotId = atoi(tempstr) - 1;
    int curSlotId = getSlotId();

    if (mainSlotId != mMainSlotId) {
        mMainSlotId = mainSlotId;
        mIsSimSwitch = true;
    }
    int old_type = old_value.asInt();
    int new_type = value.asInt();
    logD(RFX_LOG_TAG, "onCardTypeChanged, old_type is %d, new_type is %d, mMainSlotId = %d",
        old_type, new_type, mMainSlotId);

    // if old_type == -1 and new_type == 0, it is bootup without sim.
    // if old_type == -1 and new_type > 0, it is bootup with sim or sim switch finish.
    // if old_type == 0 and new_type > 0, it is plug in sim.
    // if old_type == 0 and new_type == 0, it is still no sim.
    // if old_type > 0 and new_type == -1, it is sim switch start, no need deal yet.
    if (old_type == CARD_TYPE_INVALID || old_type == CARD_TYPE_NONE) {
        if (new_type == CARD_TYPE_NONE) {
            // no sim inserted case
            property_set(PROPERTY_LAST_ICCID_SIM[curSlotId], "null");
            mIsBootUp = false;
            mIsSimSwitch = false;
        } else if (new_type > 0) {
            if (!RpFeatureOptionUtils::isOp09() &&
                RpFeatureOptionUtils::isMultipleImsSupport() == 0 &&
                mMainSlotId != curSlotId) {
                // For om project, only do the check for main capability sim.
                // Because om's design only need check CT card first insert or switch,
                // other operator sim will not set default volte setting.
                return;
            }
            char newIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
            property_get(PROPERTY_ICCID_SIM[curSlotId], newIccIdStr, "");
            logD(RFX_LOG_TAG, "onCardTypeChanged, newIccIdStr = %s",
                 givePrintableStr(newIccIdStr));
            if (strlen(newIccIdStr) == 0 || strcmp("N/A", newIccIdStr) == 0) {
                logD(RFX_LOG_TAG, "onCardTypeChanged, iccid not ready");
                // start timer waiting icc id loaded
                if (mNoIccidTimerHandle != NULL) {
                    RfxTimer::stop(mNoIccidTimerHandle);
                }
                mNoIccidRetryCount = 0;
                mNoIccidTimerHandle = RfxTimer::start(RfxCallback0(this,
                            &RpImsController::onNoIccIdTimeout), ms2ns(READ_ICCID_RETRY_TIME));
            } else {
                setDefaultVolteState(curSlotId, newIccIdStr, new_type);
            }
        }
    } else {
        // if old_type > 0 and new_type == 0, it is plug out sim.
        if (new_type == CARD_TYPE_NONE) {
            property_set(PROPERTY_LAST_ICCID_SIM[curSlotId], "null");
            mIsBootUp = false;
        }

        mIsSimSwitch = false;
    }
}

void RpImsController::onNoIccIdTimeout() {
    mNoIccidTimerHandle = NULL;
    char newIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
    int slotId = getSlotId();
    property_get(PROPERTY_ICCID_SIM[slotId], newIccIdStr, "");
    logD(RFX_LOG_TAG, "onNoIccIdTimeout, newIccIdStr = %s", givePrintableStr(newIccIdStr));

    if (strlen(newIccIdStr) == 0 || strcmp("N/A", newIccIdStr) == 0) {
        // if still fail, need revise timer
        if (mNoIccidRetryCount < READ_ICCID_MAX_RETRY_COUNT &&
            mNoIccidTimerHandle == NULL) {
            mNoIccidRetryCount++;
            mNoIccidTimerHandle = RfxTimer::start(RfxCallback0(this,
                            &RpImsController::onNoIccIdTimeout), ms2ns(READ_ICCID_RETRY_TIME));
        }
    } else {
        int newType = getStatusManager(slotId)->getIntValue(
                RFX_STATUS_KEY_CARD_TYPE, CARD_TYPE_NONE);
        setDefaultVolteState(slotId, newIccIdStr, newType);
    }
}

void RpImsController::setDefaultVolteState(int slot_id, char new_iccid[],int card_type) {
    char oldIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
    // Todo: multi ims need save every sim iccid
    property_get(PROPERTY_LAST_ICCID_SIM[slot_id], oldIccIdStr, "null");
    logD(RFX_LOG_TAG, "setDefaultVolteState, oldIccIdStr = %s", givePrintableStr(oldIccIdStr));

    if (RpFeatureOptionUtils::isMultipleImsSupport() == 0) {
        if (strlen(oldIccIdStr) == 0 || strcmp("null", oldIccIdStr) == 0
         || strcmp(new_iccid, oldIccIdStr) != 0
         || (RpFeatureOptionUtils::isOp09() && (mIsSimSwitch || mIsBootUp))) {
            property_set(PROPERTY_LAST_ICCID_SIM[slot_id], new_iccid);

            int setValue = 1;
            if (isOp09SimCard(slot_id, new_iccid, card_type)) {
                setValue = 0;
            }

            bool isNewSim = true;
            if (mIsBootUp) {
                for (int slot = 0; slot < SIM_COUNT; slot++) {
                    if (strcmp(new_iccid, sLastBootIccId[slot]) == 0) {
                        // For switch sim when powoff case, boot up need get before setting
                        int state = sLastBootVolteState;
                        setValue = (state >> slot) & 1;
                        isNewSim = false;
                        logD(RFX_LOG_TAG,
                            "setDefaultVolteState, change sim slot reboot, last= %d, %d",
                            state, setValue);
                        break;
                    }
                }
                mIsBootUp = false;
            }

            // only OM new non-CT SIM card no need set as default.
            // Other case need set as default.
            if (!RpFeatureOptionUtils::isOp09() &&
                !isOp09SimCard(slot_id, new_iccid, card_type) &&
                isNewSim) {
                return;
            }

            if (mIsSimSwitch && (strcmp(new_iccid, oldIccIdStr) == 0)) {
                char volteState[PROPERTY_VALUE_MAX] = { 0 };
                property_get(PROPERTY_VOLTE_STATE, volteState, "0");
                int stateValue = atoi(volteState);
                setValue = (stateValue >> slot_id) & 1;
                logD(RFX_LOG_TAG, "setDefaultVolteState, sim switch, setValue = %d", setValue);
            } else {
                setVolteStateProperty(slot_id, setValue);
            }
            mIsSimSwitch = false;

            if (mMainSlotId != slot_id) {
                // for single ims, only set volte state and send request for main capability sim.
                return;
            }

            char volteEnable[PROPERTY_VALUE_MAX] = { 0 };
            property_get(PROPERTY_VOLTE_ENABLE, volteEnable, "0");
            int enableValue = atoi(volteEnable);
            for (int i = 0; i < SIM_COUNT; i++) {
                if (i == slot_id) {
                    // if (enableValue != setValue)
                    {
                        // Wos init use property, URC may dealy by imsRilAdapter init delay.
                        if (setValue == 0) {
                            property_set(PROPERTY_VOLTE_ENABLE, "0");
                        } else {
                            property_set(PROPERTY_VOLTE_ENABLE, "1");
                        }
                        // state diff, send volte state to IMS FW
                        sendDefaultVolteStateUrc(slot_id, setValue);
                    }
                    getStatusManager(i)
                        -> setIntValue(RFX_STATUS_KEY_VOLTE_STATE, setValue);
                } else {
                    getStatusManager(i)
                        -> setIntValue(RFX_STATUS_KEY_VOLTE_STATE, 0);
                }
            }
        }
    } else {
        if (strlen(oldIccIdStr) == 0 || strcmp("null", oldIccIdStr) == 0
         || strcmp(new_iccid, oldIccIdStr) != 0) {
            property_set(PROPERTY_LAST_ICCID_SIM[slot_id], new_iccid);

            int setValue = 1;
            if (isOp09SimCard(slot_id, new_iccid, card_type)) {
                setValue = 0;
            }

            bool isNewSim = true;
            if (mIsBootUp) {
                for (int slot = 0; slot < SIM_COUNT; slot++) {
                    if (strcmp(new_iccid, sLastBootIccId[slot]) == 0) {
                        // For switch sim when powoff case, boot up need get before setting
                        int state = sLastBootVolteState;
                        setValue = (state >> slot) & 1;
                        isNewSim = false;
                        logD(RFX_LOG_TAG,
                            "setDefaultVolteState, change sim slot reboot, last= %d, %d",
                            state, setValue);
                        break;
                    }
                }
                mIsBootUp = false;
            }

            // only OM new non-CT SIM card no need set as default.
            // Other case need set as default.
            if (!RpFeatureOptionUtils::isOp09() &&
                !isOp09SimCard(slot_id, new_iccid, card_type) &&
                isNewSim) {
                return;
            }

            // Add for CT VoLTE v2.
            int switchState = getNonSlotScopeStatusManager()->getIntValue(
                                  RFX_STATUS_KEY_CAPABILITY_SWITCH_STATE);
            int dataSim = getNonSlotScopeStatusManager()->getIntValue(
                              RFX_STATUS_KEY_DEFAULT_DATA_SIM);
            if (switchState != CAPABILITY_SWITCH_STATE_IDLE) {
                logD(RFX_LOG_TAG, "setDefaultVolteState, do switching...");
            } else if (dataSim != -1 && dataSim != mMainSlotId) {
                // For CT+CT, main sim will same to data sim.
                logD(RFX_LOG_TAG, "setDefaultVolteState, dataSim != mainSim");
            } else if (isOp09SimCard(slot_id, new_iccid, card_type)) {
                char peerIccIdStr[PROPERTY_VALUE_MAX] = { 0 };
                int peerSlotId = (slot_id == 0 ? 1 : 0);
                property_get(PROPERTY_ICCID_SIM[peerSlotId], peerIccIdStr, "");
                if (strlen(peerIccIdStr) != 0
                        && isOp09SimCard(peerSlotId, peerIccIdStr, 0)) {
                    if (mMainSlotId == slot_id) {
                        setVolteStateProperty(peerSlotId, 1);
                        // send volte state to IMS FW
                        sendDefaultVolteStateUrc(peerSlotId, 1);
                        logD(RFX_LOG_TAG, "setDefaultVolteState, peer = %d", peerSlotId);
                        setValue = 0;
                    } else {
                        setValue = 1;
                    }
                }
            }

            setVolteStateProperty(slot_id, setValue);
            setVolteEnableProperty(slot_id, setValue);

            // state diff, send volte state to IMS FW
            sendDefaultVolteStateUrc(slot_id, setValue);
            getStatusManager(slot_id)
                -> setIntValue(RFX_STATUS_KEY_VOLTE_STATE, setValue);
        }
    }
}

void RpImsController::setVolteEnableProperty(int slot_id, bool isEnable) {
    char volteEnable[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_VOLTE_ENABLE, volteEnable, "0");
    int enableValue = atoi(volteEnable);
    int newEnabledValue = enableValue;

    if (isEnable == 1) {
        newEnabledValue = enableValue | (1 << slot_id);
    } else {
        newEnabledValue = enableValue & (~(1 << slot_id));
    }
    char temp[3] = {0};
    if (newEnabledValue > 10) {
        temp[0] = '1';
        temp[1] = '0' + (newEnabledValue - 10);
    } else {
        temp[0] = '0' + newEnabledValue;
    }
    property_set(PROPERTY_VOLTE_ENABLE, temp);
    logD(RFX_LOG_TAG, "setVolteEnableProperty, enableValue = %d, new: %d",
         enableValue, newEnabledValue);
}

void RpImsController::setVolteStateProperty(int slot_id, bool isEnable) {
    char volteState[PROPERTY_VALUE_MAX] = { 0 };
    property_get(PROPERTY_VOLTE_STATE, volteState, "0");
    int stateValue = atoi(volteState);

    if (isEnable == 1) {
        stateValue = stateValue | (1 << slot_id);
    } else {
        stateValue = stateValue & (~(1 << slot_id));
    }
    char temp[3] = {0};
    if (stateValue > 10) {
        temp[0] = '1';
        temp[1] = '0' + (stateValue - 10);
    } else {
        temp[0] = '0' + stateValue;
    }
    property_set(PROPERTY_VOLTE_STATE, temp);
    logD(RFX_LOG_TAG, "setVolteStateProperty, new volte_state = %d, %s", stateValue,
        temp);
}

bool RpImsController::isOp09SimCard(int slot_id, char icc_id[], int card_type) {
    RFX_UNUSED(slot_id);
    RFX_UNUSED(card_type);
    bool isOp09Card = false;
    if (strncmp(icc_id, "898603", 6) == 0 ||
        strncmp(icc_id, "898611", 6) == 0 ||
        strncmp(icc_id, "8985302", 7) == 0 ||
        strncmp(icc_id, "8985307", 7) == 0) {
        isOp09Card = true;
    }
    return isOp09Card;
}

void RpImsController::sendDefaultVolteStateUrc(int slot_id, int value) {
    // send default volte state to GSM RILD
    sp<RfxMessage> urcToRilj = RfxMessage::obtainUrc(slot_id,
            RIL_UNSOL_VOLTE_SETTING);
    Parcel* parcel = urcToRilj->getParcel();
    parcel->writeInt32(2);
    parcel->writeInt32(value);
    parcel->writeInt32(slot_id);
    responseToRilj(urcToRilj);
}

const char* RpImsController::givePrintableStr(const char* iccId) {
    static char iccIdToPrint[PROPERTY_VALUE_MAX] = { 0 };
    if (strlen(iccId) > 6) {
        strncpy(iccIdToPrint, iccId, 6);
        return iccIdToPrint;
    }
    return iccId;
}
/// @}
