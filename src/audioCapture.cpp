#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>

#define FRAMES_PER_BUFFER 256

struct AudioData {
  std::vector<float> buffer;
  size_t position;
  unsigned long sampleRate;
  int channels;
};

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
  return paContinue;
}

AudioData loadAudioFile(const char *filePath) {
  SF_INFO sfInfo;
  SNDFILE *file = sf_open(filePath, SFM_READ, &sfInfo);
  if (!file) {
    std::cerr << "failed to open audio file: " << sf_strerror(file)
              << std::endl;
    exit(1);
  }

  std::vector<float> buffer(sfInfo.frames * sfInfo.channels);
  sf_readf_float(file, buffer.data(), sfInfo.frames);
  sf_close(file);

  return {buffer, 0, (unsigned long)sfInfo.samplerate, sfInfo.channels};
}

int main() {
  PaError err;

  // Initialize PortAudio
  err = Pa_Initialize();
  if (err != paNoError) {
    std::cerr << "portAudio error: " << Pa_GetErrorText(err) << std::endl;
    return 1;
  }

  // Load audio file
  AudioData audioData = loadAudioFile("includes/gta.mp3");

  // Open audio stream
  PaStream *stream;
  err = Pa_OpenDefaultStream(&stream,
                             0,                    // No input channels
                             audioData.channels,   // Number of output channels
                             paFloat32,            // Sample format
                             audioData.sampleRate, // Sample rate from file
                             FRAMES_PER_BUFFER,    // Frames per buffer
                             audioCallback,        // Callback function
                             &audioData);          // User data
  if (err != paNoError) {
    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    Pa_Terminate();
    return 1;
  }

  // Start audio stream
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 1;
  }

  // Keep the stream running until all data is played
  while (Pa_IsStreamActive(stream) == 1) {
    Pa_Sleep(100);
  }

  // stop and close the stream
  err = Pa_StopStream(stream);
  if (err != paNoError) {
    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
  }
  Pa_CloseStream(stream);
  std::cout << "stream closed" << std::endl;
  // Terminate PortAudio
  Pa_Terminate();

  return 0;
}
