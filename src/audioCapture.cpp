#include <iostream>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 256

struct AudioData {
    std::vector<float> buffer;
    size_t position;
    unsigned long sampleRate; 
};

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo *timeInfo,
                         PaStreamCallbackFlags statusFlags, void *userData) {
  AudioData *audioData = (AudioData *)userData;
  float *out = (float *)outputBuffer;
  unsigned long remaining = audioData->buffer.size() - audioData->position;

  if (remaining < framesPerBuffer) {
    std::fill(out, out + framesPerBuffer, 0.0f);    // if not enough data remains in this shit, fill with silence
    std::copy(audioData->buffer.begin() + audioData->position,    // copy remaining data to output
              audioData->buffer.end(), out);
    audioData->position = audioData->buffer.size();
    return paComplete;
  } else {
    std::copy(audioData->buffer.begin() + audioData->position,     // Copy audio data to output
              audioData->buffer.begin() + audioData->position + framesPerBuffer,
              out);
    audioData->position += framesPerBuffer;
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

  return {buffer, 0};
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
                             0,             // No input channels
                             1,             // Number of output channels
                             paFloat32,     // Sample format
                             SAMPLE_RATE,         // Sample rate
                             FRAMES_PER_BUFFER,           // Frames per buffer
                             audioCallback, // Callback function
                             &audioData);   // User data
  if (err != paNoError) {
    std::cerr << "portAudio error: " << Pa_GetErrorText(err) << std::endl;
    Pa_Terminate();
    return 1;
  }

  // start audio stream
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    std::cerr << "portAudio error: " << Pa_GetErrorText(err) << std::endl;
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 1;
  }

  // Keep the stream running until all data is played
  while (Pa_IsStreamActive(stream) == 1) {
    Pa_Sleep(100);
  }
  err = Pa_StopStream(stream);
  if (err != paNoError) {
    std::cerr << "portAudio error: " << Pa_GetErrorText(err) << std::endl;
  }
  Pa_CloseStream(stream);
  std::cout << "audio playback finished" << std::endl;
  Pa_Terminate();

  return 0;
}
