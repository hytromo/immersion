#ifndef APIKEYDIALOG_H
#define APIKEYDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ApiKeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ApiKeyDialog(QWidget *parent = nullptr);
    QString getApiKey() const;

private:
    QLineEdit *input;
    QString apiKey;
};

#endif // APIKEYDIALOG_H 