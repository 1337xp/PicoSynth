#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include "Biquad.h"

void Biquad::setParams(const Parameters& params)
{
    mparams = params;
    calculateCoeffs();
}

Parameters Biquad::getParams()
{
    return mparams;
}

void Biquad::calculateCoeffs()
{
    float omega0 = 2.0f * M_PI * (mparams.f0 / mparams.fs);
    float alpha = std::sin(omega0) / (2.0 * mparams.Q);
    float A = std::pow(10, mparams.dBGain / 40.0);
    switch (mparams.filterType)
    {
    case FilterType::LowPass:
    {
        ma0 = 1.0 + alpha;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha;
        mb0 = (1.0 - std::cos(omega0)) / 2.0;
        mb1 = 1.0 - std::cos(omega0);
        mb2 = (1.0 - std::cos(omega0)) / 2.0;
        break;
    }
    case FilterType::HighPass:
    {
        ma0 = 1.0 + alpha;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha;
        mb0 = (1.0 + std::cos(omega0)) / 2.0;
        mb1 = -(1.0 + std::cos(omega0));
        mb2 = (1.0 + std::cos(omega0)) / 2.0;
        break;
    }
    case FilterType::BandPass:
    {
        ma0 = 1.0 + alpha;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha;
        mb0 = alpha;
        mb1 = 0;
        mb2 = -alpha;
        break;
    }
    case FilterType::Notch:
    {
        ma0 = 1.0 + alpha;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha;
        mb0 = 1.0;
        mb1 = -2.0 * std::cos(omega0);
        mb2 = 1.0;
        break;
    }
    case FilterType::AllPass:
    {
        ma0 = 1.0 + alpha;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha;
        mb0 = 1.0 - alpha;
        mb1 = -2.0 * std::cos(omega0);
        mb2 = 1.0 + alpha;
        break;
    }
    case FilterType::Peaking:
    {
        ma0 = 1.0 + alpha / A;
        ma1 = -2.0 * std::cos(omega0);
        ma2 = 1.0 - alpha / A;
        mb0 = 1.0 + alpha * A;
        mb1 = -2.0 * std::cos(omega0);
        mb2 = 1.0 - alpha * A;
        break;
    }
    case FilterType::LowShelf:
    {
        ma0 = (A + 1.0) + (A - 1.0) * std::cos(omega0) + 2.0 * std::sqrt(A) * alpha;
        ma1 = -2.0 * ((A - 1.0) + (A + 1.0) * std::cos(omega0));
        ma2 = (A + 1.0) + (A - 1.0) * std::cos(omega0) - 2.0 * std::sqrt(A) * alpha;
        mb0 = A * ((A + 1.0) - (A - 1.0) * std::cos(omega0) + 2.0 * std::sqrt(A) * alpha);
        mb1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * std::cos(omega0));
        mb2 = A * ((A + 1.0) - (A - 1.0) * std::cos(omega0) - 2.0 * std::sqrt(A) * alpha);
        break;
    }
    case FilterType::HighShelf:
    {
        ma0 = (A + 1.0) - (A - 1.0) * std::cos(omega0) + 2.0 * std::sqrt(A) * alpha;
        ma1 = 2.0 * ((A - 1.0) - (A + 1.0) * std::cos(omega0));
        ma2 = (A + 1.0) - (A - 1.0) * std::cos(omega0) - 2.0 * std::sqrt(A) * alpha;
        mb0 = A * ((A + 1.0) + (A - 1.0) * std::cos(omega0) + 2.0 * std::sqrt(A) * alpha);
        mb1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * std::cos(omega0));
        mb2 = A * ((A + 1.0) + (A - 1.0) * std::cos(omega0) - 2.0 * std::sqrt(A) * alpha);
        break;
    }
    default:
        break;
    }
}

float Biquad::process(float x)
{
    float y = (mb0 / ma0) * x + (mb1 / ma0) * mx_z1 + (mb2 / ma0) * mx_z2 - (ma1 / ma0) * my_z1 - (ma2 / ma0) * my_z2;

    mx_z2 = mx_z1;
    mx_z1 = x;

    my_z2 = my_z1;
    my_z1 = y;

    return y;
}
