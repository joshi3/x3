/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
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
#include "RpSimControllerBase.h"
#include "RpSimController.h"
#include "RfxStatusDefs.h"
#include <log/log.h>
#include <binder/Parcel.h>
#include <cutils/properties.h>
#include <cutils/jstring.h>
#include "util/RpFeatureOptionUtils.h"
#include "nw/RpNwRatController.h"
#include "RfxRilUtils.h"
#include <telephony/librilutilsmtk.h>

/*****************************************************************************
 * Constants definition
 *****************************************************************************/
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "RpSimController"
#endif

/*****************************************************************************
 * Class RfxController
 *****************************************************************************/

RFX_IMPLEMENT_CLASS("RpSimController", RpSimController, RpSimControllerBase);

int RpSimController::sTrayPluginCount = 0;

RpSimController::RpSimController() :
    mTimerRetryCount(0),
    mIsGetSimStatusResume(false),
    mCurrentGetSimStatusReq(0),
    mCurrentPToken(0),
    mResponsedToken(-1),
    mGsmGetSimStatusReq(0),
    mC2kGetSimStatusReq(0),
    mGsmSimStatusRespParcel(NULL),
    mC2kSimStatusRespParcel(NULL),
    mGetSimStatusErr(RIL_E_SUCCESS),
    mLastCardType(-1),
    mGsmSimCtrl(NULL),
    mC2kSimCtrl(NULL),
    mTimerHandler(0) {
    mSimControllerLog = new int[LOG_CHECK_COUNT];
    for (int i = 0; i < LOG_CHECK_COUNT; i++) {
        mSimControllerLog[i] = 0;
    }
}

RpSimController::~RpSimController() {
    if (mSimControllerLog != NULL) {
        delete[] mSimControllerLog;
        mSimControllerLog = NULL;
    }
    stopTimerForIccidCheck();
}

void RpSimController::onInit() {
    RpSimControllerBase::onInit(); // Required: invoke super class implementation

    RLOGD("[RpSimController] onInit %d (slot %d)", mCurrentGetSimStatusReq, getSlotId());

    // Create Gsm SIM Controller and C2K SIM Controller
    RFX_OBJ_CREATE(mGsmSimCtrl, RpGsmSimController, this);
    RFX_OBJ_CREATE(mC2kSimCtrl, RpC2kSimController, this);

    const int request_id_list[] = {
        RIL_REQUEST_GET_SIM_STATUS,
        RIL_REQUEST_GET_IMSI,
        RIL_REQUEST_ENTER_SIM_PIN,
        RIL_REQUEST_ENTER_SIM_PUK,
        RIL_REQUEST_ENTER_SIM_PIN2,
        RIL_REQUEST_ENTER_SIM_PUK2,
        RIL_REQUEST_CHANGE_SIM_PIN,
        RIL_REQUEST_CHANGE_SIM_PIN2,
        RIL_REQUEST_QUERY_FACILITY_LOCK,
        RIL_REQUEST_SET_FACILITY_LOCK,
        RIL_REQUEST_SIM_IO,
        RIL_REQUEST_SIM_AUTHENTICATION,
        RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC,
        RIL_REQUEST_SIM_OPEN_CHANNEL,
//        RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_SW,
        RIL_REQUEST_SIM_CLOSE_CHANNEL,
        RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL,
        RIL_REQUEST_SIM_GET_ATR,
        RIL_REQUEST_SET_UICC_SUBSCRIPTION,
        RIL_REQUEST_NV_WRITE_ITEM,
        RIL_REQUEST_NV_RESET_CONFIG,
        RIL_LOCAL_GSM_REQUEST_SWITCH_CARD_TYPE,
        RIL_LOCAL_REQUEST_SIM_GET_EFDIR,
    };

    const int urc_id_list[] = {
        RIL_LOCAL_GSM_UNSOL_CARD_TYPE_NOTIFY,
        RIL_LOCAL_GSM_UNSOL_CT3G_DUALMODE_CARD,
        RIL_UNSOL_SIM_PLUG_IN,
        RIL_UNSOL_TRAY_PLUG_IN,
        RIL_LOCAL_GSM_UNSOL_ESIMIND_APPLIST,
    };

    const int switch_cdmaslot_return_error_id_list[] = {
        RIL_REQUEST_GET_SIM_STATUS
    };

    if (RpFeatureOptionUtils::isC2kSupport()) {
        registerToHandleRequest(request_id_list, (sizeof(request_id_list)/sizeof(int)));
        registerToHandleUrc(urc_id_list, (sizeof(urc_id_list)/sizeof(int)));
    }
    addToBlackListForSwitchCDMASlot(switch_cdmaslot_return_error_id_list,
            (sizeof(switch_cdmaslot_return_error_id_list)/sizeof(int)));

    getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, -1);
    getStatusManager()->setBoolValue(RFX_STATUS_KEY_MODEM_SIM_TASK_READY, false, true);

    // register callbacks to get RADIO_UNAVAILABLE in order to reset
    // RFX_STATUS_KEY_MODEM_SIM_TASK_READY.
    getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_RADIO_STATE,
        RfxStatusChangeCallback(this, &RpSimController::onRadioStateChanged));

#if 0
    const int urc_id_list[] = {
        RIL_UNSOL_ON_USSD,
        RIL_UNSOL_ON_USSD_REQUEST,
        RIL_UNSOL_DATA_CALL_LIST_CHANGED
    };

    // register request & URC id list
    // NOTE. one id can only be registered by one controller
    if (RpFeatureOptionUtils::isC2kSupport()) {
        registerToHandleRequest(request_id_list, 3);
        registerToHandleUrc(urc_id_list, 3);
    }
    // register callbacks to get required information
    getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_AVAILABLE,
        RfxStatusChangeCallback(this, &RfxHelloController::onAvailable));

    getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_POWER_ON,
        RfxStatusChangeCallback(this, &RfxHelloController::onPowerOn));

    RfxSampleController *sample_controller;

    // create the object of RfxSampleController as the child controller
    // of this object (an instance of RfxHelloController)
    RFX_OBJ_CREATE(sample_controller, RfxSampleController, this);

    // connect the signal defined by another module
    sample_controller->m_something_changed_singal.connect(this,
                                    &RfxHelloController::onSampleControlerSomethingChanged);
#endif
}

#if 0
void RpSimController::onAvailable(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant value) {
}

void RpSimController::onPowerOn(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant value) {
}
#endif

bool RpSimController::onPreviewMessage(const sp<RfxMessage>& message) {
    if (message->getType() == URC) {
        return true;
    }
    if (message->getType() == REQUEST && message->getId() == RIL_REQUEST_GET_SIM_STATUS) {
        int cardType = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);
        if ((mCurrentGetSimStatusReq != 0) || (cardType == -1)
                || mC2kSimCtrl->onPreviewCheckRequestGetSimStatus()) {
            if (mSimControllerLog[0] != 1) {
                RLOGD("[RpSimController] Preview SIM status:(%s, %d, %d, %d, %d, %d)(slot %d)",
                        requestToString(message->getId()), mCurrentGetSimStatusReq, cardType,
                        mC2kSimCtrl->onPreviewCheckRequestGetSimStatus(),
                        message->getPToken(), mIsGetSimStatusResume, getSlotId());
                mSimControllerLog[0] = 1;
            }
            return false;
        } else {
            mSimControllerLog[0] = 0;
            mIsGetSimStatusResume = false;
        }
    } else if (message->getType() == REQUEST && message->getId() == RIL_REQUEST_GET_IMSI) {
        if (mC2kSimCtrl->onPreviewCheckRequestGetImsi(message)) {
            RLOGD("[RpSimController] Preview IMSI, put into pending list: %s  (slot %d)",
                    requestToString(message->getId()), getSlotId());
            return false;
        }
    }

    return true;
}

