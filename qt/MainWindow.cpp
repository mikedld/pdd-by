#include "MainWindow.h"

#include "ui_MainWindow.h"

#include "SectionDialog.h"
#include "TicketDialog.h"
#include "TicketNumberDialog.h"
#include "TopicDialog.h"

#include "Forest.h"
#include "Leaf/QuestionLeaf.h"
#include "Limb/IQuestionLimb.h"
#include "Shit.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSignalMapper>

class MainWindowImpl : public QObject
{
    Q_OBJECT

    typedef QMap<QAction*, QPair<QPushButton*, char const*> > ActionMap;

public:
    MainWindowImpl(MainWindow& window);

private slots:
    void OnSection(bool isExam = false);
    void OnTopic(bool isExam = false);
    void OnTicket(bool isExam = false);
    void OnRandomTicket(bool isExam = false);
    void OnTopicExam();
    void OnTicketExam();
    void OnRandomTicketExam();

    void ShowTicket(QDialog* intermediateDialog, PddBy::IQuestionCrawler& questions, bool isExam);

    void OpenOak(QString const& pathToOak = QString());
    void ActionChanged(QObject* action);

private:
    void SyncOakState();

private:
    MainWindow& m_window;
    Ui::MainWindow m_ui;
    ActionMap m_actionMap;
    QSignalMapper m_actionSignalMapper;
    PddBy::IOakPtr m_oak;
};

#include "MainWindow.moc"

