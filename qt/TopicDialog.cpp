#include "TopicDialog.h"

#include "ui_TopicDialog.h"

#include "TicketNumberDialog.h"

#include "Leaf/TopicLeaf.h"
#include "Limb/IQuestionLimb.h"
#include "Limb/ITopicLimb.h"

#include <QStandardItemModel>

namespace
{

struct TopicModelRole
{
    enum Enum
    {
        Id = Qt::UserRole + 1,
        Prefix
    };
};

} // namespace

class TopicDialogImpl : public QObject
{
    Q_OBJECT

public:
    TopicDialogImpl(TopicDialog& dialog, PddBy::ITopicLimb const& topicLimb, PddBy::IQuestionLimb const& questionLimb,
        bool isExam);

private slots:
    void PrepareTicket() const;

private:
    TopicDialog& m_dialog;
    PddBy::IQuestionLimb const& m_questionLimb;
    bool const m_isExam;
    Ui::TopicDialog m_ui;
    QStandardItemModel m_model;
};

#include "TopicDialog.moc"

TopicDialogImpl::TopicDialogImpl(TopicDialog& dialog, PddBy::ITopicLimb const& topicLimb,
    PddBy::IQuestionLimb const& questionLimb, bool isExam) :
    QObject(&dialog),
    m_dialog(dialog),
    m_questionLimb(questionLimb),
    m_isExam(isExam)
{
    m_ui.setupUi(&m_dialog);

    PddBy::ITopicCrawlerPtr topics = topicLimb.ListTopics();
    PddBy::TopicLeaf topicLeaf;
    while (topics->GetNext(topicLeaf))
    {
        QStandardItem* item = new QStandardItem(QString::fromStdString(topicLeaf.Title));

        item->setData(QString::fromStdString(topicLeaf.Id), TopicModelRole::Id);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_model.appendRow(item);
    }

    m_ui.topicsView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.topicsView->setModel(&m_model);
    m_ui.topicsView->setCurrentIndex(m_model.index(0, 0));

    connect(m_ui.topicsView, SIGNAL(doubleClicked(QModelIndex)), SLOT(PrepareTicket()));
    connect(m_ui.dialogButtons, SIGNAL(accepted()), SLOT(PrepareTicket()));
}

void TopicDialogImpl::PrepareTicket() const
{
    QModelIndex const topicIndex = m_ui.topicsView->currentIndex();
    Q_ASSERT(topicIndex.isValid());

    if (!topicIndex.isValid())
    {
        return;
    }

    TicketNumberDialog ticketNumberDialog(&m_dialog);
    ticketNumberDialog.setWindowModality(Qt::WindowModal);
    if (ticketNumberDialog.exec() != QDialog::Accepted)
    {
        return;
    }

    std::string const topicId = topicIndex.data(TopicModelRole::Id).toString().toStdString();
    PddBy::IQuestionCrawlerPtr questions = m_questionLimb.ListQuestionsByTopic(topicId, ticketNumberDialog.GetTicketNumber());
    emit m_dialog.TicketPrepared(&m_dialog, *questions, m_isExam);
}

TopicDialog::TopicDialog(PddBy::ITopicLimb const& topicLimb, PddBy::IQuestionLimb const& questionLimb, bool isExam,
    QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_impl(new TopicDialogImpl(*this, topicLimb, questionLimb, isExam))
{
    //
}