bool RpSimController::onCheckIfResumeMessage(const sp<RfxMessage>& message) {
    if (message->getType() == REQUEST && message->getId() == RIL_REQUEST_GET_SIM_STATUS) {
        int cardType = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);
        if (mCurrentGetSimStatusReq != 0) {
            if (mSimControllerLog[1] != 1) {
                RLOGD("[RpSimController] CheckResume1, (%s, %d, %d, %d, %d, %d) (slot %d)",
                        requestToString(message->getId()), mCurrentGetSimStatusReq, cardType,
                        mC2kSimCtrl->onPreviewCheckRequestGetSimStatus(),
                        message->getPToken(), mIsGetSimStatusResume, getSlotId());
                mSimControllerLog[1] = 1;
            }
        } else if (cardType < 0) {
            if (mSimControllerLog[1] != 2) {
                RLOGD("[RpSimController] CheckResume2, (%s, %d, %d, %d, %d, %d) (slot %d)",
                        requestToString(message->getId()), mCurrentGetSimStatusReq, cardType,
                        mC2kSimCtrl->onPreviewCheckRequestGetSimStatus(),
                        message->getPToken(), mIsGetSimStatusResume, getSlotId());
                mSimControllerLog[1] = 2;
            }
        } else if (mC2kSimCtrl->onPreviewCheckRequestGetSimStatus()) {
            if (mSimControllerLog[1] != 3) {
                RLOGD("[RpSimController] CheckResume3 %s %d, %d, %d, %d, %d, %d, %d, %d, slot %d",
                        requestToString(message->getId()), mCurrentGetSimStatusReq, cardType,
                        mC2kSimCtrl->onPreviewCheckRequestGetSimStatus(),
                        getNonSlotScopeStatusManager()->getIntValue(
                        RFX_STATUS_KEY_CDMALTE_MODE_SLOT_READY), getNonSlotScopeStatusManager()
                        ->getIntValue(RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT),
                        getStatusManager()->getBoolValue(RFX_STATUS_KEY_CDMA_CARD_READY, false),
                        message->getPToken(), mIsGetSimStatusResume, getSlotId());
                mSimControllerLog[1] = 3;
            }
        }
        if ((mCurrentGetSimStatusReq == 0) && (cardType >= 0)
                && !mC2kSimCtrl->onPreviewCheckRequestGetSimStatus()) {
            if (mSimControllerLog[1] != 4) {
                RLOGD("[RpSimController] CheckResume4, (%s, %d, %d, %d, %d, %d) (slot %d)",
                        requestToString(message->getId()), mCurrentGetSimStatusReq, cardType,
                        mC2kSimCtrl->onPreviewCheckRequestGetSimStatus(), message->getPToken(),
                        mIsGetSimStatusResume, getSlotId());
                mSimControllerLog[1] = 4;
            }
            if (mIsGetSimStatusResume) {
                return false;
            } else {
                mIsGetSimStatusResume = true;
                return true;
            }
        }
    } else if (message->getType() == REQUEST && message->getId() == RIL_REQUEST_GET_IMSI) {
        if (!mC2kSimCtrl->onPreviewCheckRequestGetImsi(message)) {
            return true;
        }
    }
    return false;
}

bool RpSimController::onHandleRequest(const sp<RfxMessage>& message) {

    switch (message->getId()) {
    case RIL_REQUEST_GET_SIM_STATUS: {
            handleGetSimStatusReq(message);
            break;
        }
    case RIL_REQUEST_GET_IMSI: {
            handleGetImsiReq(message);
            break;
        }
    case RIL_REQUEST_ENTER_SIM_PIN:
    case RIL_REQUEST_ENTER_SIM_PUK:
    case RIL_REQUEST_ENTER_SIM_PIN2:
    case RIL_REQUEST_ENTER_SIM_PUK2:
    case RIL_REQUEST_CHANGE_SIM_PIN:
    case RIL_REQUEST_CHANGE_SIM_PIN2:
        {
            handlePinPukReq(message);
            break;
        }
    case RIL_REQUEST_QUERY_FACILITY_LOCK: {
            handleQuerySimFacilityReq(message);
            break;
        }
    case RIL_REQUEST_SET_FACILITY_LOCK: {
            handleSetSimFacilityReq(message);
            break;
        }
    case RIL_REQUEST_SIM_OPEN_CHANNEL:
//    case RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_SW:
        {
            handleIccOpenChannelReq(message);
            break;
        }
    case RIL_REQUEST_SIM_CLOSE_CHANNEL: {
            handleIccCloseChannelReq(message);
            break;
        }
    case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL: {
            handleTransmitApduReq(message);
            break;
        }
    case RIL_REQUEST_SIM_GET_ATR: {
            handleGetAtrReq(message);
            break;
        }
    case RIL_REQUEST_SET_UICC_SUBSCRIPTION: {
            handleSetUiccSubscriptionReq(message);
            break;
        }
    case RIL_REQUEST_SIM_IO: {
            handleSimIoReq(message);
            break;
        }
    case RIL_REQUEST_SIM_AUTHENTICATION: {
            handleSimAuthenticationReq(message);
            break;
        }
    case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC: {
            handleSimTransmitApduBasicReq(message);
            break;
        }
    case RIL_REQUEST_NV_WRITE_ITEM:
    case RIL_REQUEST_NV_RESET_CONFIG: {
            handleNvNotSupportReq(message);
            break;
        }
    default:
        return false;
    }
    return true;
}

