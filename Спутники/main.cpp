// Задача «КА-группировки»: n спутников, m связей; каждая связь — интервал
// [ti, ti+len) (мин) между КА i и j. Глобальное [0, T). Каждые 10 мин — такт:
// в граф кладутся рёбра, чьи интервалы пересекают [t, t+10); группировки = компоненты связности.

#include "../common/io_ru.hpp"
#include "../common/table_ru.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int STEP = 10;  // такт, мин

struct Edge {
    int u, v;
    int len;
    int t0;  // [t0, t0+len)
};

std::string strip(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(0, 1);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

bool isSkippableLine(const std::string& line) {
    const std::string t = strip(line);
    return t.empty() || t[0] == '#';
}

// [a0,a1) и [b0,b1) — полуинтервалы
bool overlapHalfOpen(int a0, int a1, int b0, int b1) { return a0 < b1 && b0 < a1; }

struct DSU {
    std::vector<int> p;
    explicit DSU(int n) : p(n) { std::iota(p.begin(), p.end(), 0); }
    int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }
    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a != b) p[a] = b;
    }
};

// для вершин 0..n-1 (внутр.), рёбра 0-индекс; вернёт список списков (компоненты, по возр.)
std::vector<std::vector<int>> componentsFromEdges(int n, const std::vector<std::pair<int, int>>& ed) {
    DSU d(n);
    for (const auto& [a, b] : ed) d.unite(a, b);
    std::unordered_map<int, std::vector<int>> buck;
    for (int v = 0; v < n; ++v) buck[d.find(v)].push_back(v);
    std::vector<std::vector<int>> out;
    out.reserve(buck.size());
    for (auto& [root, vs] : buck) {
        std::sort(vs.begin(), vs.end());
        out.push_back(std::move(vs));
    }
    std::sort(out.begin(), out.end());
    return out;
}

struct Parsed {
    int n = 0, m = 0, T = 0;
    std::vector<Edge> edges;
};

std::string ansiForGroup(int gid) {
    if (gid == 0) return "\033[0m";
    int c = 31 + (gid % 6);
    return "\033[" + std::to_string(c) + "m";
}

std::string fmtOneBased(const std::vector<int>& v) {
    std::string s;
    for (int i = 0; i < static_cast<int>(v.size()); ++i) {
        if (i) s += ", ";
        s += std::to_string(v[i] + 1);
    }
    return s;
}

// карта: вершина(0) -> метка глоб. группы 1..G на данном такте
std::vector<int> labelGroups0(const std::vector<std::vector<int>>& comps) {
    int nsum = 0;
    for (const auto& c : comps) nsum += static_cast<int>(c.size());
    std::vector<int> lab(nsum, 0);
    int g = 1;
    for (const auto& c : comps) {
        for (int v : c) lab[v] = g;
        ++g;
    }
    return lab;
}

bool loadFromText(std::istream& in, Parsed& out, std::string& err) {
    std::vector<std::string> L;
    std::string line;
    while (std::getline(in, line)) {
        if (!isSkippableLine(line)) L.push_back(strip(line));
    }
    if (L.empty()) {
        err = "пустой вход";
        return false;
    }
    size_t p = 0;
    {
        std::istringstream is(L[p++]);
        if (!(is >> out.n >> out.m >> out.T)) {
            err = "первая строка: n m T (число КА, число связей, правый конец T мин для [0,T) )";
            return false;
        }
    }
    if (out.n < 1) {
        err = "n >= 1";
        return false;
    }
    if (out.m < 0 || out.T < 0) {
        err = "m >= 0, T >= 0";
        return false;
    }
    if (L.size() < p + static_cast<size_t>(out.m)) {
        err = "мало строк для связей (ожидается m строк: i j len ti)";
        return false;
    }
    out.edges.clear();
    for (int e = 0; e < out.m; ++e) {
        Edge ed{};
        std::istringstream is(L[p++]);
        if (!(is >> ed.u >> ed.v >> ed.len >> ed.t0)) {
            err = "связь: i j len ti  (1..n, [ti, ti+len) в мин)";
            return false;
        }
        --ed.u;
        --ed.v;
        if (ed.u < 0 || ed.v < 0 || ed.u >= out.n || ed.v >= out.n || ed.u == ed.v) {
            err = "i,j — разные КА 1..n";
            return false;
        }
        if (ed.len <= 0) {
            err = "len > 0";
            return false;
        }
        if (ed.t0 < 0 || ed.t0 + ed.len > out.T) {
            err = "интервал [ti, ti+len) должен лежать в [0, T] — проверь ti, len, T";
            return false;
        }
        out.edges.push_back(ed);
    }
    return true;
}

