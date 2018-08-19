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
 // MTK-START: SIM
 {RIL_REQUEST_SIM_GET_ATR, radio::getATRResponse},
 {RIL_REQUEST_SET_SIM_POWER, radio::setSimPowerResponse},
 // MTK-END
 // modem power
 {RIL_REQUEST_MODEM_POWERON, radio::setModemPowerResponse},
 {RIL_REQUEST_MODEM_POWEROFF, radio::setModemPowerResponse},
 // MTK-START: NW
 {RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL_WITH_ACT, radio::setNetworkSelectionModeManualWithActResponse},
 {RIL_REQUEST_QUERY_AVAILABLE_NETWORKS_WITH_ACT, radio::getAvailableNetworksWithActResponse},
 {RIL_REQUEST_ABORT_QUERY_AVAILABLE_NETWORKS, radio::cancelAvailableNetworksResponse},
 // ATCI
 {RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL, radio::sendAtciResponse},
 // M: To set language configuration for GSM cell broadcast
 {RIL_REQUEST_GSM_SET_BROADCAST_LANGUAGE, radio::setGsmBroadcastLangsResponse},
 // M: To get language configuration for GSM cell broadcast
 {RIL_REQUEST_GSM_GET_BROADCAST_LANGUAGE, radio::getGsmBroadcastLangsResponse},
 {RIL_REQUEST_GET_SMS_SIM_MEM_STATUS, radio::getSmsMemStatusResponse},
 {RIL_REQUEST_GET_SMS_PARAMS, radio::getSmsParametersResponse},
 {RIL_REQUEST_SET_SMS_PARAMS, radio::setSmsParametersResponse},
 {RIL_REQUEST_SET_ETWS, radio::setEtwsResponse},
 {RIL_REQUEST_REMOVE_CB_MESSAGE, radio::removeCbMsgResponse},
 /// M: CC: Proprietary incoming call indication
 {RIL_REQUEST_SET_CALL_INDICATION, radio::setCallIndicationResponse},
 /// M: CC: Proprietary ECC handling @{
 /// M: eMBMS feature
 {RIL_REQUEST_EMBMS_AT_CMD, radio::sendEmbmsAtCommandResponse},
 /// M: eMBMS end
 {RIL_REQUEST_EMERGENCY_DIAL, radio::emergencyDialResponse},                                     /// M: CC: Proprietary ECC handling
 {RIL_REQUEST_SET_ECC_SERVICE_CATEGORY, radio::setEccServiceCategoryResponse},                   /// M: CC: Proprietary ECC handling ([IMS] common flow)
 {RIL_REQUEST_SET_ECC_LIST, radio::setEccListResponse},
 /// @}
 /// M: CC: Proprietary call control hangup all
 {RIL_REQUEST_HANGUP_ALL, radio::hangupAllResponse},                                             /// M: CC: Proprietary call control hangup all ([IMS] common flow)
 /// M: CC: call control 3GVT @{
 {RIL_REQUEST_VT_DIAL, radio::vtDialResponse},
 {RIL_REQUEST_VOICE_ACCEPT, radio::voiceAcceptResponse},
 {RIL_REQUEST_REPLACE_VT_CALL, radio::replaceVtCallResponse},
 /// @}
 /// M: CC: Verizon E911
 {RIL_REQUEST_CURRENT_STATUS, radio::currentStatusResponse},
 {RIL_REQUEST_ECC_PREFERRED_RAT, radio::eccPreferredRatResponse},                                /// M: CC: CtVolte/Vzw E911
 {RIL_REQUEST_SET_PSEUDO_CELL_MODE, radio::setApcModeResponse},
 {RIL_REQUEST_GET_PSEUDO_CELL_INFO, radio::getApcInfoResponse},
 {RIL_REQUEST_SWITCH_MODE_FOR_ECC, radio::triggerModeSwitchByEccResponse},
 // MTK-START: SIM GBA
 {RIL_REQUEST_GENERAL_SIM_AUTH, radio::iccIOForAppResponse},
 // MTK-END
 {RIL_REQUEST_GET_SMS_RUIM_MEM_STATUS, radio::getSmsRuimMemoryStatusResponse},
 // FastDormancy
 {RIL_REQUEST_SET_FD_MODE, radio::setFdModeResponse},
 {RIL_REQUEST_RELOAD_MODEM_TYPE, radio::reloadModemTypeResponse},                                // World Phone
 {RIL_REQUEST_STORE_MODEM_TYPE, radio::storeModemTypeResponse},                                  // World Phone
 {RIL_REQUEST_RESUME_REGISTRATION, radio::setResumeRegistrationResponse},
 {RIL_REQUEST_SET_TRM, radio::setTrmResponse},
 //STK
 {RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM_WITH_RESULT_CODE, radio::handleStkCallSetupRequestFromSimWithResCodeResponse},
 //SS
 {RIL_REQUEST_SET_CLIP, radio::setClipResponse},
 {RIL_REQUEST_GET_COLP, radio::getColpResponse},
 {RIL_REQUEST_GET_COLR, radio::getColrResponse},
 {RIL_REQUEST_SEND_CNAP, radio::sendCnapResponse},
 {RIL_REQUEST_SET_COLP, radio::setColpResponse},
 {RIL_REQUEST_SET_COLR, radio::setColrResponse},
 {RIL_REQUEST_QUERY_CALL_FORWARD_IN_TIME_SLOT, radio::queryCallForwardInTimeSlotStatusResponse},
 {RIL_REQUEST_SET_CALL_FORWARD_IN_TIME_SLOT, radio::setCallForwardInTimeSlotResponse},
 {RIL_REQUEST_RUN_GBA, radio::runGbaAuthenticationResponse},
 // PHB START
 {RIL_REQUEST_QUERY_PHB_STORAGE_INFO, radio::queryPhbStorageInfoResponse},
 {RIL_REQUEST_WRITE_PHB_ENTRY, radio::writePhbEntryResponse},
 {RIL_REQUEST_READ_PHB_ENTRY, radio::readPhbEntryResponse},
 {RIL_REQUEST_QUERY_UPB_CAPABILITY, radio::queryUPBCapabilityResponse},
 {RIL_REQUEST_EDIT_UPB_ENTRY, radio::editUPBEntryResponse},
 {RIL_REQUEST_DELETE_UPB_ENTRY, radio::deleteUPBEntryResponse},
 {RIL_REQUEST_READ_UPB_GAS_LIST, radio::readUPBGasListResponse},
 {RIL_REQUEST_READ_UPB_GRP, radio::readUPBGrpEntryResponse},
 {RIL_REQUEST_WRITE_UPB_GRP, radio::writeUPBGrpEntryResponse},
 {RIL_REQUEST_GET_PHB_STRING_LENGTH, radio::getPhoneBookStringsLengthResponse},
 {RIL_REQUEST_GET_PHB_MEM_STORAGE, radio::getPhoneBookMemStorageResponse},
 {RIL_REQUEST_SET_PHB_MEM_STORAGE, radio::setPhoneBookMemStorageResponse},
 {RIL_REQUEST_READ_PHB_ENTRY_EXT, radio::readPhoneBookEntryExtResponse},
 {RIL_REQUEST_WRITE_PHB_ENTRY_EXT, radio::writePhoneBookEntryExtResponse},
 {RIL_REQUEST_QUERY_UPB_AVAILABLE, radio::queryUPBAvailableResponse},
 {RIL_REQUEST_READ_EMAIL_ENTRY, radio::readUPBEmailEntryResponse},
 {RIL_REQUEST_READ_SNE_ENTRY, radio::readUPBSneEntryResponse},
 {RIL_REQUEST_READ_ANR_ENTRY, radio::readUPBAnrEntryResponse},
 {RIL_REQUEST_READ_UPB_AAS_LIST, radio::readUPBAasListResponse},
 // PHB END
 //Femtocell (CSG) feature
 {RIL_REQUEST_GET_FEMTOCELL_LIST, radio::getFemtocellListResponse},
 // Femtocell (CSG) : abort command shall be sent in differenent channel
 {RIL_REQUEST_ABORT_FEMTOCELL_LIST, radio::abortFemtocellListResponse},
 {RIL_REQUEST_SELECT_FEMTOCELL, radio::selectFemtocellResponse},
 {RIL_REQUEST_QUERY_FEMTOCELL_SYSTEM_SELECTION_MODE, radio::queryFemtoCellSystemSelectionModeResponse},
 {RIL_REQUEST_SET_FEMTOCELL_SYSTEM_SELECTION_MODE, radio::setFemtoCellSystemSelectionModeResponse},
 // Data
 {RIL_REQUEST_SYNC_APN_TABLE, radio::setupDataCallResponse},
 // M: Data Framework - common part enhancement
 {RIL_REQUEST_SYNC_DATA_SETTINGS_TO_MD, radio::syncDataSettingsToMdResponse},
 // M: Data Framework - Data Retry enhancement
 {RIL_REQUEST_RESET_MD_DATA_RETRY_COUNT, radio::resetMdDataRetryCountResponse},
 // M: Data Framework - CC 33
 {RIL_REQUEST_SET_REMOVE_RESTRICT_EUTRAN_MODE, radio::setRemoveRestrictEutranModeResponse},
 // M: [LTE][Low Power][UL traffic shaping] @{
 {RIL_REQUEST_SET_LTE_ACCESS_STRATUM_REPORT, radio::setLteAccessStratumReportResponse},
 {RIL_REQUEST_SET_LTE_UPLINK_DATA_TRANSFER, radio::setLteUplinkDataTransferResponse},
 // M: [LTE][Low Power][UL traffic shaping] @}
 // MTK-START: SIM ME LOCK
 {RIL_REQUEST_QUERY_SIM_NETWORK_LOCK, radio::queryNetworkLockResponse},
 {RIL_REQUEST_SET_SIM_NETWORK_LOCK, radio::setNetworkLockResponse},
 // MTK-END
 // [IMS] Enable/Disable IMS
 {RIL_REQUEST_SET_IMS_ENABLE, radio::setImsEnableResponse},
 // [IMS] Enable/Disable VoLTE
 {RIL_REQUEST_SET_VOLTE_ENABLE, radio::setVolteEnableResponse},
 // [IMS] Enable/Disable WFC
 {RIL_REQUEST_SET_WFC_ENABLE, radio::setWfcEnableResponse},
 // [IMS] Enable/Disable ViLTE
 {RIL_REQUEST_SET_VILTE_ENABLE, radio::setVilteEnableResponse},
 // [IMS] Enable/Disable ViWifi
 {RIL_REQUEST_SET_VIWIFI_ENABLE, radio::setViWifiEnableResponse},
 // [IMS] Enable/Disable IMS Voice
 {RIL_REQUEST_SET_IMS_VOICE_ENABLE, radio::setImsVoiceEnableResponse},
 // [IMS] Enable/Disable IMS Video
 {RIL_REQUEST_SET_IMS_VIDEO_ENABLE, radio::setImsVideoEnableResponse},
 // [IMS] Accept Video Call
 {RIL_REQUEST_VIDEO_CALL_ACCEPT, radio::videoCallAcceptResponse},
 // [IMS] Enable/Disable IMS Features
 {RIL_REQUEST_SET_IMSCFG, radio::setImscfgResponse},
 // [IMS] Set IMS configuration to modem
 {RIL_REQUEST_SET_MD_IMSCFG, radio::setModemImsCfgResponse},
 // [IMS] Get Provision Value
 {RIL_REQUEST_GET_PROVISION_VALUE, radio::getProvisionValueResponse},
 // [IMS] Set Provision Value
 {RIL_REQUEST_SET_PROVISION_VALUE, radio::setProvisionValueResponse},
 // [IMS] Set Provision Value
 {RIL_REQUEST_IMS_BEARER_ACTIVATION_DONE, radio::imsBearerActivationDoneResponse},
 // [IMS] IMS Bearer Activiation Done
 {RIL_REQUEST_IMS_BEARER_DEACTIVATION_DONE, radio::imsBearerDeactivationDoneResponse},
 // [IMS] IMS Bearer Deactiviation Done
 {RIL_REQUEST_IMS_DEREG_NOTIFICATION, radio::imsDeregNotificationResponse},
 // [IMS] IMS Deregister Notification
 {RIL_REQUEST_IMS_ECT, radio::imsEctCommandResponse},
 // [IMS] Hold Call
 {RIL_REQUEST_HOLD_CALL, radio::holdCallResponse},
 // [IMS] Resume Call
 {RIL_REQUEST_RESUME_CALL, radio::resumeCallResponse},
 // [IMS] Dial with SIP URI
 {RIL_REQUEST_DIAL_WITH_SIP_URI, radio::dialWithSipUriResponse},
 // [IMS] Force Release Call
 {RIL_REQUEST_FORCE_RELEASE_CALL, radio::forceReleaseCallResponse},
 // [IMS] SET IMS RTP Report
 {RIL_REQUEST_SET_IMS_RTP_REPORT, radio::setImsRtpReportResponse},
 // [IMS] Conference Dial
 {RIL_REQUEST_CONFERENCE_DIAL, radio::conferenceDialResponse},
 // [IMS] Add Member to Conference
 {RIL_REQUEST_ADD_IMS_CONFERENCE_CALL_MEMBER, radio::addImsConferenceCallMemberResponse},
 // [IMS] Remove Member to Conference
 {RIL_REQUEST_REMOVE_IMS_CONFERENCE_CALL_MEMBER, radio::removeImsConferenceCallMemberResponse},
 // [IMS] Dial a VT Call with SIP URI
 {RIL_REQUEST_VT_DIAL_WITH_SIP_URI, radio::vtDialWithSipUriResponse},
 // [IMS] Send USSI
 {RIL_REQUEST_SEND_USSI, radio::sendUssiResponse},
 // [IMS] Cancel USSI
 {RIL_REQUEST_CANCEL_USSI, radio::cancelUssiResponse},
 // [IMS] Set WFC Profile
 {RIL_REQUEST_SET_WFC_PROFILE, radio::setWfcProfileResponse},
 // [IMS] Pull CALL
 {RIL_REQUEST_PULL_CALL, radio::pullCallResponse},
 // [IMS] Set IMS Registrtion Report Enable
 {RIL_REQUEST_SET_IMS_REGISTRATION_REPORT, radio::setImsRegistrationReportResponse},
 // [IMS] Dial
 {RIL_REQUEST_IMS_DIAL, radio::imsDialResponse},
 // [IMS] VT Dial
 {RIL_REQUEST_IMS_VT_DIAL, radio::imsVtDialResponse},
 // [IMS] Emergency Dial
 {RIL_REQUEST_IMS_EMERGENCY_DIAL, radio::imsEmergencyDialResponse},
 {RIL_REQUEST_GET_POL_CAPABILITY, radio::getPOLCapabilityResponse},
 {RIL_REQUEST_GET_POL_LIST, radio::getCurrentPOLListResponse},
 {RIL_REQUEST_SET_POL_ENTRY, radio::setPOLEntryResponse},
 /// M: [Network][C2K] Sprint roaming control @{
 {RIL_REQUEST_SET_ROAMING_ENABLE, radio::setRoamingEnableResponse},
 {RIL_REQUEST_GET_ROAMING_ENABLE, radio::getRoamingEnableResponse},
 /// @}
 // External SIM [START]
 {RIL_REQUEST_VSIM_NOTIFICATION, radio::vsimNotificationResponse},
 {RIL_REQUEST_VSIM_OPERATION, radio::vsimOperationResponse},
 // External SIM [END]
 {RIL_REQUEST_GET_GSM_SMS_BROADCAST_ACTIVATION, radio::getGsmBroadcastActivationRsp},
 {RIL_REQUEST_SET_VOICE_DOMAIN_PREFERENCE, radio::setVoiceDomainPreferenceResponse},
 {RIL_REQUEST_SET_E911_STATE, radio::setE911StateResponse},
 /// M: MwiService @{
 {RIL_REQUEST_SET_WIFI_ENABLED, radio::setWifiEnabledResponse},
 {RIL_REQUEST_SET_WIFI_ASSOCIATED, radio::setWifiAssociatedResponse},
 {RIL_REQUEST_SET_WIFI_SIGNAL_LEVEL, radio::setWifiSignalLevelResponse},
 {RIL_REQUEST_SET_WIFI_IP_ADDRESS, radio::setWifiIpAddressResponse},
 {RIL_REQUEST_SET_GEO_LOCATION, radio::setLocationUpdatesResponse},
 {RIL_REQUEST_SET_EMERGENCY_ADDRESS_ID, radio::setEmergencyAddressIdResponse},
 {RIL_REQUEST_SET_SERVICE_STATE, radio::setServiceStateToModemResponse},
 {RIL_REQUEST_SET_NATT_KEEP_ALIVE_STATUS, radio::setNattKeepAliveStatusResponse},
 ///@}
