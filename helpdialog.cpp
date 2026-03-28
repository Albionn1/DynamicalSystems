#include "helpdialog.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFontMetrics>

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Simulation Shortcuts");
    resize(800, 500);  // compact size
    setStyleSheet("QDialog { background-color:#1e1e24; }");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Title
    auto* title = new QLabel("<h2 style='color:#00bcd4; margin:0;'>Keyboard Shortcuts</h2>", this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet("font-size:18px;");
    mainLayout->addWidget(title);

    // Helper lambda to create a card
    auto makeCard = [this](const QString& shortcut, const QString& description) {
        auto* frame = new QFrame(this);
        frame->setStyleSheet(
            "QFrame { background:#2c2c34; border-radius:6px; padding:6px; } "
            "QLabel { color:#f5f5f5; font-size:13px; }"
            );
        auto* vbox = new QVBoxLayout(frame);
        auto* lblShortcut = new QLabel("<b>" + shortcut + "</b>", frame);
        auto* lblDesc = new QLabel(description, frame);
        lblDesc->setWordWrap(true);
        vbox->addWidget(lblShortcut);
        vbox->addWidget(lblDesc);
        return frame;
    };

    // Helper to build a category layout
    auto buildCategory = [&](const QString& name, const QStringList& shortcuts, const QStringList& descriptions) {
        auto* vbox = new QVBoxLayout();
        auto* catTitle = new QLabel("<h3 style='color:#ff9800; margin:0;'>" + name + "</h3>", this);
        catTitle->setTextFormat(Qt::RichText);
        catTitle->setStyleSheet("font-size:15px;");
        vbox->addWidget(catTitle);

        auto* grid = new QGridLayout();
        QFontMetrics fm(catTitle->font());
        grid->setVerticalSpacing(fm.height() / 2);  // dynamic spacing
        grid->setHorizontalSpacing(8);

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

    mainLayout->addStretch();

    // Close button
    auto* closeBtn = new QPushButton("Close", this);
    closeBtn->setStyleSheet("QPushButton { background:#2c2c34; color:white; padding:8px 16px; border-radius:6px; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);
}
