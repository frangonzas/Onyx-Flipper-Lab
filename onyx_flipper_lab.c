/**
 * @file onyx_flipper_lab.c
 * @brief Onyx Flipper Lab v2.0 - auditoria defensiva para Flipper Zero.
 *
 * Fran Gonzas
 *
 * Alcance:
 * - GPIO: lectura ADC / entrada, sin salida activa.
 * - Sub-GHz: monitor RSSI/LQI en RX, sin transmision.
 * - Bluetooth: postura/estado/capacidades locales, sin cambiar perfiles.
 * - Infrarrojo: recepcion/decodificacion, sin transmision.
 * - NFC: deteccion de protocolos, sin emulacion/escritura.
 * - LF RFID: lectura/identificacion, sin escritura/emulacion.
 *
 * Uso exclusivo en hardware y entornos propios o expresamente autorizados.
 */

#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_bt.h>
#include <furi_hal_random.h>

#include <gui/gui.h>
#include <input/input.h>

#include <infrared.h>
#include <infrared_worker.h>

#include <nfc/nfc.h>
#include <nfc/nfc_scanner.h>
#include <nfc/protocols/nfc_protocol.h>

#include <lfrfid/lfrfid_worker.h>
#include <lfrfid/protocols/lfrfid_protocols.h>
#include <toolbox/protocols/protocol_dict.h>

#include <lib/subghz/devices/devices.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TAG "OnyxFlipperLab"

#define ONYX_MENU_COUNT 10U
#define ONYX_MENU_VISIBLE 5U
#define ONYX_PASSWORD_LENGTH 18U
#define ONYX_RANDOM_BYTES 8U
#define ONYX_NFC_PROTOCOL_MAX 4U
#define ONYX_LFRFID_DATA_MAX 8U

typedef enum {
    OnyxScreenMenu = 0,
    OnyxScreenGpioAudit,
    OnyxScreenSubGhzAudit,
    OnyxScreenBluetoothAudit,
    OnyxScreenInfraredAudit,
    OnyxScreenNfcAudit,
    OnyxScreenLfRfidAudit,
    OnyxScreenPassword,
    OnyxScreenRandomHex,
    OnyxScreenSecurityTips,
    OnyxScreenAbout,
} OnyxScreen;

typedef struct {
    OnyxScreen screen;
    uint8_t menu_index;
    uint8_t tips_page;

    char password[ONYX_PASSWORD_LENGTH + 1U];
    uint8_t random_bytes[ONYX_RANDOM_BYTES];

    // GPIO auditor
    FuriHalAdcHandle* adc_handle;
    size_t gpio_adc_index;
    uint16_t gpio_raw;
    uint16_t gpio_mv;
    bool gpio_ready;

    // Sub-GHz auditor
    const SubGhzDevice* subghz_device;
    size_t subghz_frequency_index;
    uint32_t subghz_frequency;
    float subghz_rssi;
    float subghz_peak_rssi;
    uint8_t subghz_lqi;
    bool subghz_ready;

    // Infrarrojo
    InfraredWorker* ir_worker;
    bool ir_running;
    bool ir_has_signal;
    bool ir_decoded;
    char ir_protocol[20];
    uint32_t ir_address;
    uint32_t ir_command;
    bool ir_repeat;
    size_t ir_raw_count;
    uint32_t ir_signal_count;

    // NFC
    Nfc* nfc;
    NfcScanner* nfc_scanner;
    bool nfc_running;
    bool nfc_detected;
    size_t nfc_protocol_count;
    NfcProtocol nfc_protocols[ONYX_NFC_PROTOCOL_MAX];
    uint32_t nfc_detection_count;

    // LF RFID
    ProtocolDict* lfrfid_dict;
    LFRFIDWorker* lfrfid_worker;
    bool lfrfid_running;
    bool lfrfid_found;
    char lfrfid_protocol_name[24];
    uint8_t lfrfid_data[ONYX_LFRFID_DATA_MAX];
    size_t lfrfid_data_size;
    uint32_t lfrfid_detection_count;

    FuriMutex* mutex;
} OnyxState;

static const char* const onyx_menu_items[ONYX_MENU_COUNT] = {
    "GPIO auditor",
    "SubGHz RSSI",
    "Bluetooth audit",
    "IR inspector",
    "NFC detector",
    "LF RFID reader",
    "Password generator",
    "Random HEX",
    "Security tips",
    "About",
};

static const uint32_t onyx_subghz_frequencies[] = {
    315000000U,
    390000000U,
    433920000U,
    868350000U,
    915000000U,
};

