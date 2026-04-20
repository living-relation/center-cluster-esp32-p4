// Modified to support 800x800 and 720x720 automatically for 3.4in or 4in displays

#include "../ui.h"

//Base resolution that the project was initially coded for(800x800 round screen)
#define UI_BASE_RES 800
#define UI_SCALE(x) ((x) * LV_HOR_RES / UI_BASE_RES)

lv_obj_t *ui_Screen1 = NULL;
lv_obj_t *ui_rpm_bg = NULL;
lv_obj_t *ui_rpm_arc = NULL;
lv_obj_t *ui_label_mph_value = NULL;
lv_obj_t *ui_label_mph  = NULL;
lv_obj_t *ui_label_gear = NULL;
lv_obj_t *ui_label_gear_value = NULL;
lv_obj_t *ui_label_odometer_value = NULL;

static void ui_apply_theme(ui_theme_t theme)
{
   const ui_theme_palette_t *palette = ui_theme_get_palette(theme);
   if (!palette || !ui_Screen1) {
      return;
   }

   lv_obj_set_style_bg_color(ui_Screen1, palette->screen_bg_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);

   if (ui_rpm_bg) {
      lv_obj_set_style_img_opa(ui_rpm_bg, palette->rpm_bg_img_opa, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
   if (ui_rpm_arc) {
      lv_obj_set_style_arc_width(ui_rpm_arc, UI_SCALE(palette->rpm_arc_width), LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_arc_opa(ui_rpm_arc, palette->rpm_arc_opa, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_color(ui_rpm_arc, palette->rpm_color_low, LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_width(ui_rpm_arc, UI_SCALE(palette->rpm_glow_width), LV_PART_INDICATOR | LV_STATE_DEFAULT);
      lv_obj_set_style_shadow_opa(ui_rpm_arc, palette->rpm_glow_opa, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   }

   if (ui_label_mph_value) {
      lv_obj_set_style_text_color(ui_label_mph_value, palette->mph_value_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
   if (ui_label_mph) {
      lv_obj_set_style_text_color(ui_label_mph, palette->primary_label_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
   if (ui_label_gear) {
      lv_obj_set_style_text_color(ui_label_gear, palette->primary_label_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
   if (ui_label_gear_value) {
      lv_obj_set_style_text_color(ui_label_gear_value, palette->gear_value_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
   if (ui_label_odometer_value) {
      lv_obj_set_style_text_color(ui_label_odometer_value, palette->odometer_color, LV_PART_MAIN | LV_STATE_DEFAULT);
   }
}

void ui_Screen1_screen_init(void) {

   ui_Screen1 = lv_obj_create(NULL);
   lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

   ui_rpm_bg = lv_img_create(ui_Screen1);
   lv_img_set_src(ui_rpm_bg, &ui_img_1656279599);
   lv_obj_set_width(ui_rpm_bg, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_rpm_bg, LV_SIZE_CONTENT);
   lv_obj_set_x(ui_rpm_bg, 0);
   lv_obj_set_y(ui_rpm_bg, UI_SCALE(4));
   lv_obj_set_align(ui_rpm_bg, LV_ALIGN_CENTER);
   lv_obj_add_flag(ui_rpm_bg, LV_OBJ_FLAG_ADV_HITTEST);
   lv_obj_clear_flag(ui_rpm_bg, LV_OBJ_FLAG_SCROLLABLE);

   ui_rpm_arc = lv_arc_create(ui_Screen1);
   lv_obj_set_width(ui_rpm_arc, UI_SCALE(830));
   lv_obj_set_height(ui_rpm_arc, UI_SCALE(780));
   lv_obj_set_x(ui_rpm_arc, UI_SCALE(24));
   lv_obj_set_y(ui_rpm_arc, 0);
   lv_obj_set_align(ui_rpm_arc, LV_ALIGN_CENTER);

   lv_arc_set_bg_angles(ui_rpm_arc, 90, 0);
   lv_arc_set_range(ui_rpm_arc, 0, 9000);

   lv_obj_set_style_arc_width(ui_rpm_arc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

   lv_obj_set_style_bg_color(ui_rpm_arc, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_opa(ui_rpm_arc, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_arc_color(ui_rpm_arc, lv_color_hex(0x28FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_arc_opa(ui_rpm_arc, 180, LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_arc_width(ui_rpm_arc, UI_SCALE(140), LV_PART_INDICATOR | LV_STATE_DEFAULT);
   lv_obj_set_style_arc_rounded(ui_rpm_arc, false, LV_PART_INDICATOR | LV_STATE_DEFAULT);

   lv_obj_set_style_arc_opa(ui_rpm_arc, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_opa(ui_rpm_arc, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_border_opa(ui_rpm_arc, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);

   lv_obj_set_style_bg_color(ui_rpm_arc, lv_color_hex(0xFFFFFF), LV_PART_KNOB | LV_STATE_DEFAULT);
   lv_obj_set_style_bg_opa(ui_rpm_arc, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

   ui_label_mph_value = lv_label_create(ui_Screen1);
   lv_obj_set_width(ui_label_mph_value, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_label_mph_value, LV_SIZE_CONTENT);
   lv_obj_set_align(ui_label_mph_value, LV_ALIGN_CENTER);
   lv_obj_set_x(ui_label_mph_value, UI_SCALE(17));
   lv_obj_set_y(ui_label_mph_value, UI_SCALE(154));
   lv_label_set_text(ui_label_mph_value, "--");
   lv_obj_set_style_text_color(ui_label_mph_value, lv_palette_main(LV_PALETTE_CYAN), LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_text_font(ui_label_mph_value, &ui_font_DotoLarge, LV_PART_MAIN | LV_STATE_DEFAULT);

   ui_label_mph = lv_label_create(ui_Screen1);
   lv_obj_set_width(ui_label_mph, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_label_mph, LV_SIZE_CONTENT);
   lv_obj_set_x(ui_label_mph, UI_SCALE(7));
   lv_obj_set_y(ui_label_mph, UI_SCALE(63));
   lv_obj_set_align(ui_label_mph, LV_ALIGN_CENTER);
   lv_label_set_text(ui_label_mph, "MPH");
   lv_obj_set_style_text_font(ui_label_mph, &school_bell_48, LV_PART_MAIN | LV_STATE_DEFAULT);

   ui_label_gear = lv_label_create(ui_Screen1);
   lv_obj_set_width(ui_label_gear, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_label_gear, LV_SIZE_CONTENT);
   lv_obj_set_x(ui_label_gear, 0);
   lv_obj_set_y(ui_label_gear, UI_SCALE(-160));
   lv_obj_set_align(ui_label_gear, LV_ALIGN_CENTER);
   lv_label_set_text(ui_label_gear, "GEAR");
   lv_obj_set_style_text_font(ui_label_gear, &school_bell_48, LV_PART_MAIN | LV_STATE_DEFAULT);

   ui_label_gear_value = lv_label_create(ui_Screen1);
   lv_obj_set_width(ui_label_gear_value, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_label_gear_value, LV_SIZE_CONTENT);
   lv_obj_set_align(ui_label_gear_value, LV_ALIGN_CENTER);
   lv_obj_set_x(ui_label_gear_value, UI_SCALE(7));
   lv_obj_set_y(ui_label_gear_value, UI_SCALE(-75));
   lv_label_set_text(ui_label_gear_value, "N");
   lv_obj_set_style_text_color(ui_label_gear_value, lv_palette_main(LV_PALETTE_PURPLE), LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_text_font(ui_label_gear_value, &ui_font_DotoLarge, LV_PART_MAIN | LV_STATE_DEFAULT);

   ui_label_odometer_value = lv_label_create(ui_Screen1);
   lv_obj_set_width(ui_label_odometer_value, LV_SIZE_CONTENT);
   lv_obj_set_height(ui_label_odometer_value, LV_SIZE_CONTENT);
   lv_obj_set_align(ui_label_odometer_value, LV_ALIGN_CENTER);
   lv_obj_set_x(ui_label_odometer_value, UI_SCALE(300));
   lv_obj_set_y(ui_label_odometer_value, UI_SCALE(75));
   lv_label_set_text(ui_label_odometer_value, "0.0");
   lv_obj_set_style_text_color(ui_label_odometer_value, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
   lv_obj_set_style_text_font(ui_label_odometer_value, &ui_font_Doto_48, LV_PART_MAIN | LV_STATE_DEFAULT);

   ui_apply_theme(ui_theme_get());
}

void ui_Screen1_screen_destroy(void)
{
   if (ui_Screen1) lv_obj_del(ui_Screen1);

   ui_Screen1 = NULL;
   ui_rpm_bg = NULL;
   ui_rpm_arc = NULL;
   ui_label_mph_value = NULL;
   ui_label_mph = NULL;
   ui_label_gear = NULL;
   ui_label_gear_value = NULL;
   ui_label_odometer_value = NULL;
}