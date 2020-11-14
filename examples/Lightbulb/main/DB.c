// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// This file contains the accessory attribute database that defines the accessory information service, HAP Protocol
// Information Service, the Pairing service and finally the service signature exposed by the light bulb.

#include "App.h"
#include "DB.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * IID constants.
 */
#define kIID_AccessoryInformation                 ((uint64_t) 0x0001)
#define kIID_AccessoryInformationIdentify         ((uint64_t) 0x0002)
#define kIID_AccessoryInformationManufacturer     ((uint64_t) 0x0003)
#define kIID_AccessoryInformationModel            ((uint64_t) 0x0004)
#define kIID_AccessoryInformationName             ((uint64_t) 0x0005)
#define kIID_AccessoryInformationSerialNumber     ((uint64_t) 0x0006)
#define kIID_AccessoryInformationFirmwareRevision ((uint64_t) 0x0007)
#define kIID_AccessoryInformationHardwareRevision ((uint64_t) 0x0008)
#define kIID_AccessoryInformationADKVersion       ((uint64_t) 0x0009)
#define kIID_AccessoryInformationProductData      ((uint64_t) 0x000A)

#define kIID_HAPProtocolInformation                 ((uint64_t) 0x0010)
#define kIID_HAPProtocolInformationServiceSignature ((uint64_t) 0x0011)
#define kIID_HAPProtocolInformationVersion          ((uint64_t) 0x0012)

#define kIID_Pairing                ((uint64_t) 0x0020)
#define kIID_PairingPairSetup       ((uint64_t) 0x0022)
#define kIID_PairingPairVerify      ((uint64_t) 0x0023)
#define kIID_PairingPairingFeatures ((uint64_t) 0x0024)
#define kIID_PairingPairingPairings ((uint64_t) 0x0025)

#define kIID_LightBulb                 ((uint64_t) 0x0030)
#define kIID_LightBulbServiceSignature ((uint64_t) 0x0031)
#define kIID_LightBulbName             ((uint64_t) 0x0032)
#define kIID_LightBulbOn               ((uint64_t) 0x0033)

#define kIID_FurnaceFan                  ((uint64_t) 0x0040)
#define kIID_FurnaceFanServiceSignature  ((uint64_t) 0x0041)
#define kIID_FurnaceFanName              ((uint64_t) 0x0042)
#define kIID_FurnaceFanActive            ((uint64_t) 0x0043)
#define kIID_FurnaceFanTargetFanState    ((uint64_t) 0x0044)
#define kIID_FurnaceFanTimeout           ((uint64_t) 0x0045)
#define kIID_FurnaceFanDutyCycle         ((uint64_t) 0x0046)

#define kIID_Hrv                  ((uint64_t) 0x0050)
#define kIID_HrvServiceSignature  ((uint64_t) 0x0051)
#define kIID_HrvName              ((uint64_t) 0x0052)
#define kIID_HrvActive            ((uint64_t) 0x0053)
#define kIID_HrvTargetFanState    ((uint64_t) 0x0054)

HAP_STATIC_ASSERT(kAttributeCount == 9 + 3 + 5 + 4, AttributeCount_mismatch);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_AccessoryInformationIdentify,
    .characteristicType = &kHAPCharacteristicType_Identify,
    .debugDescription = kHAPCharacteristicDebugDescription_Identify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = NULL, .handleWrite = HAPHandleAccessoryInformationIdentifyWrite }
};

const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationManufacturer,
    .characteristicType = &kHAPCharacteristicType_Manufacturer,
    .debugDescription = kHAPCharacteristicDebugDescription_Manufacturer,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationManufacturerRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationModelCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationModel,
    .characteristicType = &kHAPCharacteristicType_Model,
    .debugDescription = kHAPCharacteristicDebugDescription_Model,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationModelRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationNameRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationSerialNumber,
    .characteristicType = &kHAPCharacteristicType_SerialNumber,
    .debugDescription = kHAPCharacteristicDebugDescription_SerialNumber,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationSerialNumberRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationFirmwareRevision,
    .characteristicType = &kHAPCharacteristicType_FirmwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_FirmwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationFirmwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationHardwareRevision,
    .characteristicType = &kHAPCharacteristicType_HardwareRevision,
    .debugDescription = kHAPCharacteristicDebugDescription_HardwareRevision,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationHardwareRevisionRead, .handleWrite = NULL }
};

