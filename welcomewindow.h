#pragma once
#include <QDialog>

class QPushButton;
class QTextEdit;

class WelcomeWindow : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeWindow(QWidget* parent = nullptr);
};
