/**
 * can_sim.c — inject Link G4X ECU frames over USB (same decoders as TWAI RX).
 *
 * Enable: menuconfig → TrackCluster → "USB console 'can' command"
 *
 * Usage in idf.py monitor:
 *   can 3e8 1f40006432323200     RPM / MAP / temps
 *   can 3e9 04E23201C2012C00     ign=25 / speed=50 / oil=450kPa(65psi) / fuel=300kPa(43psi)
 *   can 3ee 0100000000000000     knock protection (warn tone + UART byte 16)
 *   can 3ee 0000000000000000     clear warnings
 */
#include "sdkconfig.h"

#if CONFIG_TC_CAN_SIM_CONSOLE

#include "canbus.h"
#include "can_sim.h"

#include "esp_console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_payload(int argc, char **argv, uint8_t *data, int *out_len)
{
    int n = 0;
    for (int i = 2; i < argc && n < 8; i++) {
        const char *tok = argv[i];
        size_t len = strlen(tok);
        if (len == 0) {
            return -1;
        }
        if (len > 2) {
            if ((len & 1u) != 0) {
                return -1;
            }
            for (size_t p = 0; p < len && n < 8; p += 2) {
                char pair[3] = { tok[p], tok[p + 1], '\0' };
                char *end = NULL;
                unsigned long v = strtoul(pair, &end, 16);
                if (end == pair || v > 0xff) {
                    return -1;
                }
                data[n++] = (uint8_t)v;
            }
            continue;
        }
        char *end = NULL;
        unsigned long v = strtoul(tok, &end, 16);
        if (end == tok || v > 0xff) {
            return -1;
        }
        data[n++] = (uint8_t)v;
    }
    *out_len = n;
    return 0;
}

static int cmd_can(int argc, char **argv)
{
    if (argc < 3) {
        printf("Usage: can <id-hex> <payload-hex>\n");
        printf("  Payload: space-separated bytes OR one concatenated hex string (up to 8 bytes)\n");
        printf("  IDs: 3e8 engine  3e9 speed/press  3ea lambda  3eb gear/fuel  3ee protection\n");
        printf("Example: can 3ee 0100000000000000  — knock warn\n");
        return 0;
    }

    char *end = NULL;
    unsigned long id = strtoul(argv[1], &end, 16);
    if (end == argv[1] || id > 0x7ff) {
        printf("Invalid CAN id\n");
        return 0;
    }

    uint8_t data[8] = {0};
    int n = 0;
    if (parse_payload(argc, argv, data, &n) != 0) {
        printf("Invalid payload hex\n");
        return 0;
    }

    esp_err_t err = canbus_inject_frame((uint32_t)id, data, (uint8_t)n);
    if (err != ESP_OK) {
        printf("inject failed: %s\n", esp_err_to_name(err));
        return 0;
    }

    printf("CAN 0x%03lx injected (%d bytes) → g_dash updated\n", id, n);
    return 0;
}

static int cmd_warn_test(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    can_sim_trigger_warning_once();
    printf("injected 0x3EE knock warning\n");
    return 0;
}

esp_err_t can_sim_console_register(void)
{
    const esp_console_cmd_t can_cmd = {
        .command = "can",
        .help    = "Inject ECU frame: can <id> <b0> [b1..b7]",
        .func    = cmd_can,
    };
    const esp_console_cmd_t warn_cmd = {
        .command = "warn_test",
        .help    = "One-shot knock warning beep (0x3EE)",
        .func    = cmd_warn_test,
    };
    esp_err_t err = esp_console_cmd_register(&can_cmd);
    if (err != ESP_OK) {
        return err;
    }
    return esp_console_cmd_register(&warn_cmd);
}

void can_sim_trigger_warning_once(void)
{
    uint8_t clear[8] = {0};
    uint8_t knock[8] = {1, 0, 0, 0, 0, 0, 0, 0};
    canbus_inject_frame(0x3EE, clear, 8);
    canbus_inject_frame(0x3EE, knock, 8);
}

#else

#include "esp_err.h"

esp_err_t can_sim_console_register(void)
{
    return ESP_OK;
}

void can_sim_trigger_warning_once(void)
{
}

#endif
