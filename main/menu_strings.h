/**
 * menu_strings.h — encoder menu option tables, shared between center and right.
 *
 * !! KEEP THIS FILE BYTE-IDENTICAL IN BOTH REPOS !!
 *    center-cluster-esp32-p4/main/menu_strings.h
 *    right-side-cluster-esp32s3/main/menu_strings.h
 *
 * The center cluster (inputs.c) uses these to send menu_id + menu_cursor
 * over UART. The right cluster (ui_menu_popup.c) uses them to render the
 * popup. If they differ, the popup shows different text than what the center
 * confirmed — silent data corruption. Update both repos in the same PR.
 *
 * String length limit: 24 chars (popup box is 340 px wide, racehead_14).
 */
#ifndef MENU_STRINGS_H
#define MENU_STRINGS_H

#define BOOST_MAP_COUNT 4
static const char * const BOOST_MAPS[BOOST_MAP_COUNT] = {
    "STREET  12 PSI",
    "SPORT   16 PSI",
    "TRACK   20 PSI",
    "MAX     24 PSI",
};

#define TC_SLIP_COUNT 5
static const char * const TC_SLIPS[TC_SLIP_COUNT] = {
    "OFF      0\xB0",
    "LOW      3\xB0",
    "MED      6\xB0",
    "HIGH    10\xB0",
    "TRACK   15\xB0",
};

#endif /* MENU_STRINGS_H */
