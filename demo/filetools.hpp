#include <vector>
#include <string>
#include <rtree.h>
#include <fstream>
#include <streambuf>
#include <algorithm>
#include <functional>
#include <charconv>
#include <string_view>
#include <type_traits>
#include <stdexcept>

namespace Filetools
{
    namespace Detail
    {
        template <typename T>
        inline bool ParseOne(const char *&p, const char *end, T &out)
        {
            if (p >= end)
                return false;

            if constexpr (std::is_floating_point_v<T>)
            {
                const auto res = std::from_chars(p, end, out, std::chars_format::general);
                if (res.ec != std::errc())
                    return false;
                p = res.ptr;
                return true;
            }
            else
            {
                double tmp = 0.0;
                const auto res = std::from_chars(p, end, tmp, std::chars_format::general);
                if (res.ec != std::errc())
                    return false;
                p = res.ptr;
                out = static_cast<T>(tmp);
                return true;
            }
        }
    }

    template <typename T>
    void ReadObjectsFromCsvFile(std::ifstream &fileStream, std::size_t &linesRead, std::function<void(rtree::Object<T> &)> newObjectCallback)
    {
        std::string line;
        int idCounter = 0;
        std::size_t n = 0;

        // Считаем кол-во чисел в первой строке, чтобы определить размерность прямоугольников
        if (std::getline(fileStream, line))
        {
            size_t pos = 0;
            while ((pos = line.find(';')) != std::string::npos)
            {
                n++;
                line.erase(0, pos + 1);
            }
            if (!line.empty())
                n++;
            if (n % 2 != 0)
                throw std::runtime_error("Invalid line format: odd number of values");
            n /= 2;
        }
        else
            throw std::runtime_error("File is empty");

        fileStream.clear();
        fileStream.seekg(0);
        while (std::getline(fileStream, line))
        {
            try
            {
                rtree::Rectangle<T> rect = ParseCsvLine<T>(line, n);
                if (n * 2 >= 4)
                {
                    rtree::Object<T> obj(idCounter++, rect);
                    // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Имитируем задержку при чтении больших файлов
                    newObjectCallback(obj);
                }
            }
                catch (const std::exception &)
            {
                // Игнорируем строки с ошибками
            }
            linesRead++;
        }
    }

    std::size_t CountLines(std::ifstream &fileStream)
    {
        std::size_t count = std::count(std::istreambuf_iterator<char>(fileStream), std::istreambuf_iterator<char>(), '\n');
        fileStream.clear();
        fileStream.seekg(0);
        return count + 1;
    }

    std::size_t CountLines(const std::string &filePath)
    {
        std::ifstream fileStream(filePath);
        if (!fileStream.is_open())
            throw std::runtime_error("Failed to open file: " + filePath);

        return CountLines(fileStream);
    }

    template <typename T>
    inline rtree::Rectangle<T> ParseCsvLine(std::string_view line, const std::size_t n)
    {
        rtree::Rectangle<T> rect(n);
        const std::size_t expected = n * 2;
        const char *p = line.data();
        const char *const end = p + line.size();

        for (std::size_t i = 0; i < expected; ++i)
        {
            T value{};
            if (!Detail::ParseOne<T>(p, end, value))
                throw std::runtime_error("Invalid line format: failed to parse number");
            rect.size[i] = value;

            if (i + 1 < expected)
            {
                if (p >= end || *p != ';')
                    throw std::runtime_error("Invalid line format: expected ';' separator");
                ++p;
            }
        }

        if (p != end)
            throw std::runtime_error("Invalid line format: trailing characters");
        return rect;
    }

    template <typename T>
    inline rtree::Rectangle<T> ParseCsvLine(const std::string &line, const std::size_t n)
    {
        return ParseCsvLine<T>(std::string_view(line), n);
    }
}