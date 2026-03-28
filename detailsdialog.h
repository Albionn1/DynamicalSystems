#ifndef DETAILSDIALOG_H
#define DETAILSDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class DetailsDialog : public QDialog {
    Q_OBJECT
public:
    DetailsDialog(const QString& title, const QString& formulas, QWidget* parent = nullptr)
        : QDialog(parent) {
        setWindowTitle(title);
        resize(600, 400);

        auto* layout = new QVBoxLayout(this);

        auto* label = new QLabel(formulas, this);
        label->setWordWrap(true);
        label->setTextFormat(Qt::RichText);
        layout->addWidget(label);

        auto* closeBtn = new QPushButton("Close", this);
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(closeBtn, 0, Qt::AlignRight);
    }
};

#endif // DETAILSDIALOG_H
