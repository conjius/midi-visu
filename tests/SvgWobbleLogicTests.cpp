#include <JuceHeader.h>
#include "../src/SvgWobbleLogic.h"
#include <cmath>

class SvgWobbleLogicTests : public UnitTest {
public:
    SvgWobbleLogicTests() : UnitTest("SvgWobbleLogic") {}

    void runTest() override {
        beginTest("buildVertices - centroid of square");
        {
            float xy[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 4, verts);
            expectEquals((int)verts.size(), 4);
            float dx = 0.f - 0.5f;
            float dy = 0.f - 0.5f;
            float expectedR = std::sqrt(dx * dx + dy * dy);
            expectWithinAbsoluteError(verts[0].radius, expectedR, 0.001f);
            expectWithinAbsoluteError(verts[0].origX, 0.f, 0.001f);
            expectWithinAbsoluteError(verts[0].origY, 0.f, 0.001f);
        }

        beginTest("buildVertices - polar angles for known triangle");
        {
            float xy[] = {0.f, 1.f, -0.866f, -0.5f, 0.866f, -0.5f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 3, verts);
            expectEquals((int)verts.size(), 3);
            expectWithinAbsoluteError(verts[0].radius, verts[1].radius, 0.01f);
            expectWithinAbsoluteError(verts[1].radius, verts[2].radius, 0.01f);
        }

        beginTest("buildVertices - single vertex has zero radius");
        {
            float xy[] = {5.f, 3.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 1, verts);
            expectEquals((int)verts.size(), 1);
            expectWithinAbsoluteError(verts[0].radius, 0.f, 0.0001f);
        }

        beginTest("buildVertices - noise seeds are unique per vertex");
        {
            float xy[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 4, verts);
            for (int i = 0; i < 4; ++i)
                for (int j = i + 1; j < 4; ++j)
                    expect(std::abs(verts[i].noiseSeed - verts[j].noiseSeed) > 0.1f);
        }

        beginTest("deformVertices - zero wobbleAmount produces undeformed output");
        {
            float xy[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 4, verts);

            SvgWobbleLogic::VoiceState state;
            state.amplitude = 1.0f;
            state.phase = 0.5f;

            float outXY[8];
            SvgWobbleLogic::deformVertices(verts.data(), (int)verts.size(),
                                           state, 0.0f, 100.f, 100.f, 50.f,
                                           outXY);
            for (int i = 0; i < 4; ++i) {
                float expectedX = 100.f + (verts[i].origX - 0.5f) * 50.f;
                float expectedY = 100.f + (verts[i].origY - 0.5f) * 50.f;
                expectWithinAbsoluteError(outXY[i * 2], expectedX, 0.01f);
                expectWithinAbsoluteError(outXY[i * 2 + 1], expectedY, 0.01f);
            }
        }

        beginTest("deformVertices - zero amplitude produces undeformed output");
        {
            float xy[] = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 4, verts);

            SvgWobbleLogic::VoiceState state;
            state.amplitude = 0.0f;
            state.phase = 0.5f;

            float outXY[8];
            SvgWobbleLogic::deformVertices(verts.data(), (int)verts.size(),
                                           state, 1.0f, 100.f, 100.f, 50.f,
                                           outXY);
            for (int i = 0; i < 4; ++i) {
                float expectedX = 100.f + (verts[i].origX - 0.5f) * 50.f;
                float expectedY = 100.f + (verts[i].origY - 0.5f) * 50.f;
                expectWithinAbsoluteError(outXY[i * 2], expectedX, 0.01f);
                expectWithinAbsoluteError(outXY[i * 2 + 1], expectedY, 0.01f);
            }
        }

        beginTest("deformVertices - displacement bounded by kMaxDisplacement * radius");
        {
            const int N = 32;
            float xy[N * 2];
            for (int i = 0; i < N; ++i) {
                float a = (float)i / N * 2.f * 3.14159265f;
                xy[i * 2] = std::cos(a);
                xy[i * 2 + 1] = std::sin(a);
            }
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, N, verts);

