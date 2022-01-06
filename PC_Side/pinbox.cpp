#include "pinbox.h"
#include "ui_pinbox.h"

PinBox::PinBox(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::PinBox)
{
    ui->setupUi(this);
}

PinBox::~PinBox()
{
    delete ui;
}
