/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
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

#ifdef MTK_USE_HIDL
    {RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED, radio::responsePsNetworkStateChangeInd, WAKE_PARTIAL},
/// M: eMBMS feature
    {RIL_UNSOL_EMBMS_SESSION_STATUS, radio::embmsSessionStatusInd, WAKE_PARTIAL},
    {RIL_UNSOL_EMBMS_AT_INFO, radio::embmsAtInfoInd, WAKE_PARTIAL},
/// M: eMBMS end
    {RIL_UNSOL_RESPONSE_CS_NETWORK_STATE_CHANGED, radio::responseCsNetworkStateChangeInd, WAKE_PARTIAL},
    {RIL_UNSOL_INVALID_SIM, radio::responseInvalidSimInd, WAKE_PARTIAL},
    {RIL_UNSOL_NETWORK_EVENT, radio::responseNetworkEventInd, WAKE_PARTIAL},
// MTK-START: SIM
    {RIL_UNSOL_VIRTUAL_SIM_ON,radio::onVirtualSimOn, WAKE_PARTIAL},
    {RIL_UNSOL_VIRTUAL_SIM_OFF,radio::onVirtualSimOff, WAKE_PARTIAL},
    {RIL_UNSOL_IMEI_LOCK,radio::onImeiLock, WAKE_PARTIAL},
    {RIL_UNSOL_IMSI_REFRESH_DONE, radio::onImsiRefreshDone, WAKE_PARTIAL},
// MTK-END
// ATCI
    {RIL_UNSOL_ATCI_RESPONSE, radio::atciInd, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_ETWS_NOTIFICATION, radio::newEtwsInd, WAKE_PARTIAL},
    {RIL_UNSOL_ME_SMS_STORAGE_FULL, radio::meSmsStorageFullInd, WAKE_PARTIAL},
    {RIL_UNSOL_SMS_READY_NOTIFICATION, radio::smsReadyInd, WAKE_PARTIAL},
    {RIL_UNSOL_DATA_ALLOWED, radio::dataAllowedNotificationInd, WAKE_PARTIAL},
    /// M: CC: Proprietary incoming call indication ([IMS] common flow)
    {RIL_UNSOL_INCOMING_CALL_INDICATION, radio::incomingCallIndicationInd, WAKE_PARTIAL},
    /// M: CC: Cipher indication support ([IMS] common flow)
    {RIL_UNSOL_CIPHER_INDICATION, radio::cipherIndicationInd, WAKE_PARTIAL},
    /// M: CC: CRSS notification indication
    {RIL_UNSOL_CRSS_NOTIFICATION, radio::crssNotifyInd, WAKE_PARTIAL},
    /// M: CC: For 3G VT only
    {RIL_UNSOL_VT_STATUS_INFO, radio::vtStatusInfoInd, WAKE_PARTIAL},
    /// M: CC: GSA HD Voice for 2/3G network support
    {RIL_UNSOL_SPEECH_CODEC_INFO, radio::speechCodecInfoInd, WAKE_PARTIAL},
    {RIL_UNSOL_PSEUDO_CELL_INFO, radio::onPseudoCellInfoInd, WAKE_PARTIAL},
    {RIL_UNSOL_RESET_ATTACH_APN, radio::resetAttachApnInd, WAKE_PARTIAL},
    {RIL_UNSOL_DATA_ATTACH_APN_CHANGED, radio::mdChangeApnInd, WAKE_PARTIAL},
// World Phone
    {RIL_UNSOL_WORLD_MODE_CHANGED, radio::worldModeChangedIndication, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_PLMN_CHANGED, radio::plmnChangedIndication, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_REGISTRATION_SUSPENDED, radio::registrationSuspendedIndication, WAKE_PARTIAL},
    {RIL_UNSOL_GMSS_RAT_CHANGED, radio::gmssRatChangedIndication, WAKE_PARTIAL},
    {RIL_UNSOL_CDMA_CARD_INITIAL_ESN_OR_MEID, radio::esnMeidChangeInd, WAKE_PARTIAL},
    {RIL_UNSOL_FEMTOCELL_INFO, radio::responseFemtocellInfo, WAKE_PARTIAL},
// / M: BIP {
    {RIL_UNSOL_STK_BIP_PROACTIVE_COMMAND, radio::bipProactiveCommandInd, WAKE_PARTIAL},
// / M: BIP }
// SS
    {RIL_UNSOL_CALL_FORWARDING, radio::cfuStatusNotifyInd, WAKE_PARTIAL},
// PHB
    {RIL_UNSOL_PHB_READY_NOTIFICATION, radio::phbReadyNotificationInd, WAKE_PARTIAL},
// / M: OTASP {
    {RIL_UNSOL_TRIGGER_OTASP, radio::triggerOtaSPInd, WAKE_PARTIAL},
