#pragma once

#include "ICrawler.h"

#include <QDialog>

namespace PddBy
{

struct QuestionLeaf;
typedef ICrawler<QuestionLeaf> IQuestionCrawler;

class IQuestionLimb;
class ITopicLimb;

} // namespace PddBy

class TopicDialogImpl;

class TopicDialog : public QDialog
{
    Q_OBJECT

    friend class TopicDialogImpl;

public:
    TopicDialog(PddBy::ITopicLimb const& topicLimb, PddBy::IQuestionLimb const& questionLimb, bool isExam, QWidget* parent = 0,
        Qt::WindowFlags flags = 0);

signals:
    void TicketPrepared(QDialog* intermediateDialog, PddBy::IQuestionCrawler& questions, bool isExam);

private:
    TopicDialogImpl* m_impl;
};
