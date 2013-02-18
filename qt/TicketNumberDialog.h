#pragma once

#include <QDialog>

class TicketNumberDialogImpl;

class TicketNumberDialog : public QDialog
{
    Q_OBJECT

public:
    TicketNumberDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);

    int GetTicketNumber() const;

private:
    TicketNumberDialogImpl* m_impl;
};
