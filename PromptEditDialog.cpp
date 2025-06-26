#include "PromptEditDialog.h"
#include <QScrollArea>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include "SettingsManager.h"

PromptEditDialog::PromptEditDialog(PromptType type, QWidget *parent)
    : QDialog(parent)
    , promptEdit(nullptr)
    , buttonBox(nullptr)
    , groupBox(nullptr)
    , resetButton(nullptr)
    , promptType(type)
{
    setWindowTitle("Edit " + getPromptTypeName() + " Prompt");
    setModal(true);
    resize(500, 300);
    setupUI();
}

void PromptEditDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    createPromptSection();
    
    // Create button layout
    auto buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10); // Add spacing between buttons
    
    // Create reset button
    resetButton = new QPushButton("Reset to Default", this);
    resetButton->setContentsMargins(10, 10, 10, 10);
    connect(resetButton, &QPushButton::clicked, this, &PromptEditDialog::onResetToDefault);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();
    
    // Create button box
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PromptEditDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonLayout->addWidget(buttonBox);
    
    mainLayout->addWidget(groupBox);
    mainLayout->addLayout(buttonLayout);
}

void PromptEditDialog::createPromptSection()
{
    groupBox = new QGroupBox(getPromptTypeName() + " Prompt", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Edit the prompt used for " + getPromptTypeName().toLower() + ". " + getVariableInfo(), this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    promptEdit = new QPlainTextEdit(this);
    promptEdit->setMinimumHeight(150);
    layout->addWidget(promptEdit);
}

QString PromptEditDialog::getPromptTypeName() const
{
    switch (promptType) {
        case PromptType::Translation:
            return "Translation";
        case PromptType::Report:
            return "Report";
        default:
            return "Unknown";
    }
}

QString PromptEditDialog::getVariableInfo() const
{
    switch (promptType) {
        case PromptType::Translation:
            return "Use %sourceLang and %targetLang as variables.";
        case PromptType::Report:
            return "Use %sourceLang as a variable.";
        default:
            return "";
    }
}

QString PromptEditDialog::getDefaultPrompt() const
{
    SettingsManager tempManager;
    switch (promptType) {
        case PromptType::Translation:
            return tempManager.getDefaultTranslationPrompt();
        case PromptType::Report:
            return tempManager.getDefaultReportPrompt();
        default:
            return QString();
    }
}

void PromptEditDialog::setPrompt(const QString &prompt)
{
    if (promptEdit) {
        promptEdit->setPlainText(prompt);
    }
}

QString PromptEditDialog::getPrompt() const
{
    return promptEdit ? promptEdit->toPlainText() : QString();
}

void PromptEditDialog::onAccepted()
{
    accept();
}

void PromptEditDialog::onResetToDefault()
{
    if (promptEdit) {
        promptEdit->setPlainText(getDefaultPrompt());
    }
} 