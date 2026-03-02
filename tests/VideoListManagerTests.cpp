#include <JuceHeader.h>
#include "../src/VideoListManager.h"

class VideoListManagerTests : public UnitTest {
public:
    VideoListManagerTests() : UnitTest("VideoListManager") {
    }

    void runTest() override {
        beginTest("default state - empty list, no selection, stopped");
        {
            VideoListManager m;
            expectEquals(m.fileCount(), 0);
            expectEquals(m.selectedIndex(), -1);
            expect(m.playState() == VideoListManager::PlayState::Stopped);
            expect(!m.isPlaying());
        }

        beginTest("setFiles - populates list");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 10.0}, {"b.mov", 20.0}});
            expectEquals(m.fileCount(), 2);
            expect(m.filename(0) == "a.mp4");
            expect(m.filename(1) == "b.mov");
        }

        beginTest("setFiles - stores durations");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 65.0}, {"b.mov", 130.5}});
            expectWithinAbsoluteError(m.duration(0), 65.0, 1e-9);
            expectWithinAbsoluteError(m.duration(1), 130.5, 1e-9);
        }

        beginTest("setFiles - replaces previous list");
        {
            VideoListManager m;
            m.setFiles({{"old.mp4", 5.0}});
            m.setFiles({{"new.mp4", 10.0}});
            expectEquals(m.fileCount(), 1);
            expect(m.filename(0) == "new.mp4");
            expectWithinAbsoluteError(m.duration(0), 10.0, 1e-9);
        }

        beginTest("setFiles - clears out-of-range selection");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}, {"b.mp4", 2.0}});
            m.setSelectedIndex(1);
            m.setFiles({{"a.mp4", 1.0}}); // index 1 is now out of range
            expectEquals(m.selectedIndex(), -1);
        }

        beginTest("setFiles - keeps in-range selection");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}, {"b.mp4", 2.0}});
            m.setSelectedIndex(0);
            m.setFiles({{"a.mp4", 1.0}, {"b.mp4", 2.0}, {"c.mp4", 3.0}});
            expectEquals(m.selectedIndex(), 0);
        }

        beginTest("setSelectedIndex - valid index");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}, {"b.mp4", 2.0}});
            m.setSelectedIndex(1);
            expectEquals(m.selectedIndex(), 1);
        }

        beginTest("setSelectedIndex - minus-one clears selection");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}});
            m.setSelectedIndex(0);
            m.setSelectedIndex(-1);
            expectEquals(m.selectedIndex(), -1);
        }

        beginTest("setSelectedIndex - negative clamped to minus-one");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}});
            m.setSelectedIndex(-5);
            expectEquals(m.selectedIndex(), -1);
        }

        beginTest("setSelectedIndex - above count clamped to last");
        {
            VideoListManager m;
            m.setFiles({{"a.mp4", 1.0}, {"b.mp4", 2.0}});
            m.setSelectedIndex(99);
            expectEquals(m.selectedIndex(), 1);
        }

        beginTest("setSelectedIndex - on empty list gives minus-one");
        {
            VideoListManager m;
            m.setSelectedIndex(0);
            expectEquals(m.selectedIndex(), -1);
        }

        beginTest("playState - Playing sets isPlaying true");
        {
            VideoListManager m;
            m.setPlayState(VideoListManager::PlayState::Playing);
            expect(m.isPlaying());
            expect(m.playState() == VideoListManager::PlayState::Playing);
        }

        beginTest("playState - Paused is not playing");
        {
            VideoListManager m;
            m.setPlayState(VideoListManager::PlayState::Paused);
            expect(!m.isPlaying());
            expect(m.playState() == VideoListManager::PlayState::Paused);
        }

        beginTest("playState - Stopped is not playing");
        {
            VideoListManager m;
            m.setPlayState(VideoListManager::PlayState::Stopped);
            expect(!m.isPlaying());
            expect(m.playState() == VideoListManager::PlayState::Stopped);
        }

        beginTest("isPlaying - independent of selection state");
        {
            VideoListManager m;
            // No file selected, but play state can still be set
            m.setPlayState(VideoListManager::PlayState::Playing);
            expect(m.isPlaying());
            expectEquals(m.selectedIndex(), -1);
        }
    }
};

static VideoListManagerTests videoListManagerTests;