static uint32_t onyx_random_bounded(uint32_t upper_bound) {
    furi_assert(upper_bound > 0U);

    const uint64_t random_range = (uint64_t)UINT32_MAX + 1ULL;
    const uint64_t limit = random_range - (random_range % upper_bound);

    uint32_t value;
    do {
        value = furi_hal_random_get();
    } while((uint64_t)value >= limit);

    return value % upper_bound;
}

static void onyx_generate_password(char* output) {
    static const char charset[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZ"
        "abcdefghijkmnopqrstuvwxyz"
        "23456789"
        "!@#$%&*+-_?";

    const uint32_t charset_length = (uint32_t)(sizeof(charset) - 1U);

    for(size_t i = 0; i < ONYX_PASSWORD_LENGTH; i++) {
        output[i] = charset[onyx_random_bounded(charset_length)];
    }

    output[ONYX_PASSWORD_LENGTH] = '\0';
}

static void onyx_generate_random_bytes(uint8_t* output) {
    furi_hal_random_fill_buf(output, ONYX_RANDOM_BYTES);
}

static const char* onyx_nfc_protocol_name(NfcProtocol protocol) {
    switch(protocol) {
    case NfcProtocolIso14443_3a:
        return "ISO14443-3A";
    case NfcProtocolIso14443_3b:
        return "ISO14443-3B";
    case NfcProtocolIso14443_4a:
        return "ISO14443-4A";
    case NfcProtocolIso14443_4b:
        return "ISO14443-4B";
    case NfcProtocolIso15693_3:
        return "ISO15693";
    case NfcProtocolFelica:
        return "FeliCa";
    case NfcProtocolMfUltralight:
        return "Mifare UL";
    case NfcProtocolMfClassic:
        return "Mifare Classic";
    case NfcProtocolMfPlus:
        return "Mifare Plus";
    case NfcProtocolMfDesfire:
        return "DESFire";
    case NfcProtocolSlix:
        return "SLIX";
    case NfcProtocolSt25tb:
        return "ST25TB";
    default:
        return "Unknown";
    }
}

static void onyx_draw_header(Canvas* canvas, const char* title) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, title);
    canvas_draw_line(canvas, 0, 13, 127, 13);
}

static void onyx_draw_footer(Canvas* canvas, const char* text) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 63, text);
}

static void onyx_draw_menu(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "ONYX AUDIT LAB");
    canvas_set_font(canvas, FontSecondary);

    uint8_t first = 0U;
    if(state->menu_index >= ONYX_MENU_VISIBLE) {
        first = (uint8_t)(state->menu_index - (ONYX_MENU_VISIBLE - 1U));
    }

    char line[30];
    for(uint8_t row = 0; row < ONYX_MENU_VISIBLE; row++) {
        const uint8_t index = (uint8_t)(first + row);
        if(index >= ONYX_MENU_COUNT) break;

        snprintf(
            line,
            sizeof(line),
            "%c %s",
            (index == state->menu_index) ? '>' : ' ',
            onyx_menu_items[index]);

        canvas_draw_str(canvas, 2, (uint8_t)(22U + row * 9U), line);
    }
}

static const GpioPinRecord* onyx_gpio_get_adc_pin(size_t logical_index, size_t* count_out) {
    size_t count = 0U;
    const GpioPinRecord* result = NULL;

    for(size_t i = 0; i < gpio_pins_count; i++) {
        if(gpio_pins[i].debug) continue;
        if(gpio_pins[i].channel == FuriHalAdcChannelNone) continue;

        if(count == logical_index) result = &gpio_pins[i];
        count++;
    }

    if(count_out) *count_out = count;
    return result;
}

