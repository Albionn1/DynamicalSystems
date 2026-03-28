#include "doublependulumdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSvgWidget>

DoublePendulumDialog::DoublePendulumDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Double Pendulum");
    resize(1500, 740);
    setStyleSheet("QDialog { background-color: #1e1e24; }");

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 16, 16, 16);

    // Title
    auto* title = new QLabel("<h2 style='color:#4caf50; margin:0;'>Double Pendulum</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:22px; font-weight:bold; }");
    layout->addWidget(title);

    // Description
    auto* desc = new QLabel(
        "The double pendulum is a classic example of a chaotic mechanical system, "
        "showing sensitive dependence on initial conditions and complex motion.",
        this
        );
    desc->setWordWrap(true);
    desc->setStyleSheet("QLabel { color:#f5f5f5; font-size:16px; }");
    layout->addWidget(desc);

    // SVG diagram
    auto* svg = new QSvgWidget(":/images/images/DoublePendulumEquationVector.svg", this);
    svg->setFixedSize(1400, 60);
    layout->addWidget(svg, 0, Qt::AlignCenter);

    // Variables
    auto* eqTitle = new QLabel("Variables", this);
    eqTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
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
        "QLabel { background:#2c2c34; color:#f5f5f5; font-family:monospace; "
        "font-size:15px; padding:12px; border-radius:6px; }"
        );
    variables->setWordWrap(true);
    layout->addWidget(variables);

    // Notes
    auto* notesTitle = new QLabel("Notes", this);
    notesTitle->setStyleSheet("QLabel { color:#cfd8dc; font-size:18px; font-weight:bold; margin-top:12px; }");
    layout->addWidget(notesTitle);

    auto* notes = new QLabel(
        "• <b>Behavior:</b> Highly sensitive to initial conditions, leading to chaotic trajectories.<br>"
        "• <b>Applications:</b> Demonstrations of chaos in mechanics and physics education.",
        this
        );
    notes->setTextFormat(Qt::RichText);
    notes->setWordWrap(true);
    notes->setStyleSheet("QLabel { color:#f5f5f5; font-size:15px; }");
    layout->addWidget(notes);

    // Close button
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet(
        "QPushButton { background:#2c2c34; color:white; padding:8px 16px; border-radius:4px; }"
        );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(closeBtn, 0, Qt::AlignRight);
}
