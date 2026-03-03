#include "SvgShapeManager.h"
#include <cmath>

static constexpr float kTwoPi = 2.f * 3.14159265358979323846f;

void SvgShapeManager::loadShapes(const File& svgDir) {
    for (int i = 0; i < kNumVoices; ++i) {
        const File svgFile = svgDir.getChildFile("shape_" + String(i) + ".svg");

        if (svgFile.existsAsFile()) {
            auto xml = parseXML(svgFile);
            if (xml != nullptr) {
                auto drawable = Drawable::createFromSVG(*xml);
                if (drawable != nullptr) {
                    Path path;
                    // Get the outline path from the drawable
                    drawable->getDrawableBounds();
                    // Use the drawable's path
                    auto* shape = dynamic_cast<DrawablePath*>(drawable.get());
                    if (shape != nullptr) {
                        path = shape->getPath();
                    } else {
                        // Try composite — get bounds and create path from child shapes
                        auto* composite = dynamic_cast<DrawableComposite*>(drawable.get());
                        if (composite != nullptr) {
                            for (int c = 0; c < composite->getNumChildComponents(); ++c) {
                                if (auto* childShape = dynamic_cast<DrawablePath*>(
                                        composite->getChildComponent(c))) {
                                    path = childShape->getPath();
                                    break;
                                }
                            }
                        }
                    }

                    if (!path.isEmpty()) {
                        std::vector<float> xyPairs;
                        extractVertices(path, xyPairs);
                        if (xyPairs.size() >= 6) { // at least 3 vertices
                            int count = (int)(xyPairs.size() / 2);
                            SvgWobbleLogic::buildVertices(xyPairs.data(), count,
                                                          shapes_[i].vertices);
                            // Compute centroid
                            float cx = 0.f, cy = 0.f;
                            for (int v = 0; v < count; ++v) {
                                cx += xyPairs[v * 2];
                                cy += xyPairs[v * 2 + 1];
                            }
                            shapes_[i].centroidX = cx / (float)count;
                            shapes_[i].centroidY = cy / (float)count;
                            continue;
                        }
                    }
                }
            }
        }

        // Fallback: circle polygon
        buildCirclePolygon(kCircleVertices, shapes_[i]);
    }

    // Pre-allocate deform buffer for the largest shape
    int maxVerts = 0;
    for (int i = 0; i < kNumVoices; ++i)
        maxVerts = std::max(maxVerts, (int)shapes_[i].vertices.size());
    deformBuffer_.resize(maxVerts * 2);
}

void SvgShapeManager::drawShape(Graphics& g, int voiceIdx,
                                 float cx, float cy, float radius,
                                 Colour colour,
                                 const SvgWobbleLogic::VoiceState& state,
                                 float wobbleAmount) const {
    if (voiceIdx < 0 || voiceIdx >= kNumVoices) return;

    const auto& shape = shapes_[voiceIdx];
    int count = (int)shape.vertices.size();
    if (count < 3) return;

    // Ensure buffer is large enough
    if ((int)deformBuffer_.size() < count * 2)
        deformBuffer_.resize(count * 2);

    SvgWobbleLogic::deformVertices(shape.vertices.data(), count,
                                    state, wobbleAmount,
                                    cx, cy, radius,
                                    deformBuffer_.data());

    Path path = buildPathFromVertices(deformBuffer_.data(), count);
    g.setColour(colour);
    g.fillPath(path);
}

void SvgShapeManager::buildCirclePolygon(int numVertices, ShapeData& out) {
    std::vector<float> xyPairs(numVertices * 2);
    for (int i = 0; i < numVertices; ++i) {
        float angle = (float)i / (float)numVertices * kTwoPi;
        xyPairs[i * 2] = std::cos(angle);
        xyPairs[i * 2 + 1] = std::sin(angle);
    }
    SvgWobbleLogic::buildVertices(xyPairs.data(), numVertices, out.vertices);
    out.centroidX = 0.f;
    out.centroidY = 0.f;
}

void SvgShapeManager::extractVertices(const Path& path,
                                       std::vector<float>& xyPairs) {
    xyPairs.clear();
    Path::Iterator it(path);
    while (it.next()) {
        switch (it.elementType) {
            case Path::Iterator::startNewSubPath:
                xyPairs.push_back(it.x1);
                xyPairs.push_back(it.y1);
                break;
            case Path::Iterator::lineTo:
                xyPairs.push_back(it.x1);
                xyPairs.push_back(it.y1);
                break;
            case Path::Iterator::cubicTo: {
                // Subdivide cubic into ~4 segments
                if (xyPairs.size() >= 2) {
                    float startX = xyPairs[xyPairs.size() - 2];
                    float startY = xyPairs[xyPairs.size() - 1];
                    for (int s = 1; s <= 4; ++s) {
                        float t = (float)s / 4.f;
                        float mt = 1.f - t;
                        float x = mt * mt * mt * startX
                                  + 3.f * mt * mt * t * it.x1
                                  + 3.f * mt * t * t * it.x2
                                  + t * t * t * it.x3;
                        float y = mt * mt * mt * startY
                                  + 3.f * mt * mt * t * it.y1
                                  + 3.f * mt * t * t * it.y2
                                  + t * t * t * it.y3;
                        xyPairs.push_back(x);
                        xyPairs.push_back(y);
                    }
                }
                break;
            }
            case Path::Iterator::quadraticTo: {
                if (xyPairs.size() >= 2) {
                    float startX = xyPairs[xyPairs.size() - 2];
                    float startY = xyPairs[xyPairs.size() - 1];
                    for (int s = 1; s <= 4; ++s) {
                        float t = (float)s / 4.f;
                        float mt = 1.f - t;
                        float x = mt * mt * startX
                                  + 2.f * mt * t * it.x1
                                  + t * t * it.x2;
                        float y = mt * mt * startY
                                  + 2.f * mt * t * it.y1
                                  + t * t * it.y2;
                        xyPairs.push_back(x);
                        xyPairs.push_back(y);
                    }
                }
                break;
            }
            case Path::Iterator::closePath:
                break;
        }
    }
}

Path SvgShapeManager::buildPathFromVertices(const float* xyPairs, int count) {
    Path p;
    if (count < 1) return p;
    p.startNewSubPath(xyPairs[0], xyPairs[1]);
    for (int i = 1; i < count; ++i)
        p.lineTo(xyPairs[i * 2], xyPairs[i * 2 + 1]);
    p.closeSubPath();
    return p;
}
