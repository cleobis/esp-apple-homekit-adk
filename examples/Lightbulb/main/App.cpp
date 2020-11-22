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
#include "esp_timer.h"
#include "time.h"

void UpdateOutputsAndNotify();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const gpio_num_t GPIO_OUTPUT_IO_FURNACE_FAN = GPIO_NUM_32;
const gpio_num_t GPIO_OUTPUT_IO_HRV = GPIO_NUM_33;
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_OUTPUT_IO_FURNACE_FAN) | (1ULL<<GPIO_OUTPUT_IO_HRV))
const bool INVERT_OUTPUTS = true;
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

class Timer {
    protected:
    HAPPlatformTimerRef timer;
    const uint64_t TICKS_PER_MIN = 1000*60; //HAPPlatformTimer uses milliseconds
    const HAPPlatformTimerRef NULL_TIMER = 0;
    uint64_t start_ticks;
    uint64_t stop_ticks;
    uint64_t timeout_ticks;
    bool running;

    public:
    Timer() {
        timer = NULL_TIMER;
        running = false;
    }

    void init() {
        if (timer != NULL_TIMER)
            return;
        // No-op with HAPPlatformTimer
    }

    ~Timer() {
        stop();
        if (timer != NULL_TIMER){
            timer = NULL_TIMER;
        }
    }

    void stop() {
        if (running) {
            running = false;
            
           HAPPlatformTimerDeregister(timer);
           timer = NULL_TIMER;
        }
    }

    protected:
    virtual void callback() = 0;
    
    void start_once(uint64_t _timeout_ticks) {
        init();

        stop();

        timeout_ticks = _timeout_ticks;
        
        start_ticks = HAPPlatformClockGetCurrent();
        stop_ticks = start_ticks + timeout_ticks;
        HAPLog(&logObject, "Timer start_once will fire in %g min", ((float)timeout_ticks) / TICKS_PER_MIN);
        HAPError err = HAPPlatformTimerRegister(&timer, stop_ticks, &Timer::static_callback, this);
        if (err != kHAPError_None) {
            HAPLogError(&logObject, "Unable to create timer.");
            HAPFatalError();
        }
       running = true;
    }

    virtual void update_timeout(uint64_t _timeout_ticks) {
        if (!running)
            return;
        
        stop();

        uint64_t now = HAPPlatformClockGetCurrent();

        timeout_ticks = _timeout_ticks;
        stop_ticks = start_ticks + timeout_ticks;
        if (stop_ticks < now + 1000) {
            // Timer should have already fired.
            // Add some buffer so we don't start a timer that will immediately fire.
            HAPLog(&logObject, "Timer timeout changed. Turning off.");
            callback();
        } else {
            // Restart timer with new timeout
            timeout_ticks = stop_ticks - now;
            HAPLog(&logObject, "Timer timeout changed. Staring new timer for %g min.", ((float)timeout_ticks) / TICKS_PER_MIN);
            start_once(timeout_ticks);
        }
    }
    
    public:
    static void static_callback(HAPPlatformTimerRef _, void* _t) {
        HAPLog(&logObject, "In static callback");
        Timer *t = (Timer*) _t;
        t->running = false;
        t->callback();
    }
} ;

class AutoOffTimer : Timer {
    protected:
    void callback() {
        HAPLog(&logObject, "AutoOffTimer turning outputs off");
        accessoryConfiguration.state.fanActiveManual = false;
        accessoryConfiguration.state.hrvActive = false;
        UpdateOutputsAndNotify();
    }

    public:
    void start() {
        HAPLog(&logObject, "AutoOffTimer starting");
        timeout_ticks = accessoryConfiguration.state.fanTimeoutMinutes * TICKS_PER_MIN;
        this->start_once(timeout_ticks);
    }

    void update_timeout() {
        HAPLog(&logObject, "AutoOffTimer updating timeout");
        timeout_ticks = accessoryConfiguration.state.fanTimeoutMinutes * TICKS_PER_MIN;
        Timer::update_timeout(timeout_ticks);
    }
};
static AutoOffTimer autoOffTimer;

class DutyCycleTimer : Timer {
    private:
    int minutes_start = 0;
    bool fanActive = false; // Need to track seperately from accessoryConfiguration as they could manually turn off a cycle.

    protected:
    void callback() {
        // Change the fan state
        HAPLog(&logObject, "In DutyCycleTimer");
        if (fanActive) {
            // turn the fan off
            HAPLog(&logObject, "  Turning off");
            fanActive = accessoryConfiguration.state.fanActiveAuto = false;
        } else {
            if (accessoryConfiguration.state.fanDutyCycle > 0) {
                HAPLog(&logObject, "  Turning on");
                // turn the fan on
                fanActive = accessoryConfiguration.state.fanActiveAuto = true;
            } else {
                // Don't turn on the fan but keep timer running in case the duty cycle changes.
                HAPLog(&logObject, "  suppressed");
            }
        }
        UpdateOutputsAndNotify();

        start_next();
    }

