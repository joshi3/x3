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
#include "RpNwStateController.h"
#include "RpNwPhoneTypeUpdater.h"
#include "RpNwRatController.h"
#include "RpNwDefs.h"
#include "power/RadioConstants.h"
#include <cutils/jstring.h>
#include "RfxMainThread.h"
#include "util/RpFeatureOptionUtils.h"
#include <telephony/librilutilsmtk.h>

/*****************************************************************************
 * Class RfxController
 *****************************************************************************/

RFX_IMPLEMENT_CLASS("RpNwStateController", RpNwStateController, RfxController);

RpNwStateController::RpNwStateController() {
    logD(RP_NW_STATE_TAG, " Constructor RpNwStateController");
    mVoiceRegState = 0;
    mDataRegState = 0;
    mVoiceRadioTech = 0;
    mDataRadioTech = 0;
    is1xRttInFemtocell = false;
    isEvdoInFemtocell = false;
    mCdmaNetworkExist = 0;
    mDebug = isEngLoad();
}

RpNwStateController::~RpNwStateController() {
}

void RpNwStateController::onInit() {
    RfxController::onInit(); // Required: invoke super class implementation

    logD(RP_NW_STATE_TAG, " onInit()");

    const int request_id_list[] = {
        RIL_REQUEST_VOICE_REGISTRATION_STATE,
        RIL_REQUEST_DATA_REGISTRATION_STATE,
        RIL_REQUEST_SIGNAL_STRENGTH,
        RIL_REQUEST_OPERATOR,
        RIL_REQUEST_GET_CELL_INFO_LIST
    };

    const int urc_id_list[] = {
        RIL_UNSOL_SIGNAL_STRENGTH,
        RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED,
        RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
        RIL_UNSOL_FEMTOCELL_INFO,
        RIL_UNSOL_CDMA_OTA_PROVISION_STATUS,
        RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE
    };

    // register request & URC id list
    if (RpFeatureOptionUtils::isC2kSupport()) {
        registerToHandleRequest(request_id_list, (sizeof(request_id_list)/sizeof(int)));
        registerToHandleUrc(urc_id_list, (sizeof(urc_id_list)/sizeof(int)));
    }
    resetVoiceRegStateCache(&gsm_voice_reg_state_cache);
    resetVoiceRegStateCache(&cdma_voice_reg_state_cache);
    resetDataRegStateCache(&gsm_data_reg_state_cache);
    resetDataRegStateCache(&cdma_data_reg_state_cache);
    resetFemtoCellCache();
    gsm_voice_reg_state_cache.cacheType = RADIO_TECH_GROUP_GSM;
    cdma_voice_reg_state_cache.cacheType = RADIO_TECH_GROUP_C2K;
    cdma_voice_reg_state_cache.network_exsit = 1;
    gsm_data_reg_state_cache.cacheType = RADIO_TECH_GROUP_GSM;
    cdma_data_reg_state_cache.cacheType = RADIO_TECH_GROUP_C2K;
    resetSignalStrengthCache(&signal_strength_cache);
    resetOperatorCache(&gsm_operator_cache);
    resetOperatorCache(&cdma_operator_cache);

    mVoiceRegState = 0;
    mDataRegState = 0;
    mVoiceRadioTech = 0;
    mDataRadioTech = 0;
    mVoiceRadioSys = -1;
    mDataRadioSys = -1;
    mCdmaNetworkExist = 0;
    ps_reg_state.wifi_state = 0;
    ps_reg_state.gsm_data_register_state = 0;
    ps_reg_state.gsm_mcc_mnc = 0;
    ps_reg_state.gsm_data_rat = 0;

    getStatusManager()->setServiceStateValue(RFX_STATUS_KEY_SERVICE_STATE,
            RfxNwServiceState(mVoiceRegState, mDataRegState, mVoiceRadioTech, mDataRadioTech, mCdmaNetworkExist));

    RpNwPhoneTypeUpdater *phoneTypeUpdater;

    RFX_OBJ_CREATE(phoneTypeUpdater, RpNwPhoneTypeUpdater, this);
}

bool RpNwStateController::onHandleRequest(const sp<RfxMessage>& riljReq) {
    int reqId = riljReq->getId();
    int slotId = riljReq->getSlotId();
    sp<RfxMessage> child_request;

    if (getNonSlotScopeStatusManager()->getIntValue(RFX_STATUS_KEY_CDMA_SOCKET_SLOT, 0)
            == m_slot_id) {
        // logD(RP_NW_STATE_TAG, " onHandleReq() SIM%d REQ=%s", slotId, requestToString(reqId));
        switch (reqId) {
            case RIL_REQUEST_VOICE_REGISTRATION_STATE:
            case RIL_REQUEST_DATA_REGISTRATION_STATE:
            case RIL_REQUEST_SIGNAL_STRENGTH:
            case RIL_REQUEST_OPERATOR:
            case RIL_REQUEST_GET_CELL_INFO_LIST:
                child_request = RfxMessage::obtainRequest(RADIO_TECH_GROUP_C2K, reqId, riljReq);
                // send request to corresponding RILD
                requestToRild(riljReq);
                requestToRild(child_request);
                break;
            default:
                // invalid request
                break;
        }
    } else {
        requestToRild(riljReq);
    }
    return true;
}

bool RpNwStateController::onHandleUrc(const sp<RfxMessage>& urc) {
    int urcId = urc->getId();
    int slotId = urc->getSlotId();
    int isCsgCell = 0;
    int system;
    int state;
    int32_t stgCount = -1;
    char **pStrings = NULL;
    Parcel *p;
    sp<RfxMessage> urcToRilj = urc;
    bool isNeedResponseUrc = true;

    if (getNonSlotScopeStatusManager()->getIntValue(RFX_STATUS_KEY_CDMA_SOCKET_SLOT, 0)
            == m_slot_id) {
        if (urc->getError() == RIL_E_SUCCESS) {
            switch (urcId) {
                case RIL_UNSOL_SIGNAL_STRENGTH:
                    updateSignalStrengthCache(urc);
                    urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(), RIL_UNSOL_SIGNAL_STRENGTH);
                    combineSignalStrength(urcToRilj);
                    break;
                case RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED:
                case RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE:
                    if (getDeviceMode() == NWS_MODE_CDMALTE) {
                        logD(RP_NW_STATE_TAG, " PS state change in CdmaLte mode");
                        updateMalCgregState(urc);
                        if (urcId == RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED) {
                            isNeedResponseUrc = false;
                        } else {
                            urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(),
                                    RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED);
                        }
                    } else if (isEpdgSupport()
                        && (getDeviceMode() == NWS_MODE_CSFB || getDeviceMode() == NWS_MODE_LTEONLY)) {
                        logD(RP_NW_STATE_TAG, " PS state change in CSFB mode");
                        updateMalCgregState(urc);
                        urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(),
                                RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED);
                        combineMalCgregState(urcToRilj);
                    }
                    break;
                case RIL_UNSOL_FEMTOCELL_INFO:
                    int ispendingFemtoCell;
                    p = urc->getParcel();
                    p->readInt32(&stgCount);
                    if (urc->getSource() == RADIO_TECH_GROUP_GSM) {
                        if (stgCount >= 6) {
                            pStrings = (char **) alloca(sizeof(char *) * stgCount);
                            for (int i = 0; i < stgCount; i++) {
                                pStrings[i] = strdupReadString(p);
                            }
                            isCsgCell = strtol(pStrings[5], NULL, 10);
                            logD(RP_NW_STATE_TAG, " femtocell isCsgCell:%d", isCsgCell);
                            updateCsgInfoCache(pStrings);
                        }
                    } else if (urc->getSource() == RADIO_TECH_GROUP_C2K) {
                        if (stgCount >= 4) {
                            pStrings = (char **) alloca(sizeof(char *) * stgCount);
                            for (int i = 0; i < stgCount; i++) {
                                pStrings[i] = strdupReadString(p);
                            }
                            for (int i = 0; i < 2; i++) {
                                system = strtol(pStrings[2 * i], NULL, 10);
                                state = strtol(pStrings[2 * i + 1], NULL, 10);
                                logD(RP_NW_STATE_TAG, " femtocell sys:%d, state:%d", system, state);
                                if (system == 0) {
                                    // 1xRTT
                                    if (state == 1) {
                                        is1xRttInFemtocell = true;
                                    } else {
                                        is1xRttInFemtocell = false;
                                    }
                                } else {
                                    // EVDO
                                    if (state == 1) {
                                        isEvdoInFemtocell = true;
                                    } else {
                                        isEvdoInFemtocell = false;
                                    }
                                }
                            }
                        }
                    }
                    #ifdef MTK_TC1_FEATURE
                    urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(),
                            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED);
                    #else
                    ispendingFemtoCell = gsm_femto_cell_cache.is_csg_cell;
                    if (is1xRttInFemtocell == true || isEvdoInFemtocell == true) {
                        ispendingFemtoCell = 2;
                    }
                    if (gsm_femto_cell_cache.isFemtocell != ispendingFemtoCell) {
                        gsm_femto_cell_cache.isFemtocell = ispendingFemtoCell;
                        logD(RP_NW_STATE_TAG, " femtocell isFemtocell:%d", gsm_femto_cell_cache.isFemtocell);
                        urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(), RIL_UNSOL_FEMTOCELL_INFO);
                        updateFemtoCell(urcToRilj);
                    } else {
                        isNeedResponseUrc = false;
                    }
                    #endif
                    if (pStrings != NULL) {
                        for (int i = 0; i < stgCount; i++) {
                            if (pStrings[i] != NULL) {
                                free(pStrings[i]);
                                pStrings[i] = NULL;
                            }
                        }
                    }
                    break;
                case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS:
                    int count;
                    int otaState;
                    int convertState;

                    p = urc->getParcel();
                    p->readInt32(&count);
                    p->readInt32(&otaState);

                    getStatusManager()->setIntValue(RFX_STATUS_KEY_OTA_STATUS, otaState);
                    convertState = convertOtaProvisionStatus(otaState);

                    if (convertState >= 0) {
                        urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(),
                                RIL_UNSOL_CDMA_OTA_PROVISION_STATUS);
                        p = urcToRilj->getParcel();
                        p->writeInt32(count);
                        p->writeInt32(convertState);
                    } else {
                        isNeedResponseUrc = false;
                    }

                    break;
                case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED:
                default:
                    break;
            }
        } else {
            // urc contains error
            logD(RP_NW_STATE_TAG, " URC error");
        }
    } else {
        if (urc->getError() == RIL_E_SUCCESS) {
            switch (urcId) {
                case RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED:
                case RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE:
                    updateMalCgregState(urc);
                    urcToRilj = RfxMessage::obtainUrc(urc->getSlotId(),
                            RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED);
                    combineMalCgregState(urcToRilj);
                    break;
                default:
                    break;
            }
        }
    }
    if (isNeedResponseUrc) {
        responseToRilj(urcToRilj);
    }
    return true;
}

