// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

// An example that implements the light bulb HomeKit profile. It can serve as a basic implementation for
// any platform. The accessory logic implementation is reduced to internal state updates and log output.
//
// This implementation is platform-independent.
//
// The code consists of multiple parts:
//
//   1. The definition of the accessory configuration and its internal state.
//
//   2. Helper functions to load and save the state of the accessory.
//
//   3. The definitions for the HomeKit attribute database.
//
//   4. The callbacks that implement the actual behavior of the accessory, in this
//      case here they merely access the global accessory state variable and write
//      to the log to make the behavior easily observable.
//
//   5. The initialization of the accessory state.
//
//   6. Callbacks that notify the server in case their associated value has changed.

#include "HAP.h"

#include "App.h"
#include "DB.h"

#include "driver/gpio.h"
#include "esp_sntp.h"
#include "time.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define GPIO_OUTPUT_IO_FURNACE_FAN    32
#define GPIO_OUTPUT_IO_HRV    33
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_FURNACE_FAN) | (1ULL<<GPIO_OUTPUT_IO_HRV))
/**
 * Domain used in the key value store for application data.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreDomain_Configuration ((HAPPlatformKeyValueStoreDomain) 0x00)

/**
 * Key used in the key value store to store the configuration state.
 *
 * Purged: On factory reset.
 */
#define kAppKeyValueStoreKey_Configuration_State ((HAPPlatformKeyValueStoreDomain) 0x00)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const HAPLogObject logObject = { .subsystem = NULL, .category = NULL };

/**
 * Global accessory configuration.
 */
