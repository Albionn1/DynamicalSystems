#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QFormLayout>
#include <QVector>

class InitialConditionsDialog : public QDialog {
    Q_OBJECT
public:
    explicit InitialConditionsDialog(const QString& systemName, QWidget* parent = nullptr);

    std::vector<double> values() const; // return parsed initial conditions

private:
    QList<QLineEdit*> edits_;         // dynamic list of input fields
};
