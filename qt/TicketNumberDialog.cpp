#include "TicketNumberDialog.h"

#include "ui_TicketNumberDialog.h"

class TicketNumberDialogImpl : public QObject
{
    Q_OBJECT

public:
    TicketNumberDialogImpl(TicketNumberDialog& dialog);

    int GetTicketNumber() const;

private:
    TicketNumberDialog& m_dialog;
    Ui::TicketNumberDialog m_ui;
};

#include "TicketNumberDialog.moc"

TicketNumberDialogImpl::TicketNumberDialogImpl(TicketNumberDialog& dialog) :
    QObject(&dialog),
    m_dialog(dialog)
{
    m_ui.setupUi(&m_dialog);
}

int TicketNumberDialogImpl::GetTicketNumber() const
{
    return m_ui.ticketNumberEdit->value();
}

TicketNumberDialog::TicketNumberDialog(QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_impl(new TicketNumberDialogImpl(*this))
{
    //
}

int TicketNumberDialog::GetTicketNumber() const
{
    return m_impl->GetTicketNumber();
}
