/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2017. All rights reserved.
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
#include "RpAtciController.h"
#include <cutils/properties.h>
#include <telephony/mtk_ril.h>

#define RFX_LOG_TAG "RpAtciController"

/*****************************************************************************
 * Class RfxController
 *****************************************************************************/
RFX_IMPLEMENT_CLASS("RpAtciController", RpAtciController, RfxController);

void RpAtciController::onInit() {
    RfxController::onInit();
    logD(RFX_LOG_TAG, "onInit enter.");
    const int request_id_list[] = {
        RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL
    };
    registerToHandleRequest(request_id_list, sizeof(request_id_list)/sizeof(const int), NORMAL);
}

bool RpAtciController::onHandleAtciRequest(const sp<RfxMessage>& message) {
    int msg_id = message->getId();
    logD(RFX_LOG_TAG, "onHandleAtciRequest msg_id: %d, slot id: %d.", msg_id, m_slot_id);
    if (msg_id == RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL) {
        sp<RfxMessage> request = RfxMessage::obtainRequest(getTechGroup(), msg_id, message, true);
        Parcel* p = request->getParcel();
        int req, token, len;
        p->setDataPosition(0);
        p->readInt32(&req);
        p->readInt32(&token);
        p->readInt32(&len);
        logD(RFX_LOG_TAG, "req %d, len %d", req, len);
        if (len > 0) {
            char *data = (char *)p->readInplace(len);
            p->setDataPosition(0);
            p->writeInt32(RIL_REQUEST_OEM_HOOK_RAW);
            logD(RFX_LOG_TAG, "atBuffer %s", data);
        } else {
            RLOGE("len <= 0 return");
            return true;
        }
        p->setDataPosition(0);
        requestToRild(request);
    }
    return true;
}

bool RpAtciController::onHandleAtciResponse(const sp<RfxMessage>& message) {
    int msg_id = message->getId();
    logD(RFX_LOG_TAG, "onHandleAtciResponse msg_id: %d.", msg_id);
    if (msg_id == RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL) {
        responseToRilj(message);
    }
    return true;
}

RILD_RadioTechnology_Group RpAtciController::getTechGroup() {
    char simNo[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.service.atci.sim", simNo, "0");
    logD(RFX_LOG_TAG, "simNo: %s", simNo);
    if (simNo[0] == '9') {
        return RADIO_TECH_GROUP_C2K;
    } else {
        return RADIO_TECH_GROUP_GSM;
    }
}
