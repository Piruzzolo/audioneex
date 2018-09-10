/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef FFT_H
#define FFT_H

#define _USE_MATH_DEFINES

#include <math.h>
#include <vector>
#include <cassert>
#include <fftss/fftss.h>

#include "AudioBlock.h"


class FFTFrame
{
 public:

    FFTFrame() : mSize(0), mData(nullptr) {}
    FFTFrame(int size) :
        mSize(size) {
        assert(size>0);
        mData = new float[size];
    }

    ~FFTFrame()  { delete[] mData; }

    float Magnitude(int i) const { assert(mData); return sqrt(mData[i]); }
    float Energy(int i)    const { assert(mData); return mData[i]; }
    float Power(int i)     const { assert(mData); return mData[i]/mSize; }
    int   Size()           const { return mSize; }
    float *Data()                { return mData; }
    void  Resize(size_t size)    { delete[] mData; mData=new float[size]; mSize=size;}

 private:

    float *mData;
    int    mSize;
};

// Implementation of a FFT transform based on FFTSS

class FFT
{
    size_t mWindowSize;    // The original non-zero padded data frame
    size_t mFFTFrameSize;  // The data frame size after zero-padding
    double mZeroPadFac;

    std::vector<double> mWindow;
    FFTFrame            mFFTFrame;

    std::vector<double> mInput;
    std::vector<double> mOutput;
    fftss_plan          mFFTPlan;

    void PrepareWindow(){
		
        mWindow.resize(mWindowSize);

        double scale = 2.f * M_PI / (mWindow.size()-1);
        for(size_t n=0; n<mWindow.size(); n++)
            mWindow[n] = 0.54 - 0.46 * cos(scale * n);
    }

    void ApplyWindow(std::vector<double> &dataFrame){
		
        assert(dataFrame.size() >= mWindowSize);
        for(size_t i=0; i<mWindowSize; i++)
            dataFrame[i*2] *= mWindow[i];
    }

 public:

    enum eSpectrumType
    {
       PowerSpectrum,
       MagnitudeSpectrum,
       EnergySpectrum
    };

    FFT(size_t windowSize, double zeroPadFactor){
		
        mWindowSize = windowSize;
        mZeroPadFac = zeroPadFactor;
        mFFTFrameSize = mWindowSize * (1.0 + mZeroPadFac);

        mFFTFrame.Resize(mFFTFrameSize/2 + 1);

        PrepareWindow();

        mInput.resize(mFFTFrameSize*2);
        mOutput.resize(mFFTFrameSize*2);

        mFFTPlan = fftss_plan_dft_1d(mFFTFrameSize,
                                     mInput.data(),
                                     mOutput.data(),
                                     FFTSS_FORWARD,
                                     FFTSS_ESTIMATE);
    }

    ~FFT(){}

    void Compute(AudioBlock<float> &block){
		
        assert(block.Size() <= mWindowSize);

        // build the zero-padded frame
        std::fill(mInput.begin(), mInput.end(), 0);

        float* block_ptr = block.Data();
        double* input_ptr = mInput.data();
        for(size_t n=0; n<block.Size(); n++)
            input_ptr[n*2] = block_ptr[n];

        // apply hamming window to input and execute fft
        ApplyWindow(mInput);
        fftss_execute(mFFTPlan);

        double *optr = mOutput.data();
        double *rptr = mOutput.data() + mFFTFrameSize;
        float *fftdata = mFFTFrame.Data();

        // extract DC offset and Nyquist's number
        fftdata[0] = optr[0] * optr[0];
        fftdata[mFFTFrameSize/2] = rptr[0] * rptr[0];

        optr+=2;
        fftdata++;
		
        // extract frequency coefficients and compute energy content
        for(; optr < rptr; optr+=2)
            *fftdata++ = optr[0] * optr[0] + optr[1] * optr[1];
    }

    FFTFrame& GetFFTFrame() { return mFFTFrame; }

};

#endif // FFT_H
