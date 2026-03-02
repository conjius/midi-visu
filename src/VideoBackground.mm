// Include Apple frameworks ONLY — no JUCE headers to avoid 'Component' / OpenGL conflicts.
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

#include "VideoBackground.h"
#include <vector>
#include <mutex>

// ── Obj-C looper ──────────────────────────────────────────────────────────────
@interface VBLooper : NSObject
@property (nonatomic, assign) AVPlayer* player;
@end

@implementation VBLooper
- (void) itemDidFinish: (NSNotification*) note
{
    [_player seekToTime: CMTimeMakeWithSeconds(20.0, 600) toleranceBefore: kCMTimeZero
        toleranceAfter: kCMTimeZero];
    [_player play];
}
@end

// ── Pimpl ─────────────────────────────────────────────────────────────────────
struct VideoBackground::Pimpl
{
    AVPlayer*                player  = nil;
    AVPlayerItemVideoOutput* output  = nil;
    VBLooper*                looper  = nil;

    // Frame cache — written from any thread, read from message thread.
    std::mutex           mtx;
    std::vector<uint8_t> pixels;
    int                  frameW  = 0;
    int                  frameH  = 0;
    int                  frameStride = 0;

    Pimpl() {}

    ~Pimpl()
    {
        teardown();
    }

    void teardown()
    {
        if (looper && player)
        {
            [[NSNotificationCenter defaultCenter] removeObserver: looper
                                                            name: AVPlayerItemDidPlayToEndTimeNotification
                                                          object: player.currentItem];
        }
        [player pause];
        player = nil;
        output = nil;
        looper = nil;
    }

    void load (const char* path)
    {
        teardown();

        NSURL* url = [NSURL fileURLWithPath: [NSString stringWithUTF8String: path]];

        NSDictionary* settings = @{
            (NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
        };
        output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes: settings];

        AVPlayerItem* item = [AVPlayerItem playerItemWithURL: url];
        [item addOutput: output];

        player = [AVPlayer playerWithPlayerItem: item];
        player.volume = 0.0f;
        [player seekToTime: CMTimeMakeWithSeconds(20.0, 600) toleranceBefore:
            kCMTimeZero toleranceAfter: kCMTimeZero];
        [player play];

        looper = [[VBLooper alloc] init];
        looper.player = player;
        [[NSNotificationCenter defaultCenter] addObserver: looper
                                                 selector: @selector(itemDidFinish:)
                                                     name: AVPlayerItemDidPlayToEndTimeNotification
                                                   object: item];
    }

    bool getLatestFrame (int& outW, int& outH, int& outStride, const uint8_t*& outPixels)
    {
        if (! output || ! player)
            return false;

        CMTime t = [player currentTime];
        if ([output hasNewPixelBufferForItemTime: t])
        {
            CVPixelBufferRef pb = [output copyPixelBufferForItemTime: t itemTimeForDisplay: nullptr];
            if (pb)
            {
                CVPixelBufferLockBaseAddress (pb, kCVPixelBufferLock_ReadOnly);

                const int w      = (int)CVPixelBufferGetWidth (pb);
                const int h      = (int)CVPixelBufferGetHeight (pb);
                const int stride = (int)CVPixelBufferGetBytesPerRow (pb);
                const uint8_t* src = (const uint8_t*)CVPixelBufferGetBaseAddress (pb);

                {
                    std::lock_guard<std::mutex> lock (mtx);
                    pixels.resize ((size_t)h * stride);
                    std::memcpy (pixels.data(), src, pixels.size());
                    frameW      = w;
                    frameH      = h;
                    frameStride = stride;
                }

                CVPixelBufferUnlockBaseAddress (pb, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease (pb);
            }
        }

        std::lock_guard<std::mutex> lock (mtx);
        if (pixels.empty()) return false;
        outW      = frameW;
        outH      = frameH;
        outStride = frameStride;
        outPixels = pixels.data();
        return true;
    }
};

// ── VideoBackground ───────────────────────────────────────────────────────────
VideoBackground::VideoBackground()  : pimpl (std::make_unique<Pimpl>()) {}
VideoBackground::~VideoBackground() = default;

void VideoBackground::loadFile (const char* path)
{
    pimpl->load (path);
}

bool VideoBackground::getLatestFrame (int& outW, int& outH, int& outStride, const uint8_t*& outPixels)
{
    return pimpl->getLatestFrame (outW, outH, outStride, outPixels);
}
