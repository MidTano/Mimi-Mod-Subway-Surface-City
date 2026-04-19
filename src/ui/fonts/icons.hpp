#pragma once

namespace ui::icons {

constexpr const char* k_bolt                       = "\xEE\xA8\x8B";
constexpr const char* k_close                      = "\xEE\x97\x8D";
constexpr const char* k_menu_open                  = "\xEE\xA6\xBD";
constexpr const char* k_radio_button_checked       = "\xEE\xA0\xB7";
constexpr const char* k_tune                       = "\xEE\x90\xA9";
constexpr const char* k_category                   = "\xEE\x95\xB4";
constexpr const char* k_chevron_right              = "\xEE\x97\x8C";
constexpr const char* k_directions_run             = "\xEE\x95\xA6";
constexpr const char* k_visibility                 = "\xEE\xA3\xB4";
constexpr const char* k_visibility_off             = "\xEE\xA3\xB5";
constexpr const char* k_speed                      = "\xEE\xA7\xA4";
constexpr const char* k_more_horiz                 = "\xEE\x97\x93";
constexpr const char* k_analytics                  = "\xEE\xBC\xBE";
constexpr const char* k_schedule                   = "\xEE\xA2\xB5";
constexpr const char* k_shield                     = "\xEE\xA7\xA0";
constexpr const char* k_block                      = "\xEE\x85\x8B";
constexpr const char* k_blur_on                    = "\xEE\x8E\xA5";
constexpr const char* k_arrow_upward               = "\xEE\x97\x98";
constexpr const char* k_arrow_downward             = "\xEE\x97\x9B";
constexpr const char* k_skateboarding              = "\xEE\x94\x91";
constexpr const char* k_rocket_launch              = "\xEE\xAE\x9B";
constexpr const char* k_near_me                    = "\xEE\x95\xA9";
constexpr const char* k_close_fullscreen           = "\xEF\x87\x8F";
constexpr const char* k_all_inclusive              = "\xEE\xAC\xBD";
constexpr const char* k_paid                       = "\xEF\x81\x81";
constexpr const char* k_flip                       = "\xEE\x8F\xA8";
constexpr const char* k_toggle_on                  = "\xEE\xA7\xB6";
constexpr const char* k_autorenew                  = "\xEE\xA1\xA3";
constexpr const char* k_fast_forward               = "\xEE\x80\x9F";
constexpr const char* k_opacity                    = "\xEE\xA4\x9C";
constexpr const char* k_keyboard_double_arrow_up   = "\xEE\xAB\x8F";
constexpr const char* k_play_arrow                 = "\xEE\x80\xB7";
constexpr const char* k_emoji_events               = "\xEE\xA8\xA3";
constexpr const char* k_flare                      = "\xEE\x8F\xA4";
constexpr const char* k_photo_camera               = "\xEE\x90\x92";
constexpr const char* k_zoom_out_map               = "\xEE\x95\xAB";
constexpr const char* k_apps                       = "\xEE\x97\x83";
constexpr const char* k_unfold_less                = "\xEE\x97\x96";
constexpr const char* k_hourglass_empty            = "\xEE\xA2\x8B";
constexpr const char* k_folder                     = "\xEE\x8B\x87";
constexpr const char* k_file_upload                = "\xEE\x8B\x86";
constexpr const char* k_file_download              = "\xEE\x8B\x84";
constexpr const char* k_aspect_ratio               = "\xEE\xA1\x9B";
constexpr const char* k_info                       = "\xEE\xA2\x8E";
constexpr const char* k_warning                     = "\xEE\x80\x82";

constexpr const char* kAllIcons[] = {
    k_bolt, k_close, k_menu_open, k_radio_button_checked, k_tune, k_category,
    k_chevron_right, k_directions_run, k_visibility, k_visibility_off, k_speed,
    k_more_horiz, k_analytics, k_schedule, k_shield, k_block, k_blur_on,
    k_arrow_upward, k_arrow_downward, k_skateboarding, k_rocket_launch, k_near_me,
    k_close_fullscreen, k_all_inclusive, k_paid, k_flip, k_toggle_on, k_autorenew,
    k_fast_forward, k_opacity, k_keyboard_double_arrow_up, k_play_arrow,
    k_emoji_events, k_flare, k_photo_camera, k_zoom_out_map, k_apps, k_unfold_less,
    k_hourglass_empty, k_folder, k_file_upload, k_file_download, k_aspect_ratio,
    k_info, k_warning,
};
constexpr int kAllIconsCount = sizeof(kAllIcons) / sizeof(kAllIcons[0]);

constexpr int kMinCodepoint = 0xE01F;
constexpr int kMaxCodepoint = 0xF1CF;

}