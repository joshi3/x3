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
#include "RpPhoneNumberController.h"
#include <cutils/jstring.h>
#include "util/RpFeatureOptionUtils.h"
#include <string.h>
#include "utils/String16.h"
#include "utils/String8.h"

#define RFX_LOG_TAG "RpPhoneNumberController"

// Default ECC with service category 0
const char *mDefaultEccNumber = "112,0;911,0";

/*****************************************************************************
 * Class RfxController
 *****************************************************************************/

RFX_IMPLEMENT_CLASS("RpPhoneNumberController", RpPhoneNumberController, RfxController);

RpPhoneNumberController::RpPhoneNumberController():
    mCardType(-1),
    mCDMACardType(-1),
    mCDMASlotId(-1) {
    strncpy(mSimECCProperty, "ril.ecclist", MAX_ECC_PROPERTY_CHARS - 1);
    strncpy(mSimCdmaECCProperty, "ril.cdma.ecclist", MAX_ECC_PROPERTY_CHARS - 1);
}

RpPhoneNumberController::~RpPhoneNumberController() {
}

void RpPhoneNumberController::onInit() {
    RfxController::onInit();  // Required: invoke super class implementation
    char slotId[10];
    sprintf(slotId, "%d", getSlotId());
    if(getSlotId() >= 1) {
        strncat(mSimECCProperty, slotId, strlen(slotId));
        strncat(mSimCdmaECCProperty, slotId, strlen(slotId));
    }

    mCDMASlotId
        = getNonSlotScopeStatusManager()->getIntValue(RFX_STATUS_KEY_CDMA_SOCKET_SLOT, 0);

    //Set the default ECC number
    property_set(mSimECCProperty, "");
    property_set(mSimCdmaECCProperty, "");

    logD(RFX_LOG_TAG,"onInit(), slotId: %s, mCDMASlotId: %d, mSimECCProperty: %s",
         slotId, mCDMASlotId, mSimECCProperty);

    if (RpFeatureOptionUtils::isC2kSupport()) {
        const int urc_id_list[] = {
            RIL_LOCAL_GSM_UNSOL_EF_ECC,
            RIL_LOCAL_C2K_UNSOL_EF_ECC,
        };
        // register request & URC id list
        // NOTE. one id can ONLY be registered by one controller
        registerToHandleUrc(urc_id_list, sizeof(urc_id_list)/sizeof(const int));
        // Register callbacks for card type to clear SIM ECC when SIM plug out
        getStatusManager()->registerStatusChanged(RFX_STATUS_KEY_CARD_TYPE,
                RfxStatusChangeCallback(this, &RpPhoneNumberController::onCardTypeChanged));

        // Register callbacks for CDMA socket slot changed event
        getNonSlotScopeStatusManager()->registerStatusChanged(RFX_STATUS_KEY_CDMA_SOCKET_SLOT,
                RfxStatusChangeCallback(this, &RpPhoneNumberController::onCdmaSocketSlotChange));
    } else {
        const int urc_id_list[] = {
            RIL_LOCAL_GSM_UNSOL_EF_ECC,
            RIL_LOCAL_C2K_UNSOL_EF_ECC,
            // Handle SIM plug out to reset SIM ECC in this URC
            // Currently this URC is not used by other module
            // so we can register it for non-C2K project
            RIL_UNSOL_SIM_PLUG_OUT
        };
        registerToHandleUrc(urc_id_list, sizeof(urc_id_list)/sizeof(const int));
    }
}

void RpPhoneNumberController::onCardTypeChanged(RfxStatusKeyEnum key,
    RfxVariant oldValue, RfxVariant newValue) {
    RFX_UNUSED(key);
    logV(RFX_LOG_TAG,"onCardTypeChanged oldValue %d, newValue %d",
        oldValue.asInt(), newValue.asInt());
    if (oldValue.asInt() != newValue.asInt()) {
        mCardType = newValue.asInt();
        switch (newValue.asInt()) {
            case 0:
                /*  For No SIM inserted, the behavior is different on TK and BSP because
                    TK support hotplug and customized ECC list.
                    [TK]:  Reset to "" otherwise it will got wrong value in
                         isEmergencyNumberExt variable bSIMInserted. (ALPS02749228)
                    [BSP]: Reset to mDefaultEccNumber because BSP don't support custom ECC
                         So we use this property instead (ALPS02572162)
                */
                logD(RFX_LOG_TAG,"onCardTypeChanged, reset property due to No SIM");
                property_set(mSimECCProperty, "");
                property_set(mSimCdmaECCProperty, "");
                break;
        }
    }
}

