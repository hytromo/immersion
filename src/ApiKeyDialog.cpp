#include "ApiKeyDialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

// Constants
namespace {
    const QString DIALOG_TITLE = "OpenAI API key missing";
    const QString LABEL_TEXT = "Provide a new OpenAI API key below:";
    const QString LINK_TEXT = "<a href=\"https://platform.openai.com/api-keys\">Generate one at https://platform.openai.com/api-keys</a>";
    const QString PLACEHOLDER_TEXT = "Enter your OpenAI API key";
    const QString OK_BUTTON_TEXT = "OK";
    const QString QUIT_BUTTON_TEXT = "Quit";
    const int DIALOG_WIDTH = 400;
    const int DIALOG_HEIGHT = 150;
}

ApiKeyDialog::ApiKeyDialog(QWidget *parent)
    : QDialog(parent)
    , input(nullptr)
    , apiKey("")
{
    setupDialog();
    setupLayout();
    setupConnections();
    input->setFocus();
}

void ApiKeyDialog::setupDialog()
{
    setWindowTitle(DIALOG_TITLE);
    setModal(true);
    setFixedSize(DIALOG_WIDTH, DIALOG_HEIGHT);
}

void ApiKeyDialog::setupLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // Create label with clickable link
    QLabel *label = new QLabel(LABEL_TEXT);
    layout->addWidget(label);
    
    QLabel *linkLabel = new QLabel(LINK_TEXT);
    linkLabel->setOpenExternalLinks(true);
    linkLabel->setTextFormat(Qt::RichText);
    layout->addWidget(linkLabel);
    
    // Create input field
    input = new QLineEdit();
    input->setEchoMode(QLineEdit::Password);
    input->setPlaceholderText(PLACEHOLDER_TEXT);
    layout->addWidget(input);
    
    // Create buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *okButton = new QPushButton(OK_BUTTON_TEXT);
    QPushButton *quitButton = new QPushButton(QUIT_BUTTON_TEXT);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(quitButton);
    layout->addLayout(buttonLayout);
    
    // Initially disable OK button since input is empty
    okButton->setEnabled(false);
}

void ApiKeyDialog::setupConnections()
{
    QPushButton *okButton = findChild<QPushButton*>();
    if (!okButton) {
        return;
    }
    
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    
    QPushButton *quitButton = findChildren<QPushButton*>()[1];
    if (quitButton) {
        connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    }
    
    connect(input, &QLineEdit::returnPressed, this, &QDialog::accept);
    connect(input, &QLineEdit::textChanged, [okButton](const QString &text) {
        okButton->setEnabled(!text.trimmed().isEmpty());
    });
}

QString ApiKeyDialog::getApiKey() const
{
    return input ? input->text().trimmed() : "";
} 