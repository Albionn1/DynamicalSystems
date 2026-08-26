#include "doublependulumdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

DoublePendulumDialog::DoublePendulumDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Double Pendulum");
    resize(1500, 740);
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
    auto* title = new QLabel("<h2 style='color:#4caf50; margin:0;'>Double Pendulum</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#4caf50; font-size:24px; font-weight:bold; margin-bottom:8px; }");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel(
        "The double pendulum is a classic example of a chaotic mechanical system, "
        "showing sensitive dependence on initial conditions and complex motion.",
        this
        );
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#b0b0b0; font-size:15px; line-height:1.5; }");
    layout->addWidget(desc);

    // SVG diagram
    auto* svg = new QSvgWidget(":/images/images/DoublePendulumEquationVector.svg", this);
    svg->setFixedSize(1200, 80);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    // Variables
    auto* eqTitle = new QLabel("Variables", this);
    eqTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(eqTitle);

    auto* variables = new QLabel(
        "theta1 – angle of the first pendulum arm (radians)<br>"
        "theta2 – angle of the second pendulum arm (radians)<br>"
        "theta1' – angular velocity of the first arm<br>"
        "theta2' – angular velocity of the second arm<br>"
        "theta1'' – angular acceleration of the first arm<br>"
        "theta2'' – angular acceleration of the second arm<br>"
        "m1 – mass of the first pendulum bob<br>"
        "m2 – mass of the second pendulum bob<br>"
        "L1 – length of the first rod<br>"
        "L2 – length of the second rod<br>"
        "g – gravitational acceleration (9.81 m/s²)",
        this
        );

    variables->setTextFormat(Qt::RichText);
    variables->setStyleSheet(
        "QLabel { background:#3a3a3d; color:#e0e0e0; font-family:monospace; "
        "font-size:13px; padding:16px; border-radius:8px; border: 1px solid #4a4a4d; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    // Notes
    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#d0d0d0; font-size:18px; font-weight:bold; margin-top:16px; margin-bottom:8px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Behavior:</b> Highly sensitive to initial conditions, leading to chaotic trajectories.<br>"
        "• <b>Applications:</b> Demonstrations of chaos in mechanics and physics education.",
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