const uint8_t STATE_VERSION = 1;
typedef struct {
    struct {
        uint8_t version;

        bool lightBulbOn;

        bool fanActiveManual;
        bool fanActiveAuto;
        uint8_t fanTargetState;
        uint8_t fanTimeoutMinutes;
        uint8_t fanDutyCycle;

        bool hrvActive;
        uint8_t hrvTargetState;
    } state;
    HAPAccessoryServerRef* server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

//----------------------------------------------------------------------------------------------------------------------

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;

    // Load persistent state if available
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state,
            &numBytes,
            &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state || accessoryConfiguration.state.version != STATE_VERSION) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
        accessoryConfiguration.state.version = STATE_VERSION;
        accessoryConfiguration.state.fanTargetState = kHAPCharacteristicValue_TargetFanState_Manual;
        accessoryConfiguration.state.fanDutyCycle = 10;
        accessoryConfiguration.state.fanTimeoutMinutes = 60;
    }
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(void) {
    HAPPrecondition(accessoryConfiguration.keyValueStore);

    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
            accessoryConfiguration.keyValueStore,
            kAppKeyValueStoreDomain_Configuration,
            kAppKeyValueStoreKey_Configuration_State,
            &accessoryConfiguration.state,
            sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * HomeKit accessory that provides the Light Bulb service.
 *
 * Note: Not constant to enable BCT Manual Name Change.
 */
static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Lighting,
                                  .name = "ESP32 Thermostat",
                                  .manufacturer = "Cleobis",
                                  .model = "Thermostat1,1",
                                  .serialNumber = "0001",
                                  .firmwareVersion = "5",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &lightBulbService,
                                                                            &furnaceFanService,
                                                                            &hrvService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessory } };

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPAccessoryIdentifyRequest* request HAP_UNUSED,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s", __func__);
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPBoolCharacteristicReadRequest* request HAP_UNUSED,
        bool* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.lightBulbOn;
    HAPLog(&logObject, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnWrite(
        HAPAccessoryServerRef* server,
        const HAPBoolCharacteristicWriteRequest* request,
        bool value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.lightBulbOn != value) {
        accessoryConfiguration.state.lightBulbOn = value;
        gpio_set_level(GPIO_OUTPUT_IO_FURNACE_FAN, value);
        gpio_set_level(GPIO_OUTPUT_IO_HRV, value);

        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

        SaveAccessoryState();

        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

void UpdateOutputsAndNotify() {
    static bool fanActiveCache;
    static bool hrvActiveCache;

    bool fanActiveNew = accessoryConfiguration.state.fanActiveManual
        || accessoryConfiguration.state.fanActiveAuto;
    bool hrvActiveNew = accessoryConfiguration.state.hrvActive
        || (accessoryConfiguration.state.fanActiveAuto 
            && (accessoryConfiguration.state.hrvTargetState == kHAPCharacteristicValue_TargetFanState_Auto));

    gpio_set_level(GPIO_OUTPUT_IO_FURNACE_FAN, fanActiveNew);
    gpio_set_level(GPIO_OUTPUT_IO_HRV, hrvActiveNew);

    if (fanActiveNew != fanActiveCache) {
        HAPLog(&logObject, "Setting fan %s. Manual demand = %s. Auto demand = %s.", 
            fanActiveNew ? "on" : "off", 
            accessoryConfiguration.state.fanActiveManual ? "true" : "false",
            accessoryConfiguration.state.fanActiveAuto ? "true" : "false");
        fanActiveCache = fanActiveNew;
        HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, &furnaceFanActiveCharacteristic, &furnaceFanService, &accessory);
    } 

    if (hrvActiveNew != hrvActiveCache) {
        HAPLog(&logObject, "Setting HRV %s. Manual demand = %s. Mode = %s.", 
            hrvActiveNew ? "on" : "off", 
            accessoryConfiguration.state.hrvActive ? "true" : "false",
            (accessoryConfiguration.state.hrvTargetState == kHAPCharacteristicValue_TargetFanState_Auto) ? "auto" : "manual");
        hrvActiveCache = hrvActiveNew;
        HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, &hrvActiveCharacteristic, &hrvService, &accessory);
    }
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanActiveOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.fanActiveAuto || accessoryConfiguration.state.fanActiveManual;
    HAPLog(&logObject, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanActiveOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %s", __func__, value ? "true" : "false");

    bool changed = false;
    if (value) {
        if (!accessoryConfiguration.state.fanActiveManual) {
            changed = true;
            accessoryConfiguration.state.fanActiveManual = true;
        }
    } else {
        // value == false
        bool effectiveValue = accessoryConfiguration.state.fanActiveManual
            || accessoryConfiguration.state.fanActiveAuto;
        if (effectiveValue) {
            // Force fan off if it was auto-on. Force HRV off.
            changed = true;
            accessoryConfiguration.state.fanActiveManual = false;
            accessoryConfiguration.state.fanActiveAuto = false;
            accessoryConfiguration.state.hrvActive = false;
        }
    }

    if (changed) {
        SaveAccessoryState();
        UpdateOutputsAndNotify();
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanTargetFanStateOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.fanTargetState;
    HAPLog(&logObject, "%s: %u", __func__, *value);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanTargetFanStateOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %u", __func__, value);

    if (accessoryConfiguration.state.fanTargetState != value) {
        accessoryConfiguration.state.fanTargetState = value;
        SaveAccessoryState();
        UpdateOutputsAndNotify();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanTimeoutOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.fanTimeoutMinutes;
    HAPLog(&logObject, "%s: %u", __func__, *value);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanTimeoutOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %u", __func__, value);

    if (accessoryConfiguration.state.fanTimeoutMinutes != value) {
        accessoryConfiguration.state.fanTimeoutMinutes = value;
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanDutyCycleOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.fanDutyCycle;
    HAPLog(&logObject, "%s: %u", __func__, *value);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFurnaceFanDutyCycleOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %u", __func__, value);

    if (accessoryConfiguration.state.fanDutyCycle != value) {
        accessoryConfiguration.state.fanDutyCycle = value;
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------

HAP_RESULT_USE_CHECK
HAPError HandleHrvActiveOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.hrvActive;
    HAPLog(&logObject, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHrvActiveOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %s", __func__, value ? "true" : "false");

    bool oldValue = accessoryConfiguration.state.hrvActive;
    if (oldValue != value) {
        accessoryConfiguration.state.hrvActive = value;
        if (value) {
            // Automatically turn on fan
            accessoryConfiguration.state.fanActiveManual = true;
        }
        SaveAccessoryState();
        UpdateOutputsAndNotify();
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHrvTargetFanStateOnRead(
        HAPAccessoryServerRef* server HAP_UNUSED,
        const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
        uint8_t* value,
        void* _Nullable context HAP_UNUSED) {
    *value = accessoryConfiguration.state.hrvTargetState;
    HAPLog(&logObject, "%s: %u", __func__, *value);

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHrvTargetFanStateOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "%s: %u", __func__, value);

    if (accessoryConfiguration.state.hrvTargetState != value) {
        accessoryConfiguration.state.hrvTargetState = value;
        SaveAccessoryState();
        UpdateOutputsAndNotify();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }

    return kHAPError_None;
}

//----------------------------------------------------------------------------------------------------------------------


void AccessoryNotification(
        const HAPAccessory* accessory,
        const HAPService* service,
        const HAPCharacteristic* characteristic,
        void* ctx) {
    HAPLogInfo(&logObject, "Accessory Notification");

    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

void AppCreate(HAPAccessoryServerRef* server, HAPPlatformKeyValueStoreRef keyValueStore) {
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&logObject, "%s", __func__);

    // Set-up output
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_FURNACE_FAN, 0);
    gpio_set_level(GPIO_OUTPUT_IO_HRV, 0);

    // Set-up time server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    
    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void AppRelease(void) {
}

void AppAccessoryServerStart(void) {
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

//----------------------------------------------------------------------------------------------------------------------

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef* server, void* _Nullable context) {
    HAPPrecondition(server);
    HAPPrecondition(!context);

    switch (HAPAccessoryServerGetState(server)) {
        case kHAPAccessoryServerState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
            return;
        }
        case kHAPAccessoryServerState_Running: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
            return;
        }
        case kHAPAccessoryServerState_Stopping: {
            HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
            return;
        }
    }
    HAPFatalError();
}

const HAPAccessory* AppGetAccessoryInfo() {
    return &accessory;
}

void AppInitialize(
        HAPAccessoryServerOptions* hapAccessoryServerOptions,
        HAPPlatform* hapPlatform,
        HAPAccessoryServerCallbacks* hapAccessoryServerCallbacks) {
    /*no-op*/
}

void AppDeinitialize() {
    /*no-op*/
}
