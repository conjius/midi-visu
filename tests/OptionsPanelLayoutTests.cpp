#include <JuceHeader.h>
#include "../src/OptionsPanelLayout.h"

class OptionsPanelLayoutTests : public UnitTest {
public:
    OptionsPanelLayoutTests() : UnitTest("OptionsPanelLayout") {
    }

    void runTest() override {
        beginTest("default - all expanded - midiRouting header at 66");
        {
            OptionsPanelLayout layout;
            expectEquals(layout.sectionHeaderY(OptionsPanelLayout::MidiRouting), 66);
        }

        beginTest("default - all expanded - midiRouting first row at 88");
        {
            OptionsPanelLayout layout;
            expectEquals(layout.midiRoutingFirstRowY(), 88);
        }

        beginTest("default - all expanded - video header at 288");
        {
            // 66 + 20 (header+gap) + 182 (content) = 268... wait.
            // kFirstSectionY=66, kHeaderH=20, kSectionGap=20
            // MidiRouting header at 66, content starts at 86, content 182px
            // Next section: 66 + 20 + 20 + 182 = 288
            // But current code has videoY = 88 + 182 + 20 = 290
            // Let me check: in the old code firstY=88, videoY = firstY + 7*26 + 20 = 290
            // Our layout: sectionHeaderY(Video) = 66 + (20+20) + 182 = 288
            // This is 2px off from old 290. The old code had firstY=88 (66+22),
            // then videoY = 88 + 182 + 20 = 290. Our header+gap = 40, 66+40+182=288.
            // The difference is the 2px padding in midiRoutingFirstRowY.
            // Actually: the old VIDEO divider was at videoY-10=280, header text at videoY=290.
            // Our VIDEO header is at 288. Close enough, the 2px difference is acceptable
            // since we're reorganizing the layout.
            OptionsPanelLayout layout;
            expectEquals(layout.sectionHeaderY(OptionsPanelLayout::Video), 288);
        }

        beginTest("default - all expanded - circles header position");
        {
            OptionsPanelLayout layout;
            // Video: header at 288, content = kVideoContentH, headerH+gap = 40
            expectEquals(layout.sectionHeaderY(OptionsPanelLayout::Circles),
                         288 + 20 + 20 + OptionsPanelLayout::kVideoContentH);
        }

        beginTest("default - all expanded - animation header position");
        {
            OptionsPanelLayout layout;
            const int circlesH = layout.sectionHeaderY(OptionsPanelLayout::Circles);
            // Animation = circlesH + 20 + 20 + 80 = circlesH + 120
            expectEquals(layout.sectionHeaderY(OptionsPanelLayout::Animation),
                         circlesH + 20 + 20 + 80);
        }

        beginTest("fold midi routing - video shifts up by content height");
        {
            OptionsPanelLayout layout;
            const int videoBeforeFold = layout.sectionHeaderY(OptionsPanelLayout::Video);
            layout.setFolded(OptionsPanelLayout::MidiRouting, true);
            const int videoAfterFold = layout.sectionHeaderY(OptionsPanelLayout::Video);
            expectEquals(videoBeforeFold - videoAfterFold, 182);
        }

        beginTest("fold video - circles shifts up by video content height");
        {
            OptionsPanelLayout layout;
            const int circlesBefore = layout.sectionHeaderY(OptionsPanelLayout::Circles);
            layout.setFolded(OptionsPanelLayout::Video, true);
            const int circlesAfter = layout.sectionHeaderY(OptionsPanelLayout::Circles);
            expectEquals(circlesBefore - circlesAfter,
                         OptionsPanelLayout::kVideoContentH);
        }

        beginTest("fold all - content height is minimal");
        {
            OptionsPanelLayout layout;
            for (int s = 0; s < OptionsPanelLayout::SectionCount; ++s)
                layout.setFolded(static_cast<OptionsPanelLayout::Section>(s), true);
            // 66 + 4*(20+20) + 68 = 66 + 160 + 68 = 294
            expectEquals(layout.contentHeight(), 294);
        }

        beginTest("toggleFolded - flips state");
        {
            OptionsPanelLayout layout;
            expect(!layout.isFolded(OptionsPanelLayout::MidiRouting));
            layout.toggleFolded(OptionsPanelLayout::MidiRouting);
            expect(layout.isFolded(OptionsPanelLayout::MidiRouting));
            layout.toggleFolded(OptionsPanelLayout::MidiRouting);
            expect(!layout.isFolded(OptionsPanelLayout::MidiRouting));
        }

        beginTest("scroll - clamps to zero");
        {
            OptionsPanelLayout layout;
            layout.setScrollOffset(-100);
            expectEquals(layout.scrollOffset(), 0);
        }

        beginTest("scroll - clamps to max");
        {
            OptionsPanelLayout layout;
            layout.setViewportHeight(500);
            layout.setScrollOffset(99999);
            expectEquals(layout.scrollOffset(), layout.maxScrollOffset());
        }

        beginTest("scrollBy - accumulates and clamps");
        {
            OptionsPanelLayout layout;
            layout.setViewportHeight(500);
            layout.scrollBy(50);
            expectEquals(layout.scrollOffset(), 50);
            layout.scrollBy(-100);
            expectEquals(layout.scrollOffset(), 0);
        }

        beginTest("maxScrollOffset - large viewport returns 0");
        {
            OptionsPanelLayout layout;
            layout.setViewportHeight(5000);
            expectEquals(layout.maxScrollOffset(), 0);
        }

        beginTest("maxScrollOffset - small viewport returns positive");
        {
            OptionsPanelLayout layout;
            layout.setViewportHeight(500);
            expect(layout.maxScrollOffset() > 0);
            expectEquals(layout.maxScrollOffset(),
                         layout.contentHeight() - 500);
        }

        beginTest("hitTestHeader - identifies MidiRouting");
        {
            OptionsPanelLayout layout;
            const int hy = layout.sectionHeaderY(OptionsPanelLayout::MidiRouting);
            expectEquals(static_cast<int>(layout.hitTestHeader(hy)),
                         static_cast<int>(OptionsPanelLayout::MidiRouting));
            expectEquals(static_cast<int>(layout.hitTestHeader(hy + 10)),
                         static_cast<int>(OptionsPanelLayout::MidiRouting));
        }

        beginTest("hitTestHeader - identifies Video");
        {
            OptionsPanelLayout layout;
            const int hy = layout.sectionHeaderY(OptionsPanelLayout::Video);
            expectEquals(static_cast<int>(layout.hitTestHeader(hy)),
                         static_cast<int>(OptionsPanelLayout::Video));
        }

        beginTest("hitTestHeader - returns SectionCount for non-header Y");
        {
            OptionsPanelLayout layout;
            // Y=0 is above all headers
            expectEquals(static_cast<int>(layout.hitTestHeader(0)),
                         static_cast<int>(OptionsPanelLayout::SectionCount));
            // Y in content area (not header)
            const int contentY = layout.midiRoutingFirstRowY() + 50;
            expectEquals(static_cast<int>(layout.hitTestHeader(contentY)),
                         static_cast<int>(OptionsPanelLayout::SectionCount));
        }

        beginTest("hitTestHeader - correct after fold");
        {
            OptionsPanelLayout layout;
            layout.setFolded(OptionsPanelLayout::MidiRouting, true);
            const int videoH = layout.sectionHeaderY(OptionsPanelLayout::Video);
            expectEquals(static_cast<int>(layout.hitTestHeader(videoH)),
                         static_cast<int>(OptionsPanelLayout::Video));
        }

        beginTest("buttonsY - shifts with fold state");
        {
            OptionsPanelLayout layout;
            const int btnsBefore = layout.buttonsY();
            layout.setFolded(OptionsPanelLayout::Animation, true);
            const int btnsAfter = layout.buttonsY();
            expectEquals(btnsBefore - btnsAfter, 304);
        }

        beginTest("fold clamps scroll");
        {
            OptionsPanelLayout layout;
            layout.setViewportHeight(500);
            layout.setScrollOffset(layout.maxScrollOffset());
            const int scrollBefore = layout.scrollOffset();
            // Fold all sections — content shrinks, scroll should clamp down
            for (int s = 0; s < OptionsPanelLayout::SectionCount; ++s)
                layout.setFolded(static_cast<OptionsPanelLayout::Section>(s), true);
            expect(layout.scrollOffset() <= layout.maxScrollOffset());
            // With all folded and viewport 500, content is 294 < 500, so maxScroll = 0
            expectEquals(layout.scrollOffset(), 0);
        }

        beginTest("contentHeight - all expanded");
        {
            OptionsPanelLayout layout;
            const int expected = 66 + 4 * (20 + 20)
                                 + 182 + OptionsPanelLayout::kVideoContentH
                                 + 80 + 304 + 68;
            expectEquals(layout.contentHeight(), expected);
        }
    }
};

static OptionsPanelLayoutTests optionsPanelLayoutTests;
