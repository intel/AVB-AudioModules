# Changes

@subpage v2_2_0

@subpage v2_1_6

@subpage v2_1_5

@subpage v2_1_4

@subpage v2_1_3

@subpage v2_1_2

@subpage v2_1_1

@subpage v2_1_0

@subpage v2_0_0

@subpage v1_14_3

@subpage v1_14_2

@subpage v1_14_1

@subpage v1_14_0

@subpage v1_13_0

@subpage v1_12_0

@subpage v1_11_0

@subpage v1_10_0

@subpage v1_9_9

@subpage v1_9_8

@subpage v1_9_7

@subpage v1_9_6

@subpage v1_9_5

@subpage v1_9_4

@subpage v1_9_3

@subpage v1_9_0

@subpage v1_8_1

@subpage v1_8_0

@subpage v1_7_0

@subpage v1_6_1

@subpage v1_6_0

@page v2_2_0 2.2.0

* general updates to support probing of pins.

@page v2_1_6 2.1.6

* update ringbuffer mirror to show state of device when a timeout occurs.

@page v2_1_5 2.1.5

* use robust mutex instead of simple mutex to ensure the device is not incorrectly marked as busy, even when the client doesn't do a proper shutdown and clean-up.

@page v2_1_4 2.1.4

* create an error when trying to open the same ALSA device more than once. 

@page v2_1_3 2.1.3

* fix use-after-free issue in alsa-smartx-plugin, which could lead to a segfault during snd\_pcm\_close.
* fix race-condition in process shared mutex and conditional variable implementation.
* avoid exception during connect with shared memory and return with an error instead.
* add reset functionality to sample rate converter wrapper.

@page v2_1_2 2.1.2

* restructure CMake files for source delivery

@page v2_1_1 2.1.1

* Do a proper clean-up in case the shared memory ring buffer is in the locked state, so that a subsequent access is successful again.

@page v2_1_0 2.1.0

* Add method for completely zeroing out a full ring buffer.

@page v2_0_0 2.0.0

* Implement the new callback _get_real_avail_ introduced by a fix of the ioplug in the alsa-lib. This fixes the issue
  that it is not possible to differentiate between buffer full and buffer empty from the ioplug.
  @note The patch introducing the callback to the alsa-lib has to be applied before, else the build will fail.

@page v1_14_3 1.14.3

* Change log levels of some important messages of ring buffer from warning to error.

@page v1_14_2 1.14.2

* Change handling of FdSignal.
  The FdSignal is now cleared after one period has been removed from the buffer.
  This solves all issues were a different usage of the ALSA API lead to an
  unbalanced count of FdSignal write and read, which lead to incorrect triggering of the
  snd\_pcm\_wait function.
* add function to query the version of the ias-audio-common package.
* Removed attenuation by 0.0873 dB in IasSrcFarrow.cpp (coefficients were multiplied by 0.99).
  This attenuation is obsolete now, because meanwhile the file IasSrcFarrowConfig.hpp defines
  an output gain of -1.0 dB.

@page v1_14_1 1.14.1

Ring buffer triggers fdSignal only if at least avail_min frames available.
This guarantees that a snd_pcm_wait function call from the client-side does not
return before at least avail_min frames are available within the ring buffer.

@page v1_14_0 1.14.0

Several changes to return correct hw pointer to alsa-lib.

@page v1_13_0 1.13.0

* Allow new sample rate conversion ratios from input sample rate = 48 kHz to output sample rates > 48 kHz.
* Additionally change group ownership of generated fifos to same group name than shared memories after creation. This is also currently done by using the sticky bit of the /run/smartx directory.

@page v1_12_0 1.12.0

* Improved rounding for 32 bit to 16 bit conversion in copy function
* Optimized internal copy function for non-interleaved access

@page v1_11_0 1.11.0

* Fix handling of polling mechanism used for snd\_pcm\_wait. The fifo used in fdsignal is now completely cleared after every read.
* Change data layout of internally used ring-buffers from interleaved to non-interleaved.

@page v1_10_0 1.10.0

* Create all shared memory files using a specific group name.
  The default group name which is used to create shared memory files is **ias_audio**.
* Fixed potential crash of proxy server when destroy local stream is called".

@page v1_9_9 1.9.9

* Fix for memory leak in SmartXBar plugin (and possible crash when snd_pcm_close is called). Correct usage of delete[] now implemented

@page v1_9_8 1.9.8

* Ring buffer now provides function to trigger associated IasFdSignal.

@page v1_9_7 1.9.7

* Improved logging messages

@page v1_9_6 1.9.6

* Fixed stop/start behavior of ALSA handler (restart of ALSA device did not work).

@page v1_9_5 1.9.5

* Updated handling for error EAGAIN in method write of fdsignal: just print a warning once and then change the log level to verbose.
* Fixed umask setting before creating fifo: the write permission has to be kept for the group.

@page v1_9_4 1.9.4

* Fixed initialization of IasFdSignal name.

@page v1_9_3 1.9.3

* Introduced new signaling mechanism to be used for poll of smartx-plugin.
  This fixes the problem that snd_pcm_wait might block endlessly.

  A named pipe (fifo) is used, which has an entry in the runtime directory
  of the SmartXbar (/run/smartx). This is known by each process and can be
  opened and managed using the newly introduced class IasFdSignal.


@page v1_9_0 1.9.0

* Added method (to internal API) to explicitly start the ALSA pcm device.

@page v1_8_1 1.8.1

* Fixed nonblocking streaming issue for alsa_smartx_plugin in playback direction (interoperability with media player).

@page v1_8_0 1.8.0

* Fixed several Klocwork issues.
* Fixed crash that might occur if the AudioRingBufferMirror has not been initialized appropriately.

@page v1_7_0 1.7.0

* Added new types that are required for pipelines and processing modules.
* Added DLT logging to AudioRingBufferMirror.
* Adding latency test tool.
* Fixed several Klocwork issues.

@page v1_6_1 1.6.1

* Added several helper functions for data probing.
* Added new types that are required for pipelines.

@page v1_6_0 1.6.0

* Several bug fixes.