static void onyx_gpio_sample(OnyxState* state) {
    if(!state->gpio_ready || !state->adc_handle) return;

    size_t count = 0U;
    const GpioPinRecord* pin = onyx_gpio_get_adc_pin(state->gpio_adc_index, &count);
    if(!pin || count == 0U) return;

    furi_hal_gpio_init(pin->pin, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    const uint16_t raw = furi_hal_adc_read(state->adc_handle, pin->channel);
    const uint16_t mv = furi_hal_adc_convert_to_voltage(state->adc_handle, raw);

    furi_mutex_acquire(state->mutex, FuriWaitForever);
    state->gpio_raw = raw;
    state->gpio_mv = mv;
    furi_mutex_release(state->mutex);
}

static void onyx_gpio_start(OnyxState* state) {
    state->adc_handle = furi_hal_adc_acquire();
    if(!state->adc_handle) {
        state->gpio_ready = false;
        return;
    }

    furi_hal_adc_configure(state->adc_handle);

    size_t count = 0U;
    onyx_gpio_get_adc_pin(0U, &count);
    state->gpio_adc_index = 0U;
    state->gpio_ready = (count > 0U);

    onyx_gpio_sample(state);
}

static void onyx_gpio_stop(OnyxState* state) {
    if(state->adc_handle) {
        furi_hal_adc_release(state->adc_handle);
        state->adc_handle = NULL;
    }
    state->gpio_ready = false;
}

static void onyx_gpio_move(OnyxState* state, int direction) {
    size_t count = 0U;
    onyx_gpio_get_adc_pin(0U, &count);
    if(count == 0U) return;

    if(direction > 0) {
        state->gpio_adc_index = (state->gpio_adc_index + 1U) % count;
    } else {
        state->gpio_adc_index =
            (state->gpio_adc_index == 0U) ? (count - 1U) : (state->gpio_adc_index - 1U);
    }

    onyx_gpio_sample(state);
}

static void onyx_draw_gpio(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "GPIO AUDITOR");
    canvas_set_font(canvas, FontSecondary);

    size_t count = 0U;
    const GpioPinRecord* pin = onyx_gpio_get_adc_pin(state->gpio_adc_index, &count);

    char line[32];
    if(!state->gpio_ready || !pin) {
        canvas_draw_str(canvas, 2, 28, "ADC unavailable");
    } else {
        snprintf(line, sizeof(line), "Pin: %s (%u/%u)", pin->name, (unsigned)(state->gpio_adc_index + 1U), (unsigned)count);
        canvas_draw_str(canvas, 2, 24, line);

        snprintf(line, sizeof(line), "Voltage: %u mV", (unsigned)state->gpio_mv);
        canvas_draw_str(canvas, 2, 37, line);

        snprintf(line, sizeof(line), "Raw ADC: %u", (unsigned)state->gpio_raw);
        canvas_draw_str(canvas, 2, 50, line);
    }

    onyx_draw_footer(canvas, "< >:pin OK:sample Back");
}

static bool onyx_subghz_apply_frequency(OnyxState* state) {
    if(!state->subghz_ready || !state->subghz_device) return false;

    const size_t count = COUNT_OF(onyx_subghz_frequencies);
    for(size_t attempt = 0; attempt < count; attempt++) {
        uint32_t requested = onyx_subghz_frequencies[state->subghz_frequency_index];

        if(subghz_devices_is_frequency_valid(state->subghz_device, requested)) {
            subghz_devices_idle(state->subghz_device);
            state->subghz_frequency =
                subghz_devices_set_frequency(state->subghz_device, requested);
            subghz_devices_set_rx(state->subghz_device);
            state->subghz_peak_rssi = -120.0f;
            return true;
        }

        state->subghz_frequency_index = (state->subghz_frequency_index + 1U) % count;
    }

    return false;
}

static void onyx_subghz_start(OnyxState* state) {
    subghz_devices_init();

    state->subghz_device = subghz_devices_get_by_name("cc1101_int");
    if(!state->subghz_device || !subghz_devices_begin(state->subghz_device)) {
        state->subghz_ready = false;
        subghz_devices_deinit();
        state->subghz_device = NULL;
        return;
    }

    subghz_devices_reset(state->subghz_device);
    subghz_devices_load_preset(
        state->subghz_device, FuriHalSubGhzPresetOok650Async, NULL);

    state->subghz_frequency_index = 2U;
    state->subghz_ready = true;

    if(!onyx_subghz_apply_frequency(state)) {
        state->subghz_ready = false;
    }
}

static void onyx_subghz_stop(OnyxState* state) {
    if(state->subghz_device) {
        subghz_devices_idle(state->subghz_device);
        subghz_devices_sleep(state->subghz_device);
        subghz_devices_end(state->subghz_device);
        state->subghz_device = NULL;
    }

    if(state->subghz_ready) {
        state->subghz_ready = false;
    }

    subghz_devices_deinit();
}

static void onyx_subghz_move(OnyxState* state, int direction) {
    const size_t count = COUNT_OF(onyx_subghz_frequencies);

    if(direction > 0) {
        state->subghz_frequency_index = (state->subghz_frequency_index + 1U) % count;
    } else {
        state->subghz_frequency_index =
            (state->subghz_frequency_index == 0U) ? (count - 1U) :
                                                    (state->subghz_frequency_index - 1U);
    }

    onyx_subghz_apply_frequency(state);
}

static void onyx_subghz_sample(OnyxState* state) {
    if(!state->subghz_ready || !state->subghz_device) return;

    const float rssi = subghz_devices_get_rssi(state->subghz_device);
    const uint8_t lqi = subghz_devices_get_lqi(state->subghz_device);

    furi_mutex_acquire(state->mutex, FuriWaitForever);
    state->subghz_rssi = rssi;
    if(rssi > state->subghz_peak_rssi) state->subghz_peak_rssi = rssi;
    state->subghz_lqi = lqi;
    furi_mutex_release(state->mutex);
}

