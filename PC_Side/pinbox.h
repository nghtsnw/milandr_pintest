#ifndef PINBOX_H
#define PINBOX_H

#include <QGroupBox>

namespace Ui {
class PinBox;
}

class PinBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit PinBox(QWidget *parent = nullptr);
    ~PinBox();

private:
    Ui::PinBox *ui;
};

#endif // PINBOX_H
