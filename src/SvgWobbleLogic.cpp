#include "SvgWobbleLogic.h"
#include <cmath>

static constexpr float kTwoPi = 2.f * 3.14159265358979323846f;
static constexpr float kPhi = 1.6180339887f;       // golden ratio
static constexpr float kSqrtE = 1.6487212707f;     // sqrt(e)
static constexpr float kInvSqrt3 = 0.5773502692f;  // 1/sqrt(3)
static constexpr float kSlowDriftHz = 0.13f;        // always-on slow drift frequency

// Per-vertex pseudo-noise: 4 sines with irrational frequency ratios.
// Returns value in [-1, 1].
static float vertexNoise(float seed, float phase, float slowPhase) {
    float n = std::sin(seed * 1.0f   + phase * 0.7f  + slowPhase * 0.3f)  * 1.0f
            + std::sin(seed * kPhi   + phase * 1.3f  + slowPhase * 0.7f)  * 0.7f
            + std::sin(seed * kSqrtE + phase * 0.4f  + slowPhase * 1.1f)  * 0.5f
            + std::sin(seed * kInvSqrt3 + phase * 2.1f + slowPhase * 0.2f) * 0.3f;
    return n / 2.5f; // normalise to ~[-1, 1]
}

void SvgWobbleLogic::buildVertices(const float* xyPairs, int count,
                                   std::vector<Vertex>& out) {
    out.resize(count);
    if (count <= 0) return;

    // Compute centroid
    float cx = 0.f, cy = 0.f;
    for (int i = 0; i < count; ++i) {
        cx += xyPairs[i * 2];
        cy += xyPairs[i * 2 + 1];
    }
    cx /= (float)count;
    cy /= (float)count;

    // Store original coords, polar coords, and unique noise seed
    for (int i = 0; i < count; ++i) {
        float ox = xyPairs[i * 2];
        float oy = xyPairs[i * 2 + 1];
        float dx = ox - cx;
        float dy = oy - cy;
        out[i].origX = ox;
        out[i].origY = oy;
        out[i].angle = std::atan2(dy, dx);
        out[i].radius = std::sqrt(dx * dx + dy * dy);
        // Golden-ratio based seed gives good distribution across vertices
        out[i].noiseSeed = (float)(i + 1) * kPhi * 6.2831853f;
    }
}

void SvgWobbleLogic::deformVertices(const Vertex* verts, int count,
                                     const VoiceState& state, float wobbleAmount,
                                     float cx, float cy, float scale,
                                     float* outXY) {
    if (count <= 0) return;

    // Compute centroid of the original vertices for centering
    float centX = 0.f, centY = 0.f;
    for (int i = 0; i < count; ++i) {
        centX += verts[i].origX;
        centY += verts[i].origY;
    }
    centX /= (float)count;
    centY /= (float)count;

    const float wa = wobbleAmount * state.amplitude;

    for (int i = 0; i < count; ++i) {
        // Base position: translate to screen space
        float localX = (verts[i].origX - centX) * scale;
        float localY = (verts[i].origY - centY) * scale;

        if (wa > 0.f && verts[i].radius > 0.f) {
            // Per-vertex noise-based radial displacement
            float noise = vertexNoise(verts[i].noiseSeed,
                                       state.phase, state.slowPhase);
            float disp = wa * noise * verts[i].radius * kMaxDisplacement * scale;

            // Apply displacement radially
            float cosA = std::cos(verts[i].angle);
            float sinA = std::sin(verts[i].angle);
            localX += cosA * disp;
            localY += sinA * disp;
        }

        outXY[i * 2] = cx + localX;
        outXY[i * 2 + 1] = cy + localY;
    }
}

void SvgWobbleLogic::advanceState(VoiceState& state, float dt) {
    state.phase += state.frequency * kTwoPi * dt;
    while (state.phase >= kTwoPi)
        state.phase -= kTwoPi;

    // Slow drift always advances — gives organic movement even when idle
    state.slowPhase += kSlowDriftHz * kTwoPi * dt;
    while (state.slowPhase >= kTwoPi)
        state.slowPhase -= kTwoPi;
}

void SvgWobbleLogic::triggerDrumHit(VoiceState& state) {
    state.amplitude = 1.0f;
    state.targetAmplitude = 0.0f;
}

void SvgWobbleLogic::updateMelodicWobble(VoiceState& state, int midiNote) {
    if (midiNote >= 0) {
        // Slow, organic range: 0.5 Hz (low notes) to 2.5 Hz (high notes)
        state.frequency = 0.5f + (midiNote / 127.f) * 2.0f;
        state.targetAmplitude = 1.0f;
    } else {
        state.targetAmplitude = 0.0f;
    }
}

void SvgWobbleLogic::decayAmplitude(VoiceState& state, float rate) {
    state.amplitude += (state.targetAmplitude - state.amplitude) * rate;
}