bool RpSimController::onHandleResponse(const sp<RfxMessage>& message) {

    switch (message->getId()) {
    case RIL_REQUEST_GET_SIM_STATUS: {
            handleGetSimStatusRsp(message);
            break;
        }
    case RIL_REQUEST_GET_IMSI: {
            handleGetImsiRsp(message);
            break;
        }
    case RIL_REQUEST_ENTER_SIM_PIN:
    case RIL_REQUEST_ENTER_SIM_PUK:
    case RIL_REQUEST_ENTER_SIM_PIN2:
    case RIL_REQUEST_ENTER_SIM_PUK2:
    case RIL_REQUEST_CHANGE_SIM_PIN:
    case RIL_REQUEST_CHANGE_SIM_PIN2:
        {
            handlePinPukRsp(message);
            break;
        }
    case RIL_REQUEST_QUERY_FACILITY_LOCK: {
            handleQuerySimFacilityRsp(message);
            break;
        }
    case RIL_REQUEST_SET_FACILITY_LOCK: {
            handleSetSimFacilityRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_OPEN_CHANNEL:
//    case RIL_REQUEST_SIM_OPEN_CHANNEL_WITH_SW:
        {
            handleIccOpenChannelRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_CLOSE_CHANNEL: {
            handleIccCloseChannelRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL: {
            handleTransmitApduRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_GET_ATR: {
            handleGetAtrRsp(message);
            break;
        }
    case RIL_REQUEST_SET_UICC_SUBSCRIPTION: {
            handleSetUiccSubscriptionRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_IO: {
            handleSimIoRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_AUTHENTICATION: {
            handleSimAuthenticationRsp(message);
            break;
        }
    case RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC: {
            handleSimTransmitApduBasicRsp(message);
            break;
        }
    case RIL_LOCAL_REQUEST_SIM_GET_EFDIR: {
            handleSimGetEfdirRsp(message);
            break;
        }
    case RIL_LOCAL_GSM_REQUEST_SWITCH_CARD_TYPE: {
            getStatusManager()->setBoolValue(RFX_STATUS_KEY_SWITCH_CARD_TYPE_STATUS, false);
            break;
         }
    default:
        return false;
    }
    return true;
}

bool RpSimController::onHandleUrc(const sp<RfxMessage>& message) {
    RLOGD("[RpSimController] handle urc %s (slot %d)", urcToString(message->getId()),
            getSlotId());

    switch (message->getId()) {
        case RIL_LOCAL_GSM_UNSOL_CARD_TYPE_NOTIFY:
            handleLocalCardTypeNotify(message);
            break;
        case RIL_LOCAL_GSM_UNSOL_CT3G_DUALMODE_CARD:
            mC2kSimCtrl->setCt3gDualmodeValue(message);
            break;
        case RIL_UNSOL_SIM_PLUG_IN:
            if (getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE) == 0) {
                RLOGD("[RpSimController] Reset card type to -1 (slot %d)", getSlotId());
                getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, -1);
            }
            responseToRilj(message);
            break;
        case RIL_UNSOL_TRAY_PLUG_IN:
            if (isSupportCommonSlot()) {
                int vsimEnabled = 0;
                for (int i = 0; i < RFX_SLOT_COUNT; i++) {
                    if (isVsimEnabledBySlotId(i)) {
                        vsimEnabled = 1;
                        break;
                    }
                }
                RLOGD("[RpSimController] sTrayPluginCount: %d  vsimEnabled: %d (slot %d)",
                        sTrayPluginCount, vsimEnabled, getSlotId());
                // Do not open the optimization when vsim is enabled.
                if (vsimEnabled != 1) {
                    // Use static variable sTrayPluginCount to count the sim number and clear all
                    // of slot's task key vaule for the first tray_plug_in coming.It uses to reduce
                    // mode switch times for common slot plug in.
                    if (sTrayPluginCount == 0) {
                        sTrayPluginCount = RFX_SLOT_COUNT-1;
                        for (int i = 0; i < RFX_SLOT_COUNT; i++) {
                            getStatusManager(i)->setBoolValue(RFX_STATUS_KEY_MODEM_SIM_TASK_READY,
                                    false);
                        }
                    } else {
                        sTrayPluginCount--;
                    }
                }
            }
            responseToRilj(message);
            break;
        case RIL_LOCAL_GSM_UNSOL_ESIMIND_APPLIST:
            setEsimindValue(message);
            break;
        default:
            return false;
    }
    #if 0
    if (message->id == RIL_UNSOL_ON_USSD) {
        // 1. decompress message.parcel to get URC data

        // 2. if any predefined status need to be shared to other modules
        //    set it to status manager, status manager will inform
        //    registed callbacks
        getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, 2);

        RfxTimer::stop(m_timer_handle);

        // 3. if the URC need to be sent to RILJ, send it,
        //    be able to update parceled data if required
        responseToRilj(message);
    }
    #endif
    return true;
}


#if 0
void RpSimController::onTimer() {
    // do something
}
#endif


// We decode response parcel only when we got "ALL" responses which we are waiting.
RIL_CardStatus_v6* RpSimController::decodeGetSimStatusResp() {
    RIL_CardStatus_v6 *cardStatus = (RIL_CardStatus_v6*)calloc(1, sizeof(RIL_CardStatus_v6));
    int i = 0;
    size_t s16Len;
    const char16_t *s16 = NULL;
    int card_state[2] = {-1, -1};
    int universal_pin_state[2] = {-1, -1};
    // For VTS test.
    // Corresponding to rild default index, the default index should be -1
    int gsmIndex[2] = {-1, -1};
    int c2kIndex[2] = {-1, -1};
    int imsIndex[2] = {-1, -1};
    int numApplications[2] = {0, 0};
    int card_status = RFX_SIM_STATE_NOT_READY;
    RIL_AppState appState = RIL_APPSTATE_UNKNOWN;
    RIL_AppStatus* pAppStatus = NULL;
    bool isPIN1Synced = true;

    if (mGsmSimStatusRespParcel != NULL) {
        // Read data from GSM parcel
        card_state[0] = mGsmSimStatusRespParcel->readInt32();
        universal_pin_state[0] = mGsmSimStatusRespParcel->readInt32();
        gsmIndex[0] = mGsmSimStatusRespParcel->readInt32();
        c2kIndex[0] = mGsmSimStatusRespParcel->readInt32();
        imsIndex[0] = mGsmSimStatusRespParcel->readInt32();
        numApplications[0] = mGsmSimStatusRespParcel->readInt32();
        // limit to maximum allowed applications
        if (numApplications[0] > RIL_CARD_MAX_APPS) {
            numApplications[0] = RIL_CARD_MAX_APPS;
        }
        cardStatus->gsm_umts_subscription_app_index = gsmIndex[0];
        // For VTS test.
        // Corresponding to rild default index, the default index should be -1
        cardStatus->ims_subscription_app_index = ((imsIndex[0] != -1)?
                (cardStatus->gsm_umts_subscription_app_index + 1) : imsIndex[0]);
        if (mC2kSimStatusRespParcel == NULL) {
            // There is only GSM parcel for GET_SIM_STATUS
            cardStatus->card_state = (RIL_CardState)card_state[0];
            cardStatus->universal_pin_state = (RIL_PinState)universal_pin_state[0];
            cardStatus->cdma_subscription_app_index = c2kIndex[0];
            cardStatus->num_applications = numApplications[0];
        }

        for (i = 0; i < numApplications[0]; i++) {
            pAppStatus = &cardStatus->applications[i];
            pAppStatus->app_type = (RIL_AppType)mGsmSimStatusRespParcel->readInt32();
            pAppStatus->app_state = (RIL_AppState)mGsmSimStatusRespParcel->readInt32();
            pAppStatus->perso_substate = (RIL_PersoSubstate)mGsmSimStatusRespParcel->readInt32();

            // Copy AID ptr
            s16 = NULL;
            s16Len = 0;
            s16 = mGsmSimStatusRespParcel->readString16Inplace(&s16Len);
            if (s16 != NULL) {
                size_t strLen = strnlen16to8(s16, s16Len);
                pAppStatus->aid_ptr = (char*)calloc(1, (strLen+1)*sizeof(char));
                RFX_ASSERT(pAppStatus->aid_ptr != NULL);
                strncpy16to8(pAppStatus->aid_ptr, s16, strLen);
            }

            // Copy app label ptr
            s16 = NULL;
            s16Len = 0;
            s16 = mGsmSimStatusRespParcel->readString16Inplace(&s16Len);
            if (s16 != NULL) {
                size_t strLen = strnlen16to8(s16, s16Len);
                pAppStatus->app_label_ptr = (char*)calloc(1, (strLen+1)*sizeof(char));
                RFX_ASSERT(pAppStatus->app_label_ptr != NULL);
                strncpy16to8(pAppStatus->app_label_ptr, s16, strLen);
            }

            pAppStatus->pin1_replaced = mGsmSimStatusRespParcel->readInt32();
            pAppStatus->pin1 = (RIL_PinState)mGsmSimStatusRespParcel->readInt32();
            pAppStatus->pin2 = (RIL_PinState)mGsmSimStatusRespParcel->readInt32();
            RLOGD("[RpSimController] (slot %d) GSM Application %d, (%d, %d, %d, %s, %s, %d, %d, %d)",
            getSlotId(), i, pAppStatus->app_type, pAppStatus->app_state, pAppStatus->perso_substate,
            pAppStatus->aid_ptr, pAppStatus->app_label_ptr, pAppStatus->pin1_replaced, pAppStatus->pin1,
            pAppStatus->pin2);
        }

    }

    if (mC2kSimStatusRespParcel != NULL) {
        // Read data from C2K parcel
        card_state[1] = mC2kSimStatusRespParcel->readInt32();
        universal_pin_state[1] = mC2kSimStatusRespParcel->readInt32();
        gsmIndex[1] = mC2kSimStatusRespParcel->readInt32();
        c2kIndex[1] = mC2kSimStatusRespParcel->readInt32();
        imsIndex[1] = mC2kSimStatusRespParcel->readInt32();
        numApplications[1] = mC2kSimStatusRespParcel->readInt32();
        // limit to maximum allowed applications
        if (numApplications[1] > RIL_CARD_MAX_APPS) {
            numApplications[1] = RIL_CARD_MAX_APPS;
        }
        if (mGsmSimStatusRespParcel == NULL) {
            // There is only C2K parcel for GET_SIM_STATUS
            cardStatus->card_state = (RIL_CardState)card_state[1];
            cardStatus->universal_pin_state = (RIL_PinState)universal_pin_state[1];
            cardStatus->gsm_umts_subscription_app_index = -1;
            cardStatus->ims_subscription_app_index = -1;
            cardStatus->num_applications = numApplications[1];
            if (cardStatus->num_applications == 0) {
                cardStatus->cdma_subscription_app_index = -1;
            } else {
                cardStatus->cdma_subscription_app_index = 0;
            }
        } else {
            // There are both GSM parcel and C2K parcel.
            // GSM information, card_state and universal_pin_state are written in GSM parcel handling.
            cardStatus->card_state = (RIL_CardState)card_state[0];
            cardStatus->universal_pin_state = (RIL_PinState)universal_pin_state[0];
            cardStatus->cdma_subscription_app_index =
                    ((cardStatus->ims_subscription_app_index != -1)?
                    (cardStatus->ims_subscription_app_index + 1) :
                    (cardStatus->gsm_umts_subscription_app_index + 1));
            if (cardStatus->cdma_subscription_app_index > RIL_CARD_MAX_APPS - 1) {
                cardStatus->cdma_subscription_app_index = RIL_CARD_MAX_APPS - 1;
            }
            if (numApplications[1] != 0) {
                cardStatus->num_applications = (cardStatus->cdma_subscription_app_index + 1);
            } else {
                cardStatus->num_applications = numApplications[0];
                // CDMA4G, plug out -> get sim status req ->
                // rep (absent, GSM index:-1, CDMA index:-1, but CDMA index = GSM index + 1,
                // adjust CDMA index according to app num) -> card type notify
                cardStatus->cdma_subscription_app_index = -1;
            }
            if (cardStatus->num_applications > RIL_CARD_MAX_APPS) {
                RLOGE("[RpSimController] (slot %d) Card application exceeds max apps!",
                        getSlotId());
                cardStatus->num_applications = 0;
            }
        }

        if ((cardStatus->num_applications != 0) && (numApplications[1] > 0) &&
                (numApplications[1] <= RIL_CARD_MAX_APPS)) {
            pAppStatus = &cardStatus->applications[cardStatus->cdma_subscription_app_index];
            pAppStatus->app_type = (RIL_AppType)mC2kSimStatusRespParcel->readInt32();
            pAppStatus->app_state = (RIL_AppState)mC2kSimStatusRespParcel->readInt32();
            pAppStatus->perso_substate = (RIL_PersoSubstate)mC2kSimStatusRespParcel->readInt32();
            // Copy AID ptr
            s16 = NULL;
            s16Len = 0;
            s16 = mC2kSimStatusRespParcel->readString16Inplace(&s16Len);
            if (s16 != NULL) {
                size_t strLen = strnlen16to8(s16, s16Len);
                pAppStatus->aid_ptr = (char*)calloc(1, (strLen+1)*sizeof(char));
                RFX_ASSERT(pAppStatus->aid_ptr != NULL);
                strncpy16to8(pAppStatus->aid_ptr, s16, strLen);
            }

            // Copy app label ptr
            s16 = NULL;
            s16Len = 0;
            s16 = mC2kSimStatusRespParcel->readString16Inplace(&s16Len);
            if (s16 != NULL) {
                size_t strLen = strnlen16to8(s16, s16Len);
                pAppStatus->app_label_ptr = (char*)calloc(1, (strLen+1)*sizeof(char));
                RFX_ASSERT(pAppStatus->app_label_ptr != NULL);
                strncpy16to8(pAppStatus->app_label_ptr, s16, strLen);
            }

            pAppStatus->pin1_replaced = mC2kSimStatusRespParcel->readInt32();
            pAppStatus->pin1 = (RIL_PinState)mC2kSimStatusRespParcel->readInt32();
            pAppStatus->pin2 = (RIL_PinState)mC2kSimStatusRespParcel->readInt32();

            RLOGD("[RpSimController] (slot %d) C2K Application, (%d, %d, %d, %s, %s, %d, %d, %d)",
                getSlotId(), pAppStatus->app_type, pAppStatus->app_state, pAppStatus->perso_substate,
                pAppStatus->aid_ptr, pAppStatus->app_label_ptr, pAppStatus->pin1_replaced,
                pAppStatus->pin1, pAppStatus->pin2);
        }
    }

    // check if pin1 states are consistent to avoid the case that MDs don't sync.
    if ((cardStatus->num_applications > 1) && (mC2kSimStatusRespParcel != NULL)) {
        appState = cardStatus->applications[0].app_state;
        if ((appState == RIL_APPSTATE_READY) ||
                (appState == RIL_APPSTATE_PIN) ||
                (appState == RIL_APPSTATE_PUK)) {
            for (i = 1; i< cardStatus->num_applications; i++) {
                if (((cardStatus->applications[i].app_state == RIL_APPSTATE_READY) ||
                        (cardStatus->applications[i].app_state == RIL_APPSTATE_PIN) ||
                        (cardStatus->applications[i].app_state == RIL_APPSTATE_PUK)) &&
                        (appState != cardStatus->applications[i].app_state)) {
                    isPIN1Synced = false;
                }
            }
        }
    }
    // reset set appState as default
    appState = RIL_APPSTATE_UNKNOWN;

    RLOGD("[RpSimController] (slot %d) Card status: (%d, %d, %d, %d, %d, %d, %d)", getSlotId(),
            cardStatus->card_state, cardStatus->universal_pin_state,
            cardStatus->gsm_umts_subscription_app_index, cardStatus->ims_subscription_app_index,
            cardStatus->cdma_subscription_app_index, cardStatus->num_applications, isPIN1Synced);
    // MTK-START: AOSP SIM PLUG IN/OUT
    int esimsValue = -1;
    char propEsimsCause[PROPERTY_VALUE_MAX] = { 0 };

    property_get(PROPERTY_ESIMS_CAUSE, propEsimsCause, "-1");
    esimsValue = atoi(propEsimsCause);
    // MTK-END
    // Return error if gsm & c2k modem state are not synchronized with each other
    if (((getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE) == 0) && (
            (card_state[0] == RIL_CARDSTATE_PRESENT) ||
            (card_state[1] == RIL_CARDSTATE_PRESENT))) ||
            ((getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE) != 0) && (
            (card_state[0] == RIL_CARDSTATE_ABSENT) ||
            (card_state[1] == RIL_CARDSTATE_ABSENT))) || !isPIN1Synced) {
        RLOGD("[RpSimController] (slot %d) Return error as card type does not match the status",
                getSlotId());
        // MTK-START: AOSP SIM PLUG IN/OUT
        if (esimsValue != ESIMS_CAUSE_SIM_NO_INIT) {
        // MTK-END
            mGetSimStatusErr = RIL_E_GENERIC_FAILURE;

            if (!isPIN1Synced) {
                // send urc to APP for re-query
                sp<RfxMessage> urcToRilj = RfxMessage::obtainUrc(getSlotId(),
                        RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED);
                responseToRilj(urcToRilj);
            }

            return cardStatus;
        }
    }
    // Set card state according to phone type
    if (cardStatus->card_state == RIL_CARDSTATE_ABSENT) {
        card_status = RFX_SIM_STATE_ABSENT;
        getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, 0);
        // To clear IMSI due to card removed
        getStatusManager()->setString8Value(RFX_STATUS_KEY_GSM_IMSI, String8(""));
        getStatusManager()->setString8Value(RFX_STATUS_KEY_C2K_IMSI, String8(""));
        property_set(PROPERTY_UIM_SUBSCRIBER_ID[getSlotId()], "N/A");

        if ((getStatusManager()->getIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE) != CARD_NOT_INSERTED)
                && !isCdmaLockedCard()) {
            getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, CARD_NOT_INSERTED);
            property_set(PROPERTY_RIL_CDMA_CARD_TYPE[getSlotId()], "");
        }
        getStatusManager()->setIntValue(RFX_STATUS_KEY_ESIMIND_APPLIST, RFX_UICC_APPLIST_NONE);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_CDMA_CARD_READY, false);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_CDMA_FILE_READY, false);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_CT3G_DUALMODE_CARD, false);
    } else if (cardStatus->card_state == RIL_CARDSTATE_PRESENT) {
        // Check phone type and source
        RpNwRatController* nwRatController =
                    (RpNwRatController *)findController(getSlotId(),
                    RFX_OBJ_CLASS_INFO(RpNwRatController));
        int nws_mode = getStatusManager()->getIntValue(RFX_STATUS_KEY_NWS_MODE, -1);

        if ((nws_mode == NWS_MODE_CDMALTE) && (nwRatController->getAppFamilyType() ==
                APP_FAM_3GPP2) && mC2kSimStatusRespParcel != NULL) {
            // C2K
            appState =
                    cardStatus->applications[cardStatus->cdma_subscription_app_index].app_state;
        } else if (mGsmSimStatusRespParcel != NULL) {
            // GSM
            appState =
                    cardStatus->applications[cardStatus->gsm_umts_subscription_app_index].app_state;
        }

        switch (appState) {
            case RIL_APPSTATE_READY:
                card_status = RFX_SIM_STATE_READY;
                break;
            case RIL_APPSTATE_PIN:
            case RIL_APPSTATE_PUK:
            case RIL_APPSTATE_SUBSCRIPTION_PERSO:
                card_status = RFX_SIM_STATE_LOCKED;
                break;
            default:
                card_status = RFX_SIM_STATE_NOT_READY;
                break;
        }
    }
    getStatusManager()->setIntValue(RFX_STATUS_KEY_SIM_STATE, card_status);
    return cardStatus;
}

