#pragma once
#include <array>
#include <functional>
#include <vector>
#include <cmath>

using Vec = std::vector<double>;
using ODE = std::function<void(const Vec&, Vec&)>;

// Simple RK4 for n dimensional system
inline void rk4_step(const ODE& f, Vec& x, double h) {
    const size_t n = x.size();
    Vec k1(n), k2(n), k3(n), k4(n), xtmp(n);

    f(x, k1);
    for (size_t i = 0; i < n; ++i) xtmp[i] = x[i] + 0.5 * h * k1[i];
    f(xtmp, k2);
    for (size_t i = 0; i < n; ++i) xtmp[i] = x[i] + 0.5 * h * k2[i];
    f(xtmp, k3);
    for (size_t i = 0; i < n; ++i) xtmp[i] = x[i] + h * k3[i];
    f(xtmp, k4);
    for (size_t i = 0; i < n; ++i)
        x[i] += (h / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
}

// Lorenz system (classic parameters)
inline ODE lorenz(double sigma = 10.0, double rho = 28.0, double beta = 8.0/3.0) {
    return [=](const Vec& x, Vec& dx) {
        dx.resize(3);
        dx[0] = sigma * (x[1] - x[0]);
        dx[1] = x[0] * (rho - x[2]) - x[1];
        dx[2] = x[0] * x[1] - beta * x[2];
    };
}

// Rossler system
inline ODE rossler(double a = 0.2, double b = 0.2, double c = 5.7) {
    return [=](const Vec& x, Vec& dx) {
        dx.resize(3);
        dx[0] = -x[1] - x[2];
        dx[1] = x[0] + a * x[1];
        dx[2] = b + x[2] * (x[0] - c);
    };
}

// Van der Pol oscillator (2D)
inline ODE van_der_pol(double mu = 1.0) {
    return [=](const Vec& x, Vec& dx) {
        dx.resize(2);
        dx[0] = x[1];
        dx[1] = mu * (1.0 - x[0]*x[0]) * x[1] - x[0];
    };
}

// Double pendulum (4D: theta1, omega1, theta2, omega2)
inline ODE double_pendulum(double m1 = 1.0, double m2 = 1.0, double L1 = 1.0, double L2 = 1.0, double g = 9.81) {
    return [=](const Vec& x, Vec& dx) {
        dx.resize(4);
        double th1 = x[0], w1 = x[1], th2 = x[2], w2 = x[3];
        double dth = th1 - th2;
        double denom = (m1 + m2) * L1 - m2 * L1 * std::cos(dth) * std::cos(dth);

        double num1 = m2 * L1 * w1 * w1 * std::sin(dth) * std::cos(dth)
                      + m2 * g * std::sin(th2) * std::cos(dth)
                      + m2 * L2 * w2 * w2 * std::sin(dth)
                      - (m1 + m2) * g * std::sin(th1);

        double num2 = -m2 * L2 * w2 * w2 * std::sin(dth) * std::cos(dth)
                      + (m1 + m2) * (g * std::sin(th1) * std::cos(dth) - L1 * w1 * w1 * std::sin(dth))
                      - (m1 + m2) * g * std::sin(th2);

        double denom2 = (L2 / L1) * denom;

        dx[0] = w1;
        dx[1] = num1 / denom;
        dx[2] = w2;
        dx[3] = num2 / denom2;
    };
}
