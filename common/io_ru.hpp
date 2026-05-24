#pragma once

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace io_ru {

inline void print_usage_1(std::ostream& cerr, char** argv,
                          const std::string& descr = "<входной_файл>") {
    const char* exe = argv && argv[0] ? argv[0] : "./task";
    cerr << "Использование: " << exe << ' ' << descr << '\n';
}

inline bool open_only_read_strict(int argc, char** argv, std::ifstream& in,
                                  std::ostream& cerr) {
    if (argc < 2) {
        print_usage_1(cerr, argv);
        cerr << "Нужен ровно один аргумент — путь к входному текстовому файлу.\n";
        return false;
    }
    in.open(argv[1]);
    if (!in) {
        cerr << "Не удалось открыть файл «" << argv[1] << "»: ";
        cerr << std::strerror(errno) << '\n';
        return false;
    }
    return true;
}

/// Вариант для программ с необязательным вторым файлом записи результата.
inline bool open_read_optional_write(int argc, char** argv, std::ifstream& in,
                                     std::ofstream* fout, std::ostream& cerr,
                                     const char* descr = "<вход.txt> [выход.txt]") {
    if (argc < 2) {
        const char* exe = argv && argv[0] ? argv[0] : "./task";
        cerr << "Использование: " << exe << ' ' << descr << '\n';
        cerr << "Ввод только из первого файла; stdin не используется.\n";
        return false;
    }
    if (argc > 3) {
        const char* exe = argv && argv[0] ? argv[0] : "./task";
        cerr << "Слишком много аргументов. Использование: " << exe << ' ' << descr << '\n';
        return false;
    }
    in.open(argv[1]);
    if (!in) {
        cerr << "Не удалось открыть входной файл «" << argv[1] << "»: ";
        cerr << std::strerror(errno) << '\n';
        return false;
    }
    if (fout && argc >= 3) {
        fout->open(argv[2]);
        if (!(*fout)) {
            cerr << "Не удалось открыть выходной файл «" << argv[2] << "»: ";
            cerr << std::strerror(errno) << '\n';
            return false;
        }
    }
    return true;
}

}  // namespace io_ru
