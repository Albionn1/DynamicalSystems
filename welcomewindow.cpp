#include "welcomewindow.h"
#include "lorenzdialog.h"
#include "rosslerdialog.h"
#include "vanderpoldialog.h"
#include "doublependulumdialog.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>

WelcomeWindow::WelcomeWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Welcome to Dynamical Systems");
    resize(1200, 900);
    setStyleSheet(
        "QDialog { background-color: #2d2d30; }"
        "QLabel { color: #e0e0e0; }"
        );

    auto* mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(24);

    // Title
    auto* titleLabel = new QLabel("Dynamical Systems Visualizer", this);
    titleLabel->setStyleSheet("QLabel { color: #00bcd4; font-size: 28px; font-weight: bold; margin-bottom: 8px; }");
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);

    auto* subtitleLabel = new QLabel("Explore chaotic systems and nonlinear dynamics", this);
    subtitleLabel->setStyleSheet("QLabel { color: #b0b0b0; font-size: 16px; margin-bottom: 24px; }");
    mainLayout->addWidget(subtitleLabel, 0, Qt::AlignCenter);

    auto addSystemCard = [&](const QString& title, const QString& desc,
                             const QString& imgPath, const QString& accentColor,
                             std::function<void()> onClick) {
        auto* button = new QPushButton(this);
        button->setStyleSheet(QString(
                                  "QPushButton { background-color: #3a3a3d; border: 1px solid #4a4a4d; border-radius: 12px; padding: 20px; text-align:left; } "
                                  "QPushButton:hover { background-color: #4a4a4d; border: 2px solid %1; }"
                                  ).arg(accentColor));
        button->setFlat(true);

        button->setMinimumHeight(160);

        auto* hbox = new QHBoxLayout(button);
        hbox->setSpacing(24);

        auto* imgLabel = new QLabel(button);
        imgLabel->setPixmap(QPixmap(imgPath).scaled(200, 200,
                                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));

        auto* textLabel = new QLabel(
            QString("<h3 style='color:%1; font-size:20px; margin:0;'>%2</h3>"
                    "<p style='color:#e0e0e0; font-size:14px; line-height:1.5; margin-top:8px;'>%3</p>")
                .arg(accentColor, title, desc), button);
        textLabel->setWordWrap(true);

        hbox->addWidget(imgLabel);
        hbox->addWidget(textLabel, 1);

        connect(button, &QPushButton::clicked, this, onClick);
        mainLayout->addWidget(button);
    };

    addSystemCard("Lorenz Attractor",
                  "A chaotic system modeling atmospheric convection, famous for its butterfly-shaped trajectory.",
                  ":/images/images/Lorenz.png", "#00bcd4",
                  [this]{ LorenzDialog dlg(this); dlg.exec(); });

    addSystemCard("Rössler Attractor",
                  "A simpler chaotic system with spiraling trajectories, often used to study chaos theory.",
                  ":/images/images/Rossler.png", "#ff9800",
                  [this]{ RosslerDialog dlg(this); dlg.exec(); });

    addSystemCard("Van der Pol Oscillator",
                  "A nonlinear oscillator used in electronics and biology, showing self-sustained oscillations.",
                  ":/images/images/VanDerPol.png", "#9c27b0",
                  [this]{ VanDerPolDialog dlg(this); dlg.exec(); });

    addSystemCard("Double Pendulum",
                  "Two pendulums attached end to end, a mechanical system that quickly becomes chaotic.",
                  ":/images/images/DoublePendulum.png", "#4caf50",
                  [this]{ DoublePendulumDialog dlg(this); dlg.exec(); });

    mainLayout->addStretch();

    auto* startBtn = new QPushButton("Start Simulation", this);
    startBtn->setStyleSheet(
        "QPushButton { background-color: #00bcd4; color: #1a1a1a; font-size: 16px; font-weight: 600; "
        "padding: 12px 32px; border-radius: 6px; border: none; } "
        "QPushButton:hover { background-color: #00acc4; }"
        "QPushButton:pressed { background-color: #0097a7; }"
        );

    mainLayout->addWidget(startBtn, 0, Qt::AlignCenter);
    connect(startBtn, &QPushButton::clicked, this, &QDialog::accept);
}