MainWindowImpl::MainWindowImpl(MainWindow& window) :
    QObject(&window),
    m_window(window)
{
    m_ui.setupUi(&m_window);

    m_window.layout()->setSizeConstraint(QLayout::SetFixedSize);

    m_actionMap.insert(m_ui.sectionTrainingAction, qMakePair(m_ui.sectionTrainingButton, SLOT(OnSection())));
    m_actionMap.insert(m_ui.topicTrainingAction, qMakePair(m_ui.topicTrainingButton, SLOT(OnTopic())));
    m_actionMap.insert(m_ui.ticketTrainingAction, qMakePair(m_ui.ticketTrainingButton, SLOT(OnTicket())));
    m_actionMap.insert(m_ui.randomTicketTrainingAction, qMakePair(m_ui.randomTicketTrainingButton, SLOT(OnRandomTicket())));
    m_actionMap.insert(m_ui.topicExamAction, qMakePair(m_ui.topicExamButton, SLOT(OnTopicExam())));
    m_actionMap.insert(m_ui.ticketExamAction, qMakePair(m_ui.ticketExamButton, SLOT(OnTicketExam())));
    m_actionMap.insert(m_ui.randomTicketExamAction, qMakePair(m_ui.randomTicketExamButton, SLOT(OnRandomTicketExam())));

    connect(&m_actionSignalMapper, SIGNAL(mapped(QObject*)), this, SLOT(ActionChanged(QObject*)));

    for (ActionMap::const_iterator it = m_actionMap.begin(), end = m_actionMap.end(); it != end; ++it)
    {
        connect(it.value().first, SIGNAL(clicked()), it.key(), SLOT(trigger()));
        connect(it.key(), SIGNAL(triggered()), this, it.value().second);
        connect(it.key(), SIGNAL(changed()), &m_actionSignalMapper, SLOT(map()));
        m_actionSignalMapper.setMapping(it.key(), it.key());
    }

    connect(m_ui.openAction, SIGNAL(triggered()), SLOT(OpenOak()));

    connect(m_ui.aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(m_ui.quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    QStringList const args = qApp->arguments();
    OpenOak(args.size() > 1 ? args[1] : QString());
}

void MainWindowImpl::OnSection(bool isExam)
{
    SectionDialog* sectionDialog = new SectionDialog(m_oak->GetSectionLimb(), m_oak->GetQuestionLimb(), isExam, &m_window);
    sectionDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(sectionDialog, SIGNAL(TicketPrepared(QDialog*, PddBy::IQuestionCrawler&, bool)),
        SLOT(ShowTicket(QDialog*, PddBy::IQuestionCrawler&, bool)));

    sectionDialog->open();
}

void MainWindowImpl::OnTopic(bool isExam)
{
    TopicDialog* topicDialog = new TopicDialog(m_oak->GetTopicLimb(), m_oak->GetQuestionLimb(), isExam, &m_window);
    topicDialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(topicDialog, SIGNAL(TicketPrepared(QDialog*, PddBy::IQuestionCrawler&, bool)),
        SLOT(ShowTicket(QDialog*, PddBy::IQuestionCrawler&, bool)));

    topicDialog->open();
}

void MainWindowImpl::OnTicket(bool isExam)
{
    TicketNumberDialog ticketNumberDialog(&m_window);
    ticketNumberDialog.setWindowModality(Qt::WindowModal);
    if (ticketNumberDialog.exec() != QDialog::Accepted)
    {
        return;
    }

    PddBy::IQuestionCrawlerPtr questions = m_oak->GetQuestionLimb().ListQuestionsByTicket(ticketNumberDialog.GetTicketNumber());
    ShowTicket(0, *questions, isExam);
}

void MainWindowImpl::OnRandomTicket(bool isExam)
{
    PddBy::IQuestionCrawlerPtr questions = m_oak->GetQuestionLimb().ListQuestionsByRandomTicket();
    ShowTicket(0, *questions, isExam);
}

void MainWindowImpl::OnTopicExam()
{
    OnTopic(true);
}

void MainWindowImpl::OnTicketExam()
{
    OnTicket(true);
}

void MainWindowImpl::OnRandomTicketExam()
{
    OnRandomTicket(true);
}

void MainWindowImpl::ShowTicket(QDialog* intermediateDialog, PddBy::IQuestionCrawler& questions, bool isExam)
{
    TicketDialog ticketDialog(m_oak->GetImageLimb(), questions, isExam);

    if (intermediateDialog != 0)
    {
        intermediateDialog->hide();
    }

    m_window.hide();

    ticketDialog.exec();

    m_window.show();

    if (intermediateDialog != 0)
    {
        intermediateDialog->show();
    }
}

void MainWindowImpl::OpenOak(QString const& pathToOak)
{
    QString localPathToOak;
    QFileInfo const info(pathToOak);
    if (info.exists() && info.isDir())
    {
        localPathToOak = info.absoluteFilePath();
    }

    if (localPathToOak.isEmpty())
    {
        QFileDialog fileDialog(0, tr("Select path to PDD32.exe"));
        fileDialog.setFileMode(QFileDialog::ExistingFile);
        fileDialog.setNameFilter("Windows executable files (*.exe)");
        if (fileDialog.exec() == QDialog::Accepted)
        {
            localPathToOak = fileDialog.directory().absolutePath();
        }
    }

    if (!localPathToOak.isEmpty())
    {
        try
        {
            m_oak = PddBy::Forest::CreateOak(localPathToOak.toStdString());
        }
        catch (PddBy::Shit const& e)
        {
            QMessageBox messageBox(QMessageBox::Critical, qApp->applicationName(), tr("Unable to open specified path"));
            messageBox.setInformativeText(tr("Error: %0").arg(QString::fromStdString(e.what())));
            messageBox.exec();
        }
    }
    else if (m_oak.get() != 0)
    {
        return;
    }

    SyncOakState();
}

void MainWindowImpl::ActionChanged(QObject* action)
{
    ActionMap::const_iterator it = m_actionMap.find(static_cast<QAction*>(action));
    Q_ASSERT(it != m_actionMap.end());

    if (it == m_actionMap.end())
    {
        return;
    }

    it.value().first->setEnabled(it.key()->isEnabled());
}

void MainWindowImpl::SyncOakState()
{
    for (ActionMap::const_iterator it = m_actionMap.begin(), end = m_actionMap.end(); it != end; ++it)
    {
        it.key()->setEnabled(m_oak.get() != 0);
    }
}

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) :
    QMainWindow(parent, flags),
    m_impl(new MainWindowImpl(*this))
{
    //
}
