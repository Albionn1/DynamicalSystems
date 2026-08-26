#include "helpdialog.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFontMetrics>

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Simulation Shortcuts");
    resize(900, 600);
    setStyleSheet(
        "QDialog { background-color: #2d2d30; }"
        "QLabel { color: #e0e0e0; }"
        "QPushButton { background-color: #3a3a3d; color: #ffffff; border: 1px solid #4a4a4d; "
        "padding: 8px 16px; border-radius: 4px; font-weight: 500; }"
        "QPushButton:hover { background-color: #4a4a4d; border-color: #5a5a5d; }"
        "QPushButton:pressed { background-color: #2a2a2d; }"
        );

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // Title
    auto* title = new QLabel("<h2 style='color:#00bcd4; margin:0;'>Keyboard Shortcuts</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("QLabel { color:#00bcd4; font-size:22px; font-weight:bold; margin-bottom:8px; }");
    mainLayout->addWidget(title);

    // Helper lambda to create a card
    auto makeCard = [this](const QString& shortcut, const QString& description) {
        auto* frame = new QFrame(this);
        frame->setStyleSheet(
            "QFrame { background:#3a3a3d; border: 1px solid #4a4a4d; border-radius:8px; padding:12px; } "
            "QLabel { color:#e0e0e0; font-size:13px; }"
            );
        auto* vbox = new QVBoxLayout(frame);
        vbox->setSpacing(4);
        auto* lblShortcut = new QLabel("<b>" + shortcut + "</b>", frame);
        lblShortcut->setStyleSheet("QLabel { color:#00bcd4; font-size:14px; font-weight:bold; }");
        auto* lblDesc = new QLabel(description, frame);
        lblDesc->setWordWrap(true);
        lblDesc->setStyleSheet("QLabel { color:#b0b0b0; font-size:12px; }");
        vbox->addWidget(lblShortcut);
        vbox->addWidget(lblDesc);
        return frame;
    };

    // Helper to build a category layout
    auto buildCategory = [&](const QString& name, const QStringList& shortcuts, const QStringList& descriptions) {
        auto* vbox = new QVBoxLayout();
        vbox->setSpacing(12);
        auto* catTitle = new QLabel("<h3 style='color:#ff9800; margin:0;'>" + name + "</h3>", this);
        catTitle->setTextFormat(Qt::RichText);
        catTitle->setStyleSheet("QLabel { color:#ff9800; font-size:16px; font-weight:bold; margin-bottom:8px; }");
        vbox->addWidget(catTitle);

        auto* grid = new QGridLayout();
        QFontMetrics fm(catTitle->font());
        grid->setVerticalSpacing(12);
        grid->setHorizontalSpacing(12);

        for (int i = 0; i < shortcuts.size(); ++i) {
            int row = i / 3;
            int col = i % 3;
            grid->addWidget(makeCard(shortcuts[i], descriptions[i]), row, col);
        }
        vbox->addLayout(grid);
        return vbox;
    };

    // Categories
    mainLayout->addLayout(buildCategory("System Control",
                                        {"1–4", "R", "I"},
                                        {"Switch between systems (Lorenz, Rössler, Van der Pol, Double Pendulum)",
                                         "Reset simulation",
                                         "Set initial conditions (open dialog)"}));

    mainLayout->addLayout(buildCategory("Simulation Control",
                                        {"Space", "[ ]"},
                                        {"Pause / Resume simulation",
                                         "Adjust dt (time step)"}));

    mainLayout->addLayout(buildCategory("View / Navigation",
                                        {"+ / -", "C", "F", "G", "O"},
                                        {"Zoom in/out",
                                         "Toggle color mode",
                                         "Toggle fading",
                                         "Toggle grid overlay",
                                         "Cycle draw mode (Trail / Poincaré / Both)"}));

    mainLayout->addLayout(buildCategory("Overlays",
                                        {"T / Y", "H"},
                                        {"Increase / Decrease trail length (points)",
                                         "Cycle educational overlays (Phase Space → Energy → Lyapunov → Info → None)"}));

    mainLayout->addLayout(buildCategory("Sidebar Controls",
                                        {"Mouse Click"},
                                        {"Use sidebar buttons for Save, Reset, Both Directions, and Help"}));

    mainLayout->addStretch();

    // Close button
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
}
