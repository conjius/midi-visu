/*
  ==============================================================================
    StyleTokens — pure C++ (no JUCE), all visual style constants.
    Included by StyleManager, RangeSlider, and tests.
  ==============================================================================
*/

#pragma once

#include <cstdint>

namespace StyleTokens {
    // Panel backgrounds
    static constexpr uint32_t kPanelBg = 0xf0101010; // 94% dark grey
    static constexpr uint32_t kLogBg = 0xe8101010; // 91% dark grey
    static constexpr uint32_t kDialogBg = 0xff1a1a1a; // fully opaque

    // White overlay hierarchy (all on top of dark panels)
    static constexpr uint32_t kPanelTitle = 0xe6ffffff; // 90% — "PROJECT" heading
    static constexpr uint32_t kVoiceName = 0xd9ffffff; // 85% — voice name labels
    static constexpr uint32_t kValue = 0xccffffff; // 80% — numeric value labels
    static constexpr uint32_t kSectionHead = 0x80ffffff; // 50% — section headers
    static constexpr uint32_t kSecondary = 0x70ffffff;
    // 44% — secondary labels + scrollbar thumb
    static constexpr uint32_t kLabel = 0x60ffffff; // 38% — parameter labels
    static constexpr uint32_t kBorder = 0x50ffffff; // 31% — panel side borders
    static constexpr uint32_t kDivider = 0x30ffffff; // 19% — horizontal divider lines
    static constexpr uint32_t kScrollTrack = 0x20ffffff;
    // 12% — scrollbar track background
    static constexpr uint32_t kLogFallback = 0xffcccccc; // log entry fallback colour
    static constexpr uint32_t kLogInfo = 0xffb0b0b0; // info-level log entries
    static constexpr uint32_t kLogClock = 0xff5588bb; // debug clock entries

    // Video list
    static constexpr uint32_t kListRowSelected = 0x40ffffff;
    // 25% white — selected list row

    // LinearHorizontal Slider
    static constexpr uint32_t kSliderBg = 0xff404040;
    static constexpr uint32_t kSliderTrack = 0xff808080;
    static constexpr uint32_t kSliderThumb = 0xffffffff;
    static constexpr uint32_t kSliderText = 0xccffffff; // = kValue
    static constexpr uint32_t kSliderOutline = 0x30ffffff; // = kDivider

    // RangeSlider (TwoValueHorizontal)
    static constexpr uint32_t kRangeSliderBg = 0xff303030;
    static constexpr uint32_t kRangeSliderTrack = 0xbfffffff; // white 75%

    // Button / ToggleButton
    static constexpr uint32_t kButtonBg = 0xff303030;
    static constexpr uint32_t kButtonBgOn = 0xff505050;
    static constexpr uint32_t kButtonText = 0xd9ffffff; // = kVoiceName
    static constexpr uint32_t kTickOn = 0xff32cd32; // brand green
    static constexpr uint32_t kTickOff = 0x80ffffff; // = kSectionHead

    // SeekBar
    static constexpr uint32_t kSeekBarBg = 0xff252525; // dark background
    static constexpr uint32_t kSeekBarTrack = 0xff404040; // unfilled track
    static constexpr uint32_t kSeekBarLoop = 0x8c42a2c8; // loop region highlight (blue 55%)
    static constexpr uint32_t kSeekBarPlayhead = 0xffffffff; // playhead handle — white
    static constexpr uint32_t kSeekBarHandle = 0xff42a2c8; // loop handles — JUCE default blue
    static constexpr uint32_t kSeekBarTime = 0x80ffffff; // time label text (50%)

    // ComboBox
    static constexpr uint32_t kComboBoxBg = 0xff252525;
    static constexpr uint32_t kComboBoxText = 0xd9ffffff; // = kVoiceName
    static constexpr uint32_t kComboBoxOutline = 0x50ffffff; // = kBorder
    static constexpr uint32_t kComboBoxArrow = 0x80ffffff; // = kSectionHead

    // Font sizes
    static constexpr float kHeadingSize = 13.0f;
    static constexpr float kLabelSize = 12.0f;
    static constexpr float kLogSize = 13.0f;

    // Layout spacing (pixels)
    static constexpr int kPadding = 10; // panel inner padding
    static constexpr int kRowHeight = 26; // options row height
    static constexpr int kListRowHeight = 22; // file list row height
    static constexpr int kSectionGap = 20; // gap before section header
    static constexpr int kSliderHeight = 28; // slider component height
    static constexpr int kButtonHeight = 26; // button height
    static constexpr int kDividerGap = 10; // space above divider line
    static constexpr int kScrollbarW = 6; // scrollbar width

    // ARGB byte extractors
    static constexpr uint8_t alpha(uint32_t argb) {
        return static_cast<uint8_t>((argb >> 24) & 0xff);
    }

    static constexpr uint8_t red(uint32_t argb) {
        return static_cast<uint8_t>((argb >> 16) & 0xff);
    }

    static constexpr uint8_t green(uint32_t argb) {
        return static_cast<uint8_t>((argb >> 8) & 0xff);
    }

    static constexpr uint8_t blue(uint32_t argb) {
        return static_cast<uint8_t>(argb & 0xff);
    }
}