void RpPhoneNumberController::onCdmaSocketSlotChange(RfxStatusKeyEnum key,
    RfxVariant old_value, RfxVariant value) {
    RFX_UNUSED(key);
    RFX_UNUSED(old_value);
    int c2kSlot = value.asInt();
    mCDMASlotId = c2kSlot;
    int slotId = getSlotId();
    char writeEccNumber[PROPERTY_VALUE_MAX] = {0};
    logD(RFX_LOG_TAG,"onCdmaSocketSlotChange: update cdma slot ID: %d, current slot ID: %d",
        c2kSlot, slotId);
    if (mCDMASlotId != slotId) {
        property_set(mSimCdmaECCProperty, "");
    }
}

void RpPhoneNumberController::onDeinit() {
    logD(RFX_LOG_TAG,"onDeinit()");

    // Unregister callbacks for card type
    getStatusManager()->unRegisterStatusChanged(RFX_STATUS_KEY_CARD_TYPE,
        RfxStatusChangeCallback(this, &RpPhoneNumberController::onCardTypeChanged));

    // Register callbacks for CDMA socket slot changed event
    getNonSlotScopeStatusManager()->unRegisterStatusChanged(RFX_STATUS_KEY_CDMA_SOCKET_SLOT,
            RfxStatusChangeCallback(this, &RpPhoneNumberController::onCdmaSocketSlotChange));

    // Required: invoke super class implementation
    RfxController::onDeinit();
}

bool RpPhoneNumberController::onHandleUrc(const sp<RfxMessage>& message) {
    int msgId = message->getId();
    int sourceId = message->getSource();
    int cmdType = 0;

    logV(RFX_LOG_TAG,"urc %d %s source:%d", msgId,
        urcToString(msgId), sourceId);

    switch (msgId) {
        case RIL_LOCAL_GSM_UNSOL_EF_ECC:
            handleGSMEFECC(message);
            break;
        case RIL_LOCAL_C2K_UNSOL_EF_ECC:
            handleC2KEFECC(message);
            break;
        case RIL_UNSOL_SIM_PLUG_OUT:
            handleSimPlugOut(message);
            break;
        default:
            responseToRilj(message);
            break;
    }

    return true;
}

const char* RpPhoneNumberController::urcToString(int urcId) {
    switch (urcId) {
        case RIL_LOCAL_GSM_UNSOL_EF_ECC: return "UNSOL_GSM_EF_ECC";
        case RIL_LOCAL_C2K_UNSOL_EF_ECC: return "UNSOL_C2K_EF_ECC";
        default:
            return "UNKNOWN_URC";
    }
}

