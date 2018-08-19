/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2017. All rights reserved.
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
#include <log/log.h>
#include <cutils/properties.h>
#include "RfxLog.h"
#include "RfxStatusDefs.h"
#include "RpWpController.h"
#include "power/RpRadioController.h"
#include "util/RpFeatureOptionUtils.h"

#define WP_LOG_TAG "RpWpController"
/*****************************************************************************
 * Class RpWpController
 *****************************************************************************/

RFX_IMPLEMENT_CLASS("RpWpController", RpWpController, RfxController);

int RpWpController::closeRadioCount = 0;
int RpWpController::mainSlotId = 0;

RpWpController::RpWpController() {
}

RpWpController::~RpWpController() {
}

void RpWpController::onInit() {
    // Required: invoke super class implementation
    RfxController::onInit();

    const int urc_id_list[] = {
        RIL_UNSOL_WORLD_MODE_CHANGED
    };
    const int request_id_list[] {
        RIL_LOCAL_REQUEST_RESUME_WORLD_MODE
    };
    if (RpFeatureOptionUtils::isC2kSupport()) {
        registerToHandleRequest(request_id_list, 1);
        registerToHandleUrc(urc_id_list, 1);
    }
}

void RpWpController::onDeinit() {
    // Required: invoke super class implementation
    RfxController::onDeinit();
}

bool RpWpController::isWorldModeRemoveSimReset() {
    char removeSimResetState[PROPERTY_VALUE_MAX] = {0};
    property_get("ril.nw.wm.no_reset_support", removeSimResetState, "0");
    logD(WP_LOG_TAG, "[isWorldModeRemoveSimReset] state= %s", removeSimResetState);
    if (0 == strcmp(removeSimResetState, "1")) {
        return true;
    }
    return false;
}

bool RpWpController::onHandleUrc(const sp<RfxMessage>& message) {
    logD(WP_LOG_TAG, "handle urc %s (slot %d)",
            urcToString(message->getId()), getSlotId());

    switch (message->getId()) {
    case RIL_UNSOL_WORLD_MODE_CHANGED:
        processWorldModeChanged(message);
        break;
    default:
        break;
    }
    return true;
}

bool RpWpController::onHandleResponse(const sp<RfxMessage>& message) {
    int msg_id = message -> getId();
    RIL_Errno rilErrno = message -> getError();
    logD(WP_LOG_TAG, "onHandleResponse, msgid = %d Errno = %d", msg_id, rilErrno);
    return true;
}

const char* RpWpController::urcToString(int reqId) {
    switch (reqId) {
    case RIL_UNSOL_WORLD_MODE_CHANGED:
        return "RIL_UNSOL_WORLD_MODE_CHANGED";
    default:
        logD(WP_LOG_TAG, "<UNKNOWN_URC>");
        return "<UNKNOWN_URC>";
    }
}

void RpWpController::requestRadioOff() {
    int simCount = RpFeatureOptionUtils::getSimCount();
    logD(WP_LOG_TAG, "requestRadioOff, %d", simCount);
    for (int slot = 0; slot < simCount; slot++) {
        sp<RfxAction> action0 = new RfxAction1<int>(this,
                &RpWpController::onRequestRadioOffDone, slot);
        RpRadioController* radioController0 =
                (RpRadioController *)findController(slot,
                RFX_OBJ_CLASS_INFO(RpRadioController));
        radioController0->dynamicSwitchRadioOff(true, true, action0);
    }
}

void RpWpController::onRequestRadioOffDone(int slotId) {
    int simCount = RpFeatureOptionUtils::getSimCount();
    closeRadioCount++;
    logD(WP_LOG_TAG, "onRequestRadioOffDone,%d / %d, slotId:%d",
            closeRadioCount, simCount, slotId);
    if (closeRadioCount == simCount) {
        closeRadioCount = 0;
        sp<RfxMessage> request = RfxMessage::obtainRequest(mainSlotId, RADIO_TECH_GROUP_GSM,
                RIL_LOCAL_REQUEST_RESUME_WORLD_MODE);
        request->getParcel()->writeInt32(1);
        request->getParcel()->writeInt32(1);
        requestToRild(request);
    }
}

void RpWpController::requestRadioOn() {
    int simCount = RpFeatureOptionUtils::getSimCount();
    for (int index =0; index < simCount; index++) {
        RpRadioController* radioController =
                (RpRadioController *)findController(index,
                RFX_OBJ_CLASS_INFO(RpRadioController));
        radioController->dynamicSwitchRadioOn();
    }
}

void RpWpController::processWorldModeChanged(const sp<RfxMessage>& msg) {
    if (msg->getError() != RIL_E_SUCCESS) {
        logD(WP_LOG_TAG, "processWorldModeChanged, msg is not SUCCESS");
        responseToRilj(msg);
        return;
    }

    int32_t count = 0;
    int32_t state = -1;

    mainSlotId = msg -> getSlotId();
    Parcel *p = msg->getParcel();
    count = p->readInt32();
    state = p->readInt32();
    logD(WP_LOG_TAG, "processWorldModeChanged, count=%d state=%d slotId=%d", count, state, mainSlotId);

    int gsm_state = getNonSlotScopeStatusManager()->getIntValue(RFX_STATUS_KEY_GSM_WORLD_MODE_STATE, 1);
    int cdma_state = getNonSlotScopeStatusManager()->getIntValue(RFX_STATUS_KEY_CDMA_WORLD_MODE_STATE, 1);
    int cur_state = (gsm_state == 0 || cdma_state == 0) ? 0 : 1;

    if (count == 2) {
        if (cdma_state != state) {
            cdma_state = state;
            getNonSlotScopeStatusManager()->setIntValue(RFX_STATUS_KEY_CDMA_WORLD_MODE_STATE, state);
        }
    } else {
        if (gsm_state != state) {
            gsm_state = state;
            getNonSlotScopeStatusManager()->setIntValue(RFX_STATUS_KEY_GSM_WORLD_MODE_STATE, state);
        }
    }

    int new_state = (gsm_state == 0 || cdma_state == 0) ? 0 : 1;
    logD(WP_LOG_TAG, "processWorldModeChanged, cur_state=%d new_state=%d", cur_state, new_state);

    if (cur_state != new_state) {
        getNonSlotScopeStatusManager()->setIntValue(RFX_STATUS_KEY_WORLD_MODE_STATE, new_state);
        responseToRilj(msg);
        if (true == isWorldModeRemoveSimReset()) {
            if (new_state == 0) {
                requestRadioOff();
            } else {
                requestRadioOn();
            }
        }
    }
}