void RpNwStateController::updateVoiceRegStateCache(const sp<RfxMessage>& msg) {
    int skip;
    int source = msg->getSource();
    int32_t stgCount = -1;
    char **pStrings = NULL;
    Parcel *p = msg->getParcel();

    p->readInt32(&stgCount);

    if (stgCount > 0) {
        pStrings = (char **) alloca(sizeof(char *) * stgCount);
        for (int i = 0; i < stgCount; i++) {
            pStrings[i] = strdupReadString(p);
            //logD(RP_NW_STATE_TAG, " data[%d]=%s", i, pStrings[i]);
        }
        if (source == RADIO_TECH_GROUP_GSM) {
            if (mDebug) {
                logD(RP_NW_STATE_TAG, " updateVoiceRegStateCache(): src=%s, stgCount=%d",
                        sourceToString(source), stgCount);
            }
            gsm_voice_reg_state_cache.register_state   = strtol(pStrings[0], NULL, 10);
            if (stgCount > 4) {
                gsm_voice_reg_state_cache.lac              = strtol(pStrings[1], NULL, 16);
                gsm_voice_reg_state_cache.cid              = strtol(pStrings[2], NULL, 16);
                gsm_voice_reg_state_cache.radio_technology = strtol(pStrings[3], NULL, 10);
                gsm_voice_reg_state_cache.deny_reason      = strtol(pStrings[13], NULL, 10);
            }
            if (!RfxNwServiceState::isInService(gsm_voice_reg_state_cache.register_state)) {
                gsm_voice_reg_state_cache.lac              = -1;
                gsm_voice_reg_state_cache.cid              = -1;
                gsm_voice_reg_state_cache.radio_technology = 0;
            }
            printVoiceCache(gsm_voice_reg_state_cache);
        } else if (source == RADIO_TECH_GROUP_C2K) {
            cdma_voice_reg_state_cache.register_state         = strtol(pStrings[0], NULL, 10);
            cdma_voice_reg_state_cache.radio_technology       = strtol(pStrings[3], NULL, 10);
            cdma_voice_reg_state_cache.base_station_id        = strtol(pStrings[4], NULL, 10);
            cdma_voice_reg_state_cache.base_station_latitude  = strtol(pStrings[5], NULL, 10);
            cdma_voice_reg_state_cache.base_station_longitude = strtol(pStrings[6], NULL, 10);
            cdma_voice_reg_state_cache.css                    = strtol(pStrings[7], NULL, 10);
            cdma_voice_reg_state_cache.system_id              = strtol(pStrings[8], NULL, 10);
            cdma_voice_reg_state_cache.network_id             = strtol(pStrings[9], NULL, 10);
            cdma_voice_reg_state_cache.roaming_ind            = strtol(pStrings[10], NULL, 10);
            cdma_voice_reg_state_cache.prl                    = strtol(pStrings[11], NULL, 10);
            cdma_voice_reg_state_cache.prl_roaming_ind        = strtol(pStrings[12], NULL, 10);
            cdma_voice_reg_state_cache.network_exsit          = strtol(pStrings[15], NULL, 10);
            printVoiceCache(cdma_voice_reg_state_cache);
        } else {
            // source type invalid!!!
            logD(RP_NW_STATE_TAG, " updateVoiceRegStateCache(): source type invalid!!!");
        }
    }
    if (pStrings != NULL) {
        for (int i = 0; i < stgCount; i++) {
            free(pStrings[i]);
        }
    }
}

int RpNwStateController::calculateCssValue() {
    char prop[255] = {0};
    int ret = 0;

    property_get("ro.boot.opt_c2k_lte_mode", prop, "0");
    if (atoi(prop) == 1 && getDeviceMode() == NWS_MODE_CDMALTE
            && gsm_data_reg_state_cache.radio_technology == RADIO_TECH_LTE) {
        ret = 1;
    }

    return ret;
}

void RpNwStateController::updateDataRegStateCache(const sp<RfxMessage>& msg) {
    int skip;
    int source = msg->getSource();
    int32_t stgCount = -1;
    char **pStrings = NULL;
    Parcel *p = msg->getParcel();

    p->readInt32(&stgCount);

    if (stgCount > 0) {
        pStrings = (char **) alloca(sizeof(char *) * stgCount);
        for (int i = 0; i < stgCount; i++) {
            pStrings[i] = strdupReadString(p);
            //logD(RP_NW_STATE_TAG, " dataCtn[%d]=%s", i, pStrings[i]);
        }
        if (source == RADIO_TECH_GROUP_GSM) {
            if (mDebug) {
                logD(RP_NW_STATE_TAG, " updateDataRegStateCache(): src=%s, stgCount=%d",
                        sourceToString(source), stgCount);
            }
            gsm_data_reg_state_cache.register_state   = strtol(pStrings[0], NULL, 10);
            if (stgCount > 4) {
                gsm_data_reg_state_cache.lac              = strtol(pStrings[1], NULL, 16);
                gsm_data_reg_state_cache.cid              = strtol(pStrings[2], NULL, 16);
                gsm_data_reg_state_cache.radio_technology = strtol(pStrings[3], NULL, 10);
                gsm_data_reg_state_cache.deny_reason      = strtol(pStrings[4], NULL, 10);
                gsm_data_reg_state_cache.max_data_call    = strtol(pStrings[5], NULL, 10);
            } else if (stgCount == 4) {
                gsm_data_reg_state_cache.lac              = strtol(pStrings[1], NULL, 16);
                gsm_data_reg_state_cache.cid              = strtol(pStrings[2], NULL, 16);
                gsm_data_reg_state_cache.radio_technology = strtol(pStrings[3], NULL, 10);
                // IWLAN
                if (gsm_data_reg_state_cache.radio_technology == 18) {
                    gsm_data_reg_state_cache.lac          = -1;
                    gsm_data_reg_state_cache.cid          = -1;
                }
            }
            if (!RfxNwServiceState::isInService(gsm_data_reg_state_cache.register_state)) {
                gsm_data_reg_state_cache.lac              = -1;
                gsm_data_reg_state_cache.cid              = -1;
                gsm_data_reg_state_cache.radio_technology = 0;
            }
            printDataCache(gsm_data_reg_state_cache);
        } else if (source == RADIO_TECH_GROUP_C2K) {
            cdma_data_reg_state_cache.register_state   = strtol(pStrings[0], NULL, 10);
            cdma_data_reg_state_cache.radio_technology = strtol(pStrings[3], NULL, 10);
            printDataCache(cdma_data_reg_state_cache);
        } else {
            // source type invalid!!!
            logD(RP_NW_STATE_TAG, " updateDataRegStateCache(): source type invalid!!!");
        }
    }
    if (pStrings != NULL) {
        for (int i = 0; i < stgCount; i++) {
            free(pStrings[i]);
        }
    }
}

