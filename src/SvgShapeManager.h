/*
  ==============================================================================
    SvgShapeManager -- loads SVG shapes, extracts vertices, renders deformed paths.
    JUCE-dependent.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SvgWobbleLogic.h"
#include <vector>

class SvgShapeManager {
public:
    // Load shape_0.svg..shape_6.svg from svgDir.
    // Missing files fall back to a 32-vertex circle polygon.
    void loadShapes(const File& svgDir);

    // Draw a deformed shape for voice voiceIdx at screen position (cx, cy)
    // with given radius, colour, wobble state and wobble amount.
    void drawShape(Graphics& g, int voiceIdx,
                   float cx, float cy, float radius,
                   Colour colour,
                   const SvgWobbleLogic::VoiceState& state,
                   float wobbleAmount) const;

private:
    static constexpr int kNumVoices = 7;
    static constexpr int kCircleVertices = 32;

    struct ShapeData {
        std::vector<SvgWobbleLogic::Vertex> vertices;
        float centroidX = 0.f;
        float centroidY = 0.f;
    };

    ShapeData shapes_[kNumVoices];
    mutable std::vector<float> deformBuffer_;

    // Build a regular polygon with N vertices as fallback circle.
    static void buildCirclePolygon(int numVertices, ShapeData& out);

    // Extract vertices from a JUCE Path.
    static void extractVertices(const Path& path, std::vector<float>& xyPairs);

    // Build a JUCE Path from flat xy pairs.
    static Path buildPathFromVertices(const float* xyPairs, int count);
};
