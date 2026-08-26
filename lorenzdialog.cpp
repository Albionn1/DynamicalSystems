#include "lorenzdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

LorenzDialog::LorenzDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Lorenz Attractor");
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

    // Title
    auto* title = new QLabel("<h2 style='color:#00bcd4; margin:0;'>Lorenz Attractor</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:24px; font-weight:bold; margin-bottom:8px; }");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("The Lorenz system models atmospheric convection and is famous for its butterfly-shaped chaotic trajectory.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#b0b0b0; font-size:15px; line-height:1.5; }");
    layout->addWidget(desc);

    auto* svg = new QSvgWidget(":/images/images/lorenzEquationVector.svg", this);
    svg->setFixedSize(280, 220);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    // Variables
    auto* eqTitle = new QLabel("Variables", this);
    eqTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(eqTitle);

    auto* variables = new QLabel(
        "x – convection intensity<br>"
        "y – temperature difference<br>"
        "z – vertical position<br>"
        "σ – Prandtl number (fluid property)<br>"
        "ρ – Rayleigh number (temperature gradient)<br>"
        "β – geometric factor (system damping)",
        this
        );
    variables->setTextFormat(Qt::RichText);
    variables->setStyleSheet(
        "QLabel { background:#3a3a3d; color:#e0e0e0; "
        "font-family:monospace; font-size:14px; padding:16px; border-radius:8px; border: 1px solid #4a4a4d; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    auto* params = new QLabel("Typical parameters: σ = 10, ρ = 28, β = 8/3.", this);
    params->setStyleSheet("QLabel { color:#b0b0b0; font-size:14px; margin-top:8px; font-style:italic; }");
    layout->addWidget(params);

    // Notes
    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Attractor:</b> Butterfly-shaped structure in phase space.<br>"
        "• <b>Behavior:</b> Chaotic for many parameter choices.<br>"
        "• <b>Applications:</b> Meteorology, nonlinear dynamics education.",
        this
        );
    notes->setTextFormat(Qt::RichText);
    notes->setWordWrap(true);
    notes->setStyleSheet("QLabel { color:#b0b0b0; font-size:14px; line-height:1.6; }");
    layout->addWidget(notes);

    layout->addStretch();

    // Close button
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}
