#include "TicketDialog.h"

#include "ui_TicketDialog.h"

#include "Leaf/ImageLeaf.h"
#include "Leaf/QuestionLeaf.h"
#include "Limb/IImageLimb.h"
#include "Limb/IQuestionLimb.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QTime>
#include <QTimer>

namespace
{

template<typename T>
bool Contains(T const& container, typename T::value_type const& value)
{
    return std::find(container.begin(), container.end(), value) != container.end();
}

} // namespace

class TicketDialogImpl : public QObject
{
    Q_OBJECT

    struct QuestionInfo
    {
        PddBy::QuestionLeaf Question;
        QPixmap Pixmap;
        std::vector<std::size_t> AnswerIndices;
    };

    struct Statistics
    {
        std::size_t CorrectCount;
        std::size_t IncorrectCount;
        std::size_t NoAnswerCount;

        Statistics() :
            CorrectCount(0),
            IncorrectCount(0),
            NoAnswerCount(0)
        {
            //
        }
    };

public:
    TicketDialogImpl(TicketDialog& dialog, PddBy::IImageLimb const& imageLimb, PddBy::IQuestionCrawler& questions, bool isExam);

    virtual bool eventFilter(QObject* object, QEvent* event);

private slots:
    void SkipQuestion();
    void SetActive(bool isActive = true);
    void UpdateExamTimeLabel();

private:
    void FinishTest();
    bool IsActive() const;
    void SelectAnswer(std::size_t index);
    void ShowRegulations();
    void ShowComments();

    void SetupCurrentQuestion();
    void MarkAnswer(std::size_t index, bool isUsed, bool isCorrect);

    Statistics CalculateStatistics() const;

private:
    TicketDialog& m_dialog;
    bool const m_isExam;
    Ui::TicketDialog m_ui;
    std::vector<QuestionInfo> m_questions;
    std::size_t m_currentIndex;
    bool m_isSkipInProgress;
    QTime m_examTime;
    QTimer m_examTimer;
};

#include "TicketDialog.moc"

TicketDialogImpl::TicketDialogImpl(TicketDialog& dialog, PddBy::IImageLimb const& imageLimb, PddBy::IQuestionCrawler& questions,
    bool isExam) :
    QObject(&dialog),
    m_dialog(dialog),
    m_isExam(isExam),
    m_currentIndex(0),
    m_isSkipInProgress(false)
{
    m_ui.setupUi(&m_dialog);

    QuestionInfo questionInfo;
    while (questions.GetNext(questionInfo.Question))
    {
        if (!questionInfo.Question.ImageId.empty())
        {
            PddBy::ImageLeaf const imageLeaf = imageLimb.GetImage(questionInfo.Question.ImageId);
            questionInfo.Pixmap = QPixmap::fromImage(QImage::fromData(&imageLeaf.Data[0], imageLeaf.Data.size()));
        }
        else
        {
            questionInfo.Pixmap = QPixmap();
        }

        m_questions.push_back(questionInfo);
    }

    m_ui.hotkeysLabel->setAttribute(Qt::WA_MacSmallSize);

    m_dialog.installEventFilter(this);

    SetupCurrentQuestion();

    connect(m_ui.continueButton, SIGNAL(clicked()), SLOT(SetActive()));

    if (m_isExam)
    {
        m_examTime.setHMS(0, 15, 1);
        UpdateExamTimeLabel();

        connect(&m_examTimer, SIGNAL(timeout()), SLOT(UpdateExamTimeLabel()));
        m_examTimer.start(1000);
    }
}

bool TicketDialogImpl::eventFilter(QObject* /*object*/, QEvent* event)
{
    if (!m_dialog.isVisible() || !IsActive())
    {
        return false;
    }

    if (!m_isSkipInProgress && event->type() == QEvent::KeyPress)
    {
        QKeyEvent* realEvent = static_cast<QKeyEvent*>(event);
        switch (realEvent->key())
        {
        case Qt::Key_1:
        case Qt::Key_2:
        case Qt::Key_3:
        case Qt::Key_4:
        case Qt::Key_5:
        case Qt::Key_6:
        case Qt::Key_7:
        case Qt::Key_8:
        case Qt::Key_9:
            SelectAnswer(realEvent->key() - Qt::Key_1);
            return true;

        case Qt::Key_Space:
            SkipQuestion();
            return true;

        case Qt::Key_F1:
            ShowRegulations();
            return true;

        case Qt::Key_F2:
            ShowComments();
            return true;
        }
    }
    else if (event->type() == QEvent::Resize)
    {
        QResizeEvent* realEvent = static_cast<QResizeEvent*>(event);
        m_dialog.move(m_dialog.x() + (realEvent->oldSize().width() - m_dialog.size().width()) / 2, m_dialog.y());
    }
    else if (m_isExam && event->type() == QEvent::WindowDeactivate)
    {
        SetActive(false);
    }

    return false;
}

