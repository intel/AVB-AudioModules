/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file    IasSrcFarrowWrapper.cpp
 * @brief   Wrapper class for sample rate converter based on Farrow's structure.
 * @date    2016
 */

#include <internal/audio/common/samplerateconverter/IasSrcWrapper.hpp>
#include "internal/audio/common/IasAudioLogging.hpp"


namespace IasAudio {

static const std::string cClassName = "IasSrcWrapper::";
#define LOG_PREFIX cClassName + __func__ + "(" + std::to_string(__LINE__) + "):"

template <class T1, class T2>
IasSrcWrapper<T1,T2>::IasSrcWrapper()
  :mLog(IasAudioLogging::registerDltContext("SRC", "SRC Wrapper"))
  ,mInArea(nullptr)
  ,mOutArea(nullptr)
  ,mInputFormat(eIasFormatUndef)
  ,mOutputFormat(eIasFormatUndef)
  ,mInputSampleRate(0)
  ,mOutputSampleRate(0)
  ,mNumChannels(0)
  ,mInputIndex(0)
  ,mOutputIndex(0)
  ,mInputStride(0)
  ,mOutputStride(0)
  ,mSrc(nullptr)
  ,mSrcInputBuffers(nullptr)
  ,mSrcOutputBuffers(nullptr)
  ,mSrcInputBuffersStatic(nullptr)
  ,mSrcOutputBuffersStatic(nullptr)
{

}

template <class T1, class T2>
IasSrcWrapper<T1,T2>::~IasSrcWrapper()
{
  delete mSrc;
  delete[] mSrcInputBuffers;
  delete[] mSrcInputBuffersStatic;
  delete[] mSrcOutputBuffers;
  delete[] mSrcOutputBuffersStatic;
}

template <class T1, class T2>
IasSrcWrapperResult IasSrcWrapper<T1,T2>::reset()
{
  if (mSrc)
  {
    IasSrcFarrow::IasResult srcRes = mSrc->reset();
    IAS_ASSERT(srcRes == IasSrcFarrow::eIasOk);
    (void)srcRes; //at this point, it should never fail
  }
  return eIasOk;
}

template <class T1, class T2>
IasSrcWrapperResult IasSrcWrapper<T1,T2>::init(IasSrcWrapperParams& params,
                                               const IasAudioArea* inArea,
                                               const IasAudioArea* outArea)
{
  IasSrcWrapperResult res = checkParams(&params);
  if (res != eIasOk)
  {
    return eIasFailed;
  }

  if (inArea == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error, input area == nullptr");
    return eIasFailed;
  }
  if (outArea == nullptr)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error, output area == nullptr");
    return eIasFailed;
  }

  mNumChannels      = params.numChannels;
  mInputFormat      = params.inputFormat;
  mOutputFormat     = params.outputFormat;
  mInputSampleRate  = params.inputSampleRate;
  mOutputSampleRate = params.outputSampleRate;
  mInputIndex       = params.inputIndex;
  mOutputIndex      = params.outputIndex;


  mSrcInputBuffersStatic  = new const T1*[mNumChannels];
  mSrcOutputBuffersStatic = new T2*[mNumChannels];
  mSrcInputBuffers        = new const T1*[mNumChannels];
  mSrcOutputBuffers       = new T2*[mNumChannels];
  mInArea = inArea;
  mOutArea = outArea;

