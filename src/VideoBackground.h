#pragma once
#include <cstdint>
#include <memory>

// Decodes video frames via AVFoundation and exposes them as raw BGRA pixels so
// VideoBackground.mm never needs to include JuceHeader.h (which conflicts with
// AVFoundation headers due to 'Component' and OpenGL type clashes on macOS).
class VideoBackground
{
public:
    VideoBackground();
    ~VideoBackground();

    // Load and start looping the given file (pass absolute path as UTF-8 C string).
    void loadFile (const char* absolutePath);

    // Returns true if a frame is available.
    // outPixels points into an internal buffer (BGRA, 4 bytes/pixel).
    // Valid only until the next call to getLatestFrame().
    bool getLatestFrame (int& outWidth, int& outHeight, int& outStride,
                         const uint8_t*& outPixels);

private:
    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;
};
