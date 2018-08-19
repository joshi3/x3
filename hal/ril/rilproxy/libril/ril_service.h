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

#ifndef RIL_SERVICE_H
#define RIL_SERVICE_H

#include <telephony/mtk_ril.h>
#include <ril_internal.h>

namespace radio {
void registerService(RIL_RadioFunctions *callbacks, android::CommandInfo *commands);

int getIccCardStatusResponse(int slotId, int responseType,
                            int token, RIL_Errno e, void *response, size_t responselen);

int supplyIccPinForAppResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int supplyIccPukForAppResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int supplyIccPin2ForAppResponse(int slotId,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int supplyIccPuk2ForAppResponse(int slotId,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int changeIccPinForAppResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int changeIccPin2ForAppResponse(int slotId,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int supplyNetworkDepersonalizationResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responselen);

int getCurrentCallsResponse(int slotId,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responselen);

int dialResponse(int slotId,
                int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int getIMSIForAppResponse(int slotId, int responseType,
                         int serial, RIL_Errno e, void *response, size_t responselen);

int hangupConnectionResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response, size_t responselen);

int hangupWaitingOrBackgroundResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int hangupForegroundResumeBackgroundResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int switchWaitingOrHoldingAndActiveResponse(int slotId,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responselen);

int conferenceResponse(int slotId, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int rejectCallResponse(int slotId, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int getLastCallFailCauseResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int getSignalStrengthResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int getVoiceRegistrationStateResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getDataRegistrationStateResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getOperatorResponse(int slotId,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int setRadioPowerResponse(int slotId,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int sendDtmfResponse(int slotId,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int sendSmsResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int sendSMSExpectMoreResponse(int slotId,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int setupDataCallResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen);

int iccIOForAppResponse(int slotId,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int sendUssdResponse(int slotId,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int cancelPendingUssdResponse(int slotId,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int getClirResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int setClirResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int getCallForwardStatusResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int setCallForwardResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int getCallWaitingResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setCallWaitingResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int acknowledgeLastIncomingGsmSmsResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responselen);

int acceptCallResponse(int slotId,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int deactivateDataCallResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int getFacilityLockForAppResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setFacilityLockForAppResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setBarringPasswordResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int getNetworkSelectionModeResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int setNetworkSelectionModeAutomaticResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int setNetworkSelectionModeManualResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responselen);

int setNetworkSelectionModeManualWithActResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen);

int getAvailableNetworksResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int getAvailableNetworksWithActResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen);

int cancelAvailableNetworksResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen);

int startNetworkScanResponse(int slotId,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int stopNetworkScanResponse(int slotId,
                            int responseType, int serial, RIL_Errno e, void *response,
                            size_t responselen);

int startDtmfResponse(int slotId,
                     int responseType, int serial, RIL_Errno e, void *response,
                     size_t responselen);

int stopDtmfResponse(int slotId,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int getBasebandVersionResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int separateConnectionResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setMuteResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int getMuteResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int getClipResponse(int slotId,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int setClipResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int getColpResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int getColrResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int sendCnapResponse(int slotId, int responseType, int serial, RIL_Errno e,
                     void *response, size_t responseLen);

int setColpResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int setColrResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int queryCallForwardInTimeSlotStatusResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int setCallForwardInTimeSlotResponse(int slotId, int responseType, int serial, RIL_Errno e,
                     void *response, size_t responseLen);

int runGbaAuthenticationResponse(int slotId, int responseType, int serial, RIL_Errno e,
                    void *response, size_t responseLen);

int getDataCallListResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);

int setSuppServiceNotificationsResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e, void *response,
                                       size_t responselen);

int writeSmsToSimResponse(int slotId,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int deleteSmsOnSimResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setBandModeResponse(int slotId,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int getAvailableBandModesResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int sendEnvelopeResponse(int slotId,
                        int responseType, int serial, RIL_Errno e, void *response,
                        size_t responselen);

int sendTerminalResponseToSimResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int handleStkCallSetupRequestFromSimResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int explicitCallTransferResponse(int slotId,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int setPreferredNetworkTypeResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int getPreferredNetworkTypeResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int getNeighboringCidsResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setLocationUpdatesResponse(int slotId,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setCdmaSubscriptionSourceResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int setCdmaRoamingPreferenceResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getCdmaRoamingPreferenceResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int setTTYModeResponse(int slotId,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int getTTYModeResponse(int slotId,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int setPreferredVoicePrivacyResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getPreferredVoicePrivacyResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int sendCDMAFeatureCodeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int sendBurstDtmfResponse(int slotId,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int sendCdmaSmsResponse(int slotId,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int acknowledgeLastIncomingCdmaSmsResponse(int slotId,
                                          int responseType, int serial, RIL_Errno e, void *response,
                                          size_t responselen);

int getGsmBroadcastConfigResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setGsmBroadcastConfigResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setGsmBroadcastActivationResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getCdmaBroadcastConfigResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int setCdmaBroadcastConfigResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int setCdmaBroadcastActivationResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responselen);

int getCDMASubscriptionResponse(int slotId,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int writeSmsToRuimResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int deleteSmsOnRuimResponse(int slotId,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responselen);

int getDeviceIdentityResponse(int slotId,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int exitEmergencyCallbackModeResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getSmscAddressResponse(int slotId,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setCdmaBroadcastActivationResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responselen);

int setSmscAddressResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int reportSmsMemoryStatusResponse(int slotId,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responselen);

int reportStkServiceIsRunningResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen);

int getCdmaSubscriptionSourceResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int requestIsimAuthenticationResponse(int slotId,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int acknowledgeIncomingGsmSmsWithPduResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int sendEnvelopeWithStatusResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int getVoiceRadioTechnologyResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responselen);

int getCellInfoListResponse(int slotId,
                            int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responseLen);

int setCellInfoListRateResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int setInitialAttachApnResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int getImsRegistrationStateResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responselen);

int sendImsSmsResponse(int slotId, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int iccTransmitApduBasicChannelResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responselen);

int iccOpenLogicalChannelResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);


int iccCloseLogicalChannelResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responselen);

int iccTransmitApduLogicalChannelResponse(int slotId,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responselen);

int nvReadItemResponse(int slotId,
                      int responseType, int serial, RIL_Errno e,
                      void *response, size_t responselen);


int nvWriteItemResponse(int slotId,
                       int responseType, int serial, RIL_Errno e,
                       void *response, size_t responselen);

int nvWriteCdmaPrlResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int nvResetConfigResponse(int slotId,
                         int responseType, int serial, RIL_Errno e,
                         void *response, size_t responselen);

int setUiccSubscriptionResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int setDataAllowedResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int getHardwareConfigResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int requestIccSimAuthenticationResponse(int slotId,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responselen);

int setDataProfileResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int requestShutdownResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responselen);

int getRadioCapabilityResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int setRadioCapabilityResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int startLceServiceResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responselen);

int stopLceServiceResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int pullLceDataResponse(int slotId,
                        int responseType, int serial, RIL_Errno e,
                        void *response, size_t responseLen);

int getModemActivityInfoResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responselen);

int setAllowedCarriersResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int getAllowedCarriersResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int sendDeviceStateResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int setIndicationFilterResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int setSimCardPowerResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int startKeepaliveResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responselen);

int stopKeepaliveResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);
/// M: CC: call control part @{
int hangupAllResponse(int slotId,
                      int responseType, int serial, RIL_Errno e,
                      void *response, size_t responselen);

int setCallIndicationResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int emergencyDialResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int setEccServiceCategoryResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responselen);

int setEccListResponse(int slotId,
                       int responseType, int token, RIL_Errno e,
                       void *response, size_t responselen);
/// M: CC: @}

/// M: CC: For 3G VT Only @{
int vtDialResponse(int slotId,
                   int responseType, int token, RIL_Errno e, void *response,
                   size_t responselen);

int voiceAcceptResponse(int slotId,
                              int responseType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int replaceVtCallResponse(int slotId,
                          int responseType, int token, RIL_Errno e, void *response,
                          size_t responselen);
/// M: CC: @}

/// M: CC: E911 request current status response
int currentStatusResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen);

/// M: CC: E911 request set ECC preferred RAT.
int eccPreferredRatResponse(int slotId, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responselen);

/// M: CC: CDMA call accepted indication @{
int cdmaCallAcceptedInd(int slotId, int indicationType,
                    int token, RIL_Errno e, void *response, size_t responseLen);
/// @}

// Femtocell feature
int getFemtocellListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int abortFemtocellListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int selectFemtocellResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int queryFemtoCellSystemSelectionModeResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int setFemtoCellSystemSelectionModeResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int responseFemtocellInfo(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

// FastDormancy
int setFdModeResponse(int slotId, int responseType,
        int serial, RIL_Errno e, void *response, size_t responselen);

void acknowledgeRequest(int slotId, int serial);

int radioStateChangedInd(int slotId,
                          int indicationType, int token, RIL_Errno e, void *response,
                          size_t responseLen);

int callStateChangedInd(int slotId, int indType, int token,
                        RIL_Errno e, void *response, size_t responselen);

int networkStateChangedInd(int slotId, int indType,
                                int token, RIL_Errno e, void *response, size_t responselen);

int newSmsInd(int slotId, int indicationType,
              int token, RIL_Errno e, void *response, size_t responselen);

int newSmsStatusReportInd(int slotId, int indicationType,
                          int token, RIL_Errno e, void *response, size_t responselen);

int newSmsOnSimInd(int slotId, int indicationType,
                   int token, RIL_Errno e, void *response, size_t responselen);

int onUssdInd(int slotId, int indicationType,
              int token, RIL_Errno e, void *response, size_t responselen);

int nitzTimeReceivedInd(int slotId, int indicationType,
                        int token, RIL_Errno e, void *response, size_t responselen);

int currentSignalStrengthInd(int slotId,
                             int indicationType, int token, RIL_Errno e,
                             void *response, size_t responselen);

int dataCallListChangedInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);

int suppSvcNotifyInd(int slotId, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responselen);

int cfuStatusNotifyInd(int slotId, int indicationType,
                              int token, RIL_Errno e, void *response, size_t responseLen);

/// M: CC: call control part ([IMS] common flow) @{
int incomingCallIndicationInd(int slotId, int indicationType,
                    int token, RIL_Errno e, void *response, size_t responseLen);
int cipherIndicationInd(int slotId, int indicationType,
              int token, RIL_Errno e, void *response, size_t responseLen);
int crssNotifyInd(int slotId, int indicationType,
                  int token, RIL_Errno e, void *response, size_t responseLen);
int vtStatusInfoInd(int slotId, int indicationType,
                    int token, RIL_Errno e, void *response, size_t responseLen);
int speechCodecInfoInd(int slotId, int indicationType,
                       int token, RIL_Errno e, void *response, size_t responseLen);
/// M: CC: @}

int stkSessionEndInd(int slotId, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responselen);

int stkProactiveCommandInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);

int stkEventNotifyInd(int slotId, int indicationType,
                      int token, RIL_Errno e, void *response, size_t responselen);

int stkCallSetupInd(int slotId, int indicationType,
                    int token, RIL_Errno e, void *response, size_t responselen);

int simSmsStorageFullInd(int slotId, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responselen);

int simRefreshInd(int slotId, int indicationType,
                  int token, RIL_Errno e, void *response, size_t responselen);

int callRingInd(int slotId, int indicationType,
                int token, RIL_Errno e, void *response, size_t responselen);

int simStatusChangedInd(int slotId, int indicationType,
                        int token, RIL_Errno e, void *response, size_t responselen);

int cdmaNewSmsInd(int slotId, int indicationType,
                  int token, RIL_Errno e, void *response, size_t responselen);

int newBroadcastSmsInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responselen);

int cdmaRuimSmsStorageFullInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int restrictedStateChangedInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int enterEmergencyCallbackModeInd(int slotId,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responselen);

int cdmaCallWaitingInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responselen);

int cdmaOtaProvisionStatusInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int cdmaInfoRecInd(int slotId,
                   int indicationType, int token, RIL_Errno e, void *response,
                   size_t responselen);

int oemHookRawInd(int slotId,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responselen);

int atciInd(int slotId,
            int indicationType, int token, RIL_Errno e, void *response,
            size_t responselen);

int indicateRingbackToneInd(int slotId,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int resendIncallMuteInd(int slotId,
                        int indicationType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int cdmaSubscriptionSourceChangedInd(int slotId,
                                     int indicationType, int token, RIL_Errno e,
                                     void *response, size_t responselen);

int cdmaPrlChangedInd(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int exitEmergencyCallbackModeInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int rilConnectedInd(int slotId,
                    int indicationType, int token, RIL_Errno e, void *response,
                    size_t responselen);

int voiceRadioTechChangedInd(int slotId,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responselen);

int cellInfoListInd(int slotId,
                    int indicationType, int token, RIL_Errno e, void *response,
                    size_t responselen);

int imsNetworkStateChangedInd(int slotId,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int subscriptionStatusChangedInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int srvccStateNotifyInd(int slotId,
                        int indicationType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int hardwareConfigChangedInd(int slotId,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responselen);

int radioCapabilityIndicationInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int onSupplementaryServiceIndicationInd(int slotId,
                                        int indicationType, int token, RIL_Errno e,
                                        void *response, size_t responselen);

int stkCallControlAlphaNotifyInd(int slotId,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int lceDataInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responselen);

int pcoDataInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responselen);

// M: [VzW] Data Framework
int pcoDataAfterAttachedInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responselen);

// M: [VzW] Data Framework
int volteLteConnectionStatusInd(int slotId,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responseLen);

int modemResetInd(int slotId,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responselen);

int networkScanResultInd(int slotId,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responselen);

int sendRequestRawResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen);

int sendRequestStringsResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int setCarrierInfoForImsiEncryptionResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen);

int carrierInfoForImsiEncryption(int slotId,
                        int responseType, int serial, RIL_Errno e,
                        void *response, size_t responseLen);

int sendAtciResponse(int slotId,
                     int responseType, int serial, RIL_Errno e,
                     void *response, size_t responseLen);

/// [IMS] Response ======================================================================

int imsEmergencyDialResponse(int slotId,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responseLen);

int imsDialResponse(int slotId,
                    int responseType,
                    int serial, RIL_Errno e, void *response, size_t responselen);

int imsVtDialResponse(int slotId,
                      int responseType, int token, RIL_Errno e, void *response,
                      size_t responselen);
int videoCallAcceptResponse(int slotId,
                            int responseType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int imsEctCommandResponse(int slotId,
                          int responseType, int token, RIL_Errno e,
                          void *response, size_t responselen);

int holdCallResponse(int slotId,
                 int responseType, int token, RIL_Errno e,
                 void *response, size_t responselen);

int resumeCallResponse(int slotId,
                   int responseType,
                   int token, RIL_Errno e, void *response, size_t responselen);

int imsDeregNotificationResponse(int slotId,
                                 int responseType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int setImsEnableResponse(int slotId,
                         int responseType, int token, RIL_Errno e, void *response,
                         size_t responselen);

int setVolteEnableResponse(int slotId,
                          int responseType, int token, RIL_Errno e, void *response,
                          size_t responselen);

int setWfcEnableResponse(int slotId,
                         int responseType, int token, RIL_Errno e, void *response,
                         size_t responselen);

int setVilteEnableResponse(int slotId,
                        int responseType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int setViWifiEnableResponse(int slotId,
                         int responseType, int token, RIL_Errno e, void *response,
                         size_t responselen);

int setImsVoiceEnableResponse(int slotId,
                           int responseType, int token, RIL_Errno e, void *response,
                           size_t responselen);

int setImsVideoEnableResponse(int slotId,
                           int responseType, int token, RIL_Errno e, void *response,
                           size_t responselen);

int setImscfgResponse(int slotId,
                      int responseType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int setModemImsCfgResponse(int slotId,
                      int responseType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int getProvisionValueResponse(int slotId,
                              int responseType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int setProvisionValueResponse(int slotId,
                              int responseType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int addImsConferenceCallMemberResponse(int slotId,
                                int responseType, int token, RIL_Errno e, void *response,
                                size_t responselen);

int removeImsConferenceCallMemberResponse(int slotId,
                                   int responseType, int token, RIL_Errno e, void *response,
                                   size_t responselen);

int setWfcProfileResponse(int slotId,

                          int responseType, int token, RIL_Errno e, void *response,
                          size_t responselen);

int conferenceDialResponse(int slotId,
                           int responseType, int token, RIL_Errno e, void *response,
                           size_t responselen);

int vtDialWithSipUriResponse(int slotId,
                             int responseType, int token, RIL_Errno e, void *response,
                             size_t responselen);

int dialWithSipUriResponse(int slotId,
                           int responseType, int token, RIL_Errno e, void *response,
                           size_t responselen);

int sendUssiResponse(int slotId,
                     int responseType, int token, RIL_Errno e, void *response,
                     size_t responselen);

int cancelUssiResponse(int slotId,
                              int responseType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int forceReleaseCallResponse(int slotId,
                        int responseType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int setImsRtpReportResponse(int slotId,
                            int responseType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int imsBearerActivationDoneResponse(int slotId,
                                         int responseType, int token, RIL_Errno e,
                                         void *response, size_t responselen);

int imsBearerDeactivationDoneResponse(int slotId,
                                           int responseType, int token, RIL_Errno e,
                                           void *response, size_t responselen);

int setEccListResponse(int slotId,
                       int responseType, int token, RIL_Errno e,
                       void *response, size_t responselen);

int pullCallResponse(int slotId,
                     int responseType, int serial, RIL_Errno e,
                     void *response, size_t responselen);
int setImsRegistrationReportResponse(int slotId,
                                     int indicationType, int token, RIL_Errno e,
                                     void *response, size_t responseLen);
/// [IMS] Response ======================================================================

pthread_rwlock_t * getRadioServiceRwlock(int slotId);

int setTrmResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int resetAttachApnInd(int slotId, int indicationType, int token, RIL_Errno e,
                      void *response, size_t responseLen);

int mdChangeApnInd(int slotId, int indicationType, int token, RIL_Errno e,
                   void *response, size_t responseLen);

// MTK-START: SIM
int getATRResponse(int slotId,
                 int responseType, int serial, RIL_Errno e,
                 void *response, size_t responseLen);

int setSimPowerResponse(int slotId,
                 int responseType, int serial, RIL_Errno e,
                 void *response, size_t responseLen);

int onVirtualSimOn(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen);

int onVirtualSimOff(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen);

int onImeiLock(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen);

int onImsiRefreshDone(int slotId,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen);

int setModemPowerResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int onCardDetectedInd(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);
// MTK-END
/// [IMS] Indication ////////////////////////////////////////////////////////////////////
int callInfoIndicationInd(int slotId,
                          int indicationType, int token, RIL_Errno e, void *response,
                          size_t responseLen);

int econfResultIndicationInd(int slotId,
                             int indicationType, int token, RIL_Errno e,
                             void *response, size_t responseLen);

int sipCallProgressIndicatorInd(int slotId,
                                int indicationType, int token, RIL_Errno e,
                                void *response, size_t responseLen);

int callmodChangeIndicatorInd(int slotId,
                              int indicationType, int token, RIL_Errno e,
                              void *response, size_t responseLen);

int videoCapabilityIndicatorInd(int slotId,
                                int indicationType, int token, RIL_Errno e,
                                void *response, size_t responseLen);

int onUssiInd(int slotId,
              int indicationType, int token, RIL_Errno e, void *response,
             size_t responseLen);

int getProvisionDoneInd(int slotId,
                        int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);

int imsRtpInfoInd(int slotId,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responseLen);

int onXuiInd(int slotId,
             int indicationType, int token, RIL_Errno e, void *response,
             size_t responseLen);

int imsEventPackageIndicationInd(int slotId,
                                 int indicationType, int token, RIL_Errno e,
                                 void *response, size_t responseLen);

int imsRegistrationInfoInd(int slotId,
                           int indicationType, int token, RIL_Errno e,
                           void *response, size_t responseLen);

int imsEnableDoneInd(int slotId,
                     int indicationType, int token, RIL_Errno e,
                     void *response, size_t responseLen);

int imsDisableDoneInd(int slotId,
                      int indicationType, int token, RIL_Errno e,
                      void *response, size_t responseLen);

int imsEnableStartInd(int slotId,
                      int indicationType, int token, RIL_Errno e,
                      void *response, size_t responseLen);

int imsDisableStartInd(int slotId,
                       int indicationType, int token, RIL_Errno e,
                       void *response, size_t responseLen);

int ectIndicationInd(int slotId,
                     int indicationType, int token, RIL_Errno e,
                     void *response, size_t responseLen);

int volteSettingInd(int slotId,
                    int indicationType, int token, RIL_Errno e,
                    void *response, size_t responseLen);

int imsBearerActivationInd(int slotId,
                           int indicationType, int token, RIL_Errno e,
                           void *response, size_t responseLen);

int imsBearerDeactivationInd(int slotId,
                             int indicationType, int token, RIL_Errno e,
                             void *response, size_t responseLen);

int imsBearerInitInd(int slotId,
                     int indicationType, int token, RIL_Errno e,
                     void *response, size_t responseLen);

int imsDeregDoneInd(int slotId,
                    int indicationType, int token, RIL_Errno e,
                    void *response, size_t responseLen);

  int confSRVCCInd(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);

int multiImsCountInd(int slotId,
          int indicationType, int token, RIL_Errno e,
          void *response, size_t responseLen);

int imsSupportEccInd(int slotId,
                     int indicationType, int token, RIL_Errno e,
                     void *response, size_t responseLen);
/// [IMS] Indication End
// SMS-START
int getSmsParametersResponse(int slotId, int responseType,
        int token, RIL_Errno e, void *response, size_t responselen);
int setSmsParametersResponse(int slotId, int responseType,
        int token, RIL_Errno e, void *response, size_t responselen);
int setEtwsResponse(int slotId, int responseType,
        int token, RIL_Errno e, void *response, size_t responselen);
int removeCbMsgResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);
int newEtwsInd(int slotId, int indicationType, int token, RIL_Errno e, void *response,
        size_t responselen);
int getSmsMemStatusResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);
int setGsmBroadcastLangsResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);
int getGsmBroadcastLangsResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);
int getGsmBroadcastActivationRsp(int slotId,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen);
int meSmsStorageFullInd(int slotId, int indicationType, int token, RIL_Errno e,
        void *response, size_t responseLen);
int smsReadyInd(int slotId, int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);
// SMS-END

/// M: eMBMS feature
int sendEmbmsAtCommandResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);
int embmsSessionStatusInd(int slotId, int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int embmsAtInfoInd(int slotId, int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);
/// M: eMBMS end

int responsePsNetworkStateChangeInd(int slotId,
                                    int indicationType, int token, RIL_Errno e,
                                    void *response, size_t responseLen);

int responseCsNetworkStateChangeInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responseLen);

int responseInvalidSimInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responseLen);

int responseNetworkEventInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responseLen);

int responseModulationInfoInd(int slotId,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responseLen);

int dataAllowedNotificationInd(int slotId, int indicationType, int token,
        RIL_Errno e, void *response, size_t responseLen);

// APC
int setApcModeResponse(int slotId, int indicationType, int token,
        RIL_Errno e, void *response, size_t responseLen);

int getApcInfoResponse(int slotId, int indicationType, int token,
        RIL_Errno e, void *response, size_t responseLen);

int onPseudoCellInfoInd(int slotId, int indicationType, int token,
        RIL_Errno e, void *response, size_t responseLen);

// MTK-START: RIL_REQUEST_SWITCH_MODE_FOR_ECC
int triggerModeSwitchByEccResponse(int slotId, int responseType,
        int serial, RIL_Errno e, void *response, size_t responseLen);

int handleStkCallSetupRequestFromSimWithResCodeResponse(int slotId,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);
// MTK-END

int getSmsRuimMemoryStatusResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen);
// MTK-START: SIM ME LOCK
int queryNetworkLockResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int setNetworkLockResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen);
// MTK-END

// World Phone part {
int setResumeRegistrationResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int storeModemTypeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int reloadModemTypeResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int plmnChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int registrationSuspendedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int gmssRatChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int worldModeChangedIndication(int slotId,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);
// World Phone part }

int esnMeidChangeInd(int slotId,
                     int indicationType, int token, RIL_Errno e, void *response,
                     size_t responseLen);

// PHB START
int queryPhbStorageInfoResponse(int slotId,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen);

int writePhbEntryResponse(int slotId,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen);

int readPhbEntryResponse(int slotId,
                         int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int queryUPBCapabilityResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int editUPBEntryResponse(int slotId,
                         int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int deleteUPBEntryResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen);

int readUPBGasListResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen);

int readUPBGrpEntryResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);

int writeUPBGrpEntryResponse(int slotId,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen);

int getPhoneBookStringsLengthResponse(int slotId,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen);

int getPhoneBookMemStorageResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen);

int setPhoneBookMemStorageResponse(int slotId,
                                    int responseType, int serial, RIL_Errno e,
                                    void *response, size_t responseLen);

int readPhoneBookEntryExtResponse(int slotId,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen);

int writePhoneBookEntryExtResponse(int slotId,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen);

int queryUPBAvailableResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int readUPBEmailEntryResponse(int slotId,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int readUPBSneEntryResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);

int readUPBAnrEntryResponse(int slotId,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);

int readUPBAasListResponse(int slotId,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen);

int phbReadyNotificationInd(int slotId,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);
// PHB END

int resetRadioResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

// / M: BIP {
int bipProactiveCommandInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);
// / M: BIP }
// / M: OTASP {
int triggerOtaSPInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);
// / M: OTASP }

// / M: STK {
int onStkMenuResetInd(int slotId, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);
// / M: STK }

// M: Data Framework - common part enhancement @{
int syncDataSettingsToMdResponse(int slotId,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);
// M: Data Framework - common part enhancement @}

// M: Data Framework - Data Retry enhancement @{
int resetMdDataRetryCountResponse(int slotId,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int onMdDataRetryCountReset(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);
// M: Data Framework - Data Retry enhancement @}

// M: Data Framework - CC 33 @{
int setRemoveRestrictEutranModeResponse(int slotId,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int onRemoveRestrictEutran(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);
// M: Data Framework - CC 33 @}

int onPcoStatus(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);

// M: [LTE][Low Power][UL traffic shaping] @{
int setLteAccessStratumReportResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);
int setLteUplinkDataTransferResponse(int slotId,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);
int onLteAccessStratumStateChanged(int slotId, int indicationType, int token, RIL_Errno e,
                        void *response, size_t responseLen);
// M: [LTE][Low Power][UL traffic shaping] @}

// MTK-START: SIM HOT SWAP / SIM RECOVERY
int onSimPlugIn(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);

int onSimPlugOut(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);

int onSimMissing(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);

int onSimRecovery(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);
// MTK-END
// MTK-START: SIM COMMON SLOT
int onSimTrayPlugIn(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);

int onSimCommonSlotNoChanged(int slotId,
        int indicationType, int token, RIL_Errno e, void *response,
        size_t responseLen);
// MTK-END
int networkInfoInd(int slotId, int indicationType, int token, RIL_Errno e,
        void *response, size_t responseLen);

int setRxTestConfigResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);
int getRxTestResultResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int getPOLCapabilityResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int getCurrentPOLListResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

int setPOLEntryResponse(int slotId, int responseType, int serial, RIL_Errno e,
                         void *response, size_t responseLen);

/// M: [Network][C2K] Sprint roaming control @{
int setRoamingEnableResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen);

int getRoamingEnableResponse(int slotId, int responseType, int serial,
        RIL_Errno e, void *response, size_t responseLen);
/// @}

// External SIM [Start]
int vsimNotificationResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int vsimOperationResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int onVsimEventIndication(int slotId,
        int indicationType, int token, RIL_Errno e, void *response, size_t responselen);

// External SIM [End]
int setVoiceDomainPreferenceResponse(int slotId,
                            int responseType, int token, RIL_Errno e, void *response,
                            size_t responselen);

/// Ims Data Framework {@
int dedicatedBearerActivationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen);
int dedicatedBearerModificationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen);
int dedicatedBearerDeactivationInd(int slotId, int indicationType, int serial,
        RIL_Errno e, void *response, size_t responseLen);
/// @}

/// MwiService @{
int setWifiEnabledResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setWifiAssociatedResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setWifiSignalLevelResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setWifiIpAddressResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setLocationInfoResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setEmergencyAddressIdResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int setNattKeepAliveStatusResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int onWifiMonitoringThreshouldChanged(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onWifiPdnActivate(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onWfcPdnError(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onPdnHandover(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onWifiRoveout(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onLocationRequest(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onWfcPdnStateChanged(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);

int onNattKeepAliveChanged(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);
///@}
int setE911StateResponse(int slotId, int responseType, int serial, RIL_Errno e,
        void *response, size_t responseLen);

int setServiceStateToModemResponse(int slotId, int responseType, int serial,
        RIL_Errno err, void *response, size_t responseLen);

int onTxPowerIndication(int slotId, int indicationType, int token,
        RIL_Errno err, void *response, size_t responseLen);
}   // namespace radio

#endif  // RIL_SERVICE_H