void RpSimController::handleGetSimStatusReq(const sp<RfxMessage>& message) {
    char tmp[PROPERTY_VALUE_MAX] = {0};

    if (mCurrentGetSimStatusReq == 0) {
        // Check card type
        if (supportCardType(ICC_SIM) || supportCardType(ICC_USIM)) {
            // There is GSM application in the SIM card
            sp<RfxMessage> request = RfxMessage::obtainRequest(RADIO_TECH_GROUP_GSM,
                message->getId(), message);

            // send request to GSM RILD
            requestToRild(request);
            mGsmGetSimStatusReq = message->getId();
        }

        if ((supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) && (getSlotId() ==
                getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT))) {
            // There is C2K application in the SIM card
            sp<RfxMessage> c2k_request = RfxMessage::obtainRequest(RADIO_TECH_GROUP_C2K,
                message->getId(), message);

            // send request to C2K RILD
            requestToRild(c2k_request);
            mC2kGetSimStatusReq = message->getId();
        } else if ((supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) &&
                (!supportCardType(ICC_SIM)) && (!supportCardType(ICC_USIM)) &&
                (getSlotId() != getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT))) {
            getStatusManager()->setIntValue(RFX_STATUS_KEY_SIM_STATE, RFX_SIM_STATE_ABSENT);
            sp<RfxMessage> response = mC2kSimCtrl->responseCdmaAbsentState(message);
            responseToRilj(response);
            RLOGD("[RpSimController] Response RIL_CARDSTATE_ABSENT (slot %d)", getSlotId());

            // It can't distingush CT or non-CT UIM card as no iccid or imsi for non-c slot.
            property_set(PROPERTY_RIL_CDMA_CARD_TYPE[getSlotId()],
                    String8::format("%d", UIM_CARD).string());
            getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, UIM_CARD);

            return;
        }

        if (mGsmGetSimStatusReq == 0 && mC2kGetSimStatusReq == 0) {
            if (getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE) == 0) {
                sp<RfxMessage> request = RfxMessage::obtainRequest(RADIO_TECH_GROUP_GSM,
                    message->getId(), message);
                // send request to GSM RILD
                requestToRild(request);
                mGsmGetSimStatusReq = message->getId();
                RLOGD("[RpSimController] send GET_SIM_STATUS, but no card type (slot %d)",
                        getSlotId());
            } else {
                sp<RfxMessage> response = RfxMessage::obtainResponse(RIL_E_GENERIC_FAILURE,
                        message);
                responseToRilj(response);
                RLOGD("[RpSimController] Response RIL_E_GENERIC_FAILURE (slot %d)", getSlotId());
                return;
            }
        }

        if (mGsmGetSimStatusReq != 0 || mC2kGetSimStatusReq != 0) {
            // At least one request to GSM RILD or C2K RILD
            mCurrentGetSimStatusReq = message->getId();
            mCurrentPToken = message->getPToken();
            RLOGD("[RpSimController] Gsm=%d C2k=%d mCurrentGetSimStatusReq=%d Token=%d (slot %d)",
                    mGsmGetSimStatusReq, mC2kGetSimStatusReq,
                    mCurrentGetSimStatusReq, mCurrentPToken, getSlotId());
        }
    } else {
        RLOGE("[RpSimController] onHandleRequest, why onPreviewMessage not work?! (slot %d)",
                getSlotId());
    }
}