void RpNwStateController::updateSignalStrengthCache(const sp<RfxMessage>& msg) {
    int skip;
    int source = msg->getSource();
    Parcel *p = msg->getParcel();

    if (source == RADIO_TECH_GROUP_GSM) {
        p->readInt32(&signal_strength_cache.gsm_sig1);
        p->readInt32(&signal_strength_cache.gsm_sig2);
        for (int i = 0; i < 5; i++) {
            // skip C2K part
            p->readInt32(&skip);
        }
        p->readInt32(&signal_strength_cache.lte_sig);
        p->readInt32(&signal_strength_cache.lte_rsrp);
        p->readInt32(&signal_strength_cache.lte_rsrq);
        p->readInt32(&signal_strength_cache.lte_rssnr);
        p->readInt32(&signal_strength_cache.lte_cqi);
        p->readInt32(&signal_strength_cache.tdscdma_rscp);
        p->readInt32(&signal_strength_cache.rssi_qdbm);
        p->readInt32(&signal_strength_cache.rscp_qdbm);
        p->readInt32(&signal_strength_cache.ecn0_qdbm);
    } else if (source == RADIO_TECH_GROUP_C2K) {
        for (int i = 0; i < 2; i++) {
            // skip GSM part
            p->readInt32(&skip);
        }
        p->readInt32(&signal_strength_cache.cdma_dbm);
        p->readInt32(&signal_strength_cache.cdma_ecio);
        p->readInt32(&signal_strength_cache.ecdo_dbm);
        p->readInt32(&signal_strength_cache.evdo_ecio);
        p->readInt32(&signal_strength_cache.evdo_snr);
    } else {
        // source type invalid!!!
    }
}

void RpNwStateController::updateOperatorCache(const sp<RfxMessage>& msg) {
    int source = msg->getSource();
    int32_t stgCount = -1;
    char **pStrings = NULL;
    Parcel *p = msg->getParcel();

    p->readInt32(&stgCount);
    if (mDebug) {
        logD(RP_NW_STATE_TAG, " updateOperatorCache(): src=%s, stgCount=%d",
                sourceToString(source), stgCount);
    }

    if (stgCount > 0) {
        pStrings = (char **) alloca(sizeof(char *) * stgCount);
        for (int i = 0; i < stgCount; i++) {
            pStrings[i] = strdupReadString(p);
            //logD(RP_NW_STATE_TAG, " updateOperatorCache(): data=%s", pStrings[i]);
        }
        if (source == RADIO_TECH_GROUP_GSM) {
            gsm_operator_cache.count = stgCount;
            if (stgCount == 3) {
                if (pStrings[0] != NULL) {
                    strncpy(gsm_operator_cache.optrAlphaLong, pStrings[0], MAX_OPER_NAME_LENGTH);
                } else {
                    gsm_operator_cache.optrAlphaLong[0] = '\0';
                }
                if (pStrings[1] != NULL) {
                    strncpy(gsm_operator_cache.optrAlphaShort, pStrings[1], MAX_OPER_NAME_LENGTH);
                } else {
                    gsm_operator_cache.optrAlphaShort[0] = '\0';
                }
                if (pStrings[2] != NULL) {
                    strncpy(gsm_operator_cache.optrNumeric, pStrings[2], MAX_OPER_NUM_LENGTH);
                } else {
                    gsm_operator_cache.optrNumeric[0] = '\0';
                }
                if (mDebug) {
                    logD(RP_NW_STATE_TAG, " updateOperatorCache(): "
                            "pStrings[0]==null:%s, pStrings[1]==null:%s, pStrings[2]==null:%s",
                            (pStrings[0] != NULL ? "false" : "true"),
                            (pStrings[1] != NULL ? "false" : "true"),
                            (pStrings[2] != NULL ? "false" : "true"));
                }
                gsm_operator_cache.optrAlphaLong[MAX_OPER_NAME_LENGTH - 1] = '\0';
                gsm_operator_cache.optrAlphaShort[MAX_OPER_NAME_LENGTH - 1] = '\0';
                gsm_operator_cache.optrNumeric[MAX_OPER_NUM_LENGTH - 1] = '\0';
            } else if (stgCount == 1) {
                strncpy(gsm_operator_cache.optrAlphaLong, pStrings[0], MAX_OPER_NAME_LENGTH);
                gsm_operator_cache.optrAlphaLong[MAX_OPER_NAME_LENGTH - 1] = '\0';
                memset(gsm_operator_cache.optrAlphaShort, '\0',
                        MAX_OPER_NAME_LENGTH * sizeof(char));
                memset(gsm_operator_cache.optrNumeric, '\0',
                        MAX_OPER_NUM_LENGTH * sizeof(char));
            } else {
                resetOperatorCache(&gsm_operator_cache);
            }
            printOperatorCache(gsm_operator_cache);
        } else if (source == RADIO_TECH_GROUP_C2K) {
            cdma_operator_cache.count = stgCount;
            if (stgCount == 3) {
                strncpy(cdma_operator_cache.optrAlphaLong, pStrings[0], MAX_OPER_NAME_LENGTH);
                strncpy(cdma_operator_cache.optrAlphaShort, pStrings[1], MAX_OPER_NAME_LENGTH);
                strncpy(cdma_operator_cache.optrNumeric, pStrings[2], MAX_OPER_NUM_LENGTH);
                cdma_operator_cache.optrAlphaLong[MAX_OPER_NAME_LENGTH - 1] = '\0';
                cdma_operator_cache.optrAlphaShort[MAX_OPER_NAME_LENGTH - 1] = '\0';
                cdma_operator_cache.optrNumeric[MAX_OPER_NUM_LENGTH - 1] = '\0';
            } else {
                resetOperatorCache(&cdma_operator_cache);
            }
            printOperatorCache(cdma_operator_cache);
        } else {
            // source type invalid!!!
            logD(RP_NW_STATE_TAG, " updateOperatorCache(): source type invalid!!!");
        }
    }
    if (pStrings != NULL) {
        for (int i = 0; i < stgCount; i++) {
            free(pStrings[i]);
        }
    }
}

void RpNwStateController::combineVoiceRegState(const sp<RfxMessage>& msg) {
    int stgCount = 16;
    char *stgBuf = (char *) malloc(50);
    if (stgBuf == NULL) {
        logE(RP_NW_STATE_TAG, "combineVoiceRegState calloc fail");
        return;
    }
    // decide common info
    if (RfxNwServiceState::isInService(gsm_voice_reg_state_cache.register_state)) {
        mVoiceRadioSys = RADIO_TECH_GROUP_GSM;
        mVoiceRegState = gsm_voice_reg_state_cache.register_state;
        mVoiceRadioTech = gsm_voice_reg_state_cache.radio_technology;
    } else if (RfxNwServiceState::isInService(cdma_voice_reg_state_cache.register_state)) {
        mVoiceRadioSys = RADIO_TECH_GROUP_C2K;
        mVoiceRegState = cdma_voice_reg_state_cache.register_state;
        mVoiceRadioTech = cdma_voice_reg_state_cache.radio_technology;
    } else {
        if (getDeviceMode() == NWS_MODE_CDMALTE) {
            mVoiceRegState = cdma_voice_reg_state_cache.register_state;
            mVoiceRadioTech = cdma_voice_reg_state_cache.radio_technology;
        } else {
            mVoiceRegState = gsm_voice_reg_state_cache.register_state;
            mVoiceRadioTech = gsm_voice_reg_state_cache.radio_technology;
        }
    }
    mCdmaNetworkExist = cdma_voice_reg_state_cache.network_exsit;

    // set combine result to parcel
    Parcel *p = msg->getParcel();
    p->writeInt32(stgCount);

    sprintf(stgBuf, "%d", mVoiceRegState);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%x", gsm_voice_reg_state_cache.lac);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%x", gsm_voice_reg_state_cache.cid);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", mVoiceRadioTech);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.base_station_id);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.base_station_latitude);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.base_station_longitude);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.css);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.system_id);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.network_id);
    writeStringToParcel(p, stgBuf);

    // Add for MTK_TC1_FEATURE - start
    if (RfxNwServiceState::isInService(cdma_voice_reg_state_cache.register_state)) {
        sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.roaming_ind);
        writeStringToParcel(p, stgBuf);

        sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.prl);
        writeStringToParcel(p, stgBuf);

        sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.prl_roaming_ind);
        writeStringToParcel(p, stgBuf);
    } else {
        writeStringToParcel(p, NULL);
        writeStringToParcel(p, NULL);
        writeStringToParcel(p, NULL);
    }
    // Add for MTK_TC1_FEATURE - end

    sprintf(stgBuf, "%d", gsm_voice_reg_state_cache.deny_reason);
    writeStringToParcel(p, stgBuf);

    writeStringToParcel(p, "-1");  // Default value for psc, so far, nobody parse it

    sprintf(stgBuf, "%d", cdma_voice_reg_state_cache.network_exsit);
    writeStringToParcel(p, stgBuf);

    getStatusManager()->setServiceStateValue(RFX_STATUS_KEY_SERVICE_STATE,
            RfxNwServiceState(mVoiceRegState, mDataRegState, mVoiceRadioTech, mDataRadioTech, mCdmaNetworkExist));
    free(stgBuf);
    return;
}

