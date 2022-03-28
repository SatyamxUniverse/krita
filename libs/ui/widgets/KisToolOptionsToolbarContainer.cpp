/*
 *  SPDX-FileCopyrightText: 2022 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QPointer>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QPainter>

#include <klocalizedstring.h>
#include <KisOptionCollectionWidget.h>
#include <kis_painting_tweaks.h>

#include "KisToolOptionsToolbarContainer.h"

struct FadeableWidget : public QWidget
{
    static constexpr int showAnimationDuration{250};
    static constexpr int hideAnimationDuration{1000};

    FadeableWidget(QWidget *parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_fadeAnimation.setEasingCurve(QEasingCurve::OutCubic);
        m_fadeAnimation.setStartValue(1.0);
        m_fadeAnimation.setEndValue(0.0);
        m_fadeAnimation.setDuration(1);
        m_fadeAnimation.start();
        connect(&m_fadeAnimation, SIGNAL(valueChanged(const QVariant&)), SLOT(update()));
    }

    ~FadeableWidget() override {}

    void fadeIn()
    {
        if (m_fadeAnimation.endValue().toReal() > m_fadeAnimation.startValue().toReal()) {
            // Already showing
            return;
        }
        const qreal currentValue = m_fadeAnimation.currentValue().toReal();
        m_fadeAnimation.stop();
        m_fadeAnimation.setStartValue(m_fadeAnimation.currentValue());
        m_fadeAnimation.setEndValue(1.0);
        m_fadeAnimation.setDuration(static_cast<int>(qRound(static_cast<qreal>(showAnimationDuration) * (1.0 - currentValue))));
        m_fadeAnimation.start();
    }

    void fadeOut()
    {
        if (m_fadeAnimation.endValue().toReal() < m_fadeAnimation.startValue().toReal()) {
            // Already hiding
            return;
        }
        const qreal currentValue = m_fadeAnimation.currentValue().toReal();
        m_fadeAnimation.stop();
        m_fadeAnimation.setStartValue(m_fadeAnimation.currentValue());
        m_fadeAnimation.setEndValue(0.0);
        m_fadeAnimation.setDuration(static_cast<int>(qRound(static_cast<qreal>(hideAnimationDuration) * currentValue)));
        m_fadeAnimation.start();
    }

protected:
    QVariantAnimation m_fadeAnimation;
};

struct FadeWidget : public FadeableWidget
{
    static constexpr int fadeWidgetWidth{64};

    enum Direction
    {
        LeftToRight,
        RightToLeft
    };

    FadeWidget(Direction direction, QWidget *parent)
        : FadeableWidget(parent)
        , m_direction(direction)
    {}

    ~FadeWidget() override {}

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        QLinearGradient grad(
            m_direction == LeftToRight ? 0.0 : static_cast<qreal>(fadeWidgetWidth), 0.0,
            m_direction == LeftToRight ? static_cast<qreal>(fadeWidgetWidth) : 0.0, 0.0
        );
        // Kind of quadratic alpha gradient
        QColor c = palette().window().color();
        c.setAlphaF(m_fadeAnimation.currentValue().toReal() * 1.0);
        grad.setColorAt(0.0, c);
        c.setAlphaF(m_fadeAnimation.currentValue().toReal() * 0.75 * 0.75);
        grad.setColorAt(0.25, c);
        c.setAlphaF(m_fadeAnimation.currentValue().toReal() * 0.5 * 0.5);
        grad.setColorAt(0.5, c);
        c.setAlphaF(m_fadeAnimation.currentValue().toReal() * 0.25 * 0.25);
        grad.setColorAt(0.75, c);
        c.setAlphaF(0.0);
        grad.setColorAt(1.0, c);
        p.fillRect(rect(), grad);
    }

private:
    Direction m_direction;
};

struct Scrollbar : public FadeableWidget
{
    static constexpr int scrollbarHeight{2};

    Scrollbar(QWidget *parent)
        : FadeableWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        m_fadeAnimation.setEasingCurve(QEasingCurve::OutCubic);
        m_fadeAnimation.setStartValue(1.0);
        m_fadeAnimation.setEndValue(0.0);
        m_fadeAnimation.setDuration(1);
        m_fadeAnimation.start();
        connect(&m_fadeAnimation, SIGNAL(valueChanged(const QVariant&)), SLOT(update()));
    }

    ~Scrollbar() override {}

    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        const int areaShownPx = qMin(width(), static_cast<int>(qRound(static_cast<qreal>(width()) * m_areaShown)));
        const int offsetPx = static_cast<int>(qRound(static_cast<qreal>(width()) * m_offset));
        const QColor c1 = palette().window().color();
        const QColor c2 = palette().text().color();
        QColor c = KisPaintingTweaks::blendColors(c1, c2, 0.85);
        c.setAlphaF(m_fadeAnimation.currentValue().toReal());
        p.fillRect(QRect(offsetPx, 0, areaShownPx, scrollbarHeight), c);
    }

    void setAreaShown(qreal areaShown) {
        m_areaShown = areaShown;
        update();
    }

    void setOffset(qreal offset) {
        m_offset = offset;
        update();
    }

private:
    qreal m_areaShown{100.0};
    qreal m_offset{0.0};
};

class KisToolOptionsToolbarContainer::Private
{
public:
    static constexpr int scrollbarHeight{2};
    static constexpr int fadeWidgetWidth{64};
    static constexpr int showAnimationDuration{250};
    static constexpr int hideAnimationDuration{1000};

    KisToolOptionsToolbarContainer *q{nullptr};
    QWidget *hiderWidget{nullptr};
    QList<QPointer<QWidget>> currentWidgetList;
    QLabel *noOptionsLabel{nullptr};
    QWidget *currentWidget{nullptr};

    bool isDragging{false};
    QPoint lastMousePos;
    bool isHovering{false};
    FadeWidget *leftFadeWidget{nullptr};
    FadeWidget *rightFadeWidget{nullptr};
    Scrollbar *scrollbarWidget{nullptr};

    Private(KisToolOptionsToolbarContainer *q) : q(q) {}
};

KisToolOptionsToolbarContainer::KisToolOptionsToolbarContainer(QWidget *parent)
    : QWidget(parent)
    , m_d(new Private(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_d->hiderWidget = new QWidget();
    m_d->hiderWidget->setVisible(false);
    m_d->noOptionsLabel = new QLabel(i18n("No options available"), m_d->hiderWidget);
    m_d->noOptionsLabel->setAlignment(Qt::AlignVCenter);
    m_d->noOptionsLabel->setMinimumHeight(32);
    
    m_d->leftFadeWidget = new FadeWidget(FadeWidget::LeftToRight, this);
    m_d->leftFadeWidget->setFixedWidth(m_d->fadeWidgetWidth);
    m_d->rightFadeWidget = new FadeWidget(FadeWidget::RightToLeft, this);
    m_d->rightFadeWidget->setFixedWidth(m_d->fadeWidgetWidth);
    m_d->scrollbarWidget = new Scrollbar(this);
    m_d->scrollbarWidget->setFixedHeight(m_d->scrollbarHeight);
}

KisToolOptionsToolbarContainer::~KisToolOptionsToolbarContainer()
{}

void KisToolOptionsToolbarContainer::setOptionWidgets(const QList<QPointer<QWidget>> &optionWidgetList)
{
    if (m_d->currentWidget) {
        m_d->currentWidget->removeEventFilter(this);
        m_d->currentWidget->setParent(m_d->hiderWidget);
        m_d->currentWidget = nullptr;
    }
    m_d->currentWidgetList = optionWidgetList;

    for (QPointer<QWidget> w : optionWidgetList) {
        if (qobject_cast<KisOptionCollectionWidget*>(w.data()) && !m_d->currentWidget) {
            KisOptionCollectionWidget *currentWidget = qobject_cast<KisOptionCollectionWidget*>(w.data());
            currentWidget->setOrientation(Qt::Horizontal, true);
            currentWidget->setParent(this);
            m_d->currentWidget = currentWidget;
        } else {
            w->setParent(m_d->hiderWidget);
        }
    }

    if (!m_d->currentWidget) {
        m_d->noOptionsLabel->setParent(this);
        m_d->currentWidget = m_d->noOptionsLabel;
    }

    m_d->currentWidget->setContentsMargins(10, 0, 10, 0);
    m_d->currentWidget->installEventFilter(this);
    m_d->currentWidget->lower();
    m_d->currentWidget->show();
}

QSize KisToolOptionsToolbarContainer::sizeHint() const
{
    if (m_d->currentWidget) {
        return m_d->currentWidget->sizeHint() + QSize(0, m_d->scrollbarHeight + 1);
    }
    return QSize();
}

bool KisToolOptionsToolbarContainer::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_d->currentWidget) {
        if (e->type() == QEvent::LayoutRequest) {
            m_d->currentWidget->resize(m_d->currentWidget->sizeHint());
        } else if (e->type() == QEvent::Resize) {
            if (m_d->currentWidget->width() > width()) {
                if (m_d->currentWidget->width() + m_d->currentWidget->x() <= width()) {
                    m_d->currentWidget->move(-(m_d->currentWidget->width() - width()), 0);
                    m_d->rightFadeWidget->fadeOut();
                } else {
                    m_d->rightFadeWidget->fadeIn();
                }
                if (m_d->isHovering) {
                    m_d->scrollbarWidget->fadeIn();
                }
                m_d->scrollbarWidget->setAreaShown(static_cast<qreal>(width()) / static_cast<qreal>(m_d->currentWidget->width()));
                m_d->scrollbarWidget->setOffset(static_cast<qreal>(-m_d->currentWidget->x()) / static_cast<qreal>(m_d->currentWidget->width()));
            } else {
                m_d->currentWidget->move(0, 0);
                m_d->leftFadeWidget->fadeOut();
                m_d->rightFadeWidget->fadeOut();
                m_d->scrollbarWidget->fadeOut();
                m_d->scrollbarWidget->setAreaShown(100.0);
                m_d->scrollbarWidget->setOffset(0.0);
            }
            setFixedHeight(m_d->currentWidget->sizeHint().height() + m_d->scrollbarHeight + 1);
        }
    }

    return false;
}

void KisToolOptionsToolbarContainer::resizeEvent(QResizeEvent*)
{
    // Decorations
    m_d->leftFadeWidget->move(0, 0);
    m_d->leftFadeWidget->resize(0, height() - (m_d->scrollbarHeight + 1));
    m_d->rightFadeWidget->move(width() - m_d->fadeWidgetWidth, 0);
    m_d->rightFadeWidget->resize(0, height() - (m_d->scrollbarHeight + 1));
    m_d->scrollbarWidget->move(0, height() - m_d->scrollbarHeight);
    m_d->scrollbarWidget->resize(width(), 0);
    // Widget
    if (!m_d->currentWidget) {
        return;
    }
    if (m_d->currentWidget->width() > width()) {
        if (m_d->currentWidget->width() + m_d->currentWidget->x() <= width()) {
            m_d->currentWidget->move(-(m_d->currentWidget->width() - width()), 0);
            m_d->rightFadeWidget->fadeOut();
        } else {
            m_d->rightFadeWidget->fadeIn();
        }
        if (m_d->isHovering) {
            m_d->scrollbarWidget->fadeIn();
        }
        m_d->scrollbarWidget->setAreaShown(static_cast<qreal>(width()) / static_cast<qreal>(m_d->currentWidget->width()));
        m_d->scrollbarWidget->setOffset(static_cast<qreal>(-m_d->currentWidget->x()) / static_cast<qreal>(m_d->currentWidget->width()));
    } else {
        m_d->currentWidget->move(0, 0);
        m_d->leftFadeWidget->fadeOut();
        m_d->rightFadeWidget->fadeOut();
        m_d->scrollbarWidget->fadeOut();
        m_d->scrollbarWidget->setAreaShown(100.0);
        m_d->scrollbarWidget->setOffset(0.0);
    }
    m_d->scrollbarWidget->update();
}

void KisToolOptionsToolbarContainer::enterEvent(QEvent*)
{
    if (m_d->currentWidget && m_d->currentWidget->width() > width()) {
        m_d->scrollbarWidget->fadeIn();
    }
    m_d->isHovering = true;
}

void KisToolOptionsToolbarContainer::leaveEvent(QEvent*)
{
    m_d->scrollbarWidget->fadeOut();
    m_d->isHovering = false;
}

void KisToolOptionsToolbarContainer::mousePressEvent(QMouseEvent *e)
{
    if (!m_d->currentWidget) {
        return;
    }
    if (e->button() != Qt::LeftButton) {
        return;
    }
    if (width() > m_d->currentWidget->width()) {
        return;
    }
    m_d->isDragging = true;
    m_d->lastMousePos = e->pos();
}

void KisToolOptionsToolbarContainer::mouseMoveEvent(QMouseEvent *e)
{
    if (!m_d->currentWidget) {
        return;
    }
    if (!m_d->isDragging) {
        return;
    }
    if (!(e->buttons() & Qt::LeftButton)) {
        return;
    }
    const int offset = e->x() - m_d->lastMousePos.x();
    const int newPos = m_d->currentWidget->x() + offset;
    if (newPos >= 0) {
        m_d->currentWidget->move(0, 0);
        m_d->leftFadeWidget->fadeOut();
        if (m_d->currentWidget->width() > width()) {
            m_d->rightFadeWidget->fadeIn();
        } else {
            m_d->rightFadeWidget->fadeOut();
        }
    } else if (newPos < width() - m_d->currentWidget->width()) {
        m_d->currentWidget->move(width() - m_d->currentWidget->width(), 0);
        m_d->rightFadeWidget->fadeOut();
        m_d->leftFadeWidget->fadeIn();
    } else {
        m_d->currentWidget->move(newPos, 0);
        m_d->leftFadeWidget->fadeIn();
        if (m_d->currentWidget->width() > width()) {
            m_d->rightFadeWidget->fadeIn();
        } else {
            m_d->rightFadeWidget->fadeOut();
        }
    }
    m_d->scrollbarWidget->setAreaShown(static_cast<qreal>(width()) / static_cast<qreal>(m_d->currentWidget->width()));
    m_d->scrollbarWidget->setOffset(static_cast<qreal>(-m_d->currentWidget->x()) / static_cast<qreal>(m_d->currentWidget->width()));

    m_d->lastMousePos = e->pos();
}

void KisToolOptionsToolbarContainer::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_d->currentWidget) {
        return;
    }
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_d->isDragging = false;
}