void RpSimController::handleGetSimStatusRsp(const sp<RfxMessage>& message) {
    if ((mCurrentGetSimStatusReq == message->getId()) &&
            (mCurrentPToken == message->getPToken())) {
        RIL_CardStatus_v6 *cardStatus = NULL;
        size_t dataPos = 0;
        Parcel *p = NULL;

        // Check the source of message
        if ((message->getSentOnCdmaCapabilitySlot() == C_SLOT_STATUS_NOT_CURRENT_SLOT) &&
                (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT))) {
            mGsmGetSimStatusReq = 0;
            mC2kGetSimStatusReq = 0;
            mGetSimStatusErr = RIL_E_GENERIC_FAILURE;
            RLOGE("[RpSimController] Response is before dynamic switch: %d !! (slot %d)",
                    message->getSentOnCdmaCapabilitySlot(), getSlotId());
        } else if (message->getSource() == RADIO_TECH_GROUP_GSM && mGsmGetSimStatusReq != 0) {
            //cardStatus = decodeGetSimStatusRespTest(message->getParcel());
            // Clone the GSM GET_SIM_STATUS response parcel
            if (message->getError() == RIL_E_SUCCESS) {
                mGsmSimStatusRespParcel = new Parcel();
                Parcel *p = message->getParcel();
                dataPos = p->dataPosition();
                mGsmSimStatusRespParcel->appendFrom(p, dataPos, (p->dataSize() - dataPos));
                mGsmSimStatusRespParcel->setDataPosition(0);
            } else {
                mGetSimStatusErr = message->getError();
                RLOGE("[RpSimController] Response from GSM got error %d !! (slot %d)",
                        mGetSimStatusErr, getSlotId());
            }
            mGsmGetSimStatusReq = 0;
        } else if (message->getSource() == RADIO_TECH_GROUP_C2K && mC2kGetSimStatusReq != 0) {
            // Clone the C2K GET_SIM_STATUS response parcel
            if (message->getError() == RIL_E_SUCCESS) {
                mC2kSimStatusRespParcel = new Parcel();
                Parcel *p = message->getParcel();
                dataPos = p->dataPosition();
                mC2kSimStatusRespParcel->appendFrom(p, dataPos, (p->dataSize() - dataPos));
                mC2kSimStatusRespParcel->setDataPosition(0);
            } else {
                mGetSimStatusErr = message->getError();
                RLOGE("[RpSimController] Response from C2K got error %d !! (slot %d)",
                        mGetSimStatusErr, getSlotId());
            }
            mC2kGetSimStatusReq = 0;
        } else {
            RLOGE("[RpSimController] Should not receive the response!! (slot %d)", getSlotId());
            return;
        }

        if (mGsmGetSimStatusReq == 0 && mC2kGetSimStatusReq == 0) {
            if (mGetSimStatusErr == RIL_E_SUCCESS) {
                // decode parcel
                cardStatus = decodeGetSimStatusResp();

                if (mGetSimStatusErr != RIL_E_SUCCESS) {
                    sp<RfxMessage> response = RfxMessage::obtainResponse(mGetSimStatusErr,
                        message);
                    // Card state error
                    // get parcel from the message and write data
                    Parcel* p = response->getParcel();

                    p->writeInt32(RIL_CARDSTATE_ERROR);
                    p->writeInt32(RIL_PINSTATE_UNKNOWN);
                    p->writeInt32(-1);
                    p->writeInt32(-1);
                    p->writeInt32(-1);
                    p->writeInt32(0);
                    RLOGD("[RpSimController] handleGetSimStatusResp, return error code (slot %d)",
                            getSlotId());
                    responseToRilj(response);
                } else {
                    sp<RfxMessage> response = RfxMessage::obtainResponse(mGetSimStatusErr,
                        message);
                    // get parcel from the message and write data
                    Parcel* p = response->getParcel();

                    p->writeInt32(cardStatus->card_state);
                    p->writeInt32(cardStatus->universal_pin_state);
                    p->writeInt32(cardStatus->gsm_umts_subscription_app_index);
                    p->writeInt32(cardStatus->cdma_subscription_app_index);
                    p->writeInt32(cardStatus->ims_subscription_app_index);
                    p->writeInt32(cardStatus->num_applications);

                    RIL_AppStatus* pApp = NULL;
                    for (int i = 0; i < cardStatus->num_applications; i++) {
                        pApp = &cardStatus->applications[i];

                        p->writeInt32(pApp->app_type);
                        p->writeInt32(pApp->app_state);
                        p->writeInt32(pApp->perso_substate);
                        writeStringToParcel(p, (const char*)(pApp->aid_ptr));
                        writeStringToParcel(p, (const char*)(pApp->app_label_ptr));
                        p->writeInt32(pApp->pin1_replaced);
                        p->writeInt32(pApp->pin1);
                        p->writeInt32(pApp->pin2);
                    }

                    responseToRilj(response);
                }
            } else {
                sp<RfxMessage> response = RfxMessage::obtainResponse(mGetSimStatusErr,
                    message);
                responseToRilj(response);
                RLOGD("[RpSimController] handleGetSimStatusResp, return error code (slot %d)",
                        getSlotId());
            }

            mCurrentGetSimStatusReq = 0;
            mCurrentPToken = 0;
            mGetSimStatusErr = RIL_E_SUCCESS;

            if (cardStatus != NULL) {
                int i = 0;
                for (i = 0; i < RIL_CARD_MAX_APPS; i++) {
                    // release aid and app label
                    if (cardStatus->applications[i].aid_ptr != NULL) {
                        free(cardStatus->applications[i].aid_ptr);
                    }
                    if (cardStatus->applications[i].app_label_ptr != NULL) {
                        free(cardStatus->applications[i].app_label_ptr);
                    }
                }
                free(cardStatus);
            }
            if (mGsmSimStatusRespParcel != NULL) {
                delete(mGsmSimStatusRespParcel);
                mGsmSimStatusRespParcel = NULL;
            }
            if (mC2kSimStatusRespParcel != NULL) {
                delete(mC2kSimStatusRespParcel);
                mC2kSimStatusRespParcel = NULL;
            }
        }
    } else {
        RLOGI("[RpSimController] mResponsedToken: %d;  message->getPToken(): %d (slot %d)",
                getSlotId());
        if (message->getPToken() != mResponsedToken) {
            sp<RfxMessage> response = RfxMessage::obtainResponse(RIL_E_GENERIC_FAILURE, message);
            responseToRilj(response);
            mResponsedToken = message->getPToken();
        } else {
            mResponsedToken = -1;
        }
    }
}

void RpSimController::handleGetImsiReq(const sp<RfxMessage>& message) {
    Parcel *req = message->getParcel();
    size_t dataPos = 0;
    int count = 0;

    dataPos = req->dataPosition();

    count = req->readInt32();

    if (count == 1) {
        // There is only one parameter "AID" in the request
        char *aid_ptr = strdupReadString(req);
        RLOGD("[RpSimController] handleGetImsiReq aid %s (slot %d)", aid_ptr, getSlotId());
        // Pass the request to RILD directly
        req->setDataPosition(dataPos);
        sp<RfxMessage> request = RfxMessage::obtainRequest(choiceDestViaAid(aid_ptr),
            message->getId(), message, true);
        // Reset the parcel position
        //request->getParcel()->appendFrom(req, dataPos, (req->dataSize() - dataPos));
        // Reset the cloned parcel position
        //request->getParcel()->setDataPosition(0);
        requestToRild(request);

        if (aid_ptr != NULL) {
            free (aid_ptr);
        }
    } else {
        RLOGE("[RpSimController] handleGetImsiReq but payload format is wrong! (slot %d)",
                getSlotId());
    }
}

void RpSimController::handleGetImsiRsp(const sp<RfxMessage>& message) {
    Parcel *req = message->getParcel();
    size_t s16Len = 0;
    size_t dataPos = req->dataPosition();
    Parcel *p = NULL;

    // Check phone type and source
    if (message->getError() == RIL_E_SUCCESS) {
        // Get IMSI then notify.
        String8 str8(req->readString16Inplace(&s16Len));
        // RLOGD("[RpSimController] handleGetImsiRsp : %s", str8.string());
        if (message->getSource() == RADIO_TECH_GROUP_GSM) {
            getStatusManager()->setString8Value(RFX_STATUS_KEY_GSM_IMSI, str8);
        } else if (message->getSource() == RADIO_TECH_GROUP_C2K) {
            getStatusManager()->setString8Value(RFX_STATUS_KEY_C2K_IMSI, str8);
            property_set(PROPERTY_UIM_SUBSCRIBER_ID[getSlotId()], str8.string());
        }
        req->setDataPosition(dataPos);

    }
    sp<RfxMessage> response = RfxMessage::obtainResponse(message->getError(),
                message, true);

    // Reset the parcel position
    //p = response->getParcel();
    //dataPos = req->dataPosition();

    // Send to RILJ directly
    //p->appendFrom(req, dataPos, (req->dataSize() - dataPos));
    //p->setDataPosition(0);

    responseToRilj(response);
}

void RpSimController::handlePinPukReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = choiceDestViaCurrCardType();

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handlePinPukReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handlePinPukReq(message);
    } else {
        RLOGE("[RpSimController] handlePinPukReq, can't dispatch the request!");
    }
}

void RpSimController::handlePinPukRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handlePinPukRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handlePinPukRsp(message);
    } else {
        RLOGE("[RpSimController] handlePinPukRsp, can't dispatch the response!");
    }
}

