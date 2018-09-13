/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file    IasSrcFarrowConfig.hpp
 * @brief   Configuration parameters for the sample rate converter
 *          based on Farrow's structure. These parameters are processed
 *          at compile time.
 * @date    2015
 */

#ifndef IASSRCFARROWCONFIG_HPP_
#define IASSRCFARROWCONFIG_HPP_

/*
 *  Define for SSE optimization code
 */
#define IASSRCFARROWCONFIG_USE_SSE 1


/*
 *  Define whether float to int conversions shall saturate.
 *  If this flag is not set, the behavior in case of overmodulation is
 *  undefined, i.e., it might wrap-around or saturate or anything else.
 */
#define IASSRCFARROWCONFIG_USE_SATURATION 1


/*  Define the output gain, i.e., the factor that shall be applied to the
 *  output signal. If the output signal shall be converted to fixed point,
 *  this gain factor is applied before the conversion is done.
 *
 *  This factor is applied even if the output signal shall be provided in
 *  floating point format.
 *
 *  The specified value is a linear factor (not in dB).
 */
#define IASSRCFARROWCONFIG_OUTPUT_GAIN  (0.891250938f)


#endif // IASSRCFARROWCONFIG_HPP_