void RpNwStateController::combineDataRegState(const sp<RfxMessage>& msg) {
    int stgCount = 6;
    char *stgBuf = (char *) malloc(50);
    if (stgBuf == NULL) {
        logE(RP_NW_STATE_TAG, "combineDataRegState calloc fail");
        return;
    }
    // decide common info
    if (RfxNwServiceState::isInService(gsm_data_reg_state_cache.register_state)) {
        mDataRadioSys = RADIO_TECH_GROUP_GSM;
        mDataRegState = gsm_data_reg_state_cache.register_state;
        mDataRadioTech = gsm_data_reg_state_cache.radio_technology;
    } else if (RfxNwServiceState::isInService(cdma_data_reg_state_cache.register_state)) {
        mDataRadioSys = RADIO_TECH_GROUP_C2K;
        mDataRegState = cdma_data_reg_state_cache.register_state;
        mDataRadioTech = cdma_data_reg_state_cache.radio_technology;
    } else {
        if (ps_reg_state.wifi_state == 99) {
            mDataRegState = 1;
            mDataRadioTech = 18;
            mDataRadioSys = -1;
        } else {
            mDataRegState = gsm_data_reg_state_cache.register_state;
            mDataRadioTech = gsm_data_reg_state_cache.radio_technology;
            if (getDeviceMode() == NWS_MODE_CDMALTE) {
                int radioCapability = getStatusManager()->getIntValue(
                        RFX_STATUS_KEY_RADIO_CAPABILITY, RIL_CAPABILITY_NONE);
                if (radioCapability == RIL_CAPABILITY_CDMA_ONLY) {
                    mDataRegState = cdma_data_reg_state_cache.register_state;
                    mDataRadioTech = cdma_data_reg_state_cache.radio_technology;
                } else {
                    if (mDataRadioSys == RADIO_TECH_GROUP_C2K) {
                        mDataRegState = cdma_data_reg_state_cache.register_state;
                        mDataRadioTech = cdma_data_reg_state_cache.radio_technology;
                    }
                }
            }
        }
    }

    // set combine result to parcel
    Parcel *p = msg->getParcel();
    p->writeInt32(stgCount);

    sprintf(stgBuf, "%d", mDataRegState);
    writeStringToParcel(p, stgBuf);

    if (gsm_data_reg_state_cache.lac != -1) {
        sprintf(stgBuf, "%x", gsm_data_reg_state_cache.lac);
    } else {
        sprintf(stgBuf, "%d", gsm_data_reg_state_cache.lac);
    }
    writeStringToParcel(p, stgBuf);

    if (gsm_data_reg_state_cache.cid != -1) {
        sprintf(stgBuf, "%x", gsm_data_reg_state_cache.cid);
    } else {
        sprintf(stgBuf, "%d", gsm_data_reg_state_cache.cid);
    }
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", mDataRadioTech);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", gsm_data_reg_state_cache.deny_reason);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", gsm_data_reg_state_cache.max_data_call);
    writeStringToParcel(p, stgBuf);

    getStatusManager()->setServiceStateValue(RFX_STATUS_KEY_SERVICE_STATE,
            RfxNwServiceState(mVoiceRegState, mDataRegState, mVoiceRadioTech, mDataRadioTech, mCdmaNetworkExist));
    free(stgBuf);
    return;
}

void RpNwStateController::combineSignalStrength(const sp<RfxMessage>& msg) {
    Parcel *p = msg->getParcel();

    p->writeInt32(signal_strength_cache.gsm_sig1);
    p->writeInt32(signal_strength_cache.gsm_sig2);
    p->writeInt32(signal_strength_cache.cdma_dbm);
    p->writeInt32(signal_strength_cache.cdma_ecio);
    p->writeInt32(signal_strength_cache.ecdo_dbm);
    p->writeInt32(signal_strength_cache.evdo_ecio);
    p->writeInt32(signal_strength_cache.evdo_snr);
    p->writeInt32(signal_strength_cache.lte_sig);
    p->writeInt32(signal_strength_cache.lte_rsrp);
    p->writeInt32(signal_strength_cache.lte_rsrq);
    p->writeInt32(signal_strength_cache.lte_rssnr);
    p->writeInt32(signal_strength_cache.lte_cqi);
    p->writeInt32(signal_strength_cache.tdscdma_rscp);
    p->writeInt32(signal_strength_cache.rssi_qdbm);
    p->writeInt32(signal_strength_cache.rscp_qdbm);
    p->writeInt32(signal_strength_cache.ecn0_qdbm);
    printSignalStrengthCache(signal_strength_cache);
}

void RpNwStateController::combineOperatorState(const sp<RfxMessage>& msg) {
    Parcel *p = msg->getParcel();
    RIL_OPERATOR_CACHE optrCache = gsm_operator_cache;

    if (getDeviceMode() == NWS_MODE_CDMALTE) {
        if (RfxNwServiceState::isInService(cdma_data_reg_state_cache.register_state)) {
            logD(RP_NW_STATE_TAG, " combineOperatorState():count=%d, C2K ps in service", optrCache.count);
            optrCache = cdma_operator_cache;
        } else if (RfxNwServiceState::isInService(cdma_voice_reg_state_cache.register_state)
                && (!RfxNwServiceState::isInService(gsm_data_reg_state_cache.register_state)
                    || optrCache.count == 1)) {
            logD(RP_NW_STATE_TAG, " combineOperatorState():count=%d, C2K cs in service, GSM ps not in service", optrCache.count);
            optrCache = cdma_operator_cache;
        } else if (mDataRadioSys == RADIO_TECH_GROUP_C2K) {
            logD(RP_NW_STATE_TAG, " combineOperatorState():count=%d, C2K = mDataRadioSys", optrCache.count);
            optrCache = cdma_operator_cache;
        } else if (strncmp("000000", gsm_operator_cache.optrNumeric, 6) == 0) {
            logD(RP_NW_STATE_TAG, " combineOperatorState():count=%d, GSM numeric invalid", optrCache.count);
            optrCache = cdma_operator_cache;
        }
    }
    p->writeInt32(optrCache.count);
    if (optrCache.count == 3) {
        writeStringToParcel(p, optrCache.optrAlphaLong);
        writeStringToParcel(p, optrCache.optrAlphaShort);
        writeStringToParcel(p, optrCache.optrNumeric);
    } else if (optrCache.count == 1) {
        writeStringToParcel(p, optrCache.optrAlphaLong);
    }
}

