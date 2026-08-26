#include "initialconditionsdialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFormLayout>

InitialConditionsDialog::InitialConditionsDialog(const QString& systemName, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Initial Conditions");
    setStyleSheet(
        "QDialog { background-color: #2d2d30; }"
        "QLabel { color: #e0e0e0; }"
        "QLineEdit { background-color: #3a3a3d; color: #ffffff; border: 1px solid #4a4a4d; "
        "padding: 8px; border-radius: 4px; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid #00bcd4; background-color: #404043; }"
        "QPushButton { background-color: #3a3a3d; color: #ffffff; border: 1px solid #4a4a4d; "
        "padding: 10px 20px; border-radius: 4px; font-weight: 500; }"
        "QPushButton:hover { background-color: #4a4a4d; border-color: #5a5a5d; }"
        "QPushButton:pressed { background-color: #2a2a2d; }"
        );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // Title
    auto* title = new QLabel(QString("Set Initial Conditions for %1").arg(systemName), this);
    title->setStyleSheet("QLabel { color: #d0d0d0; font-size: 18px; font-weight: bold; margin-bottom: 8px; }");
    mainLayout->addWidget(title);

    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setContentsMargins(0, 0, 0, 0);

    // Choose fields based on system
    if (systemName == "Lorenz") {
        QStringList labels = {"x", "y", "z"};
        for (const QString& lbl : labels) {
            QLineEdit* edit = new QLineEdit("1.0");
            formLayout->addRow(lbl + ":", edit);
            edits_.append(edit);
        }
    } else if (systemName == "Rössler") {
        QStringList labels = {"x", "y", "z"};
        for (const QString& lbl : labels) {
            QLineEdit* edit = new QLineEdit("0.1");
            formLayout->addRow(lbl + ":", edit);
            edits_.append(edit);
        }
    } else if (systemName == "Van der Pol") {
        QStringList labels = {"x", "dx/dt"};
        for (const QString& lbl : labels) {
            QLineEdit* edit = new QLineEdit("0.0");
            formLayout->addRow(lbl + ":", edit);
            edits_.append(edit);
        }
    } else if (systemName == "Double Pendulum") {
        QStringList labels = {"θ1", "θ2", "θ1_dot", "θ2_dot"};
        for (const QString& lbl : labels) {
            QLineEdit* edit = new QLineEdit("0.1");
            formLayout->addRow(lbl + ":", edit);
            edits_.append(edit);
        }
    }

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    QPushButton* okBtn = new QPushButton("Apply");
    connect(okBtn, &QPushButton::clicked, this, &InitialConditionsDialog::accept);
    mainLayout->addWidget(okBtn, 0, Qt::AlignRight);
}

std::vector<double> InitialConditionsDialog::values() const {
    std::vector<double> vals;
    vals.reserve(edits_.size());
    for (QLineEdit* e : edits_) {
        vals.push_back(e->text().toDouble());
    }
    return vals;
}

