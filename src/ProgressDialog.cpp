#include "ProgressDialog.h"
#include "ui_progressdialog.h"

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProgressDialog)
{
    ui->setupUi(this);
    setModal(true);
    setWindowTitle("Processing...");
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}