void RpSimController::handleQuerySimFacilityReq(const sp<RfxMessage>& message) {
    Parcel* p = message->getParcel();
    size_t dataPos = p->dataPosition();
    size_t s16Len = 0;
    const char16_t *s16 = NULL;
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    char* aid = NULL;

    // count string
    p->readInt32();
    // facility
    p->readString16Inplace(&s16Len);
    // password
    s16Len = 0;
    p->readString16Inplace(&s16Len);
    // serviceClass
    s16Len = 0;
    p->readString16Inplace(&s16Len);

    // Get AID
    s16 = NULL;
    s16Len = 0;
    s16 = p->readString16Inplace(&s16Len);
    if (s16 != NULL) {
        size_t strLen = strnlen16to8(s16, s16Len);
        aid = (char*)calloc(1, (strLen+1)*sizeof(char));
        if (NULL != aid) {
            strncpy16to8(aid, s16, strLen);
        } else {
            RLOGE("[RpSimController] allocate aid OOM");
        }
        RLOGD("[RpGsmSimController] handleQuerySimFacilityReq aid %s (slot %d)",
                (aid == NULL)? "" : aid, getSlotId());
    }

    // dest = choiceDestViaAid(aid);
    dest = choiceDestViaCurrCardType();

    if (aid != NULL) {
        free(aid);
    }

    // Reset the payload position in the message before pass to GSM or C2K
    p->setDataPosition(dataPos);

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleQuerySimFacilityReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleQuerySimFacilityReq(message);
    } else {
        RLOGE("[RpSimController] handleQuerySimFacilityReq, can't dispatch the request!");
    }
}

void RpSimController::handleQuerySimFacilityRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleQuerySimFacilityRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleQuerySimFacilityRsp(message);
    } else {
        RLOGE("[RpSimController] handleQuerySimFacilityRsp, can't dispatch the response!");
    }
}

void RpSimController::handleSetSimFacilityReq(const sp<RfxMessage>& message) {
    Parcel* p = message->getParcel();
    size_t dataPos = p->dataPosition();
    size_t s16Len = 0;
    const char16_t *s16 = NULL;
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    char* aid = NULL;

    // count string
    p->readInt32();
    // facility
    p->readString16Inplace(&s16Len);
    // lockString
    s16Len = 0;
    p->readString16Inplace(&s16Len);
    // password
    s16Len = 0;
    p->readString16Inplace(&s16Len);
    // serviceClass
    s16Len = 0;
    p->readString16Inplace(&s16Len);

    // Get AID
    s16 = NULL;
    s16Len = 0;
    s16 = p->readString16Inplace(&s16Len);
    if (s16 != NULL) {
        size_t strLen = strnlen16to8(s16, s16Len);
        aid = (char*)calloc(1, (strLen+1)*sizeof(char));
        if (NULL != aid) {
            strncpy16to8(aid, s16, strLen);
        } else {
            RLOGE("[RpSimController] allocate aid OOM");
        }
        RLOGD("[RpGsmSimController] handleSetSimFacilityReq aid %s (slot %d)",
                (aid == NULL)? "" : aid, getSlotId());
    }

    // dest = choiceDestViaAid(aid);
    dest = choiceDestViaCurrCardType();

    if (aid != NULL) {
        free(aid);
    }

    // Reset the payload position in the message before pass to GSM or C2K
    p->setDataPosition(dataPos);

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSetSimFacilityReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSetSimFacilityReq(message);
    } else {
        RLOGE("[RpSimController] handleSetSimFacilityReq, can't dispatch the request!");
    }
}

void RpSimController::handleSetSimFacilityRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSetSimFacilityRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSetSimFacilityRsp(message);
    } else {
        RLOGE("[RpSimController] handleSetSimFacilityRsp, can't dispatch the response!");
    }
}

void RpSimController::handleIccOpenChannelReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    if (supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) {
        if (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT)) {
            dest = RADIO_TECH_GROUP_C2K;
        }
    }
    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleIccOpenChannelReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleIccOpenChannelReq(message);
    } else {
        RLOGE("[RpSimController] handleIccOpenChannelReq, can't dispatch the request!");
    }
}

void RpSimController::handleIccOpenChannelRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleIccOpenChannelRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleIccOpenChannelRsp(message);
    } else {
        RLOGE("[RpSimController] handleIccOpenChannelRsp, can't dispatch the response!");
    }
}

void RpSimController::handleIccCloseChannelReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    if (supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) {
        if (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT)) {
            dest = RADIO_TECH_GROUP_C2K;
        }
    }
    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleIccCloseChannelReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleIccCloseChannelReq(message);
    } else {
        RLOGE("[RpSimController] handleIccCloseChannelReq, can't dispatch the request!");
    }
}

void RpSimController::handleIccCloseChannelRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleIccCloseChannelRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleIccCloseChannelRsp(message);
    } else {
        RLOGE("[RpSimController] handleIccCloseChannelRsp, can't dispatch the response!");
    }
}

void RpSimController::handleTransmitApduReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    if (supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) {
        if (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT)) {
            dest = RADIO_TECH_GROUP_C2K;
        }
    }
    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleTransmitApduReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleTransmitApduReq(message);
    } else {
        RLOGE("[RpSimController] handleTransmitApduReq, can't dispatch the request!");
    }
}

void RpSimController::handleTransmitApduRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleTransmitApduRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleTransmitApduRsp(message);
    } else {
        RLOGE("[RpSimController] handleTransmitApduRsp, can't dispatch the response!");
    }
}

void RpSimController::handleGetAtrReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    if (supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) {
        if (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT)) {
            dest = RADIO_TECH_GROUP_C2K;
        }
    }
    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleGetAtrReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleGetAtrReq(message);
    } else {
        RLOGE("[RpSimController] handleGetAtrReq, can't dispatch the request!");
    }
}

void RpSimController::handleGetAtrRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleGetAtrRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleGetAtrRsp(message);
    } else {
        RLOGE("[RpSimController] handleGetAtrRsp, can't dispatch the response!");
    }
}

void RpSimController::handleSetUiccSubscriptionReq(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    if (supportCardType(ICC_CSIM) || supportCardType(ICC_RUIM)) {
        if (getSlotId() == getNonSlotScopeStatusManager()->getIntValue(
                RFX_STATUS_KEY_ACTIVE_CDMALTE_MODE_SLOT)) {
            dest = RADIO_TECH_GROUP_C2K;
        }
    }
    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSetUiccSubscriptionReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSetUiccSubscriptionReq(message);
    } else {
        RLOGE("[RpSimController] handleSetUiccSubscriptionReq, can't dispatch the request!");
    }
}

void RpSimController::handleSetUiccSubscriptionRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSetUiccSubscriptionRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSetUiccSubscriptionRsp(message);
    } else {
        RLOGE("[RpSimController] handleSetUiccSubscriptionRsp, can't dispatch the response!");
    }
}

void RpSimController::handleSimIoReq(const sp<RfxMessage>& message) {
    Parcel* p = message->getParcel();
    size_t dataPos = p->dataPosition();
    size_t s16Len = 0;
    const char16_t *s16 = NULL;
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    char* aid = NULL;

    // command
    p->readInt32();
    // field
    p->readInt32();
    // path
    p->readString16Inplace(&s16Len);
    // p1
    p->readInt32();
    // p2
    p->readInt32();
    // p3
    p->readInt32();
    // data
    p->readString16Inplace(&s16Len);
    // PIN2
    p->readString16Inplace(&s16Len);

    // Get AID
    s16 = NULL;
    s16Len = 0;
    s16 = p->readString16Inplace(&s16Len);
    if (s16 != NULL) {
        size_t strLen = strnlen16to8(s16, s16Len);
        aid = (char*)calloc(1, (strLen+1)*sizeof(char));
        RFX_ASSERT(aid != NULL);
        strncpy16to8(aid, s16, strLen);
        // RLOGD("[RpSimController] handleSimIoReq aid %s (slot %d)", aid, getSlotId());
    }
    RLOGD("[RpSimController] handleSimIoReq aid %s (slot %d)", aid, getSlotId());

    dest = choiceDestViaAid(aid);

    if (aid != NULL) {
        free(aid);
    }

    // Reset the payload position in the message before pass to GSM or C2K
    p->setDataPosition(dataPos);

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimIoReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimIoReq(message);
    } else {
        RLOGE("[RpSimController] handleSimIoReq, can't dispatch the request!");
    }

}

void RpSimController::handleSimIoRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimIoRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimIoRsp(message);
    } else {
        RLOGE("[RpSimController] handleSimIoRsp, can't dispatch the response!");
    }
}

void RpSimController::handleSimAuthenticationReq(const sp<RfxMessage>& message) {
    RLOGD("[RpSimController] handleSimAuthenticationReq (slot %d)", getSlotId());

    Parcel* p = message->getParcel();
    size_t s16Len = 0;
    const char16_t *s16 = NULL;
    RILD_RadioTechnology_Group dest = RADIO_TECH_GROUP_GSM;
    char* aid = NULL;

    // authContext
    p->readInt32();
    // data
    p->readString16Inplace(&s16Len);

    // Get AID
    s16 = NULL;
    s16Len = 0;
    s16 = p->readString16Inplace(&s16Len);
    if (s16 != NULL) {
        size_t strLen = strnlen16to8(s16, s16Len);
        aid = (char*)calloc(1, (strLen+1)*sizeof(char));
        RFX_ASSERT(aid != NULL);
        strncpy16to8(aid, s16, strLen);
        RLOGD("[RpSimController] handleSimAuthenticationReq aid %s (slot %d)", aid, getSlotId());
    }

    dest = choiceDestViaAid(aid);

    if (aid != NULL) {
        free(aid);
    }

    // Reset the payload position in the message before pass to GSM or C2K
    p->setDataPosition(0);

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimAuthenticationReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimAuthenticationReq(message);
    } else {
        RLOGE("[RpSimController] handleSimAuthenticationReq, can't dispatch the request!");
    }
}