// / M: OTASP }
    {RIL_UNSOL_MD_DATA_RETRY_COUNT_RESET, radio::onMdDataRetryCountReset, WAKE_PARTIAL},
    {RIL_UNSOL_REMOVE_RESTRICT_EUTRAN, radio::onRemoveRestrictEutran, WAKE_PARTIAL},
    {RIL_UNSOL_PCO_STATUS, radio::onPcoStatus, WAKE_PARTIAL},
// M: [LTE][Low Power][UL traffic shaping] @{
    {RIL_UNSOL_LTE_ACCESS_STRATUM_STATE_CHANGE, radio::onLteAccessStratumStateChanged, WAKE_PARTIAL},
// M: [LTE][Low Power][UL traffic shaping] @}
// MTK-START: SIM HOT SWAP
    {RIL_UNSOL_SIM_PLUG_IN, radio::onSimPlugIn, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_PLUG_OUT, radio::onSimPlugOut, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_MISSING, radio::onSimMissing, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_RECOVERY, radio::onSimRecovery, WAKE_PARTIAL},
// MTK-END
// MTK-START: SIM COMMON SLOT
    {RIL_UNSOL_TRAY_PLUG_IN, radio::onSimTrayPlugIn, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_COMMON_SLOT_NO_CHANGED, radio::onSimCommonSlotNoChanged, WAKE_PARTIAL},
// MTK-END
    {RIL_UNSOL_CALL_INFO_INDICATION, radio::callInfoIndicationInd, WAKE_PARTIAL},
    {RIL_UNSOL_ECONF_RESULT_INDICATION, radio::econfResultIndicationInd, WAKE_PARTIAL},
    {RIL_UNSOL_SIP_CALL_PROGRESS_INDICATOR, radio::sipCallProgressIndicatorInd, WAKE_PARTIAL},
    {RIL_UNSOL_CALLMOD_CHANGE_INDICATOR, radio::callmodChangeIndicatorInd, WAKE_PARTIAL},
    {RIL_UNSOL_VIDEO_CAPABILITY_INDICATOR, radio::videoCapabilityIndicatorInd, WAKE_PARTIAL},
    {RIL_UNSOL_ON_USSI, radio::onUssiInd, WAKE_PARTIAL},
    {RIL_UNSOL_GET_PROVISION_DONE, radio::getProvisionDoneInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_RTP_INFO, radio::imsRtpInfoInd, WAKE_PARTIAL},
    {RIL_UNSOL_ON_XUI, radio::onXuiInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_EVENT_PACKAGE_INDICATION, radio::imsEventPackageIndicationInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_REGISTRATION_INFO, radio::imsRegistrationInfoInd, WAKE_PARTIAL},
    {RIL_UNSOL_ECONF_SRVCC_INDICATION, radio::confSRVCCInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_ENABLE_DONE, radio::imsEnableDoneInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DISABLE_DONE, radio::imsDisableDoneInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_ENABLE_START, radio::imsEnableStartInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DISABLE_START, radio::imsDisableStartInd, WAKE_PARTIAL},
    {RIL_UNSOL_ECT_INDICATION, radio::ectIndicationInd, WAKE_PARTIAL},
    {RIL_UNSOL_VOLTE_SETTING, radio::volteSettingInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_ACTIVATION, radio::imsBearerActivationInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_DEACTIVATION, radio::imsBearerDeactivationInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_INIT, radio::imsBearerInitInd, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DEREG_DONE, radio::imsDeregDoneInd, WAKE_PARTIAL},
    // M: [VzW] Data Framework
    {RIL_UNSOL_PCO_DATA_AFTER_ATTACHED, radio::pcoDataAfterAttachedInd, WAKE_PARTIAL},
    {RIL_UNSOL_NETWORK_INFO, radio::networkInfoInd, WAKE_PARTIAL},
/// M: CC: CDMA call accepted indication
    {RIL_UNSOL_CDMA_CALL_ACCEPTED, radio::cdmaCallAcceptedInd, WAKE_PARTIAL},
// / M:STK {
    {RIL_UNSOL_STK_SETUP_MENU_RESET, radio::onStkMenuResetInd,  WAKE_PARTIAL},
// / M:STK }

    // External SIM [Start]
    {RIL_UNSOL_VSIM_OPERATION_INDICATION, radio::onVsimEventIndication, WAKE_PARTIAL},
    // External SIM [End]

// M: [VzW] Data Framework
    {RIL_UNSOL_VOLTE_LTE_CONNECTION_STATUS, radio::volteLteConnectionStatusInd, WAKE_PARTIAL},
