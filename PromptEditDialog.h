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

enum class PromptType {
    Translation,
    Report,
    Feedback
};

class PromptEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PromptEditDialog(PromptType type, QWidget *parent = nullptr);
    
    void setPrompt(const QString &prompt);
    QString getPrompt() const;

private slots:
    void onAccepted();
    void onResetToDefault();

private:
    QPlainTextEdit *promptEdit;
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBox;
    QPushButton *resetButton;
    PromptType promptType;
    
    void setupUI();
    void createPromptSection();
    QString getPromptTypeName() const;
    QString getVariableInfo() const;
    QString getDefaultPrompt() const;
};

#endif // PROMPTEDITDIALOG_H 