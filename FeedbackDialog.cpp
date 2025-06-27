#include "FeedbackDialog.h"
#include <QApplication>
#include <QScreen>

FeedbackDialog::FeedbackDialog(const QString &feedback, QWidget *parent)
    : QDialog(parent)
    , mainLayout(nullptr)
    , scrollArea(nullptr)
    , feedbackText(nullptr)
    , closeButton(nullptr)
{
    setWindowTitle("Language Feedback");
    setModal(true);
    
    // Set a reasonable size for the dialog
    resize(600, 400);
    
    // Center the dialog on the screen
    if (QScreen *screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    setupUI();
    setupScrollableArea(feedback);
}

void FeedbackDialog::setupUI()
{
    mainLayout = new QVBoxLayout(this);
    
    // Create scroll area
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Create text edit for feedback
    feedbackText = new QTextEdit(this);
    feedbackText->setReadOnly(true);
    feedbackText->setLineWrapMode(QTextEdit::WidgetWidth);
    feedbackText->setStyleSheet("QTextEdit { font-size: 12pt; }");
    
    // Set the text edit as the scroll area widget
    scrollArea->setWidget(feedbackText);
    
    // Create close button
    closeButton = new QPushButton("Close", this);
    closeButton->setStyleSheet("QPushButton { font-size: 12pt; padding: 8px; }");
    
    // Add widgets to layout
    mainLayout->addWidget(scrollArea, 1); // Give scroll area most of the space
    mainLayout->addWidget(closeButton, 0); // Button takes minimal space
    
    // Connect close button
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    
    // Set layout
    setLayout(mainLayout);
}

void FeedbackDialog::setupScrollableArea(const QString &feedback)
{
    feedbackText->setPlainText(feedback);
    
    // Ensure the text is visible at the top
    feedbackText->moveCursor(QTextCursor::Start);
} 