const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_AccessoryInformationADKVersion,
    .characteristicType = &kHAPCharacteristicType_ADKVersion,
    .debugDescription = kHAPCharacteristicDebugDescription_ADKVersion,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = true,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleAccessoryInformationADKVersionRead, .handleWrite = NULL }
};

const HAPService accessoryInformationService = {
    .iid = kIID_AccessoryInformation,
    .serviceType = &kHAPServiceType_AccessoryInformation,
    .debugDescription = kHAPServiceDebugDescription_AccessoryInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &accessoryInformationIdentifyCharacteristic,
                                                            &accessoryInformationManufacturerCharacteristic,
                                                            &accessoryInformationModelCharacteristic,
                                                            &accessoryInformationNameCharacteristic,
                                                            &accessoryInformationSerialNumberCharacteristic,
                                                            &accessoryInformationFirmwareRevisionCharacteristic,
                                                            &accessoryInformationHardwareRevisionCharacteristic,
                                                            &accessoryInformationADKVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPDataCharacteristic hapProtocolInformationServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HAPProtocolInformationServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

static const HAPStringCharacteristic hapProtocolInformationVersionCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HAPProtocolInformationVersion,
    .characteristicType = &kHAPCharacteristicType_Version,
    .debugDescription = kHAPCharacteristicDebugDescription_Version,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleHAPProtocolInformationVersionRead, .handleWrite = NULL }
};

const HAPService hapProtocolInformationService = {
    .iid = kIID_HAPProtocolInformation,
    .serviceType = &kHAPServiceType_HAPProtocolInformation,
    .debugDescription = kHAPServiceDebugDescription_HAPProtocolInformation,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = true } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &hapProtocolInformationServiceSignatureCharacteristic,
                                                            &hapProtocolInformationVersionCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

static const HAPTLV8Characteristic pairingPairSetupCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairSetup,
    .characteristicType = &kHAPCharacteristicType_PairSetup,
    .debugDescription = kHAPCharacteristicDebugDescription_PairSetup,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairSetupRead, .handleWrite = HAPHandlePairingPairSetupWrite }
};

static const HAPTLV8Characteristic pairingPairVerifyCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairVerify,
    .characteristicType = &kHAPCharacteristicType_PairVerify,
    .debugDescription = kHAPCharacteristicDebugDescription_PairVerify,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = true } },
    .callbacks = { .handleRead = HAPHandlePairingPairVerifyRead, .handleWrite = HAPHandlePairingPairVerifyWrite }
};

static const HAPUInt8Characteristic pairingPairingFeaturesCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_PairingPairingFeatures,
    .characteristicType = &kHAPCharacteristicType_PairingFeatures,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingFeatures,
    .manufacturerDescription = NULL,
    .properties = { .readable = false,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsDisconnectedNotification = false,
                             .supportsBroadcastNotification = false,
                             .readableWithoutSecurity = true,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_None,
    .constraints = { .minimumValue = 0,
                     .maximumValue = UINT8_MAX,
                     .stepValue = 0,
                     .validValues = NULL,
                     .validValuesRanges = NULL },
    .callbacks = { .handleRead = HAPHandlePairingPairingFeaturesRead, .handleWrite = NULL }
};

static const HAPTLV8Characteristic pairingPairingPairingsCharacteristic = {
    .format = kHAPCharacteristicFormat_TLV8,
    .iid = kIID_PairingPairingPairings,
    .characteristicType = &kHAPCharacteristicType_PairingPairings,
    .debugDescription = kHAPCharacteristicDebugDescription_PairingPairings,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HAPHandlePairingPairingPairingsRead,
                   .handleWrite = HAPHandlePairingPairingPairingsWrite }
};