/*
 * [MD1 EF ECC URC format]
 * + ESMECC: <m>[,<number>,<service category>[,<number>,<service category>]]
 * <m>: number of ecc entry
 * <number>: ecc number
 * <service category>: service category
 * Ex.
 * URC string:2,115,4,334,5,110,1
 *
 * Note:If it has no EF ECC, RpPhoneNumberController will receive "0"
*/
void RpPhoneNumberController::handleGSMEFECC(const sp<RfxMessage>& message){
    logD(RFX_LOG_TAG, "handleGSMEFECC, msg = %s", message->toString().string());
    const char *delim = ", \"";
    char writeECC[PROPERTY_VALUE_MAX] = {0};
    char *gsmEfEcc;
    int parameterCount = 0;

    Parcel *p = message->getParcel();
    gsmEfEcc = strdupReadString(p);

    property_set("ril.ef.ecc.support", "1");

    if((gsmEfEcc == NULL) || (strlen(gsmEfEcc) == 0) || !(strncmp(gsmEfEcc, "0", 1))) {
        logV(RFX_LOG_TAG, "There is no ECC number stored in GSM SIM");
    } else {
        // Split string by comma, quote, space
        char *tempEcc = strtok(gsmEfEcc, delim);
        while (tempEcc != NULL) {
            parameterCount++;
            //Just ignore the 1st parameter: it is the total number of emergency numbers.
            if(parameterCount >= 2){
                if(strlen(writeECC) > 0) {
                    if ((parameterCount % 2) == 0) {
                        strncat(writeECC, ";", 1);
                    } else {
                        strncat(writeECC, ",", 1);
                    }
                }
                strncat(writeECC, tempEcc, strlen(tempEcc));
            }
            tempEcc = strtok(NULL, delim);
        }
    }

    //Add the default ECC number
    if(strlen(writeECC) > 0) {
        strncat(writeECC, ";", 1);
    }
    strncat(writeECC, mDefaultEccNumber, strlen(mDefaultEccNumber));

    logD(RFX_LOG_TAG,"handleGSMEFECC mSimECCProperty: %s, writeECC: %s",
        mSimECCProperty,
        writeECC);
    property_set(mSimECCProperty, writeECC);

    if (gsmEfEcc != NULL) {
        free(gsmEfEcc);
        gsmEfEcc = NULL;
    }
}

/*
 * [MD3 EF ECC URC format]
 * +CECC:<m>[,<number [,<number >]]
 * <m>: number of ecc entry
 * <number>: ecc number
 * Ex.
 * URC string:2,115,334
 *
 * Note:If it has no EF ECC, RpPhoneNumberController will receive "0"
 *
*/
void RpPhoneNumberController::handleC2KEFECC(const sp<RfxMessage>& message){
    logD(RFX_LOG_TAG, "handleC2KEFECC, msg = %s", message->toString().string());
    const char *delim = ",";
    char writeECC[PROPERTY_VALUE_MAX] = {0};
    char *cdmaEfEcc;
    int parameterCount = 0;

    if (mCDMASlotId != getSlotId()) {
        logV(RFX_LOG_TAG,"handleC2KEFECC not cdma slot mCDMASlotId: %d, slot: %d",
            mCDMASlotId, getSlotId());
        return;
    }

    Parcel *p = message->getParcel();
    cdmaEfEcc = strdupReadString(p);

    property_set("ril.ef.ecc.support", "1");

    if((cdmaEfEcc == NULL) || (strlen(cdmaEfEcc) == 0) || !(strncmp(cdmaEfEcc, "0", 1))) {
        logV(RFX_LOG_TAG,"There is no ECC number stored in CDMA SIM");
    } else {
        // Split string by comma
        char *tempEcc = strtok(cdmaEfEcc, delim);
        while (tempEcc != NULL) {
            parameterCount++;
            if(parameterCount >= 2){
                //Just ignore the 1st parameter: it is the total number of emergency numbers.
                if(strlen(writeECC) > 0) {
                    strncat(writeECC, ",", 1);
                }
                strncat(writeECC, tempEcc, strlen(tempEcc));
            }
            tempEcc = strtok(NULL, delim);
        }
    }

    logD(RFX_LOG_TAG,"handleC2KEFECC mSimCdmaECCProperty: %s, writeECC: %s",
        mSimCdmaECCProperty,
        writeECC);
    property_set(mSimCdmaECCProperty, writeECC);

    if (cdmaEfEcc != NULL) {
        free(cdmaEfEcc);
        cdmaEfEcc = NULL;
    }
}

// For non-C2K project to handle SIM plugout to reset SIM ECC only.
// for C2K project use RFX_STATUS_KEY_CARD_TYPE.
void RpPhoneNumberController::handleSimPlugOut(const sp<RfxMessage>& message){
    logD(RFX_LOG_TAG,"handleSimPlugOut, reset property due to No SIM");
    property_set(mSimECCProperty, "");
    property_set(mSimCdmaECCProperty, "");
}

// The API will help to allocate memory so remember to free
// it while you don't use the buffer anymore
char* RpPhoneNumberController::strdupReadString(Parcel *p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p->readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}