static void onyx_draw_subghz(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "SUBGHZ RX AUDIT");
    canvas_set_font(canvas, FontSecondary);

    if(!state->subghz_ready) {
        canvas_draw_str(canvas, 2, 29, "Radio unavailable");
        onyx_draw_footer(canvas, "Back:menu");
        return;
    }

    char line[32];
    snprintf(
        line,
        sizeof(line),
        "Freq: %lu.%03lu MHz",
        (unsigned long)(state->subghz_frequency / 1000000U),
        (unsigned long)((state->subghz_frequency % 1000000U) / 1000U));
    canvas_draw_str(canvas, 2, 24, line);

    snprintf(line, sizeof(line), "RSSI: %.1f dBm", (double)state->subghz_rssi);
    canvas_draw_str(canvas, 2, 36, line);

    snprintf(line, sizeof(line), "Peak: %.1f  LQI:%u", (double)state->subghz_peak_rssi, state->subghz_lqi);
    canvas_draw_str(canvas, 2, 48, line);

    canvas_draw_str(canvas, 2, 58, "RX only / no transmit");
    onyx_draw_footer(canvas, "< >:band Back");
}

static void onyx_draw_bluetooth(Canvas* canvas) {
    onyx_draw_header(canvas, "BLUETOOTH AUDIT");
    canvas_set_font(canvas, FontSecondary);

    const bool alive = furi_hal_bt_is_alive();
    const bool active = furi_hal_bt_is_active();
    const bool gatt = furi_hal_bt_is_gatt_gap_supported();
    const bool testing = furi_hal_bt_is_testing_supported();
    const float rssi = active ? furi_hal_bt_get_rssi() : 0.0f;

    char line[32];
    snprintf(line, sizeof(line), "Core:%s Active:%s", alive ? "OK" : "NO", active ? "YES" : "NO");
    canvas_draw_str(canvas, 2, 24, line);

    snprintf(line, sizeof(line), "GATT/GAP:%s Test:%s", gatt ? "YES" : "NO", testing ? "YES" : "NO");
    canvas_draw_str(canvas, 2, 36, line);

    if(active) {
        snprintf(line, sizeof(line), "Link RSSI: %.1f dBm", (double)rssi);
        canvas_draw_str(canvas, 2, 48, line);
    } else {
        canvas_draw_str(canvas, 2, 48, "No active BLE link");
    }

    canvas_draw_str(canvas, 2, 58, "No profile changes");
    onyx_draw_footer(canvas, "Back:menu");
}

static void onyx_ir_received_callback(void* context, InfraredWorkerSignal* received_signal) {
    furi_assert(context);
    OnyxState* state = context;

    furi_mutex_acquire(state->mutex, FuriWaitForever);

    state->ir_has_signal = true;
    state->ir_signal_count++;

    if(infrared_worker_signal_is_decoded(received_signal)) {
        const InfraredMessage* message = infrared_worker_get_decoded_signal(received_signal);

        state->ir_decoded = true;
        snprintf(
            state->ir_protocol,
            sizeof(state->ir_protocol),
            "%s",
            infrared_get_protocol_name(message->protocol));
        state->ir_address = message->address;
        state->ir_command = message->command;
        state->ir_repeat = message->repeat;
        state->ir_raw_count = 0U;
    } else {
        const uint32_t* timings = NULL;
        size_t timings_count = 0U;
        infrared_worker_get_raw_signal(received_signal, &timings, &timings_count);
        UNUSED(timings);

        state->ir_decoded = false;
        snprintf(state->ir_protocol, sizeof(state->ir_protocol), "RAW");
        state->ir_raw_count = timings_count;
    }

    furi_mutex_release(state->mutex);
}

static void onyx_ir_start(OnyxState* state) {
    state->ir_has_signal = false;
    state->ir_decoded = false;
    state->ir_signal_count = 0U;
    state->ir_raw_count = 0U;
    snprintf(state->ir_protocol, sizeof(state->ir_protocol), "Waiting");

    state->ir_worker = infrared_worker_alloc();
    infrared_worker_rx_enable_signal_decoding(state->ir_worker, true);
    infrared_worker_rx_enable_blink_on_receiving(state->ir_worker, false);
    infrared_worker_rx_set_received_signal_callback(
        state->ir_worker, onyx_ir_received_callback, state);
    infrared_worker_rx_start(state->ir_worker);
    state->ir_running = true;
}

static void onyx_ir_stop(OnyxState* state) {
    if(state->ir_worker) {
        if(state->ir_running) infrared_worker_rx_stop(state->ir_worker);
        infrared_worker_free(state->ir_worker);
        state->ir_worker = NULL;
    }
    state->ir_running = false;
}