void TicketDialogImpl::SkipQuestion()
{
    for (std::size_t i = (m_currentIndex + 1) % m_questions.size(); i != m_currentIndex; i = (i + 1) % m_questions.size())
    {
        QuestionInfo const& questionInfo = m_questions[i];
        if ((m_isExam && !questionInfo.AnswerIndices.empty()) ||
            questionInfo.AnswerIndices.size() == 3 ||
            Contains(questionInfo.AnswerIndices, questionInfo.Question.CorrectAnswerIndex))
        {
            continue;
        }

        m_currentIndex = i;
        SetupCurrentQuestion();
        return;
    }

    if (m_questions[m_currentIndex].AnswerIndices.empty())
    {
        // Trying to skip last unanswered question, no way
        return;
    }

    FinishTest();
}

void TicketDialogImpl::SetActive(bool isActive)
{
    if (!m_isExam)
    {
        return;
    }

    if (isActive)
    {
        m_ui.pages->setCurrentIndex(0);
        m_examTimer.start(1000);
    }
    else
    {
        m_ui.pages->setCurrentIndex(1);
        m_examTimer.stop();
    }
}

void TicketDialogImpl::UpdateExamTimeLabel()
{
    m_examTime = m_examTime.addSecs(-1);

    int const secondsLeft = QTime(0, 0, 0).secsTo(m_examTime);
    if (secondsLeft == 0)
    {
        FinishTest();
        return;
    }

    if (secondsLeft % 5 == 0 || secondsLeft < 15)
    {
        m_ui.examTimeLabel->setText(tr("Left: %0").arg(m_examTime.toString("mm:ss")));
    }
}

void TicketDialogImpl::FinishTest()
{
    m_dialog.hide();

    Statistics const statistics = CalculateStatistics();

    QMessageBox messageBox(QMessageBox::Information, m_dialog.windowTitle(), statistics.CorrectCount == m_questions.size() ?
        tr("Test completed") : tr("Test failed"), QMessageBox::Close);
    messageBox.setInformativeText(tr("Correct: %0\nIncorrect: %1\nNo answer: %2").arg(statistics.CorrectCount).
        arg(statistics.IncorrectCount).arg(statistics.NoAnswerCount));
    messageBox.exec();

    m_dialog.accept();
}

bool TicketDialogImpl::IsActive() const
{
    return m_ui.pages->currentIndex() == 0;
}

void TicketDialogImpl::SelectAnswer(std::size_t index)
{
    QuestionInfo& questionInfo = m_questions[m_currentIndex];
    if (index >= questionInfo.Question.Answers.size() || Contains(questionInfo.AnswerIndices, index))
    {
        return;
    }

    questionInfo.AnswerIndices.push_back(index);

    bool const isCorrect = index == questionInfo.Question.CorrectAnswerIndex;
    MarkAnswer(index, true, isCorrect);

    if (!isCorrect && questionInfo.AnswerIndices.size() == 3)
    {
        QMessageBox messageBox(QMessageBox::Critical, tr("Incorrect answer"), tr("Correct answer: %0").
            arg(questionInfo.Question.CorrectAnswerIndex + 1));
        if (!questionInfo.Question.Advice.empty())
        {
            messageBox.setInformativeText(QString::fromStdString(questionInfo.Question.Advice));
        }

        messageBox.exec();
    }

    if (isCorrect || m_isExam || questionInfo.AnswerIndices.size() == 3)
    {
        m_isSkipInProgress = true;
        QTimer::singleShot(250, this, SLOT(SkipQuestion()));
        return;
    }
}

void TicketDialogImpl::ShowRegulations()
{
    if (m_isExam)
    {
        return;
    }

    //
}

void TicketDialogImpl::ShowComments()
{
    if (m_isExam)
    {
        return;
    }

    //
}