void RpSimController::handleSimAuthenticationRsp(const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimAuthenticationRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimAuthenticationRsp(message);
    } else {
        RLOGE("[RpSimController] handleSimAuthenticationRsp, can't dispatch the response!");
    }
}

void RpSimController::handleSimTransmitApduBasicReq(
        const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group dest = choiceDestViaCurrCardType();

    if (dest == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimTransmitApduBasicReq(message);
    } else if (dest == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimTransmitApduBasicReq(message);
    } else {
        RLOGE("[RpSimController] handleSimTransmitApduBasicReq, can't dispatch the request!");
    }
}

void RpSimController::handleSimTransmitApduBasicRsp(
        const sp<RfxMessage>& message) {
    RILD_RadioTechnology_Group src = message->getSource();

    if (src == RADIO_TECH_GROUP_GSM && mGsmSimCtrl != NULL) {
        mGsmSimCtrl->handleSimTransmitApduBasicRsp(message);
    } else if (src == RADIO_TECH_GROUP_C2K && mC2kSimCtrl != NULL) {
        mC2kSimCtrl->handleSimTransmitApduBasicRsp(message);
    } else {
        RLOGE("[RpSimController] handleSimAuthenticationRsp, can't dispatch the response!");
    }
}

void RpSimController::handleNvNotSupportReq(
        const sp<RfxMessage>& message) {
    sp<RfxMessage> voidResponse = RfxMessage::obtainResponse(RIL_E_REQUEST_NOT_SUPPORTED, message);
    responseToRilj(voidResponse);
}

void RpSimController::handleLocalCardTypeNotify(const sp<RfxMessage>& message) {
    int cardType = 0;
    char *fullUiccType = NULL;
    Parcel *payload = message->getParcel();

    do {
        fullUiccType = strdupReadString(payload);
        RFX_ASSERT(fullUiccType != NULL);
        // If there is "SIM", it always must be put in the first!
        if (strncmp(fullUiccType, "SIM", 3) == 0) {
            cardType |= RFX_CARD_TYPE_SIM;
        } else if (strncmp(fullUiccType, "N/A", 3) == 0) {
            cardType = 0;
        } else if (strlen(fullUiccType) == 0) {
            cardType = -1;
        }

        if (strstr(fullUiccType, "USIM") != NULL)  {
            cardType |= RFX_CARD_TYPE_USIM;
        }

        if (strstr(fullUiccType, "CSIM") != NULL)  {
            cardType |= RFX_CARD_TYPE_CSIM;
        }

        if (strstr(fullUiccType, "RUIM") != NULL)  {
            cardType |= RFX_CARD_TYPE_RUIM;
        }

        if (((cardType == RFX_CARD_TYPE_SIM) || (cardType == RFX_CARD_TYPE_USIM))
                && (mC2kSimCtrl != NULL)) {
            mC2kSimCtrl->resetCdmaCardStatus();
            RLOGD("[RpSimController] Clear c2k value for gsm only card (slot %d)", getSlotId());
        }

        RLOGD("[RpSimController] last card type %d, full card type (%s, %d) (slot %d)",
                mLastCardType, fullUiccType, cardType, getSlotId());

        mLastCardType = cardType;

        getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, cardType);

        if (cardType >= 0) {
            // Modem SIM task is ready because +EUSIM is coming
            getStatusManager()->setBoolValue(RFX_STATUS_KEY_MODEM_SIM_TASK_READY, true, true);
        }

        if (fullUiccType != NULL) {
            free(fullUiccType);
        }

        checkCardStatus();

    } while(0);
}

void RpSimController::onRadioStateChanged(RfxStatusKeyEnum key, RfxVariant old_value,
        RfxVariant value) {
    int oldState = -1, newState = -1;

    RFX_UNUSED(key);
    oldState = old_value.asInt();
    newState = value.asInt();

    if (newState == RADIO_STATE_UNAVAILABLE) {
        RLOGD("[RpSimController] onRadioStateChanged (%d, %d) (slot %d)", oldState, newState,
                getSlotId());

        // Reset mGsmGetSimStatusReq, mC2kGetSimStatusReq and mCurrentGetSimStatusReq due to
        // the socket was disconnected and there is no response anymore.
        mGsmGetSimStatusReq = 0;
        mC2kGetSimStatusReq = 0;
        mCurrentGetSimStatusReq = 0;
        mCurrentPToken = 0;
        sTrayPluginCount = 0;
        // Modem SIM task is not ready because radio is not available
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_MODEM_SIM_TASK_READY, false, true);
        getStatusManager()->setIntValue(RFX_STATUS_KEY_SIM_STATE, RFX_SIM_STATE_NOT_READY);
        getStatusManager()->setString8Value(RFX_STATUS_KEY_GSM_IMSI, String8(""));
        getStatusManager()->setIntValue(RFX_STATUS_KEY_CARD_TYPE, -1);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_CT3G_DUALMODE_CARD, false);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_SWITCH_CARD_TYPE_STATUS, false);
        getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, -1);
        getStatusManager()->setIntValue(RFX_STATUS_KEY_ESIMIND_APPLIST, -1);
        if (mC2kSimCtrl != NULL) {
            mC2kSimCtrl->resetCdmaCardStatus();
        }
        stopTimerForIccidCheck();
        mTimerRetryCount = 0;
    }
}

RILD_RadioTechnology_Group RpSimController::choiceDestByCardType() {
    return choiceDestViaCurrCardType();
}

/**
 * Switch RUIM card to SIM or switch SIM to RUIM.
 *@param cardtype  switch card type. 0: SIM   1: RUIM
 *@param iscdmacapability if this slot has cdma capability.
 */
void RpSimController::switchCardType(int cardtype, bool iscdmacapability) {
    RLOGD("[RpSimController] switchCardType cardtype: %d  , iscdmacapability: %d  (slot %d)",
            cardtype, iscdmacapability, getSlotId());

    if (((cardtype == 0) && supportCardType(ICC_RUIM)) ||
            ((cardtype == 1) && supportCardType(ICC_SIM))) {
        // Reset card status
        getStatusManager()->setIntValue(RFX_STATUS_KEY_SIM_STATE, RFX_SIM_STATE_NOT_READY);
        getStatusManager()->setString8Value(RFX_STATUS_KEY_GSM_IMSI, String8(""));
        if (mC2kSimCtrl != NULL) {
            mC2kSimCtrl->resetCdmaCardStatus();
        }

        // send request to gsm ril
        sp<RfxMessage> request = RfxMessage::obtainRequest(getSlotId(), RADIO_TECH_GROUP_GSM,
                RIL_LOCAL_GSM_REQUEST_SWITCH_CARD_TYPE);
        request->getParcel()->writeInt32(1);
        request->getParcel()->writeInt32(cardtype);
        requestToRild(request);
        getStatusManager()->setBoolValue(RFX_STATUS_KEY_SWITCH_CARD_TYPE_STATUS, true);

        // send request to c2k ril
        if (iscdmacapability) {
            sp<RfxMessage> c2k_request = RfxMessage::obtainRequest(getSlotId(), RADIO_TECH_GROUP_C2K,
                    RIL_LOCAL_C2K_REQUEST_SWITCH_CARD_TYPE);
            request->getParcel()->writeInt32(1);
            request->getParcel()->writeInt32(cardtype);
            requestToRild(request);
        }
    } else {
        RLOGE("[RpSimController] switchCardType, can't switch card type!");
    }
}

bool RpSimController::isCdmaLockedCard() {
    int slotId = getSlotId();
    const char *cdmacardtype = NULL;
    char tmp[PROPERTY_VALUE_MAX] = {0};
    if (slotId >= 0 && slotId < 4) {
        cdmacardtype = PROPERTY_RIL_CDMA_CARD_TYPE[slotId];
        property_get(cdmacardtype, tmp, "");
        RLOGD("[RpSimController] isCdmaLockedCard, cdmacardtype is %s  (slot %d)", tmp, slotId);
    } else {
        RLOGE("[RpSimController] isCdmaLockedCard, slotId %d is wrong!", slotId);
    }

    if (strncmp(tmp, "18", 2) == 0) {
        return true;
    } else {
        return false;
    }
}

int RpSimController::isVsimEnabledBySlotId(int slotId) {
    int enabled = 0;
    char vsim_enabled_prop[PROPERTY_VALUE_MAX] = {0};
    getMSimProperty(slotId, (char*)"gsm.external.sim.enabled", vsim_enabled_prop);

    if (atoi(vsim_enabled_prop) > 0) {
        enabled = 1;
    }
    RLOGD("[RpSimController] isVsimEnabledBySlotId, slotId:%d is %d.", slotId, enabled);

    return enabled;
}

void RpSimController::checkCardStatus() {
    int eusim = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);
    int cardType = UNKOWN_CARD;

    if (eusim < 0) {
        cardType = UNKOWN_CARD;
        RLOGD("[RpSimController] checkCardStatus Invalid CDMA card type: %d !", cardType);
    } else if (eusim == 0) {
        if ((m_slot_id == 0) && (eusim == 0) && isOP09AProject()) {
            // It may be no card or locked card state, wait UIMST to set CDMA card type.
            RLOGD("Wait UIMT to set CDMA card type !");

            return;
        } else {
            cardType = CARD_NOT_INSERTED;

            // Set cdma card type.
            property_set(PROPERTY_RIL_CDMA_CARD_TYPE[getSlotId()],
                    String8::format("%d", cardType).string());
            getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, cardType);
        }
    } else {
        checkIccidStatus();
    }
}