/// Ims Data Framework
    {RIL_UNSOL_DEDICATE_BEARER_ACTIVATED, radio::dedicatedBearerActivationInd, WAKE_PARTIAL},
    {RIL_UNSOL_DEDICATE_BEARER_MODIFIED, radio::dedicatedBearerModificationInd, WAKE_PARTIAL},
    {RIL_UNSOL_DEDICATE_BEARER_DEACTIVATED, radio::dedicatedBearerDeactivationInd, WAKE_PARTIAL},
/// @}
    {RIL_UNSOL_IMS_MULTIIMS_COUNT, radio::multiImsCountInd, WAKE_PARTIAL},
///M: MwiService @{
    {RIL_UNSOL_MOBILE_WIFI_ROVEOUT, radio::onWifiRoveout, WAKE_PARTIAL},
    {RIL_UNSOL_MOBILE_WIFI_HANDOVER, radio::onPdnHandover, WAKE_PARTIAL},
    {RIL_UNSOL_ACTIVE_WIFI_PDN_COUNT, radio::onWifiPdnActivate, WAKE_PARTIAL},
    {RIL_UNSOL_WIFI_RSSI_MONITORING_CONFIG, radio::onWifiMonitoringThreshouldChanged, WAKE_PARTIAL},
    {RIL_UNSOL_WIFI_PDN_ERROR, radio::onWfcPdnError, WAKE_PARTIAL},
    {RIL_UNSOL_REQUEST_GEO_LOCATION, radio::onLocationRequest, WAKE_PARTIAL},
    {RIL_UNSOL_WFC_PDN_STATE, radio::onWfcPdnStateChanged, WAKE_PARTIAL},
    {RIL_UNSOL_NATT_KEEP_ALIVE_CHANGED, radio::onNattKeepAliveChanged, WAKE_PARTIAL},
///@}
    {RIL_UNSOL_IMS_SUPPORT_ECC, radio::imsSupportEccInd, WAKE_PARTIAL},
    {RIL_UNSOL_TX_POWER, radio::onTxPowerIndication, WAKE_PARTIAL},
#else
    {RIL_UNSOL_RESPONSE_PLMN_CHANGED, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_REGISTRATION_SUSPENDED, responseInts, WAKE_PARTIAL},
/// M: [C2K 6M][NW] add for Iwlan @{
    {RIL_UNSOL_RESPONSE_PS_NETWORK_STATE_CHANGED, responseInts, WAKE_PARTIAL},
/// M: [C2K 6M][NW] add for Iwlan @{
    {RIL_UNSOL_GMSS_RAT_CHANGED, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_CDMA_PLMN_CHANGED, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_CS_NETWORK_STATE_CHANGED, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_INVALID_SIM, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_NETWORK_EVENT, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_MODULATION_INFO, responseInts, WAKE_PARTIAL},
// MTK-START: SIM
    {RIL_UNSOL_VIRTUAL_SIM_ON,responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_VIRTUAL_SIM_OFF,responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMEI_LOCK, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_IMSI_REFRESH_DONE, responseVoid, WAKE_PARTIAL},
// MTK-END
/// M: eMBMS feature
// only used in ril-proxy, and which use rilproxy\libril\ril.cpp use the same name file under libril
    {RIL_UNSOL_EMBMS_SESSION_STATUS, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_EMBMS_AT_INFO, responseString, WAKE_PARTIAL},
/// M: eMBMS end

// ATCI
    {RIL_UNSOL_ATCI_RESPONSE, responseRaw, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_ETWS_NOTIFICATION, responseEtwsNotification, WAKE_PARTIAL},
    {RIL_UNSOL_ME_SMS_STORAGE_FULL, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_SMS_READY_NOTIFICATION, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_DATA_ALLOWED, responseInts, WAKE_PARTIAL},
/// M: CC: Proprietary incoming call indication
    {RIL_UNSOL_INCOMING_CALL_INDICATION, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_PSEUDO_CELL_INFO, responseInts, WAKE_PARTIAL},
//Reset Attach APN
    {RIL_UNSOL_RESET_ATTACH_APN, responseVoid, WAKE_PARTIAL},
// M: IA-change attach APN
    {RIL_UNSOL_DATA_ATTACH_APN_CHANGED, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_RESPONSE_ETWS_NOTIFICATION, responseEtwsNotification, WAKE_PARTIAL},
/// M: CC: Cipher indication support
    {RIL_UNSOL_CIPHER_INDICATION, responseStrings, WAKE_PARTIAL},
/// M: CC: Call control CRSS handling
    {RIL_UNSOL_CRSS_NOTIFICATION, responseCrssN, WAKE_PARTIAL},
    /// M: CC: For 3G VT only @{
    {RIL_UNSOL_VT_STATUS_INFO, responseInts, WAKE_PARTIAL},
    /// M: CC: GSA HD Voice for 2/3G network support
    {RIL_UNSOL_SPEECH_CODEC_INFO, responseInts, WAKE_PARTIAL},
