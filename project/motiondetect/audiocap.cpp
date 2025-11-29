#include <alsa/asoundlib.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

// Device and recording parameters
static const char* PCM_DEVICE = "hw:1,0";
static const uint32_t SAMPLE_RATE = 16000;
static const uint16_t CHANNELS = 1;
static const snd_pcm_format_t FORMAT = SND_PCM_FORMAT_S16_LE;
static const uint32_t FRAME_SIZE = (sizeof(int16_t) * CHANNELS);
static uint32_t BUFFER_FRAMES = 4096;   // must be a variable, not a macro!

// Write proper WAV header
void writeWavHeader(ofstream &out, uint32_t dataSize)
{
    uint32_t chunkSize = 36 + dataSize;
    uint16_t audioFormat = 1;               // PCM
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = SAMPLE_RATE * CHANNELS * bitsPerSample / 8;
    uint16_t blockAlign = CHANNELS * bitsPerSample / 8;

    // RIFF chunk
    out.write("RIFF", 4);
    out.write((char*)&chunkSize, 4);
    out.write("WAVE", 4);

    // fmt chunk
    out.write("fmt ", 4);
    uint32_t subChunk1Size = 16;
    out.write((char*)&subChunk1Size, 4);
    out.write((char*)&audioFormat, 2);
    out.write((char*)&CHANNELS, 2);
    out.write((char*)&SAMPLE_RATE, 4);
    out.write((char*)&byteRate, 4);
    out.write((char*)&blockAlign, 2);
    out.write((char*)&bitsPerSample, 2);

    // data chunk
    out.write("data", 4);
    out.write((char*)&dataSize, 4);
}

int main()
{
    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* params;
    int pcm;

    // Open ALSA device
    pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0);
    if (pcm < 0) {
        cerr << "Error opening PCM device: " << snd_strerror(pcm) << endl;
        return -1;
    }

    // Allocate HW params
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    // Set configuration
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, FORMAT);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate(pcm_handle, params, SAMPLE_RATE, 0);

    // Set period size
    snd_pcm_uframes_t frames = BUFFER_FRAMES;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, 0);

    // Apply HW params
    pcm = snd_pcm_hw_params(pcm_handle, params);
    if (pcm < 0) {
        cerr << "Error setting HW params: " << snd_strerror(pcm) << endl;
        return -1;
    }

    vector<int16_t> buffer(BUFFER_FRAMES);

    cout << "Recording 5 seconds to audio.wav...\n";

    // Prepare WAV file
    ofstream out("audio.wav", ios::binary);
    for (int i = 0; i < 44; i++) out.put(0);  // placeholder header

    uint32_t totalBytesWritten = 0;
    int totalFrames = SAMPLE_RATE * 5;
    int framesCaptured = 0;

    while (framesCaptured < totalFrames)
    {
        pcm = snd_pcm_readi(pcm_handle, buffer.data(), BUFFER_FRAMES);

        if (pcm == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
            continue;
        } else if (pcm < 0) {
            cerr << "Error reading PCM: " << snd_strerror(pcm) << endl;
            break;
        }

        int bytes = pcm * FRAME_SIZE;
        out.write((char*)buffer.data(), bytes);
        totalBytesWritten += bytes;
        framesCaptured += pcm;
    }

    // Finalize WAV header
    out.seekp(0);
    writeWavHeader(out, totalBytesWritten);
    out.close();

    snd_pcm_close(pcm_handle);

    cout << "Saved audio.wav (" << totalBytesWritten << " bytes)\n";
    return 0;
}