void printTicks(std::ostream& os, const Parsed& w, int step, bool color) {
    int numSlots = (w.T + step - 1) / step;  // покрытие [0, T) слотами step
    os << "──────── Такт (каждые " << step
       << " мин) — в граф попадает связь, если её интервал пересекает [t, t+" << step
       << ") —────────\n\n";
    for (int slot = 0; slot < numSlots; ++slot) {
        int t0 = slot * step;
        int t1 = std::min(t0 + step, w.T);
        if (t0 >= w.T) break;
        std::vector<std::pair<int, int>> act;
        for (const auto& e : w.edges) {
            int a0 = e.t0, a1 = e.t0 + e.len;
            if (overlapHalfOpen(a0, a1, t0, t1)) {
                int u = e.u, v = e.v;
                if (u > v) std::swap(u, v);
                act.push_back({u, v});
            }
        }
        std::sort(act.begin(), act.end());
        act.erase(std::unique(act.begin(), act.end()), act.end());
        auto comps = componentsFromEdges(w.n, act);
        auto lab = labelGroups0(comps);
        os << "Такт [" << t0 << ", " << t1 << ") мин. — активных рёбер: " << act.size() << "\n";
        for (const auto& c : comps) {
            int g = lab[c[0]];
            if (color) os << "  Группировка " << g << ansiForGroup(g) << " — КА: " << fmtOneBased(c)
                          << "\033[0m\n";
            else
                os << "  Группировка " << g << " — КА: " << fmtOneBased(c) << "\n";
        }
        os << "\n";
    }
}

void printGantt(std::ostream& os, const Parsed& w, int step, bool color) {
    int numSlots = (w.T + step - 1) / step;
    os << "──────── График Ганта (строка — КА, столбец — [" << step
       << "·k, " << step << "·(k+1) ); символ = № группировки на такте) ────────\n\n";
    std::string pad = "   ";
    os << std::string(8, ' ');
    for (int s = 0; s < numSlots; ++s) {
        int a = s * step, b = std::min(a + step, w.T);
        if (a >= w.T) break;
        std::string hl = "[" + std::to_string(a) + "," + std::to_string(b) + ")";
        os << std::left << std::setw(10) << hl;
    }
    os << std::right << "\n";
    for (int v = 0; v < w.n; ++v) {
        os << "КА " << std::setw(2) << (v + 1) << pad;
        for (int slot = 0; slot < numSlots; ++slot) {
            int t0 = slot * step;
            int t1 = std::min(t0 + step, w.T);
            if (t0 >= w.T) break;
            std::vector<std::pair<int, int>> act;
            for (const auto& e : w.edges) {
                int a0 = e.t0, a1 = e.t0 + e.len;
                if (overlapHalfOpen(a0, a1, t0, t1)) {
                    int u = e.u, v2 = e.v;
                    if (u > v2) std::swap(u, v2);
                    act.push_back({u, v2});
                }
            }
            std::sort(act.begin(), act.end());
            act.erase(std::unique(act.begin(), act.end()), act.end());
            auto comps = componentsFromEdges(w.n, act);
            auto lab = labelGroups0(comps);
            int g = lab[v];
            char ch = (g < 10) ? static_cast<char>('0' + g) : static_cast<char>('A' + (g - 10) % 26);
            if (color) os << ansiForGroup(g) << "  " << ch << "  " << "\033[0m";
            else
                os << "  " << ch << "  ";
            os << " ";
        }
        os << "\n";
    }
    os << "\n";
}

