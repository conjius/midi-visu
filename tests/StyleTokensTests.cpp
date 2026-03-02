#include <JuceHeader.h>
#include "../src/StyleTokens.h"

class StyleTokensTests : public UnitTest {
public:
    StyleTokensTests() : UnitTest("StyleTokens") {
    }

    void runTest() override {
        beginTest("text alpha hierarchy");
        {
            expect(StyleTokens::alpha(StyleTokens::kPanelTitle) > StyleTokens::alpha(
                StyleTokens::kSectionHead));
            expect(StyleTokens::alpha(StyleTokens::kSectionHead) > StyleTokens::alpha(
                StyleTokens::kLabel));
            expect(StyleTokens::alpha(StyleTokens::kLabel) > StyleTokens::alpha(
                StyleTokens::kDivider));
            expect(StyleTokens::alpha(StyleTokens::kDivider) > StyleTokens::alpha(
                StyleTokens::kScrollTrack));
        }

        beginTest("panel backgrounds are semi-transparent");
        {
            expect(StyleTokens::alpha(StyleTokens::kPanelBg) < uint8_t(0xff));
            expect(StyleTokens::alpha(StyleTokens::kLogBg) < uint8_t(0xff));
        }

        beginTest("dialog background is opaque");
        {
            expect(StyleTokens::alpha(StyleTokens::kDialogBg) == uint8_t(0xff));
        }

        beginTest("font sizes");
        {
            expect(StyleTokens::kHeadingSize >= StyleTokens::kLabelSize);
            expect(StyleTokens::kHeadingSize > 0.0f);
            expect(StyleTokens::kLabelSize > 0.0f);
        }

        beginTest("slider track is brighter than slider background");
        {
            expect(StyleTokens::red(StyleTokens::kSliderTrack) > StyleTokens::red(
                StyleTokens::kSliderBg));
        }

        beginTest("range slider track is brighter than range slider background");
        {
            expect(StyleTokens::red(StyleTokens::kRangeSliderTrack) > StyleTokens::red(
                StyleTokens::kRangeSliderBg));
        }

        beginTest("seekbar track is brighter than seekbar background");
        {
            expect(StyleTokens::red(StyleTokens::kSeekBarTrack) > StyleTokens::red(
                StyleTokens::kSeekBarBg));
        }
    }
};

static StyleTokensTests styleTokensTests;