static void onyx_draw_ir(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "IR RX INSPECTOR");
    canvas_set_font(canvas, FontSecondary);

    char line[32];
    if(!state->ir_has_signal) {
        canvas_draw_str(canvas, 2, 30, "Waiting for IR signal...");
    } else if(state->ir_decoded) {
        snprintf(line, sizeof(line), "Proto: %s", state->ir_protocol);
        canvas_draw_str(canvas, 2, 23, line);

        snprintf(line, sizeof(line), "Addr:%08lX", (unsigned long)state->ir_address);
        canvas_draw_str(canvas, 2, 35, line);

        snprintf(line, sizeof(line), "Cmd :%08lX%s", (unsigned long)state->ir_command, state->ir_repeat ? " R" : "");
        canvas_draw_str(canvas, 2, 47, line);

        snprintf(line, sizeof(line), "Signals:%lu", (unsigned long)state->ir_signal_count);
        canvas_draw_str(canvas, 2, 58, line);
    } else {
        snprintf(line, sizeof(line), "RAW timings: %u", (unsigned)state->ir_raw_count);
        canvas_draw_str(canvas, 2, 31, line);

        snprintf(line, sizeof(line), "Signals:%lu", (unsigned long)state->ir_signal_count);
        canvas_draw_str(canvas, 2, 45, line);
    }

    onyx_draw_footer(canvas, "RX only  Back:menu");
}

static void onyx_nfc_scanner_callback(NfcScannerEvent event, void* context) {
    furi_assert(context);
    OnyxState* state = context;

    if(event.type != NfcScannerEventTypeDetected) return;

    furi_mutex_acquire(state->mutex, FuriWaitForever);

    state->nfc_detected = true;
    state->nfc_detection_count++;
    state->nfc_protocol_count = MIN(event.data.protocol_num, ONYX_NFC_PROTOCOL_MAX);

    for(size_t i = 0; i < state->nfc_protocol_count; i++) {
        state->nfc_protocols[i] = event.data.protocols[i];
    }

    furi_mutex_release(state->mutex);
}

static void onyx_nfc_start(OnyxState* state) {
    state->nfc_detected = false;
    state->nfc_protocol_count = 0U;
    state->nfc_detection_count = 0U;

    state->nfc = nfc_alloc();
    state->nfc_scanner = nfc_scanner_alloc(state->nfc);
    nfc_scanner_start(state->nfc_scanner, onyx_nfc_scanner_callback, state);
    state->nfc_running = true;
}

static void onyx_nfc_stop(OnyxState* state) {
    if(state->nfc_scanner) {
        if(state->nfc_running) nfc_scanner_stop(state->nfc_scanner);
        nfc_scanner_free(state->nfc_scanner);
        state->nfc_scanner = NULL;
    }

    if(state->nfc) {
        nfc_free(state->nfc);
        state->nfc = NULL;
    }

    state->nfc_running = false;
}

static void onyx_draw_nfc(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "NFC DETECTOR");
    canvas_set_font(canvas, FontSecondary);

    char line[32];

    if(!state->nfc_detected) {
        canvas_draw_str(canvas, 2, 31, "Present an NFC tag...");
        canvas_draw_str(canvas, 2, 44, "Protocol scan only");
    } else {
        snprintf(line, sizeof(line), "Detected:%u", (unsigned)state->nfc_protocol_count);
        canvas_draw_str(canvas, 2, 22, line);

        for(size_t i = 0; i < state->nfc_protocol_count && i < 3U; i++) {
            snprintf(line, sizeof(line), "%u. %s", (unsigned)(i + 1U), onyx_nfc_protocol_name(state->nfc_protocols[i]));
            canvas_draw_str(canvas, 2, (uint8_t)(33U + i * 10U), line);
        }
    }

    onyx_draw_footer(canvas, "Read only  Back:menu");
}

static void onyx_lfrfid_read_callback(
    LFRFIDWorkerReadResult result,
    ProtocolId protocol,
    void* context) {
    furi_assert(context);
    OnyxState* state = context;

    if(result != LFRFIDWorkerReadDone || protocol == PROTOCOL_NO) return;

    const char* name = protocol_dict_get_name(state->lfrfid_dict, protocol);
    const size_t full_size = protocol_dict_get_data_size(state->lfrfid_dict, protocol);
    const size_t copy_size = MIN(full_size, ONYX_LFRFID_DATA_MAX);

    uint8_t data[ONYX_LFRFID_DATA_MAX] = {0};
    if(copy_size > 0U) {
        protocol_dict_get_data(state->lfrfid_dict, protocol, data, copy_size);
    }

    furi_mutex_acquire(state->mutex, FuriWaitForever);

    state->lfrfid_found = true;
    state->lfrfid_detection_count++;
    state->lfrfid_data_size = copy_size;
    snprintf(
        state->lfrfid_protocol_name,
        sizeof(state->lfrfid_protocol_name),
        "%s",
        name ? name : "Unknown");

    memcpy(state->lfrfid_data, data, copy_size);

    furi_mutex_release(state->mutex);
}

