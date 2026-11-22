#include <iostream>
#include <vector>
#include <complex>
#include <iomanip>
#include <cmath>
#include <limits>
#include <algorithm>
#include <sstream>
#include <string>
#include <cctype>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 打印一个实数：整数按整数打印，小数去掉末尾多余的 0
static void print_clean_real(double x) {
    if (std::abs(x) < 1e-15) x = 0.0;

    double rounded = std::round(x);
    if (std::abs(x - rounded) < 1e-12) {
        std::cout << (long long)rounded;
        return;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(12) << x;
    std::string s = oss.str();

    s.erase(s.find_last_not_of('0') + 1);
    if (!s.empty() && s.back() == '.') s.pop_back();

    std::cout << s;
}

// 打印虚部绝对值并加 i；绝对值是 1 时只打印 i
static void print_clean_imag_abs(double x) {
    if (std::abs(x) < 1e-15) x = 0.0;

    double rounded = std::round(x);
    if (std::abs(x - rounded) < 1e-12) {
        long long v = (long long)rounded;
        if (v != 1) std::cout << v;
    } else {
        print_clean_real(x);
    }
    std::cout << "i";
}

int main() {
    long long degree = 0;

    // 严格读取次数：整行读取，必须是 1~10 的整数
    while (true) {
        std::cout << "请选择方程次数 (1~10): ";
        std::string line;
        std::getline(std::cin, line);

        size_t begin = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (begin == std::string::npos) {
            std::cout << "输入无效，请重新输入。\n";
            continue;
        }
        line = line.substr(begin, end - begin + 1);

        bool ok = true;
        for (char ch : line) {
            if (!std::isdigit((unsigned char)ch)) {
                ok = false;
                break;
            }
        }
        if (!ok) {
            std::cout << "输入无效，请重新输入。\n";
            continue;
        }

        try {
            degree = std::stoll(line);
        } catch (...) {
            std::cout << "输入无效，请重新输入。\n";
            continue;
        }

        if (degree >= 1 && degree <= 10) break;
        std::cout << "次数必须在 1 到 10 之间，请重新输入。\n";
    }

    std::vector<double> coeffs(degree + 1);

    while (true) {
        std::cout << "请依次输入 " << degree + 1
                  << " 个系数（从高次到低次，最高次项不能为 0）:\n";
        for (long long i = 0; i <= degree; ++i) {
            std::cin >> coeffs[i];
        }
        if (coeffs[0] != 0.0) break;
        std::cout << "第一项系数为 0，不是 " << degree
                  << " 次方程，请重新输入。\n";
    }

    // 打印方程
    std::cout << "\n对应的方程为: ";
    bool first = true;
    for (long long i = 0; i <= degree; ++i) {
        double c = coeffs[i];
        if (c == 0.0) continue;
        long long power = degree - i;

        if (!first) {
            std::cout << (c > 0 ? " + " : " - ");
        } else {
            if (c < 0) std::cout << "-";
        }

        double abs_c = std::abs(c);
        if (std::abs(abs_c - std::round(abs_c)) < 1e-12) {
            std::cout << std::fixed << std::setprecision(0) << std::round(abs_c);
        } else {
            print_clean_real(abs_c);
        }

        if (power > 0) {
            std::cout << "x";
            if (power > 1) std::cout << "^" << power;
        }
        first = false;
    }
    std::cout << " = 0\n";

    long long n = degree;
    std::vector<std::complex<double>> roots(n);

    for (long long i = 0; i < n; ++i) {
        double angle = 2.0 * M_PI * i / n + 0.5;
        roots[i] = std::complex<double>(std::cos(angle), std::sin(angle));
    }

    auto eval_poly = [&](std::complex<double> x) {
        std::complex<double> val(0.0, 0.0);
        for (long long i = 0; i <= n; ++i) {
            val = val * x + coeffs[i];
        }
        return val;
    };

    auto eval_deriv = [&](std::complex<double> x) {
        std::complex<double> val(0.0, 0.0);
        for (long long i = 0; i < n; ++i) {
            val = val * x + coeffs[i] * (n - i);
        }
        return val;
    };

    const long long max_iter = 50000;
    const double eps = 1e-13;

    for (long long iter = 0; iter < max_iter; ++iter) {
        double max_delta = 0.0;
        for (long long i = 0; i < n; ++i) {
            std::complex<double> f = eval_poly(roots[i]);
            std::complex<double> df = eval_deriv(roots[i]);

            if (std::abs(f) < 1e-14) continue;

            std::complex<double> sum(0.0, 0.0);
            for (long long j = 0; j < n; ++j) {
                if (i != j) {
                    std::complex<double> diff = roots[i] - roots[j];
                    if (std::abs(diff) < 1e-30) diff = std::complex<double>(1e-30, 0.0);
                    sum += 1.0 / diff;
                }
            }

            std::complex<double> denom = df - f * sum;
            if (std::abs(denom) < 1e-30) denom = std::complex<double>(1e-30, 0.0);

            std::complex<double> delta = f / denom;
            roots[i] -= delta;
            max_delta = std::max(max_delta, std::abs(delta));
        }
        if (max_delta < eps) break;
    }

    struct RootInfo {
        std::complex<double> value;
        long long multiplicity;
    };
    std::vector<RootInfo> merged;
    const double tol = 1e-5;

    for (const auto& r : roots) {
        bool found = false;
        for (auto& m : merged) {
            if (std::abs(r - m.value) < tol) {
                m.value = (m.value * (double)m.multiplicity + r) / (double)(m.multiplicity + 1);
                m.multiplicity++;
                found = true;
                break;
            }
        }
        if (!found) merged.push_back({r, 1});
    }

    // 特别处理 x^n = 0
    bool only_leading = true;
    for (long long i = 1; i <= n; ++i) {
        if (std::abs(coeffs[i]) > 1e-12) {
            only_leading = false;
            break;
        }
    }
    if (only_leading) {
        merged.clear();
        merged.push_back({std::complex<double>(0.0, 0.0), n});
    }

    std::cout << "\n方程的根为（已合并重复根）:\n";

    for (long long i = 0; i < (long long)merged.size(); ++i) {
        double re = merged[i].value.real();
        double im = merged[i].value.imag();

        if (std::abs(re) < 1e-15) re = 0.0;
        if (std::abs(im) < 1e-15) im = 0.0;

        std::cout << "x[" << i + 1 << "] = ";

        if (im == 0.0) {
            print_clean_real(re);
        } else if (re == 0.0) {
            if (im < 0) std::cout << "-";
            print_clean_imag_abs(std::abs(im));
        } else {
            print_clean_real(re);
            if (im > 0) std::cout << "+";
            else std::cout << "-";
            print_clean_imag_abs(std::abs(im));
        }

        if (merged[i].multiplicity > 1) {
            std::cout << " (重数 " << merged[i].multiplicity << ")";
        }
        std::cout << "\n";
    }

    return 0;
}
