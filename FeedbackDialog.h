#ifndef FEEDBACKDIALOG_H
#define FEEDBACKDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QLabel>

class FeedbackDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FeedbackDialog(const QString &feedback, QWidget *parent = nullptr);

private:
    void setupUI();
    void setupScrollableArea(const QString &feedback);
    
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QTextEdit *feedbackText;
    QPushButton *closeButton;
};

#endif // FEEDBACKDIALOG_H 