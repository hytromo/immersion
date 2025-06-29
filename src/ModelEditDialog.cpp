#include "ModelEditDialog.h"
#include "SettingsManager.h"
#include <QApplication>
#include <QScreen>
#include <QDebug>

ModelEditDialog::ModelEditDialog(QWidget *parent)
    : QDialog(parent)
    , translationModelEdit(nullptr)
    , feedbackModelEdit(nullptr)
    , reportModelEdit(nullptr)
    , buttonBox(nullptr)
    , resetButton(nullptr)
{
    setWindowTitle("Edit Models");
    setModal(true);
    
    // Set a reasonable size for the dialog
    resize(500, 400);
    
    // Center the dialog on the screen
    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    setupUI();
}

void ModelEditDialog::setupUI()
{
    auto mainLayout = new QVBoxLayout(this);
    
    // Create sections for each model type first
    createTranslationSection();
    createFeedbackSection();
    createReportSection();
    
    // Now add sections to main layout (widgets exist now)
    mainLayout->addWidget(translationModelEdit->parentWidget());
    mainLayout->addWidget(feedbackModelEdit->parentWidget());
    mainLayout->addWidget(reportModelEdit->parentWidget());
    
    // Add reset button
    resetButton = new QPushButton("Reset to Defaults", this);
    resetButton->setStyleSheet("QPushButton { font-size: 12pt; padding: 8px; }");
    connect(resetButton, &QPushButton::clicked, this, &ModelEditDialog::onResetToDefaults);
    mainLayout->addWidget(resetButton);
    
    // Add dialog buttons
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ModelEditDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

void ModelEditDialog::createTranslationSection()
{
    auto groupBox = new QGroupBox("Translation Model", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Model used for translating text between languages.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    auto inputLayout = new QHBoxLayout();
    auto label = new QLabel("Model name:", this);
    translationModelEdit = new QLineEdit(this);
    translationModelEdit->setPlaceholderText("e.g., gpt-4o-mini, gpt-4, gpt-3.5-turbo");
    translationModelEdit->setStyleSheet("QLineEdit { font-size: 12pt; padding: 4px; }");
    
    inputLayout->addWidget(label);
    inputLayout->addWidget(translationModelEdit);
    layout->addLayout(inputLayout);
}

void ModelEditDialog::createFeedbackSection()
{
    auto groupBox = new QGroupBox("Feedback Model", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Model used for providing quick feedback on language usage.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    auto inputLayout = new QHBoxLayout();
    auto label = new QLabel("Model name:", this);
    feedbackModelEdit = new QLineEdit(this);
    feedbackModelEdit->setPlaceholderText("e.g., gpt-4o-mini, gpt-4, gpt-3.5-turbo");
    feedbackModelEdit->setStyleSheet("QLineEdit { font-size: 12pt; padding: 4px; }");
    
    inputLayout->addWidget(label);
    inputLayout->addWidget(feedbackModelEdit);
    layout->addLayout(inputLayout);
}

void ModelEditDialog::createReportSection()
{
    auto groupBox = new QGroupBox("Report Model", this);
    auto layout = new QVBoxLayout(groupBox);
    
    auto infoLabel = new QLabel("Model used for generating detailed reports on language mistakes.", this);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);
    
    auto inputLayout = new QHBoxLayout();
    auto label = new QLabel("Model name:", this);
    reportModelEdit = new QLineEdit(this);
    reportModelEdit->setPlaceholderText("e.g., gpt-4o-mini, gpt-4, gpt-3.5-turbo");
    reportModelEdit->setStyleSheet("QLineEdit { font-size: 12pt; padding: 4px; }");
    
    inputLayout->addWidget(label);
    inputLayout->addWidget(reportModelEdit);
    layout->addLayout(inputLayout);
}

void ModelEditDialog::setTranslationModel(const QString &model)
{
    if (translationModelEdit) {
        translationModelEdit->setText(model);
    }
}

void ModelEditDialog::setFeedbackModel(const QString &model)
{
    if (feedbackModelEdit) {
        feedbackModelEdit->setText(model);
    }
}

void ModelEditDialog::setReportModel(const QString &model)
{
    if (reportModelEdit) {
        reportModelEdit->setText(model);
    }
}

QString ModelEditDialog::getTranslationModel() const
{
    return translationModelEdit ? translationModelEdit->text().trimmed() : QString();
}

QString ModelEditDialog::getFeedbackModel() const
{
    return feedbackModelEdit ? feedbackModelEdit->text().trimmed() : QString();
}

QString ModelEditDialog::getReportModel() const
{
    return reportModelEdit ? reportModelEdit->text().trimmed() : QString();
}

void ModelEditDialog::onAccepted()
{
    // Validate that all models are not empty
    if (getTranslationModel().isEmpty() || getFeedbackModel().isEmpty() || getReportModel().isEmpty()) {
        return; // Don't accept if any model is empty
    }
    accept();
}

void ModelEditDialog::onResetToDefaults()
{
    qDebug() << "Reset to defaults clicked!";
    
    // Add visual feedback
    resetButton->setText("Resetting...");
    resetButton->setEnabled(false);
    
    // Create SettingsManager instance to get default values
    SettingsManager settingsManager;
    
    // Reset the values using defaults from SettingsManager
    setTranslationModel(settingsManager.getDefaultTranslationModel());
    setFeedbackModel(settingsManager.getDefaultFeedbackModel());
    setReportModel(settingsManager.getDefaultReportModel());
    
    // Restore button state
    resetButton->setText("Reset to Defaults");
    resetButton->setEnabled(true);
    
    qDebug() << "Reset complete. Translation:" << getTranslationModel() 
             << "Feedback:" << getFeedbackModel() 
             << "Report:" << getReportModel();
} 