void RpNwStateController::updateCsgInfoCache(char **pStrings) {

    gsm_femto_cell_cache.domain = strtol(pStrings[0], NULL, 10);

    gsm_femto_cell_cache.state = strtol(pStrings[1], NULL, 10);

    if (pStrings[2] != NULL && strlen(pStrings[2]) > 0) {
        strncpy(gsm_femto_cell_cache.optrAlphaLong, pStrings[2], MAX_OPER_NAME_LENGTH-1);
        gsm_femto_cell_cache.optrAlphaLong[MAX_OPER_NAME_LENGTH - 1] = '\0';
    } else {
        memset(gsm_femto_cell_cache.optrAlphaLong, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
    }

    gsm_femto_cell_cache.plmn_id = strtol(pStrings[3], NULL, 10);

    gsm_femto_cell_cache.act = strtol(pStrings[4], NULL, 10);

    gsm_femto_cell_cache.is_csg_cell = strtol(pStrings[5], NULL, 10);

    if (gsm_femto_cell_cache.is_csg_cell == 1) {
        gsm_femto_cell_cache.csg_id = strtol(pStrings[6], NULL, 10);
        gsm_femto_cell_cache.csg_icon_type = strtol(pStrings[7], NULL, 10);
        if (pStrings[8] != NULL && strlen(pStrings[8]) > 0) {
            strncpy(gsm_femto_cell_cache.hnbName, pStrings[8], MAX_OPER_NAME_LENGTH-1);
            gsm_femto_cell_cache.hnbName[MAX_OPER_NAME_LENGTH - 1] = '\0';
        } else {
            memset(gsm_femto_cell_cache.hnbName, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
        }
    } else {
        gsm_femto_cell_cache.csg_id = -1;
        gsm_femto_cell_cache.csg_icon_type = -1;
        memset(gsm_femto_cell_cache.hnbName, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
    }
    gsm_femto_cell_cache.cause = strtol(pStrings[9], NULL, 10);
}

void RpNwStateController::updateFemtoCell(const sp<RfxMessage>& urc) {
    Parcel *p = urc->getParcel();
    char *stgBuf = (char *) alloca(MAX_OPER_NAME_LENGTH);

    p->writeInt32(10);

    // <domain>,<state>,<plmn_id>,<lognname>,<act>,<is_csg_cell/is_femto_cell>,<csg_id>,<csg_icon_type>,<hnb_name>,<cause> */
    sprintf(stgBuf, "%d", gsm_femto_cell_cache.domain);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", gsm_femto_cell_cache.state);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", gsm_femto_cell_cache.plmn_id);
    writeStringToParcel(p, stgBuf);

    if (strlen(gsm_femto_cell_cache.optrAlphaLong) != 0) {
        writeStringToParcel(p, gsm_femto_cell_cache.optrAlphaLong);
    } else {
        writeStringToParcel(p, NULL);
    }

    sprintf(stgBuf, "%d", gsm_femto_cell_cache.act);
    writeStringToParcel(p, stgBuf);

    sprintf(stgBuf, "%d", gsm_femto_cell_cache.isFemtocell);
    writeStringToParcel(p, stgBuf);

    if (gsm_femto_cell_cache.isFemtocell == 1) {
        sprintf(stgBuf, "%d", gsm_femto_cell_cache.csg_id);
        writeStringToParcel(p, stgBuf);

        sprintf(stgBuf, "%d", gsm_femto_cell_cache.csg_icon_type);
        writeStringToParcel(p, stgBuf);

        writeStringToParcel(p, gsm_femto_cell_cache.hnbName);
    } else {
        writeStringToParcel(p, NULL);
        writeStringToParcel(p, NULL);
        writeStringToParcel(p, NULL);
    }
    sprintf(stgBuf, "%d", gsm_femto_cell_cache.cause);
    writeStringToParcel(p, stgBuf);
}

bool RpNwStateController::onHandleResponse(const sp<RfxMessage>& response) {
    int reqId = response->getId();
    int slotId = response->getSlotId();
    int source = response->getSource();
    sp<RfxMessage> responseInQueue;
    sp<RfxMessage> responseMd1;
    sp<RfxMessage> responseMd3;
    ResponseStatus responseStatus;

    if (response->getSentOnCdmaCapabilitySlot() == C_SLOT_STATUS_IS_CURRENT_SLOT) {
        responseInQueue = sp<RfxMessage>(NULL);
        responseStatus = preprocessResponse(response, responseInQueue,
                RfxWaitResponseTimedOutCallback(this, &RpNwStateController::onResponseTimeOut),
                s2ns(RESPONSE_TIME_OUT_SEC));

        if (responseStatus == RESPONSE_STATUS_HAVE_MATCHED
            || responseStatus == RESPONSE_STATUS_HAVE_MATCHED_WITH_C_CHANGED) {
            if (responseInQueue->getSource() == RADIO_TECH_GROUP_GSM) {
                responseMd1 = responseInQueue;
                responseMd3 = response;
            } else {
                responseMd1 = response;
                responseMd3 = responseInQueue;
            }
            switch (reqId) {
                case RIL_REQUEST_VOICE_REGISTRATION_STATE:
                    onReceiveVoiceRegStateResponse(responseMd1, responseMd3);
                    break;
                case RIL_REQUEST_DATA_REGISTRATION_STATE:
                    onReceiveDataRegStateResponse(responseMd1, responseMd3);
                    break;
                case RIL_REQUEST_SIGNAL_STRENGTH:
                    onReceiveSignalStrengthResponse(responseMd1, responseMd3);
                    break;
                case RIL_REQUEST_OPERATOR:
                    onReceiveOperatorResponse(responseMd1, responseMd3);
                    break;
                case RIL_REQUEST_GET_CELL_INFO_LIST:
                    onReceiveCellInfoList(responseMd1, responseMd3);
                    break;
                default:
                    break;
            }
        } else if (responseStatus == RESPONSE_STATUS_NO_MATCH_AND_SAVE) {
            // logD(RP_NW_STATE_TAG, " Response saved");
        } else if (responseStatus == RESPONSE_STATUS_ALREADY_SAVED) {
            // logD(RP_NW_STATE_TAG, " Response have saved -> ignore");
        } else if (responseStatus == RESPONSE_STATUS_HAVE_BEEN_DELETED) {
            logD(RP_NW_STATE_TAG, " Response have time out! REQ=%s src=%s token=%d",
                    requestToString(reqId), sourceToString(source), response->getPToken());
        } else {
            logD(RP_NW_STATE_TAG, " Response misc error -> ignore");
        }
    } else {
        sp<RfxMessage> resToRilj = response;

        switch (reqId) {
            case RIL_REQUEST_DATA_REGISTRATION_STATE:
                resetDataRegStateCache(&cdma_data_reg_state_cache);
                if (resToRilj->getError() == RIL_E_SUCCESS) {
                    updateDataRegStateCache(resToRilj);

                    resToRilj = RfxMessage::obtainResponse(RIL_E_SUCCESS, resToRilj);
                    combineDataRegState(resToRilj);
                } else {
                    // response contains error
                    resetDataRegStateCache(&gsm_data_reg_state_cache);
                }
                break;
            case RIL_REQUEST_VOICE_REGISTRATION_STATE:
                resetVoiceRegStateCache(&cdma_voice_reg_state_cache);
                if (resToRilj->getError() == RIL_E_SUCCESS) {
                    updateVoiceRegStateCache(resToRilj);

                    resToRilj = RfxMessage::obtainResponse(RIL_E_SUCCESS, resToRilj);
                    combineVoiceRegState(resToRilj);
                } else {
                    // response contains error
                    resetVoiceRegStateCache(&gsm_voice_reg_state_cache);
                }
                break;
            default:
                break;
        }

        responseToRilj(resToRilj);
    }
    return true;
}

void RpNwStateController::onReceiveVoiceRegStateResponse(
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    int errInd = 0;
    sp<RfxMessage> resToRilj;

    if (responseMd1->getError() == RIL_E_SUCCESS) {
        updateVoiceRegStateCache(responseMd1);
    } else {
        // response contains error
        errInd |= 1;
        resetVoiceRegStateCache(&gsm_voice_reg_state_cache);
    }
    if (responseMd3->getError() == RIL_E_SUCCESS) {
        updateVoiceRegStateCache(responseMd3);
    } else {
        // response contains error
        errInd |= 2;
        resetVoiceRegStateCache(&cdma_voice_reg_state_cache);
    }

    if (errInd == 1) {
        logD(RP_NW_STATE_TAG, " GSM voice response error");
    } else if (errInd == 2) {
        logD(RP_NW_STATE_TAG, " C2K voice response error");
    } else if (errInd == 3) {
        // logD(RP_NW_STATE_TAG, " GSM and C2K voice response all error");
    }

    resToRilj = genResponseForRilj(errInd, responseMd1, responseMd3);
    if (errInd < 3) {
        combineVoiceRegState(resToRilj);
    }

    responseToRilj(resToRilj);
}

void RpNwStateController::onReceiveDataRegStateResponse(
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    int errInd = 0;
    sp<RfxMessage> resToRilj;

    if (responseMd1->getError() == RIL_E_SUCCESS) {
        updateDataRegStateCache(responseMd1);
    } else {
        // response contains error
        errInd |= 1;
        resetDataRegStateCache(&gsm_data_reg_state_cache);
    }
    if (responseMd3->getError() == RIL_E_SUCCESS) {
        updateDataRegStateCache(responseMd3);
    } else {
        // response contains error
        errInd |= 2;
        resetDataRegStateCache(&cdma_data_reg_state_cache);
    }

    resToRilj = genResponseForRilj(errInd, responseMd1, responseMd3);
    if (errInd < 3) {
        combineDataRegState(resToRilj);
    }

    responseToRilj(resToRilj);
}

void RpNwStateController::onReceiveSignalStrengthResponse(
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    int errInd = 0;
    sp<RfxMessage> resToRilj;

    if (responseMd1->getError() == RIL_E_SUCCESS) {
        updateSignalStrengthCache(responseMd1);
    } else {
        // response contains error
        errInd |= 1;
    }
    if (responseMd3->getError() == RIL_E_SUCCESS) {
        updateSignalStrengthCache(responseMd3);
    } else {
        // response contains error
        errInd |= 2;
    }

    resToRilj = genResponseForRilj(errInd, responseMd1, responseMd3);
    if (errInd < 3) {
        combineSignalStrength(resToRilj);
    }

    responseToRilj(resToRilj);
}

void RpNwStateController::onReceiveOperatorResponse(
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    int errInd = 0;
    sp<RfxMessage> resToRilj;

    if (responseMd1->getError() == RIL_E_SUCCESS) {
        updateOperatorCache(responseMd1);
    } else {
        // response contains error
        errInd |= 1;
        resetOperatorCache(&gsm_operator_cache);
    }
    if (responseMd3->getError() == RIL_E_SUCCESS) {
        updateOperatorCache(responseMd3);
    } else {
        // response contains error
        errInd |= 2;
        resetOperatorCache(&cdma_operator_cache);
    }

    resToRilj = genResponseForRilj(errInd, responseMd1, responseMd3);
    if (errInd < 3) {
        combineOperatorState(resToRilj);
    }

    responseToRilj(resToRilj);
}

void RpNwStateController::updateCellInfoCache(const sp<RfxMessage>& msg) {
    int source = msg->getSource();
    int32_t infoCount = 0;
    Parcel *p = msg->getParcel();

    p->readInt32(&infoCount);
    if (mDebug) {
        logD(RP_NW_STATE_TAG, " updateCellInfoCache(): src=%s, infoCount=%d",
                sourceToString(source), infoCount);
    }

    for (int i = 0; i < infoCount; i++) {
        int infoType;
        int stampType;
        int64_t timeStamp;
        RIL_CellInfo cellInfo;
        p->readInt32(&infoType);
        p->readInt32(&cellInfo.registered);
        p->readInt32(&stampType);
        p->readInt64(&timeStamp);
        cellInfo.timeStamp = (uint64_t) timeStamp;
        logD(RP_NW_STATE_TAG, " updateCellInfoCache(): info%d , type:%d",
                i, infoType);
        switch(stampType) {
            case RIL_TIMESTAMP_TYPE_UNKNOWN: {
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_UNKNOWN;
                break;
            }
            case RIL_TIMESTAMP_TYPE_ANTENNA: {
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_ANTENNA;
                break;
            }
            case RIL_TIMESTAMP_TYPE_MODEM: {
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_MODEM;
                break;
            }
            case RIL_TIMESTAMP_TYPE_OEM_RIL: {
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_OEM_RIL;
                break;
            }
            case RIL_TIMESTAMP_TYPE_JAVA_RIL: {
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_JAVA_RIL;
                break;
            }
            default:
                cellInfo.timeStampType = RIL_TIMESTAMP_TYPE_UNKNOWN;
        }
        switch(infoType) {
            case RIL_CELL_INFO_TYPE_GSM: {
                cellInfo.cellInfoType = RIL_CELL_INFO_TYPE_GSM;
                p->readInt32(&cellInfo.CellInfo.gsm.cellIdentityGsm.mcc);
                p->readInt32(&cellInfo.CellInfo.gsm.cellIdentityGsm.mnc);
                p->readInt32(&cellInfo.CellInfo.gsm.cellIdentityGsm.lac);
                p->readInt32(&cellInfo.CellInfo.gsm.cellIdentityGsm.cid);
                p->readInt32(&cellInfo.CellInfo.gsm.signalStrengthGsm.signalStrength);
                p->readInt32(&cellInfo.CellInfo.gsm.signalStrengthGsm.bitErrorRate);
                break;
            }
            case RIL_CELL_INFO_TYPE_WCDMA: {
                cellInfo.cellInfoType = RIL_CELL_INFO_TYPE_WCDMA;
                p->readInt32(&cellInfo.CellInfo.wcdma.cellIdentityWcdma.mcc);
                p->readInt32(&cellInfo.CellInfo.wcdma.cellIdentityWcdma.mnc);
                p->readInt32(&cellInfo.CellInfo.wcdma.cellIdentityWcdma.lac);
                p->readInt32(&cellInfo.CellInfo.wcdma.cellIdentityWcdma.cid);
                p->readInt32(&cellInfo.CellInfo.wcdma.cellIdentityWcdma.psc);
                p->readInt32(&cellInfo.CellInfo.wcdma.signalStrengthWcdma.signalStrength);
                p->readInt32(&cellInfo.CellInfo.wcdma.signalStrengthWcdma.bitErrorRate);
                break;
            }
            case RIL_CELL_INFO_TYPE_CDMA: {
                cellInfo.cellInfoType = RIL_CELL_INFO_TYPE_CDMA;
                p->readInt32(&cellInfo.CellInfo.cdma.cellIdentityCdma.networkId);
                p->readInt32(&cellInfo.CellInfo.cdma.cellIdentityCdma.systemId);
                p->readInt32(&cellInfo.CellInfo.cdma.cellIdentityCdma.basestationId);
                p->readInt32(&cellInfo.CellInfo.cdma.cellIdentityCdma.longitude);
                p->readInt32(&cellInfo.CellInfo.cdma.cellIdentityCdma.latitude);

                p->readInt32(&cellInfo.CellInfo.cdma.signalStrengthCdma.dbm);
                p->readInt32(&cellInfo.CellInfo.cdma.signalStrengthCdma.ecio);
                p->readInt32(&cellInfo.CellInfo.cdma.signalStrengthEvdo.dbm);
                p->readInt32(&cellInfo.CellInfo.cdma.signalStrengthEvdo.ecio);
                p->readInt32(&cellInfo.CellInfo.cdma.signalStrengthEvdo.signalNoiseRatio);
                break;
            }
            case RIL_CELL_INFO_TYPE_LTE: {
                cellInfo.cellInfoType = RIL_CELL_INFO_TYPE_LTE;
                p->readInt32(&cellInfo.CellInfo.lte.cellIdentityLte.mcc);
                p->readInt32(&cellInfo.CellInfo.lte.cellIdentityLte.mnc);
                p->readInt32(&cellInfo.CellInfo.lte.cellIdentityLte.ci);
                p->readInt32(&cellInfo.CellInfo.lte.cellIdentityLte.pci);
                p->readInt32(&cellInfo.CellInfo.lte.cellIdentityLte.tac);

                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.signalStrength);
                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.rsrp);
                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.rsrq);
                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.rssnr);
                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.cqi);
                p->readInt32(&cellInfo.CellInfo.lte.signalStrengthLte.timingAdvance);
                break;
            }
            case RIL_CELL_INFO_TYPE_TD_SCDMA: {
                cellInfo.cellInfoType = RIL_CELL_INFO_TYPE_TD_SCDMA;
                p->readInt32(&cellInfo.CellInfo.tdscdma.cellIdentityTdscdma.mcc);
                p->readInt32(&cellInfo.CellInfo.tdscdma.cellIdentityTdscdma.mnc);
                p->readInt32(&cellInfo.CellInfo.tdscdma.cellIdentityTdscdma.lac);
                p->readInt32(&cellInfo.CellInfo.tdscdma.cellIdentityTdscdma.cid);
                p->readInt32(&cellInfo.CellInfo.tdscdma.cellIdentityTdscdma.cpid);
                p->readInt32(&cellInfo.CellInfo.tdscdma.signalStrengthTdscdma.rscp);
                break;
            }
        }
        cell_info_cache.add(cellInfo);
    }
}

