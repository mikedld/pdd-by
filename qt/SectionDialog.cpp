#include "SectionDialog.h"

#include "ui_SectionDialog.h"

#include "Leaf/SectionLeaf.h"
#include "Limb/IQuestionLimb.h"
#include "Limb/ISectionLimb.h"

#include <QItemDelegate>
#include <QPainter>
#include <QStandardItemModel>

namespace
{

struct SectionModelRole
{
    enum Enum
    {
        Id = Qt::UserRole + 1,
        Prefix
    };
};

class SectionItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    SectionItemDelegate(QObject* parent = 0);

    virtual void paint(QPainter* painter, QStyleOptionViewItem const& option, QModelIndex const& index) const;
    virtual QSize sizeHint(QStyleOptionViewItem const& option, QModelIndex const& index) const;

private:
    void GetCellInfo(QString const& prefixText, QString const& titleText, QStyleOptionViewItem const& option,
        QRect& prefixRect, QRect& titleRect, QFont& prefixFont) const;
};

SectionItemDelegate::SectionItemDelegate(QObject* parent) :
    QItemDelegate(parent)
{
    //
}

void SectionItemDelegate::paint(QPainter* painter, QStyleOptionViewItem const& option, QModelIndex const& index) const
{
    drawBackground(painter, option, index);

    QString const prefixText = index.data(SectionModelRole::Prefix).toString();
    QString const titleText = index.data(Qt::DisplayRole).toString();
    QRect prefixRect;
    QRect titleRect;
    QFont prefixFont;
    GetCellInfo(prefixText, titleText, option, prefixRect, titleRect, prefixFont);

    QColor const textColor = (option.state & QStyle::QStyle::State_Selected ? option.palette.highlightedText() :
        option.palette.text()).color();

    painter->save();
    painter->setFont(prefixFont);
    painter->setPen(QColor(textColor.red(), textColor.green(), textColor.blue(), textColor.alpha() / 2));
    painter->drawText(prefixRect, Qt::AlignLeft | Qt::TextWordWrap, prefixText);
    painter->restore();

    painter->save();
    painter->setPen(textColor);
    painter->drawText(titleRect, Qt::AlignLeft | Qt::TextWordWrap, titleText);
    painter->restore();

    drawFocus(painter, option, option.rect);
}

QSize SectionItemDelegate::sizeHint(QStyleOptionViewItem const& option, QModelIndex const& index) const
{
    QString const prefixText = index.data(SectionModelRole::Prefix).toString();
    QString const titleText = index.data(Qt::DisplayRole).toString();
    QRect prefixRect;
    QRect titleRect;
    QFont prefixFont;
    GetCellInfo(prefixText, titleText, option, prefixRect, titleRect, prefixFont);

    return QSize(option.rect.width(), prefixRect.height() + titleRect.height() + 1 +
        (prefixRect.top() - option.rect.top()) * 2);
}

void SectionItemDelegate::GetCellInfo(QString const& prefixText, QString const& titleText, QStyleOptionViewItem const& option,
    QRect& prefixRect, QRect& titleRect, QFont& prefixFont) const
{
    prefixFont = option.font;
    prefixFont.setPointSizeF(prefixFont.pointSizeF() * 0.9);

    QFontMetrics const prefixFontMetrics(prefixFont);

    int const horzMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option);
    int const vertMargin = qApp->style()->pixelMetric(QStyle::PM_FocusFrameVMargin, &option);

    prefixRect = option.rect.adjusted(horzMargin, vertMargin, -horzMargin, 0);
    prefixRect = prefixFontMetrics.boundingRect(prefixRect, Qt::AlignLeft | Qt::TextWordWrap, prefixText);

    titleRect = option.rect.adjusted(horzMargin, vertMargin + prefixRect.height() + 1, -horzMargin, 0);
    titleRect = option.fontMetrics.boundingRect(titleRect, Qt::AlignLeft | Qt::TextWordWrap, titleText);
}

} // namespace

class SectionDialogImpl : public QObject
{
    Q_OBJECT

public:
    SectionDialogImpl(SectionDialog& dialog, PddBy::ISectionLimb const& sectionLimb, PddBy::IQuestionLimb const& questionLimb,
        bool isExam);

private slots:
    void PrepareTicket() const;

private:
    SectionDialog& m_dialog;
    PddBy::IQuestionLimb const& m_questionLimb;
    bool const m_isExam;
    Ui::SectionDialog m_ui;
    SectionItemDelegate m_itemDelegate;
    QStandardItemModel m_model;
};

#include "SectionDialog.moc"

SectionDialogImpl::SectionDialogImpl(SectionDialog& dialog, PddBy::ISectionLimb const& sectionLimb,
    PddBy::IQuestionLimb const& questionLimb, bool isExam) :
    QObject(&dialog),
    m_dialog(dialog),
    m_questionLimb(questionLimb),
    m_isExam(isExam)
{
    m_ui.setupUi(&m_dialog);

    PddBy::ISectionCrawlerPtr sections = sectionLimb.ListSections();
    PddBy::SectionLeaf sectionLeaf;
    while (sections->GetNext(sectionLeaf))
    {
        QStandardItem* item = new QStandardItem(QString::fromStdString(sectionLeaf.Title));

        item->setData(QString::fromStdString(sectionLeaf.Id), SectionModelRole::Id);
        item->setData(QString::fromStdString(sectionLeaf.Prefix), SectionModelRole::Prefix);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        m_model.appendRow(item);
    }

    m_ui.sectionsView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_ui.sectionsView->setItemDelegate(&m_itemDelegate);
    m_ui.sectionsView->setModel(&m_model);
    m_ui.sectionsView->setCurrentIndex(m_model.index(0, 0));

    connect(m_ui.sectionsView, SIGNAL(doubleClicked(QModelIndex)), SLOT(PrepareTicket()));
    connect(m_ui.dialogButtons, SIGNAL(accepted()), SLOT(PrepareTicket()));
}

void SectionDialogImpl::PrepareTicket() const
{
    QModelIndex const sectionIndex = m_ui.sectionsView->currentIndex();
    Q_ASSERT(sectionIndex.isValid());

    if (!sectionIndex.isValid())
    {
        return;
    }

    std::string const sectionId = sectionIndex.data(SectionModelRole::Id).toString().toStdString();
    PddBy::IQuestionCrawlerPtr questions = m_questionLimb.ListQuestionsBySection(sectionId);
    emit m_dialog.TicketPrepared(&m_dialog, *questions, m_isExam);
}

SectionDialog::SectionDialog(PddBy::ISectionLimb const& sectionLimb, PddBy::IQuestionLimb const& questionLimb, bool isExam,
    QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_impl(new SectionDialogImpl(*this, sectionLimb, questionLimb, isExam))
{
    //
}