const HAPService pairingService = {
    .iid = kIID_Pairing,
    .serviceType = &kHAPServiceType_Pairing,
    .debugDescription = kHAPServiceDebugDescription_Pairing,
    .name = NULL,
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &pairingPairSetupCharacteristic,
                                                            &pairingPairVerifyCharacteristic,
                                                            &pairingPairingFeaturesCharacteristic,
                                                            &pairingPairingPairingsCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------

/**
 * The 'Service Signature' characteristic of the Light Bulb service.
 */
static const HAPDataCharacteristic lightBulbServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_LightBulbServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the Light Bulb service.
 */
static const HAPStringCharacteristic lightBulbNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_LightBulbName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'On' characteristic of the Light Bulb service.
 */
const HAPBoolCharacteristic lightBulbOnCharacteristic = {
    .format = kHAPCharacteristicFormat_Bool,
    .iid = kIID_LightBulbOn,
    .characteristicType = &kHAPCharacteristicType_On,
    .debugDescription = kHAPCharacteristicDebugDescription_On,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .callbacks = { .handleRead = HandleLightBulbOnRead, .handleWrite = HandleLightBulbOnWrite }
};

/**
 * The Light Bulb service that contains the 'On' characteristic.
 */
const HAPService lightBulbService = {
    .iid = kIID_LightBulb,
    .serviceType = &kHAPServiceType_LightBulb,
    .debugDescription = kHAPServiceDebugDescription_LightBulb,
    .name = "Light Bulb",
    .properties = { .primaryService = false, .hidden = false, .ble = { .supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) { &lightBulbServiceSignatureCharacteristic,
                                                            &lightBulbNameCharacteristic,
                                                            &lightBulbOnCharacteristic,
                                                            &lightBulbCountdownCharacteristic,
                                                            NULL }
};

//----------------------------------------------------------------------------------------------------------------------
/* The fan service / characteristics
 * 
 * Fan service
 *  - Service Signature Characteristic - is this required? ****
 *  - Name characteristic
 *  - Active characteristic - active/inactive
 *  - Target fan state characteristic - man/auto
 *  - Auto-off timeout custom characteristic
 *  - Duty cycle custom characteristic
 */

/**
 * The 'Service Signature' characteristic of the furnace fan service.
 */
static const HAPDataCharacteristic furnaceFanServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_FurnaceFanServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the furnace fan service.
 */
static const HAPStringCharacteristic furnaceFanNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_FurnaceFanName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = "Furnace fan",
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'Active' charactristic of the furnace fan service.
 */
const HAPUInt8Characteristic furnaceFanActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FurnaceFanActive, 
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = "Furnace fan",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 1,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleFurnaceFanActiveOnRead, .handleWrite = HandleFurnaceFanActiveOnWrite }
};

/**
 * The 'TargetFanState' charactristic of the furnace fan service.
 */
static const HAPUInt8Characteristic furnaceFanTargetFanStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FurnaceFanTargetFanState, 
    .characteristicType = &kHAPCharacteristicType_TargetFanState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetFanState,
    .manufacturerDescription = "Furnace fan mode",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 2,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleFurnaceFanTargetFanStateOnRead, .handleWrite = HandleFurnaceFanTargetFanStateOnWrite }
};


const HAPUUID kHAPCustomCharacteristicType_Timeout = {
        { 0xD7, 0x25, 0xD9, 0x1B, 0x30, 0x55, 0x84, 0x9A, 0x9D, 0x48, 0xCC, 0x0A, 0x00, 0x00, 0x00, 0x10}
    } ;
/**
 * The 'Timeout' custom charactristic of the furnace fan service.
 */
static const HAPUInt8Characteristic furnaceFanTimeoutCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FurnaceFanTimeout,
    .characteristicType = &kHAPCustomCharacteristicType_Timeout,
    .debugDescription = "fan.timeout",
    .manufacturerDescription = "Fan timeout (min)",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 180,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleFurnaceFanTimeoutOnRead, .handleWrite = HandleFurnaceFanTimeoutOnWrite }
} ;

const HAPUUID kHAPCustomCharacteristicType_DutyCycle = {
        { 0xD7, 0x25, 0xD9, 0x1B, 0x30, 0x55, 0x84, 0x9A, 0x9D, 0x48, 0xCC, 0x0A, 0x00, 0x00, 0x00, 0x11}
    } ;

/**
 * The 'Duty Cycle' custom charactristic of the furnace fan service.
 */
static const HAPUInt8Characteristic furnaceFanDutyCycleCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_FurnaceFanDutyCycle,
    .characteristicType = &kHAPCustomCharacteristicType_DutyCycle,
    .debugDescription = "fan.duty.cycle",
    .manufacturerDescription = "Fan duty cycle",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .units = kHAPCharacteristicUnits_Percentage,
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 100,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleFurnaceFanDutyCycleOnRead, .handleWrite = HandleFurnaceFanDutyCycleOnWrite }
} ;

