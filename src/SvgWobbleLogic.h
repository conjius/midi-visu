/*
  ==============================================================================
    SvgWobbleLogic -- pure C++ wobble deformation math for vertex arrays.
    No JUCE dependency, so it can be unit-tested independently.
  ==============================================================================
*/

#pragma once

#include <vector>

class SvgWobbleLogic {
public:
    static constexpr float kMaxDisplacement = 0.4f;

    struct Vertex {
        float origX = 0.f;
        float origY = 0.f;
        float angle = 0.f;      // polar angle from centroid
        float radius = 0.f;     // distance from centroid
        float noiseSeed = 0.f;  // unique per-vertex seed for noise function
    };

    struct VoiceState {
        float phase = 0.f;
        float slowPhase = 0.f;  // always-drifting slow phase for organic movement
        float amplitude = 0.f;
        float targetAmplitude = 0.f;
        float frequency = 1.f;  // Hz (slow default)
    };

    // Build vertices with polar coords from a flat xy array.
    // centroidX/Y are computed internally and stored as the origin for angles/radii.
    // The centroid is returned implicitly via the Vertex origX/origY fields (unchanged)
    // and the angle/radius fields (relative to centroid).
    static void buildVertices(const float* xyPairs, int count,
                              std::vector<Vertex>& out);

    // Deform vertices and write scaled/translated results to outXY (flat x,y pairs).
    // cx, cy = screen-space centre; scale = radius in pixels.
    // Vertices are positioned relative to centroid, so origX/origY are in shape-local space.
    static void deformVertices(const Vertex* verts, int count,
                               const VoiceState& state, float wobbleAmount,
                               float cx, float cy, float scale,
                               float* outXY);

    // Advance phase by frequency * 2pi * dt, wrap at 2pi.
    static void advanceState(VoiceState& state, float dt);

    // Drum hit: snap amplitude to 1.0, target to 0.0.
    static void triggerDrumHit(VoiceState& state);

    // Melodic wobble: map note 0-127 to frequency [2..8 Hz].
    // note >= 0 -> targetAmplitude = 1.0; note < 0 -> targetAmplitude = 0.0.
    static void updateMelodicWobble(VoiceState& state, int midiNote);

    // Smooth-step amplitude toward targetAmplitude at the given rate.
    static void decayAmplitude(VoiceState& state, float rate);
};
