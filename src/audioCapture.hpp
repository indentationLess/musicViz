#include <cmath>
#include <fftw3.h>
#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>

#define FRAMES_PER_BUFFER 256
class AudioPlayer {
private:
  struct AudioData {
    std::vector<float> buffer;
    size_t position;
    unsigned long sampleRate;
    int channels;
    fftw_complex *fftw_output;
    fftw_plan fftw_plan_;
  };
  AudioData audioData;
  PaStream *stream;

  static int audioCallback(const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo *timeInfo,
                           PaStreamCallbackFlags statusFlags, void *userData) {
    AudioData *audioData = (AudioData *)userData;
    float *out = (float *)outputBuffer;
    unsigned long remaining = audioData->buffer.size() - audioData->position;
    unsigned long samplesToCopy = framesPerBuffer * audioData->channels;

    if (remaining < samplesToCopy) {
      std::fill(out, out + samplesToCopy, 0.0f);
      std::copy(audioData->buffer.begin() + audioData->position,
                audioData->buffer.end(), out);
      audioData->position = audioData->buffer.size();
      return paComplete;
    } else {
      std::copy(audioData->buffer.begin() + audioData->position,
                audioData->buffer.begin() + audioData->position + samplesToCopy,
                out);
      audioData->position += samplesToCopy;
    }

    // Perform FFT
    std::vector<double> fft_input(framesPerBuffer);
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
      fft_input[i] = out[i];
    }
    fftw_execute_dft_r2c(audioData->fftw_plan_, fft_input.data(),
                         audioData->fftw_output);

    // Output FFT results for debugging (e.g., printing magnitudes)
    for (unsigned long i = 0; i < framesPerBuffer / 2 + 1; ++i) {
      double magnitude =
          sqrt(audioData->fftw_output[i][0] * audioData->fftw_output[i][0] +
               audioData->fftw_output[i][1] * audioData->fftw_output[i][1]);
      std::cout << "Frequency bin " << i << ": " << magnitude << std::endl;
    }

    return paContinue;
  }

  void loadAudioFile(const char *filePath) {
    SF_INFO sfInfo;
    SNDFILE *file = sf_open(filePath, SFM_READ, &sfInfo);
    if (!file) {
      std::cerr << "Failed to open audio file: " << sf_strerror(file)
                << std::endl;
      exit(1);
    }

    audioData.buffer.resize(sfInfo.frames * sfInfo.channels);
    sf_readf_float(file, audioData.buffer.data(), sfInfo.frames);
    sf_close(file);

    audioData.position = 0;
    audioData.sampleRate = sfInfo.samplerate;
    audioData.channels = sfInfo.channels;

    audioData.fftw_output = (fftw_complex *)fftw_malloc(
        sizeof(fftw_complex) * (FRAMES_PER_BUFFER / 2 + 1));
    audioData.fftw_plan_ = fftw_plan_dft_r2c_1d(
        FRAMES_PER_BUFFER, nullptr, audioData.fftw_output, FFTW_ESTIMATE);
  }

public:
  AudioPlayer(const char *filePath) {
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      exit(1);
    }

    loadAudioFile(filePath);
  }

  ~AudioPlayer() {
    // Cleanup FFTW
    fftw_destroy_plan(audioData.fftw_plan_);
    fftw_free(audioData.fftw_output);

    // Terminate PortAudio
    Pa_Terminate();
  }

  void play() {
    PaError err;

    // Open audio stream
    err = Pa_OpenDefaultStream(&stream,
                               0,                  // No input channels
                               audioData.channels, // Number of output channels
                               paFloat32,          // Sample format
                               audioData.sampleRate, // Sample rate from file
                               FRAMES_PER_BUFFER,    // Frames per buffer
                               audioCallback,        // Callback function
                               &audioData);          // User data
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      return;
    }

    // Start audio stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
      Pa_CloseStream(stream);
      return;
    }

    // Keep the stream running until all data is played
    while (Pa_IsStreamActive(stream) == 1) {
      Pa_Sleep(100);
    }

    // Stop and close the stream
    err = Pa_StopStream(stream);
    if (err != paNoError) {
      std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }
    Pa_CloseStream(stream);
  }
};