#ifndef __SERIALIZE_H_
#define __SERIALIZE_H_
#include <fstream>

class StateSerializer
{
  public:
    enum class Mode
    {
        Read,
        Write,
    };

    StateSerializer(std::fstream& stream, Mode mode) : m_stream(stream), m_mode(mode)
    {
    }

    //~StateSerializer();

    Mode GetMode() { return m_mode; };

    template <typename T>
    void Do(T* ptr)
    {
        if (m_mode == Mode::Read)
        {
            m_stream.read((char*)ptr, sizeof(T));
        }
        else
        {
            m_stream.write((char*)ptr, sizeof(T));
        }
    }

    template <typename T>
    void DoBytes(T* ptr, size_t len)
    {
        if (m_mode == Mode::Read)
        {
            m_stream.read((char*)ptr, len);
        }
        else
        {
            m_stream.write((char*)ptr, len);
        }
    }

    template <typename T>
    void DoArray(T* ptr, size_t count)
    {
        for (size_t i = 0; i < count; i++)
            Do(&ptr[i]);
    }

  private:
    Mode m_mode;
    std::fstream& m_stream;
};

#endif // __SERIALIZE_H_