// World Phone
    {RIL_UNSOL_WORLD_MODE_CHANGED, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_CDMA_CARD_INITIAL_ESN_OR_MEID, responseString, WAKE_PARTIAL},
    {RIL_UNSOL_FEMTOCELL_INFO, responseStrings, WAKE_PARTIAL},
// / M: BIP {
    {RIL_UNSOL_STK_BIP_PROACTIVE_COMMAND, responseString, WAKE_PARTIAL},
// / M: BIP }
// SS
    {RIL_UNSOL_CALL_FORWARDING, responseInts, WAKE_PARTIAL},
// PHB
    {RIL_UNSOL_PHB_READY_NOTIFICATION, responseInts, WAKE_PARTIAL},
// / M: OTASP {
    {RIL_UNSOL_TRIGGER_OTASP, responseVoid, WAKE_PARTIAL},
// / M: OTASP }
    {RIL_UNSOL_MD_DATA_RETRY_COUNT_RESET, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_REMOVE_RESTRICT_EUTRAN, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_PCO_STATUS, responseInts, WAKE_PARTIAL},
// M: [LTE][Low Power][UL traffic shaping] @{
    {RIL_UNSOL_LTE_ACCESS_STRATUM_STATE_CHANGE, responseInts, WAKE_PARTIAL},
// M: [LTE][Low Power][UL traffic shaping] @}
// MTK-START: SIM HOT SWAP
    {RIL_UNSOL_SIM_PLUG_IN, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_PLUG_OUT, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_MISSING, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_RECOVERY, responseInts, WAKE_PARTIAL},
// MTK-END
// MTK-START: SIM COMMON SLOT
    {RIL_UNSOL_TRAY_PLUG_IN, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_SIM_COMMON_SLOT_NO_CHANGED, responseVoid, WAKE_PARTIAL},
// MTK-END
    /// [IMS] IMS Indication ================================================================
    {RIL_UNSOL_CALL_INFO_INDICATION, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_ECONF_RESULT_INDICATION, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_SIP_CALL_PROGRESS_INDICATOR, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_CALLMOD_CHANGE_INDICATOR, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_VIDEO_CAPABILITY_INDICATOR, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_ON_USSI, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_GET_PROVISION_DONE, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_RTP_INFO, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_ON_XUI, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_EVENT_PACKAGE_INDICATION, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_ECONF_SRVCC_INDICATION, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_REGISTRATION_INFO, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_ENABLE_DONE, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DISABLE_DONE, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_ENABLE_START, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DISABLE_START, responseVoid, WAKE_PARTIAL},
    {RIL_UNSOL_ECT_INDICATION, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_VOLTE_SETTING, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_ACTIVATION, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_DEACTIVATION, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_BEARER_INIT, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_DEREG_DONE, responseVoid, WAKE_PARTIAL},
    /// [IMS] IMS Indication ================================================================
    // M: [VzW] Data Framework
    {RIL_UNSOL_PCO_DATA_AFTER_ATTACHED, responsePcoDataAfterAttached, WAKE_PARTIAL},
    {RIL_UNSOL_NETWORK_INFO, responseStrings, WAKE_PARTIAL},
// MTK-START: SIM TMO RSU
    {RIL_UNSOL_MELOCK_NOTIFICATION,responseInts, WAKE_PARTIAL},
// MTK-END
/// M: CC: CDMA call accepted indication
    {RIL_UNSOL_CDMA_CALL_ACCEPTED, responseVoid, WAKE_PARTIAL},
// /M: STK{
    {RIL_UNSOL_STK_SETUP_MENU_RESET, responseVoid, WAKE_PARTIAL},
// /M: STK}

    // External SIM [Start]
    {RIL_UNSOL_VSIM_OPERATION_INDICATION, responseVsimOperationEvent, WAKE_PARTIAL},
    // External SIM [End]

// M: [VzW] Data Framework
    {RIL_UNSOL_VOLTE_LTE_CONNECTION_STATUS, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_IMS_MULTIIMS_COUNT, responseInts, WAKE_PARTIAL},

///M: MwiService @{
    {RIL_UNSOL_MOBILE_WIFI_ROVEOUT, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_MOBILE_WIFI_HANDOVER, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_ACTIVE_WIFI_PDN_COUNT, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_WIFI_RSSI_MONITORING_CONFIG, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_WIFI_PDN_ERROR, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_REQUEST_GEO_LOCATION, responseStrings, WAKE_PARTIAL},
    {RIL_UNSOL_WFC_PDN_STATE, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_NATT_KEEP_ALIVE_CHANGED, responseStrings, WAKE_PARTIAL},
///@}
    {RIL_UNSOL_IMS_SUPPORT_ECC, responseInts, WAKE_PARTIAL},
    {RIL_UNSOL_TX_POWER, responseInts, WAKE_PARTIAL},
    #endif // MTK_USE_HIDL
