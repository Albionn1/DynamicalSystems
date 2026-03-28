#include "rosslerdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

RosslerDialog::RosslerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Rössler System");
    resize(1200, 740);
    setStyleSheet("QDialog { background-color: #1e1e24; }");

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("<h2 style='color:#ff9800; margin:0;'>Rössler Attractor</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:22px; font-weight:bold; }");
    layout->addWidget(title);

    auto* desc = new QLabel("The Rössler system is a continuous-time dynamical system exhibiting chaotic behavior similar to the Lorenz system.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#f5f5f5; font-size:16px; }");
    layout->addWidget(desc);

    auto* svg = new QSvgWidget(":/images/images/RosslerEquationVector.svg", this);
    svg->setFixedSize(250, 200);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    auto* eqTitle = new QLabel("Equations", this);
    eqTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
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
        "QLabel { background:#2c2c34; color:#f5f5f5; font-family:monospace; "
        "font-size:15px; padding:12px; border-radius:6px; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Behavior:</b> Chaotic attractor with spiral structure.<br>"
        "• <b>Applications:</b> Chaos theory demonstrations, nonlinear dynamics.",
        this
        );
    notes->setTextFormat(Qt::RichText);
    notes->setWordWrap(true);
    notes->setStyleSheet("QLabel { color:#f5f5f5; font-size:15px; }");
    layout->addWidget(notes);

    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet("QPushButton { background:#2c2c34; color:white; padding:8px 16px; border-radius:4px; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}
