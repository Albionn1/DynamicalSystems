#include "vanderpoldialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

VanDerPolDialog::VanDerPolDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Van der Pol Oscillator");
    resize(1200, 740);
    setStyleSheet("QDialog { background-color: #1e1e24; }");

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* title = new QLabel("<h2 style='color:#9c27b0; margin:0;'>Van der Pol Oscillator</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:22px; font-weight:bold; }");
    layout->addWidget(title);

    auto* desc = new QLabel("The Van der Pol oscillator is a nonlinear system with self-sustained oscillations, widely used in electronics and biology.", this);
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#f5f5f5; font-size:16px; }");
    layout->addWidget(desc);

    auto* svg = new QSvgWidget(":/images/images/VanDerPolEquationVector.svg", this);
    svg->setFixedSize(300, 60);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    auto* eqTitle = new QLabel("Equations", this);
    eqTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
    layout->addWidget(eqTitle);

    auto* variables = new QLabel(
        "x – displacement (position of the oscillator)<br>"
        "dx/dt – velocity (rate of change of x)<br>"
        "d²x/dt² – acceleration (rate of change of velocity)<br>"
        "μ – nonlinearity parameter (controls strength of damping)<br>"
        "t – time",
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
        "• <b>Behavior:</b> Limit cycles depending on μ.<br>"
        "• <b>Applications:</b> Electrical circuits, heart dynamics, biological rhythms.",
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