void print_input_edges_and_params(std::ostream& os, const Parsed& w) {
    std::vector<std::vector<std::string>> er;
    for (size_t i = 0; i < w.edges.size(); ++i) {
        const Edge& e = w.edges[i];
        er.push_back({std::to_string(static_cast<int>(i) + 1), std::to_string(e.u + 1),
                      std::to_string(e.v + 1), std::to_string(e.len), std::to_string(e.t0),
                      "[" + std::to_string(e.t0) + "," + std::to_string(e.t0 + e.len) + ")"});
    }
    table_ru::print_table(os,
                         "Загрузка из файла: связи между КА (интервал [ti, ti+len) мин, номера КА как во "
                         "входе 1…n)",
                         {"№", "КА i", "КА j", "len", "ti (начало)", "интервал [ti, ti+len), мин"}, er);

    table_ru::print_table(os, "Параметры экземпляра", {"Поле", "Значение"},
                         {{"Число КА n", std::to_string(w.n)},
                          {"Число связей m", std::to_string(w.m)},
                          {"Горизонт T мин (слоты [0, T))", std::to_string(w.T)}});
}

void printAdjMatrix(std::ostream& os, const Parsed& w) {
    const int wcell = 14;
    os << "──────── Матрица: на пересечении (КА i, КА j) — интервал прямой связи [ti, ti+len) "
          "(мин) ────────\n\n";
    std::vector<std::vector<std::string>> cell(w.n, std::vector<std::string>(w.n, "     —     "));
    for (int i = 0; i < w.n; ++i) cell[i][i] = "      ·     ";
    for (const auto& e : w.edges) {
        int a = e.u, b = e.v;
        std::string s = "[" + std::to_string(e.t0) + "," + std::to_string(e.t0 + e.len) + ")";
        cell[a][b] = s;
        cell[b][a] = s;
    }
    os << "      ";
    for (int j = 0; j < w.n; ++j) {
        std::string h = "К" + std::to_string(j + 1);
        if (h.size() < 5) h = " " + h;
        os << std::setw(wcell) << h;
    }
    os << "\n";
    for (int i = 0; i < w.n; ++i) {
        std::string h = "К" + std::to_string(i + 1);
        os << std::setw(6) << h;
        for (int j = 0; j < w.n; ++j) {
            const std::string& c = cell[i][j];
            os << std::setw(wcell) << c;
        }
        os << "\n";
    }
    os << "\n";
}

void printMachineFile(std::ostream& os, const Parsed& w, int step) {
    os << "# формат: такт, затем g групп, строки 'group id k v1 v2 ...'\n";
    int numSlots = (w.T + step - 1) / step;
    for (int slot = 0; slot < numSlots; ++slot) {
        int t0 = slot * step;
        int t1 = std::min(t0 + step, w.T);
        if (t0 >= w.T) break;
        std::vector<std::pair<int, int>> act;
        for (const auto& e : w.edges) {
            if (overlapHalfOpen(e.t0, e.t0 + e.len, t0, t1)) {
                int u = e.u, v = e.v;
                if (u > v) std::swap(u, v);
                act.push_back({u, v});
            }
        }
        std::sort(act.begin(), act.end());
        act.erase(std::unique(act.begin(), act.end()), act.end());
        auto comps = componentsFromEdges(w.n, act);
        os << "tick " << t0 << " " << t1 << " edges " << act.size() << "\n";
        int gid = 1;
        for (const auto& c : comps) {
            os << "group " << gid++ << " " << c.size();
            for (int v : c) os << " " << (v + 1);
            os << "\n";
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::ifstream fin;
    std::ofstream fout;
    if (!io_ru::open_read_optional_write(argc, argv, fin, &fout, std::cerr,
                                         "<вход.txt> [выход.txt — опционально]"))
        return 2;

    std::string err;
    Parsed w;
    if (!loadFromText(fin, w, err)) {
        std::cerr << "Файл «" << argv[1] << "» содержит ошибку формата: " << err << '\n';
        return 1;
    }

    if (w.T == 0) {
        std::cout << "\nT=0: на шкале времени нет активных интервалов; ниже только сводные таблицы.\n";
    }

    print_input_edges_and_params(std::cout, w);

    const bool toFile = fout.is_open();
    std::ostream& fref = toFile ? static_cast<std::ostream&>(fout) : std::cout;

    printTicks(std::cout, w, STEP, true);
    printGantt(std::cout, w, STEP, true);
    printAdjMatrix(std::cout, w);
    if (toFile) {
        print_input_edges_and_params(fref, w);
        printTicks(fref, w, STEP, false);
        printGantt(fref, w, STEP, false);
        printAdjMatrix(fref, w);
        printMachineFile(fref, w, STEP);
    }
    return 0;
}