void RpNwStateController::onReceiveCellInfoList(
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    int errInd = 0;
    int infoCount1 = 0;
    int infoCount3 = 0;
    int offset = 0;
    Parcel *p;
    Parcel *p1;
    Parcel *p3;
    sp<RfxMessage> resToRilj;

    if (responseMd1->getError() == RIL_E_SUCCESS) {
        // logD(RP_NW_STATE_TAG, " GSM cell info response ok");
    } else {
        // response contains error
        errInd |= 1;
    }
    if (responseMd3->getError() == RIL_E_SUCCESS) {
        // logD(RP_NW_STATE_TAG, " C2K cell info response ok");
    } else {
        // response contains error
        errInd |= 2;
    }
    if (errInd == 0) {
        p1 = responseMd1->getParcel();
        p1->readInt32(&infoCount1);
        p3 = responseMd3->getParcel();
        p3->readInt32(&infoCount3);
        resToRilj = RfxMessage::obtainResponse(RIL_E_SUCCESS, responseMd1);
        p = resToRilj->getParcel();
        offset = sizeof(int32_t) * 4;
        p->writeInt32(infoCount1 + infoCount3);
        p->appendFrom(p1, offset, p1->dataSize() - offset);
        p->appendFrom(p3, offset, p3->dataSize() - offset);
        logD(RP_NW_STATE_TAG, " onReceiveCellInfoList(): infoCount=%d", infoCount1 + infoCount3);
    } else if (errInd == 2) {
        resToRilj = responseMd1;
        logD(RP_NW_STATE_TAG, " onReceiveCellInfoList(): C2K response error, return GSM cell info");
    } else if (errInd == 1) {
        resToRilj = responseMd3;
        logD(RP_NW_STATE_TAG, " onReceiveCellInfoList(): GSM response error, return C2K cell info");
    } else {
        resToRilj = responseMd1;
        logD(RP_NW_STATE_TAG, " onReceiveCellInfoList(): GSM/C2K cell info both err");
    }

    responseToRilj(resToRilj);
}