static void onyx_lfrfid_start(OnyxState* state) {
    state->lfrfid_found = false;
    state->lfrfid_data_size = 0U;
    state->lfrfid_detection_count = 0U;
    snprintf(state->lfrfid_protocol_name, sizeof(state->lfrfid_protocol_name), "Waiting");

    state->lfrfid_dict = protocol_dict_alloc(lfrfid_protocols, LFRFIDProtocolMax);
    state->lfrfid_worker = lfrfid_worker_alloc(state->lfrfid_dict);

    lfrfid_worker_start_thread(state->lfrfid_worker);
    lfrfid_worker_read_start(
        state->lfrfid_worker,
        LFRFIDWorkerReadTypeAuto,
        onyx_lfrfid_read_callback,
        state);

    state->lfrfid_running = true;
}

static void onyx_lfrfid_stop(OnyxState* state) {
    if(state->lfrfid_worker) {
        if(state->lfrfid_running) lfrfid_worker_stop(state->lfrfid_worker);
        lfrfid_worker_stop_thread(state->lfrfid_worker);
        lfrfid_worker_free(state->lfrfid_worker);
        state->lfrfid_worker = NULL;
    }

    if(state->lfrfid_dict) {
        protocol_dict_free(state->lfrfid_dict);
        state->lfrfid_dict = NULL;
    }

    state->lfrfid_running = false;
}

static void onyx_draw_lfrfid(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "LF RFID READER");
    canvas_set_font(canvas, FontSecondary);

    char line[40];

    if(!state->lfrfid_found) {
        canvas_draw_str(canvas, 2, 30, "Present LF RFID tag...");
        canvas_draw_str(canvas, 2, 43, "ASK/PSK auto read");
    } else {
        snprintf(line, sizeof(line), "Proto:%s", state->lfrfid_protocol_name);
        canvas_draw_str(canvas, 2, 22, line);

        char hex_a[28] = {0};
        char hex_b[28] = {0};

        size_t a_count = MIN(state->lfrfid_data_size, 4U);
        size_t b_count = (state->lfrfid_data_size > 4U) ? (state->lfrfid_data_size - 4U) : 0U;

        size_t pos = 0U;
        for(size_t i = 0; i < a_count; i++) {
            pos += snprintf(hex_a + pos, sizeof(hex_a) - pos, "%02X ", state->lfrfid_data[i]);
        }

        pos = 0U;
        for(size_t i = 0; i < b_count; i++) {
            pos += snprintf(hex_b + pos, sizeof(hex_b) - pos, "%02X ", state->lfrfid_data[4U + i]);
        }

        canvas_draw_str(canvas, 2, 35, hex_a);
        if(b_count > 0U) canvas_draw_str(canvas, 2, 46, hex_b);

        snprintf(line, sizeof(line), "Reads:%lu", (unsigned long)state->lfrfid_detection_count);
        canvas_draw_str(canvas, 2, 57, line);
    }

    onyx_draw_footer(canvas, "Read only  Back:menu");
}

static void onyx_draw_password(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "PASSWORD");

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 25, "Local RNG password:");
    canvas_draw_str(canvas, 2, 39, state->password);
    canvas_draw_str(canvas, 2, 51, "Not stored / not sent");

    onyx_draw_footer(canvas, "OK:new  Back:menu");
}

static void onyx_draw_random_hex(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "RANDOM HEX");

    char line_a[20];
    char line_b[20];

    snprintf(
        line_a,
        sizeof(line_a),
        "%02X %02X %02X %02X",
        state->random_bytes[0],
        state->random_bytes[1],
        state->random_bytes[2],
        state->random_bytes[3]);

    snprintf(
        line_b,
        sizeof(line_b),
        "%02X %02X %02X %02X",
        state->random_bytes[4],
        state->random_bytes[5],
        state->random_bytes[6],
        state->random_bytes[7]);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 27, line_a);
    canvas_draw_str(canvas, 2, 40, line_b);
    canvas_draw_str(canvas, 2, 51, "8 bytes from HW RNG");

    onyx_draw_footer(canvas, "OK:new  Back:menu");
}

