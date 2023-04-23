#ifndef BIQUAD_H_
#define BIQUAD_H_

enum class FilterType
{
    LowPass = 1,
    HighPass,
    BandPass,
    Notch,
    AllPass,
    Peaking,
    LowShelf,
    HighShelf,
};

struct Parameters
{
    FilterType filterType;
    float fs;
    float f0;
    float Q;
    float dBGain;
};

class Biquad
{
private:
    FilterType mfilterType;
    
    Parameters mparams;

    // coefficients
    float ma0, ma1, ma2, mb0, mb1, mb2;

    // buffers
    float mx_z1, mx_z2, my_z1, my_z2;
    
    void calculateCoeffs();
    
public:
    Biquad(){};
    ~Biquad(){};
    void setParams(const Parameters& params);
    Parameters getParams();
    float process(float x);
};

#endif // BIQUAD_H_

