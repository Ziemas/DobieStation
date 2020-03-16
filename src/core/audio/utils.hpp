#ifndef __UTILS_H_
#define __UTILS_H_
#include <fstream>
#include <vector>
#include <string>

void writewav(std::vector<int16_t> pcm, std::string filename);

class WAVWriter
{
    public:
        WAVWriter(std::string filename);
        ~WAVWriter();
        void append_pcm(std::vector<int16_t> pcm);
    private:
        void update_header();

        std::fstream file;
        uint32_t data_size = 0;
        uint32_t sample_rate = 48000;
        uint16_t channels = 1 ;
        uint16_t sample_size = 16;

        const char* data = "data";
        const char* fmt = "fmt ";
        const char* WAVE = "WAVE";
        const char* RIFF = "RIFF";

        const uint32_t header_size = 36;
        const uint32_t format_size = 16;
        const uint16_t format = 1;

};


#endif // __UTILS_H_