sp<RfxMessage> RpNwStateController::genResponseForRilj(int errInd,
        const sp<RfxMessage>& responseMd1, const sp<RfxMessage>& responseMd3) {
    sp<RfxMessage> rlt;

    if (errInd == 0 || errInd == 2) {
        rlt = RfxMessage::obtainResponse(RIL_E_SUCCESS, responseMd1);
    } else if (errInd == 1) {
        rlt = RfxMessage::obtainResponse(RIL_E_SUCCESS, responseMd3);
    } else {
        rlt = responseMd1;
    }

    return rlt;
}

void RpNwStateController::onResponseTimeOut(const sp<RfxMessage>& response) {
    int reqId = response->getId();
    int slotId = response->getSlotId();
    logD(RP_NW_STATE_TAG, " onResponseTimeOut() REQ=%s src=%s token=%d",
            requestToString(reqId), sourceToString(response->getSource()),
            response->getPToken());
    responseToRilj(response);
}

int RpNwStateController::getDeviceMode() {
    int mode = getStatusManager()->getIntValue(RFX_STATUS_KEY_NWS_MODE, NWS_MODE_CSFB);
    return mode;
}

int RpNwStateController::convertOtaProvisionStatus(int rawState){
    int state;
    switch(rawState) {
        case 2: {  // service programming lock unlocked
            state = CDMA_OTA_PROVISION_STATUS_SPL_UNLOCKED;
            break;
        }
        case 3: {  // NAM parameters downloaded successfully
            state = CDMA_OTA_PROVISION_STATUS_NAM_DOWNLOADED;
            break;
        }
        case 4: {  // MDN downloaded successfully
            state = CDMA_OTA_PROVISION_STATUS_MDN_DOWNLOADED;
            break;
        }
        case 5: {  // IMSI downloaded successfully
            state = CDMA_OTA_PROVISION_STATUS_IMSI_DOWNLOADED;
            break;
        }
        case 6: {  // PRL downloaded successfully
            state = CDMA_OTA_PROVISION_STATUS_PRL_DOWNLOADED;
            break;
        }
        case 7: {  // commit successfully
            state = CDMA_OTA_PROVISION_STATUS_COMMITTED;
            break;
        }
        case 10: {  // verify SPC failed
            state = CDMA_OTA_PROVISION_STATUS_SPC_RETRIES_EXCEEDED;
            break;
        }
        case 11: {  // A key Exchanged
            state = CDMA_OTA_PROVISION_STATUS_A_KEY_EXCHANGED;
            break;
        }
        case 12: {  // SSD updated
            state = CDMA_OTA_PROVISION_STATUS_SSD_UPDATED;
            break;
        }
        case 13: {  // OTAPA strated
            state = CDMA_OTA_PROVISION_STATUS_OTAPA_STARTED;
            break;
        }
        case 14: {  // OTAPA stopped
            state = CDMA_OTA_PROVISION_STATUS_OTAPA_STOPPED;
            break;
        }
        default:
            state = -1;
    }

    return state;
}

char *RpNwStateController::strdupReadString(Parcel *p) {
    size_t stringlen;
    const char16_t *s16;

    s16 = p->readString16Inplace(&stringlen);

    return strndup16to8(s16, stringlen);
}

void RpNwStateController::writeStringToParcel(Parcel *p, const char *s) {
    char16_t *s16;
    size_t s16_len;
    s16 = strdup8to16(s, &s16_len);
    p->writeString16(s16, s16_len);
    free(s16);
}

void RpNwStateController::resetVoiceRegStateCache(RIL_VOICE_REG_STATE_CACHE *voiceCache) {
    (*voiceCache).register_state = 0;
    (*voiceCache).lac = 0xffffffff;
    (*voiceCache).cid = 0x0fffffff;
    (*voiceCache).radio_technology = 0;
    (*voiceCache).base_station_id = 0;
    (*voiceCache).base_station_latitude = 0;
    (*voiceCache).base_station_longitude = 0;
    (*voiceCache).css = 0;
    (*voiceCache).system_id = 0;
    (*voiceCache).network_id = 0;
    (*voiceCache).roaming_ind = 1;  // home
    (*voiceCache).prl = 0;
    (*voiceCache).prl_roaming_ind = 0;
    (*voiceCache).deny_reason = 0;
    (*voiceCache).psc = -1;
    (*voiceCache).network_exsit = 0;
}

void RpNwStateController::resetDataRegStateCache(RIL_DATA_REG_STATE_CACHE *dataCache) {
    (*dataCache).register_state = 0;
    (*dataCache).lac = 0xffffffff;
    (*dataCache).cid = 0x0fffffff;
    (*dataCache).radio_technology = 0;
    (*dataCache).deny_reason = 0;
    (*dataCache).max_data_call = 1;
    (*dataCache).tac = 0;
    (*dataCache).physical_cid = 0;
    (*dataCache).eci = 0;
    (*dataCache).csgid = 0;
    (*dataCache).tadv = 0;
}

void RpNwStateController::resetSignalStrengthCache(RIL_SIGNAL_STRENGTH_CACHE *sigCache) {
    (*sigCache).gsm_sig1 = 99;
    (*sigCache).gsm_sig2 = -1;
    (*sigCache).cdma_dbm = -1;
    (*sigCache).cdma_ecio = -1;
    (*sigCache).ecdo_dbm = -1;
    (*sigCache).evdo_ecio = -1;
    (*sigCache).evdo_snr = -1;
    (*sigCache).lte_sig = -99;
    (*sigCache).lte_rsrp = 0x7FFFFFFF;
    (*sigCache).lte_rsrq = 0x7FFFFFFF;
    (*sigCache).lte_rssnr = 0x7FFFFFFF;
    (*sigCache).lte_cqi = 0x7FFFFFFF;
    (*sigCache).tdscdma_rscp = 0x7FFFFFFF;
    (*sigCache).rssi_qdbm = 0;
    (*sigCache).rscp_qdbm = 0;
    (*sigCache).ecn0_qdbm = 0;
}

