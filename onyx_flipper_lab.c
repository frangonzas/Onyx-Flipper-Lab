/**
 * @file onyx_flipper_lab.c
 * @brief Onyx Flipper Lab - utilidades defensivas para Flipper Zero.
 *
 * Proyecto de Fran Gonzas.
 *
 * Esta aplicación está diseñada como laboratorio educativo y defensivo.
 * No contiene funciones destinadas a interferir con sistemas de terceros.
 */

#include <furi.h>
#include <furi_hal_random.h>

#include <gui/gui.h>
#include <input/input.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TAG "OnyxFlipperLab"

#define ONYX_MENU_COUNT 5U
#define ONYX_PASSWORD_LENGTH 18U
#define ONYX_RANDOM_BYTES 8U

typedef enum {
    OnyxScreenMenu = 0,
    OnyxScreenPassword,
    OnyxScreenRandomHex,
    OnyxScreenSecurityTips,
    OnyxScreenGpioSafety,
    OnyxScreenAbout,
} OnyxScreen;

typedef struct {
    OnyxScreen screen;
    uint8_t menu_index;
    uint8_t tips_page;
    char password[ONYX_PASSWORD_LENGTH + 1U];
    uint8_t random_bytes[ONYX_RANDOM_BYTES];
    FuriMutex* mutex;
} OnyxState;

static const char* const onyx_menu_items[ONYX_MENU_COUNT] = {
    "Password generator",
    "Random HEX",
    "Security tips",
    "GPIO safety",
    "About",
};

/**
 * Devuelve un valor aleatorio sin sesgo de módulo apreciable.
 *
 * Se usa rejection sampling sobre el RNG hardware del Flipper.
 */
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

/**
 * Genera una contraseña local de 18 caracteres.
 *
 * No se guarda en almacenamiento y no se transmite fuera del dispositivo.
 */
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

/**
 * Genera bytes aleatorios utilizando el RNG hardware expuesto por Furi HAL.
 */
static void onyx_generate_random_bytes(uint8_t* output) {
    furi_hal_random_fill_buf(output, ONYX_RANDOM_BYTES);
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
    onyx_draw_header(canvas, "ONYX FLIPPER LAB");
    canvas_set_font(canvas, FontSecondary);

    char line[30];

    for(uint8_t i = 0; i < ONYX_MENU_COUNT; i++) {
        snprintf(
            line,
            sizeof(line),
            "%c %s",
            (i == state->menu_index) ? '>' : ' ',
            onyx_menu_items[i]);

        canvas_draw_str(canvas, 2, (uint8_t)(23U + (i * 9U)), line);
    }
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
        canvas_draw_str(canvas, 2, 24, "1/2  AUTHORIZATION FIRST");
        canvas_draw_str(canvas, 2, 35, "- Own/approved systems");
        canvas_draw_str(canvas, 2, 46, "- Never log secrets");
        canvas_draw_str(canvas, 2, 57, "- Minimize stored data");
    } else {
        canvas_draw_str(canvas, 2, 24, "2/2  VERIFY INPUT");
        canvas_draw_str(canvas, 2, 35, "- Treat data untrusted");
        canvas_draw_str(canvas, 2, 46, "- Bound sizes/timeouts");
        canvas_draw_str(canvas, 2, 57, "- Review permissions");
    }

    onyx_draw_footer(canvas, "OK:page  Back:menu");
}

static void onyx_draw_gpio_safety(Canvas* canvas) {
    onyx_draw_header(canvas, "GPIO SAFETY");
    canvas_set_font(canvas, FontSecondary);

    canvas_draw_str(canvas, 2, 24, "Read-only guide (v1)");
    canvas_draw_str(canvas, 2, 35, "- Check voltage first");
    canvas_draw_str(canvas, 2, 46, "- Common GND matters");
    canvas_draw_str(canvas, 2, 57, "- Power off to rewire");

    onyx_draw_footer(canvas, "Back:menu");
}

static void onyx_draw_about(Canvas* canvas) {
    onyx_draw_header(canvas, "ABOUT");
    canvas_set_font(canvas, FontSecondary);

    canvas_draw_str(canvas, 2, 24, "Onyx Flipper Lab v1.0");
    canvas_draw_str(canvas, 2, 35, "Fran Gonzas");
    canvas_draw_str(canvas, 2, 46, "Defensive research");
    canvas_draw_str(canvas, 2, 57, "Build > Analyze > Secure");

    onyx_draw_footer(canvas, "Back:menu");
}

/**
 * Callback de dibujo ejecutado por el servicio GUI.
 *
 * El mutex evita leer el estado mientras el hilo principal lo modifica.
 */
static void onyx_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    OnyxState* state = context;

    if(furi_mutex_acquire(state->mutex, FuriWaitForever) != FuriStatusOk) {
        return;
    }

    canvas_clear(canvas);

    switch(state->screen) {
    case OnyxScreenMenu:
        onyx_draw_menu(canvas, state);
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
    case OnyxScreenGpioSafety:
        onyx_draw_gpio_safety(canvas);
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

/**
 * Los eventos de botones llegan desde GUI.
 * Los copiamos a una cola para tratarlos en el hilo principal de la app.
 */
static void onyx_input_callback(InputEvent* input_event, void* context) {
    furi_assert(context);
    FuriMessageQueue* event_queue = context;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void onyx_open_selected_screen(OnyxState* state) {
    switch(state->menu_index) {
    case 0:
        onyx_generate_password(state->password);
        state->screen = OnyxScreenPassword;
        break;
    case 1:
        onyx_generate_random_bytes(state->random_bytes);
        state->screen = OnyxScreenRandomHex;
        break;
    case 2:
        state->tips_page = 0U;
        state->screen = OnyxScreenSecurityTips;
        break;
    case 3:
        state->screen = OnyxScreenGpioSafety;
        break;
    case 4:
        state->screen = OnyxScreenAbout;
        break;
    default:
        state->screen = OnyxScreenMenu;
        break;
    }
}

/**
 * Punto de entrada definido en application.fam.
 */
int32_t onyx_flipper_lab_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Starting Onyx Flipper Lab");

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
        if(furi_message_queue_get(event_queue, &event, FuriWaitForever) != FuriStatusOk) {
            continue;
        }

        if(event.type != InputTypePress) {
            continue;
        }

        furi_mutex_acquire(state.mutex, FuriWaitForever);

        if(state.screen == OnyxScreenMenu) {
            if(event.key == InputKeyUp) {
                state.menu_index =
                    (state.menu_index == 0U) ? (ONYX_MENU_COUNT - 1U) : (state.menu_index - 1U);
            } else if(event.key == InputKeyDown) {
                state.menu_index = (uint8_t)((state.menu_index + 1U) % ONYX_MENU_COUNT);
            } else if(event.key == InputKeyOk) {
                onyx_open_selected_screen(&state);
            } else if(event.key == InputKeyBack) {
                running = false;
            }
        } else {
            if(event.key == InputKeyBack) {
                state.screen = OnyxScreenMenu;
            } else if(event.key == InputKeyOk) {
                if(state.screen == OnyxScreenPassword) {
                    onyx_generate_password(state.password);
                } else if(state.screen == OnyxScreenRandomHex) {
                    onyx_generate_random_bytes(state.random_bytes);
                } else if(state.screen == OnyxScreenSecurityTips) {
                    state.tips_page ^= 1U;
                }
            }
        }

        furi_mutex_release(state.mutex);
        view_port_update(view_port);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);

    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    furi_mutex_free(state.mutex);

    FURI_LOG_I(TAG, "Onyx Flipper Lab stopped");
    return 0;
}
