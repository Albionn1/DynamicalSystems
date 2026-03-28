#include "lorenzdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

LorenzDialog::LorenzDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Lorenz Attractor");
    resize(1200, 740);
    setStyleSheet("QDialog { background-color: #1e1e24; }");

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);   // tighter spacing control
    layout->setContentsMargins(16, 16, 16, 16);

    // Title
    auto* title = new QLabel("<h2 style='color:#00bcd4; margin:0;'>Lorenz Attractor</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:22px; font-weight:bold; }");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel("The Lorenz system models atmospheric convection and is famous for its butterfly-shaped chaotic trajectory.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#f5f5f5; font-size:16px; }");
    layout->addWidget(desc);

    auto* svg = new QSvgWidget(":/images/images/lorenzEquationVector.svg", this);
    svg->setFixedSize(250, 200);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    // Variables
    auto* eqTitle = new QLabel("Variables", this);
    eqTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
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
        "QLabel { background:#2c2c34; color:#f5f5f5; "
        "font-family:monospace; font-size:15px; padding:12px; border-radius:6px; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    auto* params = new QLabel("Typical parameters: σ = 10, ρ = 28, β = 8/3.", this);
    params->setStyleSheet("QLabel { color:#f5f5f5; font-size:15px; margin-top:8px; }");
    layout->addWidget(params);

    // Notes
    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Attractor:</b> Butterfly-shaped structure in phase space.<br>"
        "• <b>Behavior:</b> Chaotic for many parameter choices.<br>"
        "• <b>Applications:</b> Meteorology, nonlinear dynamics education.",
        this
        );
    notes->setTextFormat(Qt::RichText);
    notes->setWordWrap(true);
    notes->setStyleSheet("QLabel { color:#f5f5f5; font-size:15px; }");
    layout->addWidget(notes);

    // Close button
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet("QPushButton { background:#2c2c34; color:white; padding:8px 16px; border-radius:4px; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}
