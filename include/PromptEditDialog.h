#ifndef PROMPTEDITDIALOG_H
#define PROMPTEDITDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>

class PromptEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PromptEditDialog(QWidget *parent = nullptr);
    
    void setTranslationPrompt(const QString &prompt);
    void setFeedbackPrompt(const QString &prompt);
    void setReportPrompt(const QString &prompt);
    
    QString getTranslationPrompt() const;
    QString getFeedbackPrompt() const;
    QString getReportPrompt() const;

private slots:
    void onAccepted();
    void onResetToDefaults();

private:
    QPlainTextEdit *translationPromptEdit;
    QPlainTextEdit *feedbackPromptEdit;
    QPlainTextEdit *reportPromptEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *resetButton;
    
    void setupUI();
    void createTranslationSection();
    void createFeedbackSection();
    void createReportSection();
};

#endif // PROMPTEDITDIALOG_H 