#else

#ifdef C2K_RIL
  // Define GSM RILD channel id to avoid build error
  #define RIL_CMD_PROXY_1 RIL_CHANNEL_1
  #define RIL_CMD_PROXY_2 RIL_CHANNEL_1
  #define RIL_CMD_PROXY_3 RIL_CHANNEL_1
  #define RIL_CMD_PROXY_4 RIL_CHANNEL_1
  #define RIL_CMD_PROXY_5 RIL_CHANNEL_1
  #define RIL_CMD_PROXY_6 RIL_CHANNEL_1
#else
  // Define C2K RILD channel id to avoid build error
  #define RIL_CHANNEL_1 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_2 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_3 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_4 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_5 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_6 RIL_CMD_PROXY_1
  #define RIL_CHANNEL_7 RIL_CMD_PROXY_1
#endif // C2K_RIL

// External SIM [Start]
#ifdef MTK_MUX_CHANNEL_64
    #define MTK_RIL_CHANNEL_VSIM RIL_CMD_PROXY_11
#else
    #define MTK_RIL_CHANNEL_VSIM RIL_CMD_PROXY_2
#endif
// External SIM [End]

{RIL_REQUEST_RESUME_REGISTRATION, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_3},  // [C2K][IR] Support SVLTE IR feature

/// M: eMBMS feature
{RIL_REQUEST_EMBMS_AT_CMD, dispatchString, responseString, RIL_CMD_PROXY_6, RIL_CHANNEL_1},
/// M: eMBMS end
// MTK-START: SIM
{RIL_REQUEST_SIM_GET_ATR, dispatchVoid, responseString, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_SIM_POWER, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
// MTK-END
// MTK-START: SIM GBA
{RIL_REQUEST_GENERAL_SIM_AUTH, dispatchSimAuth, responseSIM_IO, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
// MTK-END

// modem power
{RIL_REQUEST_MODEM_POWERON, dispatchVoid, responseVoid,RIL_CMD_PROXY_1, RIL_CHANNEL_3},
{RIL_REQUEST_MODEM_POWEROFF, dispatchVoid, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_3},

// MTK-START: NW
{RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL_WITH_ACT, dispatchStrings, responseVoid,RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_QUERY_AVAILABLE_NETWORKS_WITH_ACT, dispatchVoid, responseStrings, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_ABORT_QUERY_AVAILABLE_NETWORKS, dispatchVoid, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},//for PLMN List abort

// ATCI
{RIL_REQUEST_OEM_HOOK_ATCI_INTERNAL, dispatchRaw, responseRaw, RIL_CMD_PROXY_6, RIL_CHANNEL_6},
// M: To set language configuration for GSM cell broadcast
{RIL_REQUEST_GSM_SET_BROADCAST_LANGUAGE, dispatchString, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_5},
// M: To get language configuration for GSM cell broadcast
{RIL_REQUEST_GSM_GET_BROADCAST_LANGUAGE, dispatchVoid, responseString, RIL_CMD_PROXY_1, RIL_CHANNEL_5},
{RIL_REQUEST_GET_SMS_SIM_MEM_STATUS, dispatchVoid, responseGetSmsSimMemStatusCnf, RIL_CMD_PROXY_1, RIL_CHANNEL_5},
{RIL_REQUEST_GET_SMS_PARAMS, dispatchVoid, responseSmsParams, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_SMS_PARAMS, dispatchSmsParams, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_ETWS, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_REMOVE_CB_MESSAGE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
/// M: CC: Proprietary incoming call indication
{RIL_REQUEST_SET_CALL_INDICATION, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
/// M: CC: Proprietary ECC handling @{
{RIL_REQUEST_EMERGENCY_DIAL, dispatchDial, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_2},
{RIL_REQUEST_SET_ECC_SERVICE_CATEGORY, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_ECC_LIST, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
/// @}
/// M: CC: Proprietary call control hangup all
{RIL_REQUEST_HANGUP_ALL, dispatchVoid, responseVoid, RIL_CMD_PROXY_4, RIL_CHANNEL_2},
/// M: CC: Call control 3GVT @{
{RIL_REQUEST_VT_DIAL, dispatchDial, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_VOICE_ACCEPT, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_REPLACE_VT_CALL, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
/// @}
// Verizon E911
{RIL_REQUEST_CURRENT_STATUS, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_3},
{RIL_REQUEST_ECC_PREFERRED_RAT, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_2},
{RIL_REQUEST_SET_PS_REGISTRATION, dispatchInts, responseVoid, RIL_CMD_PROXY_5, RIL_CHANNEL_1},
{RIL_REQUEST_SET_PSEUDO_CELL_MODE, dispatchInts, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_GET_PSEUDO_CELL_INFO, dispatchVoid, responseInts, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_SWITCH_MODE_FOR_ECC, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_GET_SMS_RUIM_MEM_STATUS, dispatchVoid, responseGetSmsSimMemStatusCnf, RIL_CMD_PROXY_1, RIL_CHANNEL_5},
// FastDormancy
{RIL_REQUEST_SET_FD_MODE, dispatchFdMode, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_RELOAD_MODEM_TYPE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_STORE_MODEM_TYPE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_TRM, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_3},
//STK
{RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM_WITH_RESULT_CODE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_3},
//SS
{RIL_REQUEST_SET_CLIP, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_GET_COLP, dispatchVoid, responseInts, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_GET_COLR, dispatchVoid, responseInts, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SEND_CNAP, dispatchString, responseInts, RIL_CMD_PROXY_2, RIL_CHANNEL_1},

{RIL_REQUEST_SET_COLP, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_COLR, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_QUERY_CALL_FORWARD_IN_TIME_SLOT, dispatchCallForwardEx, responseCallForwardsEx, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_CALL_FORWARD_IN_TIME_SLOT, dispatchCallForwardEx, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_RUN_GBA, dispatchStrings, responseStrings, RIL_CMD_PROXY_2, RIL_CHANNEL_1},

// PHB START
{RIL_REQUEST_QUERY_PHB_STORAGE_INFO, dispatchInts, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_WRITE_PHB_ENTRY, dispatchPhbEntry, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_PHB_ENTRY, dispatchInts, responsePhbEntries, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_QUERY_UPB_CAPABILITY, dispatchVoid, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_EDIT_UPB_ENTRY, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_DELETE_UPB_ENTRY, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_UPB_GAS_LIST, dispatchInts, responseStrings, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_UPB_GRP, dispatchInts, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_WRITE_UPB_GRP, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_GET_PHB_STRING_LENGTH, dispatchVoid, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_GET_PHB_MEM_STORAGE, dispatchVoid, responseGetPhbMemStorage, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_PHB_MEM_STORAGE, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_PHB_ENTRY_EXT, dispatchInts, responseReadPhbEntryExt, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_WRITE_PHB_ENTRY_EXT, dispatchWritePhbEntryExt, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_QUERY_UPB_AVAILABLE, dispatchInts, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_EMAIL_ENTRY, dispatchInts, responseString, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_SNE_ENTRY, dispatchInts, responseString, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_ANR_ENTRY, dispatchInts, responsePhbEntries, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_READ_UPB_AAS_LIST, dispatchInts, responseStrings, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
// PHB END
//Femtocell (CSG) feature
{RIL_REQUEST_GET_FEMTOCELL_LIST, dispatchVoid, responseStrings, RIL_CMD_PROXY_3, RIL_CHANNEL_1},
// Femtocell (CSG) : abort command shall be sent in differenent channel
{RIL_REQUEST_ABORT_FEMTOCELL_LIST, dispatchVoid, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SELECT_FEMTOCELL, dispatchStrings, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_1},
{RIL_REQUEST_QUERY_FEMTOCELL_SYSTEM_SELECTION_MODE, dispatchVoid, responseInts, RIL_CMD_PROXY_3, RIL_CHANNEL_1},
{RIL_REQUEST_SET_FEMTOCELL_SYSTEM_SELECTION_MODE, dispatchInts, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_1},
 // Data
{RIL_REQUEST_SYNC_APN_TABLE, dispatchStrings, responseVoid, RIL_CMD_PROXY_5, RIL_CHANNEL_4},
{RIL_REQUEST_SYNC_DATA_SETTINGS_TO_MD, dispatchInts, responseVoid, RIL_CMD_PROXY_5, RIL_CHANNEL_4},
{RIL_REQUEST_RESET_MD_DATA_RETRY_COUNT, dispatchStrings, responseVoid, RIL_CMD_PROXY_5, RIL_CHANNEL_4},
// M: Data Framework - CC 33
{RIL_REQUEST_SET_REMOVE_RESTRICT_EUTRAN_MODE, dispatchInts, responseVoid, RIL_CMD_PROXY_5, RIL_CHANNEL_4},
// M: [LTE][Low Power][UL traffic shaping] @{
{RIL_REQUEST_SET_LTE_ACCESS_STRATUM_REPORT, dispatchInts, responseVoid},
{RIL_REQUEST_SET_LTE_UPLINK_DATA_TRANSFER, dispatchInts, responseVoid},
// M: [LTE][Low Power][UL traffic shaping] @}
// MTK-START: SIM ME LOCK
{RIL_REQUEST_QUERY_SIM_NETWORK_LOCK, dispatchInts, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_SIM_NETWORK_LOCK, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
// MTK-END
{RIL_REQUEST_SET_IMS_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_VOLTE_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_WFC_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_VILTE_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_VIWIFI_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_IMS_VOICE_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_IMS_VIDEO_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_VIDEO_CALL_ACCEPT, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_IMSCFG, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_MD_IMSCFG, dispatchStrings, responseString, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_GET_PROVISION_VALUE, dispatchString, responseStrings, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_PROVISION_VALUE, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_BEARER_ACTIVATION_DONE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_BEARER_DEACTIVATION_DONE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_DEREG_NOTIFICATION, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_ECT, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_HOLD_CALL, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_RESUME_CALL, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_DIAL_WITH_SIP_URI, dispatchString, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_FORCE_RELEASE_CALL, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_IMS_RTP_REPORT, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_CONFERENCE_DIAL, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_ADD_IMS_CONFERENCE_CALL_MEMBER, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_REMOVE_IMS_CONFERENCE_CALL_MEMBER, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_VT_DIAL_WITH_SIP_URI, dispatchString, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SEND_USSI, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_CANCEL_USSI, dispatchVoid, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_WFC_PROFILE, dispatchInts, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_PULL_CALL, dispatchStrings, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_IMS_REGISTRATION_REPORT, dispatchVoid, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_DIAL, dispatchDial, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_2},
{RIL_REQUEST_IMS_VT_DIAL, dispatchDial, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_IMS_EMERGENCY_DIAL, dispatchDial, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_2},
// MTK_TC1_FEATURE for Antenna Testing start
{RIL_REQUEST_VSS_ANTENNA_CONF, dispatchInts, responseInts, RIL_CMD_PROXY_3, RIL_CHANNEL_1}, // Antenna Testing
{RIL_REQUEST_VSS_ANTENNA_INFO, dispatchInts, responseInts, RIL_CMD_PROXY_3, RIL_CHANNEL_1}, // Antenna Testing
// MTK_TC1_FEATURE for Antenna Testing end
// Preferred Operator List
{RIL_REQUEST_GET_POL_CAPABILITY, dispatchVoid, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_GET_POL_LIST, dispatchVoid, responseStrings, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_POL_ENTRY, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
/// M: [Network][C2K] Sprint roaming control @{
{RIL_REQUEST_SET_ROAMING_ENABLE, dispatchInts, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
{RIL_REQUEST_GET_ROAMING_ENABLE, dispatchInts, responseInts, RIL_CMD_PROXY_3, RIL_CHANNEL_3},
/// @}
// External SIM [START]
{RIL_REQUEST_VSIM_NOTIFICATION, dispatchVsimEvent, responseInts, MTK_RIL_CHANNEL_VSIM, RIL_CHANNEL_1},
{RIL_REQUEST_VSIM_OPERATION, dispatchVsimOperationEvent, responseInts, MTK_RIL_CHANNEL_VSIM, RIL_CHANNEL_1},
// External SIM [END]
{RIL_REQUEST_GET_GSM_SMS_BROADCAST_ACTIVATION, dispatchVoid, responseInts, RIL_CMD_PROXY_1, RIL_CHANNEL_5},
{RIL_REQUEST_SET_VOICE_DOMAIN_PREFERENCE, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
{RIL_REQUEST_SET_E911_STATE, dispatchInts, responseVoid, RIL_CMD_PROXY_2, RIL_CHANNEL_1},
/// M: MwiService @{
{RIL_REQUEST_SET_WIFI_ENABLED, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_WIFI_ASSOCIATED, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_WIFI_SIGNAL_LEVEL, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_WIFI_IP_ADDRESS, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_GEO_LOCATION, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_EMERGENCY_ADDRESS_ID, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
{RIL_REQUEST_SET_NATT_KEEP_ALIVE_STATUS, dispatchStrings, responseVoid, RIL_CMD_PROXY_1, RIL_CHANNEL_1},
///@}
/// M: Network @{
{RIL_REQUEST_SET_SERVICE_STATE, dispatchInts, responseVoid, RIL_CMD_PROXY_3, RIL_CHANNEL_1},
///@}
#endif // MTK_USE_HIDL
