#include "ApiKeyDialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

ApiKeyDialog::ApiKeyDialog(QWidget *parent)
    : QDialog(parent)
    , input(nullptr)
    , apiKey("")
{
    setWindowTitle("OpenAI API key missing");
    setModal(true);
    setFixedSize(400, 150);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Create label with clickable link
    QLabel *label = new QLabel("Provide a new OpenAI API key below:");
    layout->addWidget(label);
    
    QLabel *linkLabel = new QLabel("<a href=\"https://platform.openai.com/api-keys\">Generate one at https://platform.openai.com/api-keys</a>");
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setTextFormat(Qt::RichText);
    layout->addWidget(linkLabel);
    
    // Create input field
    input = new QLineEdit();
    input->setEchoMode(QLineEdit::Password);
    input->setPlaceholderText("Enter your OpenAI API key");
    layout->addWidget(input);
    
    // Create buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton("OK");
    QPushButton *quitButton = new QPushButton("Quit");
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(quitButton);
    layout->addLayout(buttonLayout);
    
    // Initially disable OK button since input is empty
    okButton->setEnabled(false);
    
    // Connect signals
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(input, &QLineEdit::returnPressed, this, &QDialog::accept);
    connect(input, &QLineEdit::textChanged, [okButton](const QString &text) {
        okButton->setEnabled(!text.trimmed().isEmpty());
    });
    
    // Set focus to input field
    input->setFocus();
}

QString ApiKeyDialog::getApiKey() const
{
    return input ? input->text().trimmed() : "";
} 