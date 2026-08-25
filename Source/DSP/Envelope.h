#pragma once

#include <cmath>
#include <algorithm>

namespace zx::dsp
{

/** Classic one-pole attack/release follower operating in the dB domain. */
class Ballistics
{
public:
    void reset (float initial = 0.0f) noexcept { env = initial; }

    void setTimes (double sampleRate, float attackMs, float releaseMs) noexcept
    {
        attackCoeff  = coeffFor (sampleRate, attackMs);
        releaseCoeff = coeffFor (sampleRate, releaseMs);
    }

    /** Rises with `attack`, falls with `release`. */
    float process (float x) noexcept
    {
        const auto c = (x > env) ? attackCoeff : releaseCoeff;
        env = c * env + (1.0f - c) * x;
        return env;
    }

    /** Instant attack, smoothed release — used for gain-reduction smoothing. */
    float processPeak (float x) noexcept
    {
        env = (x > env) ? x : releaseCoeff * env + (1.0f - releaseCoeff) * x;
        return env;
    }

    float current() const noexcept { return env; }

private:
    static float coeffFor (double sampleRate, float ms) noexcept
    {
        if (ms <= 0.0f)
            return 0.0f;
        return std::exp (-1.0f / (float) (0.001 * (double) ms * sampleRate));
    }

    float env = 0.0f, attackCoeff = 0.0f, releaseCoeff = 0.0f;
};

/** Simple de-normal-safe linear smoother for control-rate values. */
class Smoothed
{
public:
    void prepare (double sampleRate, float timeMs) noexcept
    {
        coeff = std::exp (-1.0f / (float) (0.001 * (double) timeMs * sampleRate));
    }

    void setTarget (float t) noexcept { target = t; }
    void snap (float t) noexcept      { target = value = t; }

    float next() noexcept
    {
        value = coeff * value + (1.0f - coeff) * target;
        if (std::abs (value - target) < 1.0e-7f)
            value = target;
        return value;
    }

    float current() const noexcept { return value; }

private:
    float value = 0.0f, target = 0.0f, coeff = 0.0f;
};

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

inline float gainToDb (float g) noexcept
{
    return 20.0f * std::log10 (std::max (g, 1.0e-7f));
}

} // namespace zx::dsp