void TicketDialogImpl::SetupCurrentQuestion()
{
    QuestionInfo const& questionInfo = m_questions[m_currentIndex];

    m_ui.imageLabel->setVisible(!questionInfo.Pixmap.isNull());
    if (!questionInfo.Pixmap.isNull())
    {
        m_ui.imageLabel->setPixmap(questionInfo.Pixmap);
        m_ui.imageLabel->setFixedHeight(questionInfo.Pixmap.height() * m_ui.imageLabel->width() / questionInfo.Pixmap.width());
    }

    m_ui.questionLabel->setText(QString::fromStdString(questionInfo.Question.Text));

    for (std::size_t i = 0, answerCount = questionInfo.Question.Answers.size(), rowCount = m_ui.answersLayout->rowCount() - 1;
        i < rowCount; i++)
    {
        QLabel* numberLabel = static_cast<QLabel*>(m_ui.answersLayout->itemAtPosition(i, 0)->widget());
        QLabel* textLabel = static_cast<QLabel*>(m_ui.answersLayout->itemAtPosition(i, 1)->widget());

        numberLabel->setVisible(i < answerCount);
        textLabel->setVisible(i < answerCount);
        if (i >= answerCount)
        {
            continue;
        }

        textLabel->setText(QString::fromStdString(questionInfo.Question.Answers[i]));

        bool const isUsed = Contains(questionInfo.AnswerIndices, i);
        bool const isCorrect = i == questionInfo.Question.CorrectAnswerIndex;
        MarkAnswer(i, isUsed, isCorrect);
    }

    if (m_isExam)
    {
        m_ui.hotkeysLabel->setText(tr("<b>1&ndash;%0</b> &mdash; answer, <b>Space</b> &mdash; skip").
            arg(questionInfo.Question.Answers.size()));
    }
    else
    {
        m_ui.hotkeysLabel->setText(tr("<b>1&ndash;%0</b> &mdash; answer, <b>Space</b> &mdash; skip, "
            "<b>F1</b> &mdash; regulations, <b>F2</b> &mdash; comments").arg(questionInfo.Question.Answers.size()));
    }

    Statistics const statistics = CalculateStatistics();
    if (statistics.CorrectCount != 0 && statistics.IncorrectCount != 0)
    {
        m_ui.progressLabel->setText(tr("%0 of %1 (%2, %3)").arg(m_currentIndex + 1).arg(m_questions.size()).
            arg(tr("%0 correct", 0, statistics.CorrectCount).arg(statistics.CorrectCount)).
            arg(tr("%0 incorrect", 0, statistics.IncorrectCount).arg(statistics.IncorrectCount)));
    }
    else if (statistics.CorrectCount != 0 && statistics.IncorrectCount == 0)
    {
        m_ui.progressLabel->setText(tr("%0 of %1 (%2)").arg(m_currentIndex + 1).arg(m_questions.size()).
            arg(tr("%0 correct", 0, statistics.CorrectCount).arg(statistics.CorrectCount)));
    }
    else if (statistics.CorrectCount == 0 && statistics.IncorrectCount != 0)
    {
        m_ui.progressLabel->setText(tr("%0 of %1 (%2)").arg(m_currentIndex + 1).arg(m_questions.size()).
            arg(tr("%0 incorrect", 0, statistics.IncorrectCount).arg(statistics.IncorrectCount)));
    }
    else
    {
        m_ui.progressLabel->setText(tr("%0 of %1").arg(m_currentIndex + 1).arg(m_questions.size()));
    }

    m_dialog.setWindowTitle(tr("%0 / %1").arg(m_currentIndex + 1).arg(m_questions.size()));
    m_dialog.adjustSize();

    m_isSkipInProgress = false;
}

void TicketDialogImpl::MarkAnswer(std::size_t index, bool isUsed, bool isCorrect)
{
    QLabel* numberLabel = static_cast<QLabel*>(m_ui.answersLayout->itemAtPosition(index, 0)->widget());
    QLabel* textLabel = static_cast<QLabel*>(m_ui.answersLayout->itemAtPosition(index, 1)->widget());

    if (isUsed)
    {
        numberLabel->setPixmap(QPixmap(isCorrect ? ":/Images/good.png" : ":/Images/bad.png"));
    }
    else
    {
        numberLabel->setText(tr("%0.").arg(index + 1));
    }

    textLabel->setEnabled(!isUsed || isCorrect);
}

TicketDialogImpl::Statistics TicketDialogImpl::CalculateStatistics() const
{
    Statistics result;
    for (std::size_t i = 0, count = m_questions.size(); i < count; i++)
    {
        QuestionInfo const& questionInfo = m_questions[i];
        if (Contains(questionInfo.AnswerIndices, questionInfo.Question.CorrectAnswerIndex))
        {
            ++result.CorrectCount;
        }
        else if (!questionInfo.AnswerIndices.empty())
        {
            ++result.IncorrectCount;
        }
        else
        {
            ++result.NoAnswerCount;
        }
    }

    return result;
}

TicketDialog::TicketDialog(PddBy::IImageLimb const& imageLimb, PddBy::IQuestionCrawler& questions, bool isExam, QWidget* parent,
    Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_impl(new TicketDialogImpl(*this, imageLimb, questions, isExam))
{
    //
}