void RpSimController::checkIccidStatus() {
    char iccid[PROPERTY_VALUE_MAX] = {0};
    property_get(PROPERTY_ICCID_SIM[getSlotId()], iccid, "");

    if (((strlen(iccid) == 0) || (strncmp(iccid, "N/A", 3) == 0)) && (mTimerRetryCount < 30)) {
        mTimerRetryCount++;

        startTimerForIccidCheck();
    } else {
        mTimerRetryCount = 0;

        handleCdmaCardType(iccid);
    }
}

void RpSimController::startTimerForIccidCheck() {
    mTimerHandler = RfxTimer::start(
                    RfxCallback0(this, &RpSimController::checkIccidStatus),
                    ms2ns(INITIAL_RETRY_INTERVAL_MSEC));
}

void RpSimController::stopTimerForIccidCheck() {
    if (mTimerHandler != 0) {
        RfxTimer::stop(mTimerHandler);
        mTimerHandler = 0;
    }
}

void RpSimController::handleCdmaCardType(const char *iccid) {
    int cardType = UNKOWN_CARD;

    int eusim = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);

    if ((strlen(iccid) == 0) || (strncmp(iccid, "N/A", 3) == 0)) {
        RLOGD("[RpSimController] handleCdmaCardType invalid iccid: %s", iccid);
        return;
    } else if (isOp09Card(iccid)) {
        // OP09 card type
        if (getStatusManager()->getBoolValue(RFX_STATUS_KEY_CT3G_DUALMODE_CARD)
                == 1) {
            // OP09 3G dual mode card.
            cardType = CT_UIM_SIM_CARD;
        } else {
            if (eusim < 0) {
                cardType = UNKOWN_CARD;
                RLOGD("[RpSimController] 1 invalid CDMA card type: %d !", cardType);
            } else if (eusim == 0) {
                // Rarely happen, iccid exists but card type is empty.
                cardType = CARD_NOT_INSERTED;
            } else if (((eusim & RFX_CARD_TYPE_SIM) == 0) &&
                    ((eusim & RFX_CARD_TYPE_USIM) == 0)) {
                // OP09 3G single mode card.
                cardType = CT_3G_UIM_CARD;
            } else if (((eusim & RFX_CARD_TYPE_CSIM) != 0) &&
                    ((eusim & RFX_CARD_TYPE_USIM) != 0)) {
                // Typical OP09 4G dual mode card.
                cardType = CT_4G_UICC_CARD;
            } else if (eusim == RFX_CARD_TYPE_SIM) {
                // For OP09 A lib no C capability slot, CT 3G card type is reported as
                // "SIM" card and "+ECT3G" value is false.
                cardType = CT_UIM_SIM_CARD;
            } else if (((eusim & RFX_CARD_TYPE_USIM) != 0) &&
                    (strncmp(iccid, "8985231", 7) == 0)) {
                // OP09 CTEXCEL card.
                cardType = CT_EXCEL_GG_CARD;
            } else if (eusim == RFX_CARD_TYPE_USIM) {
                if (isOP09AProject() && (m_slot_id == 1)) {
                    int applist = getStatusManager()->getIntValue(RFX_STATUS_KEY_ESIMIND_APPLIST);
                    if (applist < 0) {
                        // For OP09 A lib no C capability slot, the card may have CSIM but not
                        // reported by Modem. To check full uicc card type by reading EFdir file.
                        handleSimGetEfdirReq();
                        return;
                    } else {
                        if (applist == (RFX_UICC_APPLIST_USIM | RFX_UICC_APPLIST_CSIM)) {
                            cardType = CT_4G_UICC_CARD;
                        } else {
                            // USIM card if there is decode error or no CSIM aid exist.
                            cardType = SIM_CARD;
                        }
                    }
                } else {
                    // SIM card.
                    cardType = SIM_CARD;
                }
            } else {
                cardType = UNKOWN_CARD;
                RLOGD("[RpSimController] 2 invalid CDMA card type: %d !", cardType);
            }
        }
    } else {
        // Non-OP09 card type.
        if (getStatusManager()->getBoolValue(RFX_STATUS_KEY_CT3G_DUALMODE_CARD)
                == 1) {
            // Non-OP09 CDMA 3G dual mode card.
            cardType = UIM_SIM_CARD;
        } else {
            int eusim = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);

            if (eusim < 0) {
                cardType = UNKOWN_CARD;
                RLOGD("[RpSimController] 3 invalid CDMA card type: %d !", cardType);
            } else if (eusim == 0) {
                // Rarely happen, iccid exists but card type is empty.
                cardType = CARD_NOT_INSERTED;
            } else if (((eusim & RFX_CARD_TYPE_SIM) == 0) &&
                    ((eusim & RFX_CARD_TYPE_USIM) == 0)) {
                // Non-OP09 3G single mode card.
                cardType = UIM_CARD;
            } else if (((eusim & RFX_CARD_TYPE_CSIM) != 0) &&
                    ((eusim & RFX_CARD_TYPE_USIM) != 0)) {
                // Typical non-OP09 4G dual mode card.
                cardType = NOT_CT_UICC_CARD;
            } else if (eusim == RFX_CARD_TYPE_SIM) {
                // SIM card.
                cardType = SIM_CARD;
            } else if (eusim == RFX_CARD_TYPE_USIM) {
                if (isOP09AProject() && (m_slot_id == 1)) {
                    int applist = getStatusManager()->getIntValue(RFX_STATUS_KEY_ESIMIND_APPLIST);
                    if (applist < 0) {
                        // For OP09 A lib no C capability slot, the card may have CSIM but not
                        // reported by Modem. To check full uicc card type by reading EFdir file.
                        handleSimGetEfdirReq();
                        return;
                    } else {
                        if (applist == (RFX_UICC_APPLIST_USIM | RFX_UICC_APPLIST_CSIM)) {
                            cardType = CT_4G_UICC_CARD;
                        } else {
                            // USIM card if there is decode error or no CSIM aid exist.
                            cardType = SIM_CARD;
                        }
                    }
                } else {
                    // SIM card.
                    cardType = SIM_CARD;
                }
            } else {
                cardType = UNKOWN_CARD;
                RLOGD("[RpSimController] 4 invalid CDMA card type: %d !", cardType);
            }
        }
    }

    // Set cdma card type.
    property_set(PROPERTY_RIL_CDMA_CARD_TYPE[getSlotId()],
            String8::format("%d", cardType).string());
    getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, cardType);
}

void RpSimController::handleSimGetEfdirReq() {
    // send request to gsm ril
    sp<RfxMessage> request = RfxMessage::obtainRequest(getSlotId(), RADIO_TECH_GROUP_GSM,
            RIL_LOCAL_REQUEST_SIM_GET_EFDIR);

    requestToRild(request);
}

void RpSimController::handleSimGetEfdirRsp(const sp<RfxMessage>& message) {
    char *efdir = NULL;
    int cardType = UNKOWN_CARD;
    char iccid[PROPERTY_VALUE_MAX] = {0};
    int error = message->getError();

    if (error != RIL_E_SUCCESS) {
        // USIM card if there is decode error or no CSIM aid exist.
        cardType = SIM_CARD;
    } else {
        Parcel *payload = message->getParcel();
        efdir = strdupReadString(payload);

        property_get(PROPERTY_ICCID_SIM[getSlotId()], iccid, "");

        if ((strlen(iccid) == 0) || (strncmp(iccid, "N/A", 3) == 0)) {
            RLOGD("[RpSimController] handleSimGetEfdirRsp invalid iccid: %s", iccid);
            return;
        } else if (isOp09Card(iccid)) {
            if (strstr(efdir, "A0000003431002") != NULL) {
                cardType = CT_4G_UICC_CARD;
            } else {
                // USIM card if there is decode error or no CSIM aid exist.
                cardType = SIM_CARD;
            }
        } else {
            if (strstr(efdir, "A0000003431002") != NULL) {
                cardType = NOT_CT_UICC_CARD;
            } else {
                // USIM card if there is decode error or no CSIM aid exist.
                cardType = SIM_CARD;
            }
        }
    }

    // Set cdma card type.
    property_set(PROPERTY_RIL_CDMA_CARD_TYPE[getSlotId()],
            String8::format("%d", cardType).string());
    getStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_CARD_TYPE, cardType);
}

void RpSimController::setEsimindValue(const sp<RfxMessage>& message) {
    int applist = 0;
    int count;
    Parcel *p = message->getParcel();
    count = p->readInt32();
    applist = p->readInt32();
    RLOGD("[RpSimController] setEsimindValue (slot %d) count: %d, applist: %d",
            getSlotId(), count, applist);
    if (count == 1) {
            getStatusManager()->setIntValue(RFX_STATUS_KEY_ESIMIND_APPLIST, applist);
    } else {
        RLOGE("[RpSimController] setEsimindValue but payload format is wrong! (slot %d)",
                getSlotId());
    }
}
