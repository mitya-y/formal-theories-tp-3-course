#pragma once

#include <algorithm>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace table_ru {

/// Длина строки UTF-8: байты многобайтовых символов считаем как 1 «визуальная»
/// позиция (грубо, но для кириллицы в терминале обычно ок).
inline size_t utf8_display_width(std::string_view s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
        } else if ((c & 0xe0u) == 0xc0u) {
            i += 2;
        } else if ((c & 0xf0u) == 0xe0u) {
            i += 3;
        } else if ((c & 0xf8u) == 0xf0u) {
            i += 4;
        } else {
            ++i;
        }
        ++w;
    }
    return w;
}

inline void pad_utf8_cell(std::ostream& os, const std::string& cell, unsigned width,
                          bool align_right = false) {
    size_t w = utf8_display_width(cell);
    if (align_right && w < width)
        os << std::string(width - w, ' ');
    os << cell;
    if (!align_right && w < width) os << std::string(width - w, ' ');
}

inline std::vector<unsigned> compute_col_width(const std::vector<std::string>& headers,
                                               const std::vector<std::vector<std::string>>& rows,
                                               unsigned min_w = 2) {
    std::vector<unsigned> w(headers.size());
    for (size_t i = 0; i < headers.size(); ++i)
        w[i] = std::max(min_w,
                        static_cast<unsigned>(std::max(utf8_display_width(headers[i]), size_t{})));
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size() && i < w.size(); ++i)
            w[i] = std::max(w[i], static_cast<unsigned>(utf8_display_width(row[i])));
    }
    return w;
}

/// Рисует таблицу с заголовком (UTF-8 рамки).
inline void print_table(std::ostream& os, std::string_view title,
                       const std::vector<std::string>& headers,
                       const std::vector<std::vector<std::string>>& rows) {
    if (!title.empty()) {
        os << "\n═══════════════════════════════════════════════════════════════\n";
        os << "  " << title << '\n';
        os << "═══════════════════════════════════════════════════════════════\n";
    }

    const auto cw = compute_col_width(headers, rows);

    os << "┌";
    for (size_t ci = 0; ci < cw.size(); ++ci) {
        for (unsigned u = 0; u < cw[ci] + 2u; ++u) os << "─";
        if (ci + 1 < cw.size())
            os << "┬";
        else
            os << "┐";
    }
    os << '\n';

    os << "│ ";
    for (size_t i = 0; i < headers.size(); ++i) {
        pad_utf8_cell(os, headers[i], cw[i]);
        os << ' ';
        if (i + 1 < headers.size()) os << "│ "; else os << "│";
    }
    os << '\n';

    os << "├";
    for (size_t ci = 0; ci < cw.size(); ++ci) {
        for (unsigned u = 0; u < cw[ci] + 2u; ++u) os << "─";
        if (ci + 1 < cw.size())
            os << "┼";
        else
            os << "┤";
    }
    os << '\n';

    for (const auto& row : rows) {
        os << "│ ";
        for (size_t i = 0; i < headers.size(); ++i) {
            const std::string cell = i < row.size() ? row[i] : std::string();
            pad_utf8_cell(os, cell, cw[i], true);  // как правило числа выравниваем вправо
            os << ' ';
            if (i + 1 < headers.size())
                os << "│ ";
            else
                os << "│";
        }
        os << '\n';
    }

    os << "└";
    for (size_t ci = 0; ci < cw.size(); ++ci) {
        for (unsigned u = 0; u < cw[ci] + 2u; ++u) os << "─";
        if (ci + 1 < cw.size())
            os << "┴";
        else
            os << "┘";
    }
    os << "\n";
}

inline void matrix_int_corner(std::ostream& os, std::string_view title,
                              const std::vector<std::vector<int>>& m,
                              const std::vector<std::string>& col_headers,
                              const std::vector<std::string>& row_headers, int no_path_sentinel,
                              std::string_view no_path_disp = "∞") {
    os << '\n';
    os << "──────── " << title << " ────────\n\n";
    std::vector<std::string> h;
    h.reserve(1 + col_headers.size());
    h.push_back("Из \\ В");  // уголок матрицы
    for (const auto& c : col_headers) h.push_back(c);
    std::vector<std::vector<std::string>> rows;
    for (size_t i = 0; i < m.size(); ++i) {
        std::vector<std::string> rr;
        rr.push_back(i < row_headers.size() ? row_headers[i] : std::string("?"));
        for (size_t j = 0; j < m[i].size(); ++j) {
            if (m[i][j] >= no_path_sentinel)
                rr.push_back(std::string(no_path_disp));
            else
                rr.push_back(std::to_string(m[i][j]));
        }
        rows.push_back(std::move(rr));
    }
    print_table(os, {}, h, rows);
}

}  // namespace table_ru
