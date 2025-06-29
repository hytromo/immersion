#ifndef MODELEDITDIALOG_H
#define MODELEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QGroupBox>

class ModelEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelEditDialog(QWidget *parent = nullptr);
    
    void setTranslationModel(const QString &model);
    void setFeedbackModel(const QString &model);
    void setReportModel(const QString &model);
    
    QString getTranslationModel() const;
    QString getFeedbackModel() const;
    QString getReportModel() const;

private slots:
    void onAccepted();
    void onResetToDefaults();

private:
    QLineEdit *translationModelEdit;
    QLineEdit *feedbackModelEdit;
    QLineEdit *reportModelEdit;
    QDialogButtonBox *buttonBox;
    QPushButton *resetButton;
    
    void setupUI();
    void createTranslationSection();
    void createFeedbackSection();
    void createReportSection();
};

#endif // MODELEDITDIALOG_H 