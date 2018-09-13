/*
 * @COPYRIGHT_TAG@
 */
/**
 * @file    IasTimeStampCounter.hpp
 * @brief   Time Stamp Counter, based on rdtsc, to be used for performance optimizations.
 * @date    May 06, 2013
 */

#ifndef IASTIMESTAMPCOUNTER_HPP_
#define IASTIMESTAMPCOUNTER_HPP_



namespace IasAudio {


/**
 * @brief Private inline function to get the current time stamp (rdtsc)
 */
inline uint32_t getTimeStamp()
{
  uint32_t  tsc=0;

  __asm__ volatile(
                   "rdtsc;"
                   "movl %%eax, %0;"
                   : "=r" (tsc)
                   : "0" (tsc)
                   : "%eax", "%edx"
                   );
  return tsc;
}


/**
 * @brief Private inline function to get the current time stamp (rdtsc), 64 bit version
 */
inline uint64_t getTimeStamp64(void)
{
  uint32_t a, d;

  __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

  return (((uint64_t)a) | (((uint64_t)d) << 32));
}


class IasTimeStampCounter
{
  public:
    /**
     * @brief Constructor.
     */
    IasTimeStampCounter() { mTimeStampStart = getTimeStamp(); };

    /**
     * @brief Destructor, virtual by default.
     */
    virtual ~IasTimeStampCounter() {};

    /**
     * @brief reset() method -> starts a new measurement.
     */
    void reset() { mTimeStampStart = static_cast<uint64_t>(getTimeStamp()); };

    /**
     * @brief get() method -> gets the number of (rdtsc) clock ticks since the last reset.
     */
    uint32_t get() { return (getTimeStamp() - mTimeStampStart); };


  private:
    /**
     * @brief Copy constructor, private unimplemented to prevent misuse.
     */
    IasTimeStampCounter(IasTimeStampCounter const &other);

    /**
     * @brief Assignment operator, private unimplemented to prevent misuse.
     */
    IasTimeStampCounter& operator=(IasTimeStampCounter const &other);

    // Member variables
    uint32_t  mTimeStampStart; ///< The rdtsc time stamp when the reset() method was called.
};



} // namespace IasAudio


#endif // IASTIMESTAMPCOUNTER_HPP_