            SvgWobbleLogic::VoiceState state;
            state.amplitude = 1.0f;
            state.phase = 1.23f;
            state.slowPhase = 2.45f;

            float outXY[N * 2];
            const float scale = 100.f;
            SvgWobbleLogic::deformVertices(verts.data(), N,
                                           state, 1.0f, 0.f, 0.f, scale,
                                           outXY);

            for (int i = 0; i < N; ++i) {
                float undeformedX = (verts[i].origX - 0.f) * scale;
                float undeformedY = (verts[i].origY - 0.f) * scale;
                float dx = outXY[i * 2] - undeformedX;
                float dy = outXY[i * 2 + 1] - undeformedY;
                float disp = std::sqrt(dx * dx + dy * dy);
                // Noise is normalised to [-1,1], so max disp = kMaxDisplacement * radius * scale
                float maxDisp = SvgWobbleLogic::kMaxDisplacement
                                * verts[i].radius * scale * 1.01f; // small epsilon
                expect(disp <= maxDisp + 0.01f);
            }
        }

        beginTest("deformVertices - different vertices get different displacements");
        {
            const int N = 8;
            float xy[N * 2];
            for (int i = 0; i < N; ++i) {
                float a = (float)i / N * 2.f * 3.14159265f;
                xy[i * 2] = std::cos(a);
                xy[i * 2 + 1] = std::sin(a);
            }
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, N, verts);

            SvgWobbleLogic::VoiceState state;
            state.amplitude = 1.0f;
            state.phase = 1.0f;
            state.slowPhase = 0.5f;

            float outXY[N * 2];
            SvgWobbleLogic::deformVertices(verts.data(), N,
                                           state, 1.0f, 0.f, 0.f, 100.f, outXY);
            // Compute per-vertex radial displacement
            float disps[N];
            bool allSame = true;
            for (int i = 0; i < N; ++i) {
                float ux = verts[i].origX * 100.f;
                float uy = verts[i].origY * 100.f;
                float dx = outXY[i * 2] - ux;
                float dy = outXY[i * 2 + 1] - uy;
                disps[i] = std::sqrt(dx * dx + dy * dy);
            }
            for (int i = 1; i < N; ++i)
                if (std::abs(disps[i] - disps[0]) > 0.01f) allSame = false;
            expect(!allSame); // vertices should NOT all have the same displacement
        }

        beginTest("deformVertices - deterministic for same state");
        {
            float xy[] = {0.f, 0.f, 1.f, 0.f, 0.5f, 1.f};
            std::vector<SvgWobbleLogic::Vertex> verts;
            SvgWobbleLogic::buildVertices(xy, 3, verts);

            SvgWobbleLogic::VoiceState state;
            state.amplitude = 0.7f;
            state.phase = 2.0f;
            state.slowPhase = 1.0f;

            float out1[6], out2[6];
            SvgWobbleLogic::deformVertices(verts.data(), 3,
                                           state, 0.5f, 50.f, 50.f, 30.f, out1);
            SvgWobbleLogic::deformVertices(verts.data(), 3,
                                           state, 0.5f, 50.f, 50.f, 30.f, out2);
            for (int i = 0; i < 6; ++i)
                expectWithinAbsoluteError(out1[i], out2[i], 0.0001f);
        }

        beginTest("advanceState - phase increments correctly");
        {
            SvgWobbleLogic::VoiceState state;
            state.phase = 0.0f;
            state.frequency = 1.0f;
            SvgWobbleLogic::advanceState(state, 0.25f);
            float expected = 2.f * 3.14159265f * 0.25f;
            expectWithinAbsoluteError(state.phase, expected, 0.001f);
        }

        beginTest("advanceState - phase wraps at 2pi");
        {
            SvgWobbleLogic::VoiceState state;
            state.phase = 6.0f;
            state.frequency = 1.0f;
            SvgWobbleLogic::advanceState(state, 0.1f);
            float twoPi = 2.f * 3.14159265f;
            expect(state.phase >= 0.f);
            expect(state.phase < twoPi);
        }

        beginTest("advanceState - slowPhase always advances");
        {
            SvgWobbleLogic::VoiceState state;
            state.slowPhase = 0.0f;
            state.frequency = 0.0f; // even with zero main frequency
            SvgWobbleLogic::advanceState(state, 1.0f);
            expect(state.slowPhase > 0.f); // slow drift should still advance
        }

        beginTest("advanceState - zero frequency no change to phase");
        {
            SvgWobbleLogic::VoiceState state;
            state.phase = 1.0f;
            state.frequency = 0.0f;
            SvgWobbleLogic::advanceState(state, 1.0f);
            expectWithinAbsoluteError(state.phase, 1.0f, 0.0001f);
        }

        beginTest("triggerDrumHit - amplitude 1 and targetAmplitude 0");
        {
            SvgWobbleLogic::VoiceState state;
            state.amplitude = 0.2f;
            state.targetAmplitude = 0.8f;
            SvgWobbleLogic::triggerDrumHit(state);
            expectWithinAbsoluteError(state.amplitude, 1.0f, 0.0001f);
            expectWithinAbsoluteError(state.targetAmplitude, 0.0f, 0.0001f);
        }

        beginTest("updateMelodicWobble - note maps to frequency range");
        {
            SvgWobbleLogic::VoiceState state;
            SvgWobbleLogic::updateMelodicWobble(state, 0);
            expectWithinAbsoluteError(state.frequency, 0.5f, 0.01f);

            SvgWobbleLogic::updateMelodicWobble(state, 127);
            expectWithinAbsoluteError(state.frequency, 2.5f, 0.01f);

            SvgWobbleLogic::updateMelodicWobble(state, 64);
            expect(state.frequency > 0.5f && state.frequency < 2.5f);
        }

        beginTest("updateMelodicWobble - note >= 0 sets target 1");
        {
            SvgWobbleLogic::VoiceState state;
            state.targetAmplitude = 0.0f;
            SvgWobbleLogic::updateMelodicWobble(state, 60);
            expectWithinAbsoluteError(state.targetAmplitude, 1.0f, 0.0001f);
        }

        beginTest("updateMelodicWobble - note < 0 sets target 0");
        {
            SvgWobbleLogic::VoiceState state;
            state.targetAmplitude = 1.0f;
            SvgWobbleLogic::updateMelodicWobble(state, -1);
            expectWithinAbsoluteError(state.targetAmplitude, 0.0f, 0.0001f);
        }

        beginTest("decayAmplitude - converges toward target");
        {
            SvgWobbleLogic::VoiceState state;
            state.amplitude = 1.0f;
            state.targetAmplitude = 0.0f;
            for (int i = 0; i < 100; ++i)
                SvgWobbleLogic::decayAmplitude(state, 0.1f);
            expect(state.amplitude < 0.01f);
        }

        beginTest("decayAmplitude - converges upward toward target");
        {
            SvgWobbleLogic::VoiceState state;
            state.amplitude = 0.0f;
            state.targetAmplitude = 1.0f;
            for (int i = 0; i < 100; ++i)
                SvgWobbleLogic::decayAmplitude(state, 0.1f);
            expect(state.amplitude > 0.99f);
        }

        beginTest("decayAmplitude - rate 0 means no change");
        {
            SvgWobbleLogic::VoiceState state;
            state.amplitude = 0.5f;
            state.targetAmplitude = 1.0f;
            SvgWobbleLogic::decayAmplitude(state, 0.0f);
            expectWithinAbsoluteError(state.amplitude, 0.5f, 0.0001f);
        }
    }
};

static SvgWobbleLogicTests svgWobbleLogicTests;
