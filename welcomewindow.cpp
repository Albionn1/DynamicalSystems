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
    setStyleSheet("QDialog { background-color: #1e1e24; }");

    auto* mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(30);

    auto addSystemCard = [&](const QString& title, const QString& desc,
                             const QString& imgPath, const QString& accentColor,
                             std::function<void()> onClick) {
        auto* button = new QPushButton(this);
        button->setStyleSheet(QString(
                                  "QPushButton { background-color: #2c2c34; border-radius: 12px; padding: 20px; text-align:left; } "
                                  "QPushButton:hover { background-color: #3a3a44; border: 2px solid %1; }"
                                  ).arg(accentColor));
        button->setFlat(true);

        button->setMinimumHeight(180);

        auto* hbox = new QHBoxLayout(button);
        hbox->setSpacing(20);

        auto* imgLabel = new QLabel(button);
        imgLabel->setPixmap(QPixmap(imgPath).scaled(220, 220,
                                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));

        auto* textLabel = new QLabel(
            QString("<h3 style='color:%1; font-size:18px; margin:0;'>%2</h3>"
                    "<p style='color:#f5f5f5; font-size:14px; line-height:1.4; margin-top:8px;'>%3</p>")
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

    auto* startBtn = new QPushButton("Start Simulation", this);
    startBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: #ffffff; font-size: 16px; font-weight: 600; "
        "padding: 10px 24px; border-radius: 6px; border: 1px solid #2980b9; } "
        "QPushButton:hover { background-color: #2980b9; border-color: #1f6391; }"
        );

    mainLayout->addWidget(startBtn, 0, Qt::AlignCenter);
    connect(startBtn, &QPushButton::clicked, this, &QDialog::accept);
}
