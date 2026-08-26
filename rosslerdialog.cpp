#include "rosslerdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

RosslerDialog::RosslerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Rössler System");
    resize(1200, 740);
    setStyleSheet(
        "QDialog { background-color: #2d2d30; }"
        "QLabel { color: #e0e0e0; }"
        "QPushButton { background-color: #3a3a3d; color: #ffffff; border: 1px solid #4a4a4d; "
        "padding: 8px 16px; border-radius: 4px; font-weight: 500; }"
        "QPushButton:hover { background-color: #4a4a4d; border-color: #5a5a5d; }"
        "QPushButton:pressed { background-color: #2a2a2d; }"
        );

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(24, 24, 24, 24);

    auto* title = new QLabel("<h2 style='color:#ff9800; margin:0;'>Rössler Attractor</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#ff9800; font-size:24px; font-weight:bold; margin-bottom:8px; }");
    layout->addWidget(title);

    auto* desc = new QLabel("The Rössler system is a continuous-time dynamical system exhibiting chaotic behavior similar to the Lorenz system.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#b0b0b0; font-size:15px; line-height:1.5; }");
    layout->addWidget(desc);

    auto* svg = new QSvgWidget(":/images/images/RosslerEquationVector.svg", this);
    svg->setFixedSize(280, 220);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    auto* eqTitle = new QLabel("Equations", this);
    eqTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(eqTitle);

    // Variables
    auto* variables = new QLabel(
        "x – first coordinate (oscillatory component)<br>"
        "y – second coordinate (spiral component)<br>"
        "z – third coordinate (growth/decay component)<br>"
        "a – parameter controlling twisting of the spiral<br>"
        "b – parameter controlling vertical rise<br>"
        "c – parameter controlling damping strength",
        this
        );

    variables->setTextFormat(Qt::RichText);
    variables->setStyleSheet(
        "QLabel { background:#3a3a3d; color:#e0e0e0; font-family:monospace; "
        "font-size:14px; padding:16px; border-radius:8px; border: 1px solid #4a4a4d; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Behavior:</b> Chaotic attractor with spiral structure.<br>"
        "• <b>Applications:</b> Chaos theory demonstrations, nonlinear dynamics.",
        this
        );
    notes->setTextFormat(Qt::RichText);
    notes->setWordWrap(true);
    notes->setStyleSheet("QLabel { color:#b0b0b0; font-size:14px; line-height:1.6; }");
    layout->addWidget(notes);

    layout->addStretch();

    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}
