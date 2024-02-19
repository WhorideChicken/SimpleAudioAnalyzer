#include <iostream>
#include <vector>
#include <cmath>
#include <fftw3.h>
#include <sndfile.h>

std::vector<int> processAudioAndGenerateNotes(const char* filename) {
    SF_INFO fileInfo{};
    SNDFILE* file = sf_open(filename, SFM_READ, &fileInfo);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return {};
    }

    // 샘플레이트에 따라 1초에 해당하는 샘플 수 계산
    int samplesPerSecond = fileInfo.samplerate;
    long totalSamples = fileInfo.frames * fileInfo.channels;
    std::vector<double> samples(totalSamples);
    std::vector<int> notes;

    // 모노로 변환 (멀티 채널 데이터를 평균화)
    std::vector<double> tempBuffer(fileInfo.frames * fileInfo.channels);
    sf_read_double(file, tempBuffer.data(), totalSamples);
    for (int i = 0; i < fileInfo.frames; i++) {
        double monoSample = 0.0;
        for (int channel = 0; channel < fileInfo.channels; channel++) {
            monoSample += tempBuffer[i * fileInfo.channels + channel];
        }
        samples[i] = monoSample / fileInfo.channels;
    }
    sf_close(file);

    // FFTW 입력 및 출력 배열 준비
    fftw_complex* in, * out;
    fftw_plan plan = nullptr; // nullptr로 초기화
    in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * samplesPerSecond);
    out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * samplesPerSecond);

    // 1초 단위로 데이터를 처리하기 전에 plan을 초기화
    plan = fftw_plan_dft_1d(samplesPerSecond, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
    double tempmaxEnergy = 0;

    for (int start = 0; start < fileInfo.frames; start += samplesPerSecond) {
        // FFTW 입력 배열에 샘플 복사
        for (int i = 0; i < samplesPerSecond && (start + i) < fileInfo.frames; i++) {
            in[i][0] = samples[start + i];
            in[i][1] = 0.0; // 허수 부분은 0으로 초기화
        }

        // FFT 실행
        if (plan != nullptr) {
            fftw_execute(plan);
        }

        // FFT 결과 분석하여 노트 생성 여부 결정 (여기서는 단순화를 위해 최대 에너지 확인)
        double maxEnergy = 0.0;
        for (int i = 0; i < samplesPerSecond; i++) {
            double energy = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            if (energy > maxEnergy) {
                maxEnergy = energy;
            }
        }

        if (tempmaxEnergy < maxEnergy)
            tempmaxEnergy = maxEnergy;


        //TODO : 노트 판단 기준 주파수 에너지 임계값
        // 에너지가 임계값을 초과하면 노트 생성 (임계값은 실험적으로 조정 필요)
        if (maxEnergy > 5000) {
            notes.push_back(2); // 5000을 초과하면 2를 노트로
        }
        else if (maxEnergy > 1000) {
            notes.push_back(1); // 2500을 초과하면 1을 노트로
        }
        else {
            notes.push_back(0); // 그 외의 경우 0을 노트로
        }
    }



    if (plan != nullptr) {
        fftw_destroy_plan(plan);
    }
    fftw_free(in);
    fftw_free(out);

    return notes;
}



int main() {
    const char* filename = "test.wav";
    int sampleRate = 44100; // 대부분의 오디오 파일에 대한 일반적인 샘플레이트
    int bufferSize = 1024; // FFT 분석을 위한 버퍼 크기
    auto notes = processAudioAndGenerateNotes(filename);

    // 결과 출력
    for (size_t i = 0; i < notes.size(); i++) {
        std::cout << "Second " << i << ": " << (notes[i] == 2 ? "Note 2 generated" : (notes[i] == 1 ? "Note 1 generated" : "No note")) << std::endl;
    }



    return 0;
}