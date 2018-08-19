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
#include "RpSmsCtrl.h"
#include "RfxNwServiceState.h"
#include "util/RpFeatureOptionUtils.h"
#include "nw/RpNwDefs.h"

/*****************************************************************************
 * Class RpSmsCtrl
 *****************************************************************************/
RFX_IMPLEMENT_CLASS("RpSmsCtrl", RpSmsCtrl, RpSmsCtrlBase);

RpSmsCtrl::RpSmsCtrl() :
    mGsmSmsCtrl(NULL),
    mCdmaSmsCtrl(NULL) {
    setName(String8("RpSmsCtrl"));
}

void RpSmsCtrl::onInit() {
    // Required: invoke super class implementation
    RpSmsCtrlBase::onInit();

    log(String8("onInit"));

    // Create Gsm/Cdma sms controller
    RFX_OBJ_CREATE(mGsmSmsCtrl, RpGsmSmsCtrl, this);
    RFX_OBJ_CREATE(mCdmaSmsCtrl, RpCdmaSmsCtrl, this);

    // Start to listen the hybrid requests
    const int request_id_list[] = {
        RIL_REQUEST_IMS_SEND_SMS,
        RIL_REQUEST_REPORT_SMS_MEMORY_STATUS,
    };

    const int urc_id_list[] = {
        RIL_UNSOL_SMS_READY_NOTIFICATION
    };

    if (RpFeatureOptionUtils::isC2kSupport()) {
        registerToHandleRequest(request_id_list, (sizeof(request_id_list)/sizeof(int)));
        registerToHandleUrc(urc_id_list, (sizeof(urc_id_list)/sizeof(int)));
    }
}

bool RpSmsCtrl::onPreviewMessage(const sp<RfxMessage>& message) {
    if (RIL_REQUEST_PREVIEW_REASON_PREVIEWED == mGsmSmsCtrl->isPreviewed(message)) {
        log(String8("RpGsmSmsCtrl is previewed"));
        return false;
    }

    if (RIL_REQUEST_PREVIEW_REASON_PREVIEWED == mCdmaSmsCtrl->isPreviewed(message)) {
        log(String8("RpCdmaSmsCtrl is previewed"));
        return false;
    }

    // default return true to make this ril request keeps going to handle
    return true;
}

bool RpSmsCtrl::onHandleRequest(const sp<RfxMessage>& message) {

    // check if any sub-module need to handle
    if (RIL_REQUEST_HANDLE_REASON_HANDLED == mGsmSmsCtrl->isHandled(message)) {
        // Not handle due to Gsm is handled
        log(String8("RpGsmSmsCtrl is handled"));
        return true;
    }

    if (RIL_REQUEST_HANDLE_REASON_HANDLED == mCdmaSmsCtrl->isHandled(message)) {
        // Not handle due to Gsm is handled
        log(String8("RpCdmaSmsCtrl is handled"));
        return true;
    }

    switch (message->getId()) {
        default:
            // Pass the request to related CS domain RILD
            int nwMode = getStatusManager(getSlotId())->getIntValue(
                    RFX_STATUS_KEY_NWS_MODE, NWS_MODE_CSFB);
            RILD_RadioTechnology_Group group = RADIO_TECH_GROUP_GSM;
            if (nwMode == NWS_MODE_CDMALTE) {
                group = RADIO_TECH_GROUP_C2K;
            }
            requestToRild(RfxMessage::obtainRequest(group, message->getId(), message, true));
            break;
    }
    return true;
}

bool RpSmsCtrl::onCheckIfResumeMessage(const sp<RfxMessage>& message) {
    if (RIL_REQUEST_RESUME_REASON_RESUMED == mGsmSmsCtrl->isResumed(message)) {
        log(String8("RpGsmSmsCtrl is resumed"));
        return true;
    }

    if (RIL_REQUEST_RESUME_REASON_RESUMED == mCdmaSmsCtrl->isResumed(message)) {
        log(String8("RpCdmaSmsCtrl is resumed"));
        return true;
    }

    // default return false to keep it as suspend
    return false;
}

bool RpSmsCtrl::isCdmaDualModeSimCard(void) {
    if (getStatusManager()->getBoolValue(RFX_STATUS_KEY_CT3G_DUALMODE_CARD)) {
        log(String8("isCdmaDualModeSimCard, is CT3G dual mode card"));
        return true;
    } else {
        int nCardType = getStatusManager()->getIntValue(RFX_STATUS_KEY_CARD_TYPE);
        bool ret = RFX_FLAG_HAS_ALL(nCardType, (RFX_CARD_TYPE_USIM | RFX_CARD_TYPE_CSIM));
        log(String8::format(
            "isCdmaDualModeSimCard, nCardType=0x%x", nCardType));
        return ret;
    }
}

bool RpSmsCtrl::onHandleUrc(const sp<RfxMessage>& message) {
    int msg_id = message->getId();
    int slot_id = message->getSlotId();
    if (msg_id == RIL_UNSOL_SMS_READY_NOTIFICATION) {
        RILD_RadioTechnology_Group source;
        source = message->getSource();
        if (source == RADIO_TECH_GROUP_GSM) {
            mGsmSmsCtrl->setSmsReady(true);
            log(String8::format("GSM SMS ready slot %d", slot_id));
        } else {
            mCdmaSmsCtrl->setSmsReady(true);
            log(String8::format("CDMA SMS ready %d", slot_id));
        }
        int nwMode = getStatusManager(slot_id)->getIntValue(
                RFX_STATUS_KEY_NWS_MODE, NWS_MODE_CSFB);

        if (isCdmaDualModeSimCard() && (nwMode == NWS_MODE_CDMALTE)) {
            if (!(mGsmSmsCtrl->getSmsReady()) || !(mCdmaSmsCtrl->getSmsReady())) {
                return true;
            }
        }
        log(String8::format("response to RILJ SMS ready %d", slot_id));
    }
    responseToRilj(message);
    return true;
}