void RpNwStateController::resetOperatorCache(RIL_OPERATOR_CACHE *optrCache) {
    (*optrCache).count = 0;
    memset((*optrCache).optrAlphaLong, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
    memset((*optrCache).optrAlphaShort, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
    memset((*optrCache).optrNumeric, '\0', MAX_OPER_NUM_LENGTH * sizeof(char));
}

void RpNwStateController::resetFemtoCellCache() {
    gsm_femto_cell_cache.domain = 0;
    gsm_femto_cell_cache.state = 0;
    gsm_femto_cell_cache.plmn_id = 0;
    gsm_femto_cell_cache.act = 0;
    gsm_femto_cell_cache.isFemtocell = 0;
    gsm_femto_cell_cache.csg_id = -1;
    gsm_femto_cell_cache.csg_icon_type = -1;
    gsm_femto_cell_cache.cause = -1;
    memset(gsm_femto_cell_cache.optrAlphaLong, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
    memset(gsm_femto_cell_cache.hnbName, '\0', MAX_OPER_NAME_LENGTH * sizeof(char));
}

const char *RpNwStateController::requestToString(int reqId) {
    switch (reqId) {
        case RIL_REQUEST_VOICE_REGISTRATION_STATE:
            return "REQ_VOICE_REG_STATE";
        case RIL_REQUEST_DATA_REGISTRATION_STATE:
            return "REQ_DATA_REG_STATE";
        case RIL_REQUEST_SIGNAL_STRENGTH:
            return "REQ_SIGNAL_STRENGTH";
        case RIL_REQUEST_OPERATOR:
            return "REQ_OPERATOR";
        case RIL_REQUEST_GET_CELL_INFO_LIST:
            return "REQ_GET_CELL_INFO_LIST";
        default:
            return "INVALID REQUEST";
    }
}

const char *RpNwStateController::urcToString(int urcId) {
    switch (urcId) {
        case RIL_UNSOL_SIGNAL_STRENGTH:
            return "UNSOL_SIGNAL_STRENGTH";
        case RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED:
            return "UNSOL_PS_NW_STATE_CHANGED";
        case RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED:
            return "UNSOL_CS_NW_STATE_CHANGED";
        case RIL_UNSOL_FEMTOCELL_INFO:
            return "RIL_UNSOL_FEMTOCELL_INFO";
        case RIL_UNSOL_CDMA_OTA_PROVISION_STATUS:
            return "RIL_UNSOL_CDMA_OTA_PROVISION_STATUS";
        case RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE:
            return "RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE";
        default:
            return "INVALID URC";
    }
}

const char *RpNwStateController::sourceToString(int srcId) {
    switch (srcId) {
        case RADIO_TECH_GROUP_GSM:
            return "GSM";
        case RADIO_TECH_GROUP_C2K:
            return "C2K";
        default:
            return "INVALID SRC";
    }
}

void RpNwStateController::printVoiceCache(RIL_VOICE_REG_STATE_CACHE cache) {
    if (cache.cacheType == RADIO_TECH_GROUP_GSM) {
        logD(RP_NW_STATE_TAG, " VoiceCache[GSM]: reg=%d lac=%x cid=%x rat=%d css=%d \
sid=%d nid=%d roam=%d css=%d",
                cache.register_state, cache.lac, cache.cid, cache.radio_technology,
                cache.css, cache.system_id, cache.network_id, cache.roaming_ind, cache.css);
    } else if (cache.cacheType == RADIO_TECH_GROUP_C2K) {
        logD(RP_NW_STATE_TAG, " VoiceCache[C2K]: reg=%d lac=%x cid=%x rat=%d css=%d \
sid = %d nid = %d roam = %d css = %d exsit = %d",
                cache.register_state, cache.lac, cache.cid, cache.radio_technology,
                cache.css, cache.system_id, cache.network_id, cache.roaming_ind, cache.css,
                cache.network_exsit);
    }
}

void RpNwStateController::printDataCache(RIL_DATA_REG_STATE_CACHE cache) {
    if (cache.cacheType == RADIO_TECH_GROUP_GSM) {
        logD(RP_NW_STATE_TAG, " DataCache[GSM]: reg=%d lac=%x cid=%x rat=%d den=%d mdc=%d",
                cache.register_state, cache.lac, cache.cid, cache.radio_technology,
                cache.deny_reason, cache.max_data_call);
    } else if (cache.cacheType == RADIO_TECH_GROUP_C2K) {
        logD(RP_NW_STATE_TAG, " DataCache[C2K]: reg=%d lac=%x cid=%x rat=%d den=%d mdc=%d",
                cache.register_state, cache.lac, cache.cid, cache.radio_technology,
                cache.deny_reason, cache.max_data_call);
    }
}

void RpNwStateController::printSignalStrengthCache(RIL_SIGNAL_STRENGTH_CACHE cache) {
    logD(RP_NW_STATE_TAG,
            " Sig: gsm[%d %d] c2k[%d %d %d %d %d] lte[%d %d %d %d %d] tds[%d] umts[%d %d %d]",
            cache.gsm_sig1, cache.gsm_sig2,
            cache.cdma_dbm, cache.cdma_ecio, cache.ecdo_dbm, cache.evdo_ecio, cache.evdo_snr,
            cache.lte_sig, cache.lte_rsrp, cache.lte_rsrq, cache.lte_rssnr, cache.lte_cqi,
            cache.tdscdma_rscp,
            cache.rssi_qdbm, cache.rscp_qdbm, cache.ecn0_qdbm);
}

void RpNwStateController::printOperatorCache(RIL_OPERATOR_CACHE cache) {
    logD(RP_NW_STATE_TAG, " OptrCache: %s,%s,%s",
            cache.optrAlphaLong, cache.optrAlphaShort, cache.optrNumeric);
}

sp<RfxMessage> RpNwStateController::onMalReceiveDataRegStateResponse(
        const sp<RfxMessage>& responseMd1) {
    sp<RfxMessage> rlt = responseMd1;
    if (rlt->getError() == RIL_E_SUCCESS) {
        int skip;
        int source = rlt->getSource();
        int32_t stgCount = -1;
        char **pStrings = NULL;
        Parcel *p = rlt->getParcel();

        p->readInt32(&stgCount);
        if (mDebug) {
            logD(RP_NW_STATE_TAG, " onMalReceiveDataRegStateResponse(): src=%s, stgCount=%d",
                    sourceToString(source), stgCount);
        }

        if (stgCount > 0) {
            pStrings = (char **) alloca(sizeof(char *) * stgCount);
            for (int i = 0; i < stgCount; i++) {
                pStrings[i] = strdupReadString(p);
            }
            if (source == RADIO_TECH_GROUP_GSM) {
                int gsm_register_state = strtol(pStrings[0], NULL, 10);
                if (!RfxNwServiceState::isInService(gsm_register_state)) {
                    if (ps_reg_state.wifi_state == 99) {
                        logD(RP_NW_STATE_TAG," onMalReceiveDataRegStateResponse():"
                                        "requestMalGprsRegistrationState: IWLAN is On");

                        char *stgBuf = (char *) malloc(50);
                        if (stgBuf == NULL) goto error;
                        int stgCount = 11;
                        int dataRegState = 1;
                        int dataRadioTechnology = 18;

                        rlt = RfxMessage::obtainResponse(RIL_E_SUCCESS, responseMd1);
                        Parcel *p = rlt->getParcel();
                        p->writeInt32(stgCount);

                        sprintf(stgBuf, "%d", dataRegState);
                        writeStringToParcel(p, stgBuf);

                        sprintf(stgBuf, "%d", -1);
                        writeStringToParcel(p, stgBuf);

                        sprintf(stgBuf, "%d", -1);
                        writeStringToParcel(p, stgBuf);

                        sprintf(stgBuf, "%d", dataRadioTechnology);
                        writeStringToParcel(p, stgBuf);

                        for (int i = 0; i < 7; i++) {
                            sprintf(stgBuf, "%s", "");
                            writeStringToParcel(p, stgBuf);
                        }

                        free(stgBuf);
                    }
                }
            }
        }
    }
    return rlt;
error:
    logE(RP_NW_STATE_TAG, "onMalReceiveDataRegStateResponse calloc fail");
    return rlt;
}
void RpNwStateController::updateMalCgregState(const sp<RfxMessage>& urc) {
    int skip;
    int source = urc->getSource();
    Parcel *p = urc->getParcel();
    if (source == RADIO_TECH_GROUP_GSM) {
        p->readInt32(&skip);
        if (urc->getId() == RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED) {
            p->readInt32(&ps_reg_state.gsm_data_register_state);
            p->readInt32(&ps_reg_state.gsm_mcc_mnc);
            p->readInt32(&ps_reg_state.gsm_data_rat);
        } else if (urc->getId() == RIL_LOCAL_GSM_UNSOL_MAL_PS_RGEGISTRATION_STATE) {
            p->readInt32(&ps_reg_state.wifi_state);
        }
    }
    logD(RP_NW_STATE_TAG, "updateMalCgregState, gsm data state is %d,"
            "wifi state is %d,mcc-mnc is %d,"
            "gsm data rat is %d. ", ps_reg_state.gsm_data_register_state,
            ps_reg_state.wifi_state, ps_reg_state.gsm_mcc_mnc, ps_reg_state.gsm_data_rat);
}

void RpNwStateController::combineMalCgregState(const sp<RfxMessage>& msg) {
    Parcel *p = msg->getParcel();
    p->writeInt32(3);
    if (!RfxNwServiceState::isInService(ps_reg_state.gsm_data_register_state)
            && ps_reg_state.wifi_state == 99) {
        p->writeInt32(1);
        p->writeInt32(ps_reg_state.gsm_mcc_mnc);
        p->writeInt32(18);
    } else {
        p->writeInt32(ps_reg_state.gsm_data_register_state);
        p->writeInt32(ps_reg_state.gsm_mcc_mnc);
        p->writeInt32(ps_reg_state.gsm_data_rat);
    }
}

PsRatFamily RpNwStateController::getPsRatFamily(int radioTechnology) {
    if (radioTechnology == RADIO_TECH_GPRS
            || radioTechnology == RADIO_TECH_EDGE
            || radioTechnology == RADIO_TECH_UMTS
            || radioTechnology == RADIO_TECH_HSDPA
            || radioTechnology == RADIO_TECH_HSUPA
            || radioTechnology == RADIO_TECH_HSPA
            || radioTechnology == RADIO_TECH_LTE
            || radioTechnology == RADIO_TECH_HSPAP
            || radioTechnology == RADIO_TECH_GSM
            || radioTechnology == RADIO_TECH_TD_SCDMA
            || radioTechnology == RADIO_TECH_LTE_CA) {
        return PS_RAT_FAMILY_GSM;
    } else if (radioTechnology == RADIO_TECH_IS95A
            || radioTechnology == RADIO_TECH_IS95B
            || radioTechnology == RADIO_TECH_1xRTT
            || radioTechnology == RADIO_TECH_EVDO_0
            || radioTechnology == RADIO_TECH_EVDO_A
            || radioTechnology == RADIO_TECH_EVDO_B
            || radioTechnology == RADIO_TECH_EHRPD) {
        return PS_RAT_FAMILY_CDMA;
    } else if (radioTechnology == RADIO_TECH_IWLAN) {
        return PS_RAT_FAMILY_IWLAN;
    } else {
        return PS_RAT_FAMILY_UNKNOWN;
    }
}