  if ((mInputIndex+mNumChannels) > (mInArea[0].maxIndex+1))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "number of channels and input start index do not fit to  input audio area");
    return eIasFailed;
  }

  if ((mOutputIndex+mNumChannels) > (mOutArea[0].maxIndex+1))
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "number of channels and output start index do not fit to output audio area");
    return eIasFailed;
  }

  for (uint32_t i=0 ; i<mNumChannels; i++)
  {
    mSrcInputBuffersStatic[i]  = (T1*)mInArea[i+mInputIndex].start + mInArea[i+mInputIndex].first/(8*sizeof(T1));

    mSrcOutputBuffersStatic[i] = (T2*)mOutArea[i+mOutputIndex].start + mOutArea[i+mOutputIndex].first/(8*sizeof(T2));
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "inputStatic:", (int64_t)mSrcInputBuffersStatic[i]);
    DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "outputStatic:", (int64_t)mSrcOutputBuffersStatic[i]);

  }
  mInputStride =  mInArea[0].step /(uint32_t)(8*sizeof(T1));
  mOutputStride = mOutArea[0].step /(uint32_t)(8*sizeof(T2));
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "inputStride:", mInputStride);
  DLT_LOG_CXX(*mLog, DLT_LOG_VERBOSE, LOG_PREFIX, "outputStride", mOutputStride);
  mSrc = new IasSrcFarrow();

  IasSrcFarrow::IasResult srcRes  = mSrc->init(mNumChannels);
  if(srcRes != IasSrcFarrow::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error in init call of src, error code:",toString(srcRes));
    return eIasFailed;
  }
  srcRes = mSrc->setConversionRatio(mInputSampleRate, mOutputSampleRate);
  if(srcRes != IasSrcFarrow::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error setting conversion ratio for input:",mInputSampleRate," and output:",mOutputSampleRate, ",error code:",toString(srcRes));
    return eIasFailed;
  }
  mSrc->setBufferMode(IasSrcFarrow::eIasLinearBufferMode);

  return eIasOk;
}

template <class T1, class T2>
IasSrcWrapperResult IasSrcWrapper<T1,T2>::process(uint32_t *numGeneratedSamples,
                                                  uint32_t *numConsumedSamples,
                                                  uint32_t lengthInputBuffers,
                                                  uint32_t numOutputSamples,
                                                  uint32_t srcOffset,
                                                  uint32_t sinkOffset)
{
  uint32_t readIndex = 0;

  for (uint32_t i = 0; i < mNumChannels; i++)
  {
    mSrcInputBuffers[i] =  mSrcInputBuffersStatic[i] + srcOffset * mInputStride;
    mSrcOutputBuffers[i] =  mSrcOutputBuffersStatic[i] + sinkOffset * mOutputStride;
  }

  IasSrcFarrow::IasResult srcRes = mSrc->processPullMode(mSrcOutputBuffers,
                                                         mSrcInputBuffers,
                                                         mOutputStride,
                                                         mInputStride,
                                                         numGeneratedSamples,
                                                         numConsumedSamples,
                                                         &readIndex,
                                                         0,
                                                         lengthInputBuffers,
                                                         numOutputSamples,
                                                         mNumChannels,
                                                         1.0f);

  if(srcRes != IasSrcFarrow::eIasOk)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Error during processing of sample rate converter");
    return eIasFailed;
  }
  else
  {
    return eIasOk;
  }
}

template < class T1, class T2>
IasSrcWrapperResult IasSrcWrapper<T1,T2>::checkParams(IasSrcWrapperParams *params) const
{
  if (params->inputFormat == eIasFormatUndef || params->outputFormat == eIasFormatUndef)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Sample format error, input or output format not defined");
    return eIasFailed;
  }
  if(params->inputSampleRate < cMinSampleRate || params->inputSampleRate > cMaxSampleRate)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Input sample rate out of range:", params->inputSampleRate);
    return eIasFailed;
  }
  if (params->outputSampleRate < cMinSampleRate || params->outputSampleRate > cMaxSampleRate)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Output sample rate out of range:", params->outputSampleRate);
    return eIasFailed;
  }
  if (params->numChannels == 0)
  {
    DLT_LOG_CXX(*mLog, DLT_LOG_ERROR, LOG_PREFIX, "Invalid number of channels:", params->numChannels);
    return eIasFailed;
  }

  return eIasOk;

}

template class IasSrcWrapper<float, float>;
template class IasSrcWrapper<float, int32_t>;
template class IasSrcWrapper<float, int16_t>;
template class IasSrcWrapper<int16_t, int16_t>;
template class IasSrcWrapper<int16_t, int32_t>;
template class IasSrcWrapper<int16_t, float>;
template class IasSrcWrapper<int32_t, int32_t>;
template class IasSrcWrapper<int32_t, int16_t>;
template class IasSrcWrapper<int32_t, float>;

}