static void onyx_draw_security_tips(Canvas* canvas, const OnyxState* state) {
    onyx_draw_header(canvas, "SECURITY TIPS");
    canvas_set_font(canvas, FontSecondary);

    if(state->tips_page == 0U) {
        canvas_draw_str(canvas, 2, 24, "1/2 AUTHORIZATION FIRST");
        canvas_draw_str(canvas, 2, 35, "- Own/approved systems");
        canvas_draw_str(canvas, 2, 46, "- Passive audit first");
        canvas_draw_str(canvas, 2, 57, "- Never log secrets");
    } else {
        canvas_draw_str(canvas, 2, 24, "2/2 VERIFY INPUT");
        canvas_draw_str(canvas, 2, 35, "- Treat data untrusted");
        canvas_draw_str(canvas, 2, 46, "- Bound sizes/timeouts");
        canvas_draw_str(canvas, 2, 57, "- Minimize persistence");
    }

    onyx_draw_footer(canvas, "OK:page  Back:menu");
}

static void onyx_draw_about(Canvas* canvas) {
    onyx_draw_header(canvas, "ABOUT");
    canvas_set_font(canvas, FontSecondary);

    canvas_draw_str(canvas, 2, 24, "Onyx Flipper Lab v2.0");
    canvas_draw_str(canvas, 2, 35, "Fran Gonzas");
    canvas_draw_str(canvas, 2, 46, "Defensive audit suite");
    canvas_draw_str(canvas, 2, 57, "RX / read-only radio");

    onyx_draw_footer(canvas, "Back:menu");
}

static void onyx_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    OnyxState* state = context;

    if(furi_mutex_acquire(state->mutex, FuriWaitForever) != FuriStatusOk) return;

    canvas_clear(canvas);

    switch(state->screen) {
    case OnyxScreenMenu:
        onyx_draw_menu(canvas, state);
        break;
    case OnyxScreenGpioAudit:
        onyx_draw_gpio(canvas, state);
        break;
    case OnyxScreenSubGhzAudit:
        onyx_draw_subghz(canvas, state);
        break;
    case OnyxScreenBluetoothAudit:
        onyx_draw_bluetooth(canvas);
        break;
    case OnyxScreenInfraredAudit:
        onyx_draw_ir(canvas, state);
        break;
    case OnyxScreenNfcAudit:
        onyx_draw_nfc(canvas, state);
        break;
    case OnyxScreenLfRfidAudit:
        onyx_draw_lfrfid(canvas, state);
        break;
    case OnyxScreenPassword:
        onyx_draw_password(canvas, state);
        break;
    case OnyxScreenRandomHex:
        onyx_draw_random_hex(canvas, state);
        break;
    case OnyxScreenSecurityTips:
        onyx_draw_security_tips(canvas, state);
        break;
    case OnyxScreenAbout:
        onyx_draw_about(canvas);
        break;
    default:
        onyx_draw_menu(canvas, state);
        break;
    }

    furi_mutex_release(state->mutex);
}

static void onyx_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static OnyxScreen onyx_menu_target(uint8_t menu_index) {
    switch(menu_index) {
    case 0:
        return OnyxScreenGpioAudit;
    case 1:
        return OnyxScreenSubGhzAudit;
    case 2:
        return OnyxScreenBluetoothAudit;
    case 3:
        return OnyxScreenInfraredAudit;
    case 4:
        return OnyxScreenNfcAudit;
    case 5:
        return OnyxScreenLfRfidAudit;
    case 6:
        return OnyxScreenPassword;
    case 7:
        return OnyxScreenRandomHex;
    case 8:
        return OnyxScreenSecurityTips;
    case 9:
        return OnyxScreenAbout;
    default:
        return OnyxScreenMenu;
    }
}

static void onyx_start_screen_module(OnyxState* state, OnyxScreen screen) {
    switch(screen) {
    case OnyxScreenGpioAudit:
        onyx_gpio_start(state);
        break;
    case OnyxScreenSubGhzAudit:
        onyx_subghz_start(state);
        break;
    case OnyxScreenInfraredAudit:
        onyx_ir_start(state);
        break;
    case OnyxScreenNfcAudit:
        onyx_nfc_start(state);
        break;
    case OnyxScreenLfRfidAudit:
        onyx_lfrfid_start(state);
        break;
    case OnyxScreenPassword:
        onyx_generate_password(state->password);
        break;
    case OnyxScreenRandomHex:
        onyx_generate_random_bytes(state->random_bytes);
        break;
    case OnyxScreenSecurityTips:
        state->tips_page = 0U;
        break;
    default:
        break;
    }
}

