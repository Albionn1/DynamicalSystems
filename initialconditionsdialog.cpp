#include "initialconditionsdialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

InitialConditionsDialog::InitialConditionsDialog(const QString& systemName, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Initial Conditions");
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QFormLayout* formLayout = new QFormLayout();

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

    QPushButton* okBtn = new QPushButton("Apply");
    connect(okBtn, &QPushButton::clicked, this, &InitialConditionsDialog::accept);
    mainLayout->addWidget(okBtn);
}

std::vector<double> InitialConditionsDialog::values() const {
    std::vector<double> vals;
    vals.reserve(edits_.size());
    for (QLineEdit* e : edits_) {
        vals.push_back(e->text().toDouble());
    }
    return vals;
}

