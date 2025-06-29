#include "PromptEditDialog.h"
#include "SettingsManager.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>

PromptEditDialog::PromptEditDialog(QWidget *parent)
    : QDialog(parent)
    , translationPromptEdit(nullptr)
    , feedbackPromptEdit(nullptr)
    , reportPromptEdit(nullptr)
    , buttonBox(nullptr)
    , resetButton(nullptr)
{
    setWindowTitle("Edit Prompts");
    setModal(true);
    
    // Set a reasonable size for the dialog
    resize(700, 600);
    
    // Center the dialog on the screen
    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    setupUI();
}

void PromptEditDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Create sections for each prompt type first
    createTranslationSection();
    createFeedbackSection();
    createReportSection();
    
    // Now add sections to main layout (widgets exist now)
    mainLayout->addWidget(translationPromptEdit->parentWidget());
    mainLayout->addWidget(feedbackPromptEdit->parentWidget());
    mainLayout->addWidget(reportPromptEdit->parentWidget());
    
    // Add reset button
    resetButton = new QPushButton("Reset to Defaults", this);
    resetButton->setStyleSheet("QPushButton { font-size: 12pt; padding: 8px; }");
    connect(resetButton, &QPushButton::clicked, this, &PromptEditDialog::onResetToDefaults);
    mainLayout->addWidget(resetButton);
    
    // Add dialog buttons
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PromptEditDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

void PromptEditDialog::createTranslationSection()
{
    auto groupBox = new QGroupBox("Translation Prompt", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Edit the prompt used for translation. Use %sourceLang and %targetLang as variables.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    translationPromptEdit = new QPlainTextEdit(this);
    translationPromptEdit->setMinimumHeight(120);
    translationPromptEdit->setStyleSheet("QPlainTextEdit { font-size: 11pt; }");
    layout->addWidget(translationPromptEdit);
}

void PromptEditDialog::createFeedbackSection()
{
    auto groupBox = new QGroupBox("Feedback Prompt", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Edit the prompt used for quick feedback. Use %sourceLang as a variable.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    feedbackPromptEdit = new QPlainTextEdit(this);
    feedbackPromptEdit->setMinimumHeight(120);
    feedbackPromptEdit->setStyleSheet("QPlainTextEdit { font-size: 11pt; }");
    layout->addWidget(feedbackPromptEdit);
}

void PromptEditDialog::createReportSection()
{
    auto groupBox = new QGroupBox("Report Prompt", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Edit the prompt used for generating reports. Use %sourceLang as a variable.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    reportPromptEdit = new QPlainTextEdit(this);
    reportPromptEdit->setMinimumHeight(120);
    reportPromptEdit->setStyleSheet("QPlainTextEdit { font-size: 11pt; }");
    layout->addWidget(reportPromptEdit);
}

void PromptEditDialog::setTranslationPrompt(const QString &prompt)
{
    if (translationPromptEdit) {
        translationPromptEdit->setPlainText(prompt);
    }
}

void PromptEditDialog::setFeedbackPrompt(const QString &prompt)
{
    if (feedbackPromptEdit) {
        feedbackPromptEdit->setPlainText(prompt);
    }
}

void PromptEditDialog::setReportPrompt(const QString &prompt)
{
    if (reportPromptEdit) {
        reportPromptEdit->setPlainText(prompt);
    }
}

QString PromptEditDialog::getTranslationPrompt() const
{
    return translationPromptEdit ? translationPromptEdit->toPlainText().trimmed() : QString();
}

QString PromptEditDialog::getFeedbackPrompt() const
{
    return feedbackPromptEdit ? feedbackPromptEdit->toPlainText().trimmed() : QString();
}

QString PromptEditDialog::getReportPrompt() const
{
    return reportPromptEdit ? reportPromptEdit->toPlainText().trimmed() : QString();
}

void PromptEditDialog::onAccepted()
{
    // Validate that all prompts are not empty
    if (getTranslationPrompt().isEmpty() || getFeedbackPrompt().isEmpty() || getReportPrompt().isEmpty()) {
        return; // Don't accept if any prompt is empty
    }
    accept();
}

void PromptEditDialog::onResetToDefaults()
{
    qDebug() << "Reset prompts to defaults clicked!";
    
    // Add visual feedback
    resetButton->setText("Resetting...");
    resetButton->setEnabled(false);
    
    // Create SettingsManager instance to get default values
    SettingsManager settingsManager;
    
    // Reset the values using defaults from SettingsManager
    setTranslationPrompt(settingsManager.getDefaultTranslationPrompt());
    setFeedbackPrompt(settingsManager.getDefaultFeedbackPrompt());
    setReportPrompt(settingsManager.getDefaultReportPrompt());
    
    // Restore button state
    resetButton->setText("Reset to Defaults");
    resetButton->setEnabled(true);
    
    qDebug() << "Prompt reset complete.";
} 