    void start_next() {
        if (fanActive) {
            // timeout is based on duty cycle.
            // Convert duty cycle in % of hour to microseconds
            timeout_ticks = accessoryConfiguration.state.fanDutyCycle * TICKS_PER_MIN * 60 / 100;
        } else {
            // timeout determined by minutes
            time_t now_raw = time(NULL);
            struct tm *now;
            now = localtime (&now_raw);
            float timeout = minutes_start - (now->tm_min + now->tm_sec/60.0f);
            if (timeout <= 0)
                timeout += 60;
            timeout_ticks = (uint64_t)(timeout * TICKS_PER_MIN);
        }
        HAPLog(&logObject, "DutyCycle fan is %s. Will toggle in %g min.",
            fanActive ? "on" : "off", ((float)timeout_ticks) / TICKS_PER_MIN);
        start_once(timeout_ticks);
    }

    public:
    void start() {
        start_next();
    }

    void time_changed_callback() {
        HAPLog(&logObject, "Time changed. Fan auto cycling is currently %s.", fanActive ? "on" : "off");
        if (fanActive) {
            // Do nothing
        } else {
            stop() ;
            start_next();
        }
    }

    void duty_cycle_changed_callback() {
        HAPLog(&logObject, "DutyCycle changed. Fans are currently %s.", fanActive ? "on" : "off");
        if (fanActive) {
            timeout_ticks = accessoryConfiguration.state.fanDutyCycle * TICKS_PER_MIN * 60 / 100;
            Timer::update_timeout(timeout_ticks);
        } else {
            // Do nothing
        }
    }
};
static DutyCycleTimer dutyCycleTimer;

void sntp_sync_callback(struct timeval *tv) {
    dutyCycleTimer.time_changed_callback();
}

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
    accessoryConfiguration.state.fanActiveAuto = false;
    accessoryConfiguration.state.fanActiveManual = false;
    accessoryConfiguration.state.hrvActive = false;
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
                                  .category = kHAPAccessoryCategory_Fans,
                                  .name = "ESP32 Thermostat",
                                  .manufacturer = "Cleobis",
                                  .model = "Thermostat1,1",
                                  .serialNumber = "0001",
                                  .firmwareVersion = "5",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService* const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
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

//----------------------------------------------------------------------------------------------------------------------

inline bool DutyCycleEnabledEffective() {
    return accessoryConfiguration.state.fanActiveAuto
        && (accessoryConfiguration.state.fanTargetState == kHAPCharacteristicValue_TargetFanState_Auto);
}
inline bool FanActiveEffective() {
    return accessoryConfiguration.state.fanActiveManual || DutyCycleEnabledEffective();
}
inline bool HrvActiveEffective() {
    return accessoryConfiguration.state.hrvActive
    || (DutyCycleEnabledEffective() && (accessoryConfiguration.state.hrvTargetState == kHAPCharacteristicValue_TargetFanState_Auto));
}

void UpdateOutputsAndNotify() {
    static bool fanActiveCache;
    static bool hrvActiveCache;

    bool fanActiveNew = FanActiveEffective();
    bool hrvActiveNew = HrvActiveEffective();

    gpio_set_level(GPIO_OUTPUT_IO_FURNACE_FAN, fanActiveNew ^ INVERT_OUTPUTS);
    gpio_set_level(GPIO_OUTPUT_IO_HRV, hrvActiveNew ^ INVERT_OUTPUTS);

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
    *value = FanActiveEffective();
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
        // Even if the value doesn't change, sending an on message should restart the countdown.
        autoOffTimer.start();
    } else {
        // value == false
        if (FanActiveEffective()) {
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
    HAPLogInfo(&logObject, "%s: %u", __func__, *value);

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
    HAPLogInfo(&logObject, "%s: %u", __func__, *value);

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
        autoOffTimer.update_timeout();
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
    HAPLogInfo(&logObject, "%s: %u", __func__, *value);

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
        dutyCycleTimer.duty_cycle_changed_callback();
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
    *value = HrvActiveEffective();
    HAPLog(&logObject, "%s: %s", __func__, *value ? "true" : "false");

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleHrvActiveOnWrite(
        HAPAccessoryServerRef* server,
        const HAPUInt8CharacteristicWriteRequest* request,
        uint8_t value,
        void* _Nullable context HAP_UNUSED) {
    HAPLog(&logObject, "****************");
    HAPLog(&logObject, "%s: %s", __func__, value ? "true" : "false");

    bool oldValue = HrvActiveEffective();
    if (oldValue != value) {
        HAPLog(&logObject, "  Value changed");
        accessoryConfiguration.state.hrvActive = value;
        if (value) {
            HAPLog(&logObject, "  Turning on fan and timer.");
            // Automatically turn on fan
            accessoryConfiguration.state.fanActiveManual = true;
            autoOffTimer.start();
        } else {
            if (HrvActiveEffective()) {
                // Was turned on by Duty cycle. Turn off current duty cycle.
                accessoryConfiguration.state.fanActiveAuto = false;
            }
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
    HAPLogInfo(&logObject, "%s: %u", __func__, *value);

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
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_FURNACE_FAN, INVERT_OUTPUTS);
    gpio_set_level(GPIO_OUTPUT_IO_HRV, INVERT_OUTPUTS);

    // Set-up time server
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    sntp_set_time_sync_notification_cb(&sntp_sync_callback);
    
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