static void onyx_stop_screen_module(OnyxState* state, OnyxScreen screen) {
    switch(screen) {
    case OnyxScreenGpioAudit:
        onyx_gpio_stop(state);
        break;
    case OnyxScreenSubGhzAudit:
        onyx_subghz_stop(state);
        break;
    case OnyxScreenInfraredAudit:
        onyx_ir_stop(state);
        break;
    case OnyxScreenNfcAudit:
        onyx_nfc_stop(state);
        break;
    case OnyxScreenLfRfidAudit:
        onyx_lfrfid_stop(state);
        break;
    default:
        break;
    }
}

static void onyx_periodic_update(OnyxState* state, OnyxScreen screen) {
    switch(screen) {
    case OnyxScreenGpioAudit:
        onyx_gpio_sample(state);
        break;
    case OnyxScreenSubGhzAudit:
        onyx_subghz_sample(state);
        break;
    default:
        break;
    }
}

int32_t onyx_flipper_lab_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Starting Onyx Flipper Lab v2");

    OnyxState state = {
        .screen = OnyxScreenMenu,
        .menu_index = 0U,
        .tips_page = 0U,
        .mutex = NULL,
    };

    state.mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    onyx_generate_password(state.password);
    onyx_generate_random_bytes(state.random_bytes);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8U, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, onyx_draw_callback, &state);
    view_port_input_callback_set(view_port, onyx_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    bool running = true;
    InputEvent event;

    while(running) {
        const FuriStatus queue_status = furi_message_queue_get(event_queue, &event, 120U);

        furi_mutex_acquire(state.mutex, FuriWaitForever);
        const OnyxScreen screen_snapshot = state.screen;
        furi_mutex_release(state.mutex);

        if(queue_status != FuriStatusOk) {
            onyx_periodic_update(&state, screen_snapshot);
            view_port_update(view_port);
            continue;
        }

        if(event.type != InputTypePress) continue;

        if(screen_snapshot == OnyxScreenMenu) {
            if(event.key == InputKeyUp || event.key == InputKeyDown) {
                furi_mutex_acquire(state.mutex, FuriWaitForever);

                if(event.key == InputKeyUp) {
                    state.menu_index =
                        (state.menu_index == 0U) ? (ONYX_MENU_COUNT - 1U) :
                                                  (state.menu_index - 1U);
                } else {
                    state.menu_index = (uint8_t)((state.menu_index + 1U) % ONYX_MENU_COUNT);
                }

                furi_mutex_release(state.mutex);
            } else if(event.key == InputKeyOk) {
                furi_mutex_acquire(state.mutex, FuriWaitForever);
                const OnyxScreen target = onyx_menu_target(state.menu_index);
                furi_mutex_release(state.mutex);

                onyx_start_screen_module(&state, target);

                furi_mutex_acquire(state.mutex, FuriWaitForever);
                state.screen = target;
                furi_mutex_release(state.mutex);
            } else if(event.key == InputKeyBack) {
                running = false;
            }
        } else {
            if(event.key == InputKeyBack) {
                furi_mutex_acquire(state.mutex, FuriWaitForever);
                state.screen = OnyxScreenMenu;
                furi_mutex_release(state.mutex);

                onyx_stop_screen_module(&state, screen_snapshot);
            } else if(event.key == InputKeyOk) {
                if(screen_snapshot == OnyxScreenPassword) {
                    furi_mutex_acquire(state.mutex, FuriWaitForever);
                    onyx_generate_password(state.password);
                    furi_mutex_release(state.mutex);
                } else if(screen_snapshot == OnyxScreenRandomHex) {
                    furi_mutex_acquire(state.mutex, FuriWaitForever);
                    onyx_generate_random_bytes(state.random_bytes);
                    furi_mutex_release(state.mutex);
                } else if(screen_snapshot == OnyxScreenSecurityTips) {
                    furi_mutex_acquire(state.mutex, FuriWaitForever);
                    state.tips_page ^= 1U;
                    furi_mutex_release(state.mutex);
                } else if(screen_snapshot == OnyxScreenGpioAudit) {
                    onyx_gpio_sample(&state);
                }
            } else if(event.key == InputKeyLeft || event.key == InputKeyRight) {
                const int direction = (event.key == InputKeyRight) ? 1 : -1;

                if(screen_snapshot == OnyxScreenGpioAudit) {
                    onyx_gpio_move(&state, direction);
                } else if(screen_snapshot == OnyxScreenSubGhzAudit) {
                    onyx_subghz_move(&state, direction);
                    onyx_subghz_sample(&state);
                }
            }
        }

        view_port_update(view_port);
    }

    furi_mutex_acquire(state.mutex, FuriWaitForever);
    const OnyxScreen final_screen = state.screen;
    furi_mutex_release(state.mutex);
    onyx_stop_screen_module(&state, final_screen);

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);

    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    furi_mutex_free(state.mutex);

    FURI_LOG_I(TAG, "Onyx Flipper Lab v2 stopped");
    return 0;
}