const HAPService furnaceFanService = {
    .iid = kIID_FurnaceFan,
    .serviceType = &kHAPServiceType_Fan,
    .debugDescription = "Furnace fan",
    .name = "Furnace fan",
    .properties = {.primaryService = true, .hidden = false, .ble = {.supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) {
        &furnaceFanServiceSignatureCharacteristic,
        &furnaceFanNameCharacteristic,
        &furnaceFanActiveCharacteristic,
        &furnaceFanTargetFanStateCharacteristic,
        &furnaceFanTimeoutCharacteristic,
        &furnaceFanDutyCycleCharacteristic,
        NULL }
};


//----------------------------------------------------------------------------------------------------------------------
/* The HRV service / characteristics
 * 
 * HRV  service
 *  - Service Signature Characteristic - is this required? ****
 *  - Name characteristic
 *  - Active characteristic - active/inactive
 *  - Target fan state characteristic - man/auto
 */

/**
 * The 'Service Signature' characteristic of the HRV fan service.
 */
static const HAPDataCharacteristic hrvServiceSignatureCharacteristic = {
    .format = kHAPCharacteristicFormat_Data,
    .iid = kIID_HrvServiceSignature,
    .characteristicType = &kHAPCharacteristicType_ServiceSignature,
    .debugDescription = kHAPCharacteristicDebugDescription_ServiceSignature,
    .manufacturerDescription = NULL,
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = true },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 2097152 },
    .callbacks = { .handleRead = HAPHandleServiceSignatureRead, .handleWrite = NULL }
};

/**
 * The 'Name' characteristic of the furnace HRV service.
 */
static const HAPStringCharacteristic hrvNameCharacteristic = {
    .format = kHAPCharacteristicFormat_String,
    .iid = kIID_HrvName,
    .characteristicType = &kHAPCharacteristicType_Name,
    .debugDescription = kHAPCharacteristicDebugDescription_Name,
    .manufacturerDescription = "HRV",
    .properties = { .readable = true,
                    .writable = false,
                    .supportsEventNotification = false,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = false,
                             .supportsDisconnectedNotification = false,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = { .maxLength = 64 },
    .callbacks = { .handleRead = HAPHandleNameRead, .handleWrite = NULL }
};

/**
 * The 'Active' charactristic of the HRV service.
 */
const HAPUInt8Characteristic hrvActiveCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HrvActive, 
    .characteristicType = &kHAPCharacteristicType_Active,
    .debugDescription = kHAPCharacteristicDebugDescription_Active,
    .manufacturerDescription = "HRV fan",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 1,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleHrvActiveOnRead, .handleWrite = HandleHrvActiveOnWrite }
};

/**
 * The 'TargetFanState' charactristic of the HRV service.
 */
static const HAPUInt8Characteristic hrvTargetFanStateCharacteristic = {
    .format = kHAPCharacteristicFormat_UInt8,
    .iid = kIID_HrvTargetFanState, 
    .characteristicType = &kHAPCharacteristicType_TargetFanState,
    .debugDescription = kHAPCharacteristicDebugDescription_TargetFanState,
    .manufacturerDescription = "HRV fan mode",
    .properties = { .readable = true,
                    .writable = true,
                    .supportsEventNotification = true,
                    .hidden = false,
                    .requiresTimedWrite = false,
                    .supportsAuthorizationData = false,
                    .ip = { .controlPoint = false, .supportsWriteResponse = false },
                    .ble = { .supportsBroadcastNotification = true,
                             .supportsDisconnectedNotification = true,
                             .readableWithoutSecurity = false,
                             .writableWithoutSecurity = false } },
    .constraints = {
        .minimumValue = 0,
        .maximumValue = 2,
        .stepValue = 1,
    },
    .callbacks = { .handleRead = HandleHrvTargetFanStateOnRead, .handleWrite = HandleHrvTargetFanStateOnWrite }
};

const HAPService hrvService = {
    .iid = kIID_Hrv,
    .serviceType = &kHAPServiceType_Fan,
    .debugDescription = "HRV",
    .name = "HRV",
    .properties = {.primaryService = false, .hidden = false, .ble = {.supportsConfiguration = false } },
    .linkedServices = NULL,
    .characteristics = (const HAPCharacteristic* const[]) {
        &hrvServiceSignatureCharacteristic,
        &hrvNameCharacteristic,
        &hrvActiveCharacteristic,
        &hrvTargetFanStateCharacteristic,
        NULL }
};
