/*
 *  SPDX-FileCopyrightText: 2023 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SvgTextCursor.h"
#include "KoCanvasBase.h"
#include "KoSvgTextProperties.h"
#include "SvgTextInsertCommand.h"
#include "SvgTextRemoveCommand.h"
#include "SvgTextShapeManagerBlocker.h"

#include "KoViewConverter.h"
#include "kis_coordinates_converter.h"

#include "kundo2command.h"
#include <QTimer>
#include <QDebug>
#include <QClipboard>
#include <QMimeData>
#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QAction>
#include <kis_assert.h>
#include <QInputMethodEvent>

struct IMEDecorationInfo {
    int start = -1; ///< The startPos from the attribute.
    int length = 0; ///< The length from the attribute.
    KoSvgText::TextDecorations decor = KoSvgText::DecorationNone; ///< Which sides get decorated.
    KoSvgText::TextDecorationStyle style = KoSvgText::Solid; ///< The style.
    bool thick = false; ///< Whether the decoration needs to be doubled in size.

    void setDecorationFromQStyle(QTextCharFormat::UnderlineStyle s) {
        if (s == QTextCharFormat::DotLine) {
            style = KoSvgText::Dotted;
        } else if (s == QTextCharFormat::DashUnderline) {
            style = KoSvgText::Dashed;
        } else if (s == QTextCharFormat::WaveUnderline) {
            style = KoSvgText::Wavy;
        } else if (s == QTextCharFormat::SpellCheckUnderline) {
            style = KoSvgText::Wavy;
#ifdef Q_OS_MACOS
            style = KoSvgText::Dotted;
#endif
        } else {
            style = KoSvgText::Solid;
        }
    }
};

struct Q_DECL_HIDDEN SvgTextCursor::Private {
    KoCanvasBase *canvas;
    bool isAddingCommand = false;
    int pos = 0;
    int anchor = 0;
    KoSvgTextShape *shape {nullptr};

    QTimer cursorFlash;
    QTimer cursorFlashLimit;
    bool cursorVisible = false;

    QPainterPath cursorShape;
    QRectF oldCursorRect;
    QLineF cursorCaret;
    QLineF anchorCaret;
    int cursorWidth = 1;
    QPainterPath selection;
    QRectF oldSelectionRect;


    // This is used to adjust cursorpositions better on text-shape relayouts.
    int posIndex = 0;
    int anchorIndex = 0;

    bool visualNavigation = true;

    SvgTextInsertCommand *preEditCommand {nullptr};
    int preEditStart = -1;
    int preEditLength = -1;
    QPainterPath IMEDecoration;
    QVector<IMEDecorationInfo> styleMap;
    QRectF oldIMEDecorationRect;
};

SvgTextCursor::SvgTextCursor(KoCanvasBase *canvas) :
    d(new Private)
{
    d->canvas = canvas;
}

SvgTextCursor::~SvgTextCursor()
{
    d->cursorFlash.stop();
    d->shape = nullptr;
}

void SvgTextCursor::setShape(KoSvgTextShape *textShape)
{
    if (d->shape) {
        commitIMEPreEdit();
        d->shape->removeShapeChangeListener(this);
    }
    d->shape = textShape;
    if (d->shape) {
        d->shape->addShapeChangeListener(this);
    }
    d->pos = 0;
    d->anchor = 0;
    updateCursor();
    updateSelection();
}

void SvgTextCursor::setCaretSetting(int cursorWidth, int cursorFlash, int cursorFlashLimit)
{
    d->cursorFlash.setInterval(cursorFlash/2);
    d->cursorFlashLimit.setInterval(cursorFlashLimit);
    d->cursorWidth = cursorWidth;
    connect(&d->cursorFlash, SIGNAL(timeout()), this, SLOT(blinkCursor()));
    connect(&d->cursorFlashLimit, SIGNAL(timeout()), this, SLOT(stopBlinkCursor()));
}

void SvgTextCursor::setVisualMode(bool visualMode)
{
    d->visualNavigation = visualMode;
}

int SvgTextCursor::getPos()
{
    return d->pos;
}

int SvgTextCursor::getAnchor()
{
    return d->anchor;
}

void SvgTextCursor::setPos(int pos, int anchor)
{
    d->pos = pos;
    d->anchor = anchor;
    updateCursor();
    updateSelection();
}

void SvgTextCursor::setPosToPoint(QPointF point, bool moveAnchor)
{
    if (d->shape) {
        int pos = d->pos = d->shape->posForPointLineSensitive(d->shape->documentToShape(point));
        if (d->preEditCommand) {
            int start = d->shape->indexForPos(d->preEditStart);
            int end = start + d->preEditLength;
            int posIndex = d->shape->indexForPos(pos);
            if (posIndex > start && posIndex <= end) {
                qDebug() << "Clicked on preedit string";
                qApp->inputMethod()->invokeAction(QInputMethod::Click, posIndex - start);
                return;
            } else {qDebug() << "clicked outside preedit string";
                commitIMEPreEdit();
            }
        }

        d->pos = pos;
        if (moveAnchor) {
            d->anchor = d->pos;
        }
        updateCursor();
        updateSelection();
    }
}

void SvgTextCursor::moveCursor(MoveMode mode, bool moveAnchor)
{
    if (d->shape) {

        d->pos = moveModeResult(mode, d->pos, d->visualNavigation);

        if (moveAnchor) {
            d->anchor = d->pos;
        }
        updateSelection();
        updateCursor();
    }
}

void SvgTextCursor::insertText(QString text)
{

    if (d->shape) {
        //KUndo2Command *parentCmd = new KUndo2Command;
        if (hasSelection()) {
            SvgTextRemoveCommand *removeCmd = removeSelectionImpl();
            addCommandToUndoAdapter(removeCmd);
        }

        SvgTextInsertCommand *insertCmd = new SvgTextInsertCommand(d->shape, d->pos, d->anchor, text);
        addCommandToUndoAdapter(insertCmd);

    }
}

void SvgTextCursor::removeText(SvgTextCursor::MoveMode first, SvgTextCursor::MoveMode second)
{
    if (d->shape) {
        SvgTextRemoveCommand *removeCmd;
        if (hasSelection()) {
            removeCmd = removeSelectionImpl();
            addCommandToUndoAdapter(removeCmd);
        } else {
            int posA = moveModeResult(first, d->pos, d->visualNavigation);
            int posB = moveModeResult(second, d->pos, d->visualNavigation);

            int posStart = qMin(posA, posB);
            int posEnd = qMax(posA, posB);
            int length = d->shape->indexForPos(posEnd) - d->shape->indexForPos(posStart);

            removeCmd = new SvgTextRemoveCommand(d->shape, posStart, d->pos, d->anchor, length);
            addCommandToUndoAdapter(removeCmd);
        }
    }
}

void SvgTextCursor::removeSelection()
{
    KUndo2Command *removeCmd = removeSelectionImpl();
    addCommandToUndoAdapter(removeCmd);
}

SvgTextRemoveCommand *SvgTextCursor::removeSelectionImpl(KUndo2Command *parent)
{
    SvgTextRemoveCommand *removeCmd = nullptr;
    if (d->shape) {
        if (d->anchor != d->pos) {
            int start = qMin(d->anchor, d->pos);
            int length = d->shape->indexForPos(qMax(d->anchor, d->pos)) - d->shape->indexForPos(start);
            removeCmd = new SvgTextRemoveCommand(d->shape, start, d->pos, d->anchor, length, parent);
        }
    }
    return removeCmd;
}

void SvgTextCursor::copy() const
{
    if (d->shape) {
        int start = d->shape->indexForPos(qMin(d->anchor, d->pos));
        int length = d->shape->indexForPos(qMax(d->anchor, d->pos)) - start;
        QString copied = d->shape->plainText().mid(start, length);
        QClipboard *cb = QApplication::clipboard();
        cb->setText(copied);

    }
}

bool SvgTextCursor::paste()
{
    bool success = false;
    if (d->shape) {
        QClipboard *cb = QApplication::clipboard();
        const QMimeData *mimeData = cb->mimeData();
        if (mimeData->hasText()) {
            insertText(mimeData->text());
            success = true;
        }
    }
    return success;
}

void SvgTextCursor::deselectText()
{
    setPos(d->pos, d->pos);
}

void SvgTextCursor::paintDecorations(QPainter &gc, QColor selectionColor)
{
    if (d->shape) {
        gc.save();
        gc.setTransform(d->shape->absoluteTransformation(), true);
        if (d->pos != d->anchor) {
            gc.save();
            gc.setOpacity(0.5);
            QBrush brush(selectionColor);
            gc.fillPath(d->selection, brush);
            gc.restore();
        } else {
            if (d->cursorVisible) {
                QPen pen;
                pen.setCosmetic(true);
                pen.setColor(Qt::black);
                pen.setWidth(d->cursorWidth);
                gc.setPen(pen);
                gc.drawPath(d->cursorShape);

            }
        }
        if (d->preEditCommand) {
            gc.save();
            QBrush brush(selectionColor);
            gc.setOpacity(0.5);
            gc.fillPath(d->IMEDecoration, brush);
            gc.restore();
        }
        gc.restore();
    }
}

QVariant SvgTextCursor::inputMethodQuery(Qt::InputMethodQuery query) const
{
    qDebug() << "receiving inputmethod query" << query;

    switch(query) {
    case Qt::ImEnabled:
        return d->shape? true: false;
        break;
    case Qt::ImCursorRectangle:
        // The platform integration will always define the cursor as the 'left side' handle.
        if (d->shape) {
            QPointF caret1(d->shape->shapeToDocument(d->cursorCaret.p1()));
            QPointF caret2(d->shape->shapeToDocument(d->cursorCaret.p2()));

            caret1 = d->canvas->viewConverter()->documentToView().map(caret1);
            caret2 = d->canvas->viewConverter()->documentToView().map(caret2);
            caret1 = d->canvas->viewConverter()->viewToWidget().map(caret1);
            caret2 = d->canvas->viewConverter()->viewToWidget().map(caret2);
            QRectF rect = QRectF(caret1, caret2).normalized();
            if (!rect.isValid()) {
                if (rect.height() < 1) {
                    rect.adjust(0, 1, 0, 0);
                }
                if (rect.width() < 1) {
                    rect.adjust(0, 0, 1, 0);
                }

            }
            return rect.toAlignedRect();
        }
        break;
    case Qt::ImAnchorRectangle:
        // The platform integration will always define the anchor as the 'right side' handle.
        if (d->shape) {
            QPointF caret1(d->shape->shapeToDocument(d->anchorCaret.p1()));
            QPointF caret2(d->shape->shapeToDocument(d->anchorCaret.p2()));

            caret1 = d->canvas->viewConverter()->documentToView().map(caret1);
            caret2 = d->canvas->viewConverter()->documentToView().map(caret2);
            caret1 = d->canvas->viewConverter()->viewToWidget().map(caret1);
            caret2 = d->canvas->viewConverter()->viewToWidget().map(caret2);
            QRectF rect = QRectF(caret1, caret2).normalized();
            if (rect.isEmpty()) {
                if (rect.height() < 1) {
                    rect.adjust(0, 1, 0, 0);
                }
                if (rect.width() < 1) {
                    rect = rect.adjusted(-1, 0, 0, 0).normalized();
                }
            }
            return rect.toAlignedRect();
        }
        break;
    //case Qt::ImFont: // not sure what this is used for, but we cannot sent out without access to properties.
    case Qt::ImAbsolutePosition:
    case Qt::ImCursorPosition:
        if (d->shape) {
            return d->shape->indexForPos(d->pos);
        }
        break;
    case Qt::ImSurroundingText:
        if (d->shape) {
            return d->shape->plainText();
        }
        break;
    case Qt::ImCurrentSelection:
        if (d->shape) {
            int start = d->shape->indexForPos(qMin(d->anchor, d->pos));
            int length = d->shape->indexForPos(qMax(d->anchor, d->pos)) - start;
            return d->shape->plainText().mid(start, length);
        }
        break;
    case Qt::ImTextBeforeCursor:
        if (d->shape) {
            int start = d->shape->indexForPos(d->pos);
            return d->shape->plainText().left(start);
        }
        break;
    case Qt::ImTextAfterCursor:
        if (d->shape) {
            int start = d->shape->indexForPos(d->pos);
            return d->shape->plainText().right(start);
        }
        break;
    case Qt::ImMaximumTextLength:
        return QVariant(); // infinite text length!
        break;
    case Qt::ImAnchorPosition:
        if (d->shape) {
            return d->shape->indexForPos(d->anchor);
        }
        break;
    case Qt::ImHints:
        // It would be great to use Qt::ImhNoTextHandles or Qt::ImhNoEditMenu,
        // but neither are implemented for anything but web platform integration
        return Qt::ImhMultiLine;
        break;
    // case Qt::ImPreferredLanguage: // requires access to properties.
    // case Qt::ImPlatformData: // this is only for iOS at time of writing.
    case Qt::ImEnterKeyType:
        if (d->shape) {
            return Qt::EnterKeyDefault; // because input method hint is always multiline, this will show a return key.
        }
        break;
    // case Qt::ImInputItemClipRectangle // whether the input item is clipped?
    default:
        return QVariant();
    }
    return QVariant();
}

void SvgTextCursor::inputMethodEvent(QInputMethodEvent *event)
{
    qDebug() << "Commit:"<< event->commitString() << "predit:"<< event->preeditString();
    qDebug() << "Replacement:"<< event->replacementStart() << event->replacementLength();

    QRectF updateRect = d->shape? d->shape->boundingRect(): QRectF();
    //SvgTextShapeManagerBlocker blocker(d->canvas->shapeManager());
    //blocker.lock();
    bool oldUpdatesBlocked = d->canvas->shapeManager()->updatesBlocked();
    d->canvas->shapeManager()->setUpdatesBlocked(true);

    bool isGettingInput = !event->commitString().isEmpty() || !event->preeditString().isEmpty()
                || event->replacementLength() > 0;

    // Remove previous preedit string.
    if (d->preEditCommand) {
        d->preEditCommand->undo();
        d->preEditCommand = 0;
        d->preEditStart = -1;
        d->preEditLength = -1;
        updateRect |= d->shape? d->shape->boundingRect(): QRectF();
    }

    if (!d->shape || !isGettingInput) {
        //blocker.unlock();
        d->canvas->shapeManager()->setUpdatesBlocked(oldUpdatesBlocked);
        d->shape->updateAbsolute(updateRect);
        event->ignore();
        return;
    }


    // remove the selection if any.
    addCommandToUndoAdapter(removeSelectionImpl());

    // set the text insertion pos to replacement start and also remove replacement length, if any.
    int originalPos = d->pos;
    int index = d->shape->indexForPos(d->pos) + event->replacementStart();
    d->pos = d->shape->posForIndex(index);
    if (event->replacementLength() > 0) {
        SvgTextRemoveCommand *cmd = new SvgTextRemoveCommand(d->shape,
                                                             d->pos,
                                                             originalPos,
                                                             d->anchor,
                                                             event->replacementLength());
        addCommandToUndoAdapter(cmd);
    }

    // add the commit string, if any.
    if (!event->commitString().isEmpty()) {
        insertText(event->commitString());
    }

    // set the selection...
    Q_FOREACH(QInputMethodEvent::Attribute attribute, event->attributes()) {
        if (attribute.type == QInputMethodEvent::Selection) {
            d->pos = d->shape->posForIndex(attribute.start);
            int index = d->shape->indexForPos(d->pos);
            d->anchor = d->shape->posForIndex(index + attribute.length);
        }
    }


    // insert a preedit string, if any.
    if (!event->preeditString().isEmpty()) {
        int index = d->shape->indexForPos(d->pos);
        d->preEditCommand = new SvgTextInsertCommand(d->shape, d->pos, d->anchor, event->preeditString());
        d->preEditCommand->redo();
        d->preEditLength = event->preeditString().size();
        d->preEditStart = d->shape->posForIndex(index, true);
    } else {
        d->preEditCommand = 0;
    }

    // Apply the cursor offset for the preedit.
    QVector<IMEDecorationInfo> styleMap;
    Q_FOREACH(QInputMethodEvent::Attribute attribute, event->attributes()) {
        qDebug() << "attribute: "<< attribute.type << "start: " << attribute.start
                 << "length: " << attribute.length << "val: " << attribute.value;
        // Text Format is about setting the look of the preedit string, and there can be multiple per event
        // we primarily interpret the underline. When a background color is set, we increase the underline
        // thickness, as that's what is actually supossed to happen according to the comments in the
        // platform input contexts for both macOS and Windows.

        if (attribute.type == QInputMethodEvent::TextFormat) {
            IMEDecorationInfo decoration;
            QVariant val = attribute.value;
            QTextCharFormat form = val.value<QTextFormat>().toCharFormat();

            decoration.start = attribute.start;
            decoration.length = attribute.length;

            decoration.decor.setFlag(KoSvgText::DecorationUnderline, form.font().underline());
            decoration.decor.setFlag(KoSvgText::DecorationOverline, form.font().overline());
            decoration.decor.setFlag(KoSvgText::DecorationLineThrough, form.font().strikeOut());

            decoration.setDecorationFromQStyle(form.underlineStyle());
            if (form.background().isOpaque()) {
                decoration.thick = true;
                if (decoration.decor == KoSvgText::DecorationNone) {
                    decoration.decor.setFlag(KoSvgText::DecorationUnderline, true);
                }
            }
            styleMap.append(decoration);
        }

        // QInputMethodEvent::Language is about setting the locale on the given  preedit string, which is not possible yet.
        // QInputMethodEvent::Ruby is supossedly ruby info for the preedit string, but none of the platform integrations
        // actually implement this at time of writing, and it may have been something from a previous live of Qt's.
        if (attribute.type == QInputMethodEvent::Cursor) {
            if (d->preEditStart < 0) {
                d->anchor = d->pos;
            } else {
                int index = d->shape->indexForPos(d->preEditStart);
                d->pos = d->shape->posForIndex(index + attribute.start);
                d->anchor = d->pos;
            }

            // attribute value is the cursor color, and should be used to paint the cursor.
            // attribute length is about whether the cursor should be visible at all...
        }
    }

    //blocker.unlock();

    d->canvas->shapeManager()->setUpdatesBlocked(oldUpdatesBlocked);
    updateRect |= d->shape->boundingRect();
    d->shape->updateAbsolute(updateRect);
    //qDebug() << "update" << updateRect << d->canvas->shapeManager()->updatesBlocked();
    d->styleMap = styleMap;
    updateIMEDecoration();
    updateSelection();
    updateCursor();
    event->accept();
}

void SvgTextCursor::blinkCursor()
{
    if (d->shape) {
        emit updateCursorDecoration(d->shape->shapeToDocument(d->cursorShape.boundingRect()) | d->oldCursorRect);
        d->cursorVisible = !d->cursorVisible;
    }
}

void SvgTextCursor::stopBlinkCursor()
{
    d->cursorFlash.stop();
    d->cursorFlashLimit.stop();
    d->cursorVisible = true;
    if (d->shape) {
        emit updateCursorDecoration(d->shape->shapeToDocument(d->cursorShape.boundingRect()) | d->oldCursorRect);
    }
}

bool SvgTextCursor::hasSelection()
{
    return d->pos != d->anchor;
}

void SvgTextCursor::notifyShapeChanged(KoShape::ChangeType type, KoShape *shape)
{
    Q_UNUSED(type);
    Q_UNUSED(shape);
    if (d->shape->indexForPos(d->pos) != d->posIndex) {
        d->pos = d->shape->posForIndex(d->posIndex);
    }
    if (d->shape->indexForPos(d->anchor) != d->anchorIndex) {
        d->anchor = d->shape->posForIndex(d->anchorIndex);
    }
    updateCursor();
    updateSelection();
}

void SvgTextCursor::notifyCursorPosChanged(int pos, int anchor)
{
    d->pos = pos;
    d->anchor = anchor;
    updateCursor();
    updateSelection();
}

void SvgTextCursor::keyPressEvent(QKeyEvent *event)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(d->shape);

    if (d->preEditCommand) {
        //MacOS will keep sending keyboard events during IME handling.
        event->accept();
        return;
    }

    bool select = event->modifiers().testFlag(Qt::ShiftModifier);

    if (!((Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier) & event->modifiers())) {

        switch (event->key()) {
        case Qt::Key_Right:
            moveCursor(SvgTextCursor::MoveRight, !select);
            event->accept();
            break;
        case Qt::Key_Left:
            moveCursor(SvgTextCursor::MoveLeft, !select);
            event->accept();
            break;
        case Qt::Key_Up:
            moveCursor(SvgTextCursor::MoveUp, !select);
            event->accept();
            break;
        case Qt::Key_Down:
            moveCursor(SvgTextCursor::MoveDown, !select);
            event->accept();
            break;
        case Qt::Key_Delete:
            removeText(MoveNone, MoveNextChar);
            event->accept();
            break;
        case Qt::Key_Backspace:
            removeText(MovePreviousChar, MoveNone);
            event->accept();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            insertText("\n");
            event->accept();
            break;
        default:
            event->ignore();
        }

        if (event->isAccepted()) {
            return;
        }
    }
    if (acceptableInput(event)) {
        insertText(event->text());
        event->accept();
        return;
    }

    KoSvgTextProperties props = d->shape->textProperties();

    KoSvgText::WritingMode mode = KoSvgText::WritingMode(props.propertyOrDefault(KoSvgTextProperties::WritingModeId).toInt());
    KoSvgText::Direction direction = KoSvgText::Direction(props.propertyOrDefault(KoSvgTextProperties::DirectionId).toInt());

    // Qt's keysequence stuff doesn't handle vertical, so to test all the standard keyboard shortcuts as if it did,
    // we reinterpret the direction keys according to direction and writing mode, and test against that.

    int newKey = event->key();

    if (direction == KoSvgText::DirectionRightToLeft) {
        switch (newKey) {
        case Qt::Key_Left:
            newKey = Qt::Key_Right;
            break;
        case Qt::Key_Right:
            newKey = Qt::Key_Left;
            break;
        default:
            break;
        }
    }

    if (mode == KoSvgText::VerticalRL) {
        switch (newKey) {
        case Qt::Key_Left:
            newKey = Qt::Key_Down;
            break;
        case Qt::Key_Right:
            newKey = Qt::Key_Up;
            break;
        case Qt::Key_Up:
            newKey = Qt::Key_Left;
            break;
        case Qt::Key_Down:
            newKey = Qt::Key_Right;
            break;
        default:
            break;
        }
    } else if (mode == KoSvgText::VerticalRL) {
        switch (newKey) {
        case Qt::Key_Left:
            newKey = Qt::Key_Up;
            break;
        case Qt::Key_Right:
            newKey = Qt::Key_Down;
            break;
        case Qt::Key_Up:
            newKey = Qt::Key_Left;
            break;
        case Qt::Key_Down:
            newKey = Qt::Key_Right;
            break;
        default:
            break;
        }
    }

    QKeySequence testSequence(event->modifiers() | newKey);


    // Note for future, when we have format changing actions:
    // We'll need to test format change actions before the standard
    // keys, as one of the standard keys for deleting a line is ctrl+u
    // which would probably be expected to do underline before deleting.

    // This first set is already tested above, however, if they still
    // match, then it's one of the extra sequences for MacOs, which
    // seem to be purely logical, instead of the visual set we tested
    // above.
    if (testSequence == QKeySequence::MoveToNextChar) {
        moveCursor(SvgTextCursor::MoveNextChar, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectNextChar) {
        moveCursor(SvgTextCursor::MoveNextChar, false);
        event->accept();
    } else if (testSequence == QKeySequence::MoveToPreviousChar) {
        moveCursor(SvgTextCursor::MovePreviousChar, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectPreviousChar) {
        moveCursor(SvgTextCursor::MovePreviousChar, false);
        event->accept();
    } else if (testSequence == QKeySequence::MoveToNextLine) {
        moveCursor(SvgTextCursor::MoveNextLine, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectNextLine) {
        moveCursor(SvgTextCursor::MoveNextLine, false);
        event->accept();
    } else if (testSequence == QKeySequence::MoveToPreviousLine) {
        moveCursor(SvgTextCursor::MovePreviousLine, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectPreviousLine) {
        moveCursor(SvgTextCursor::MovePreviousLine, false);
        event->accept();

    } else if (testSequence == QKeySequence::MoveToNextWord) {
        moveCursor(SvgTextCursor::MoveWordEnd, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectNextWord) {
        moveCursor(SvgTextCursor::MoveWordEnd, false);
        event->accept();
    } else if (testSequence == QKeySequence::MoveToPreviousWord) {
        moveCursor(SvgTextCursor::MoveWordStart, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectPreviousWord) {
        moveCursor(SvgTextCursor::MoveWordStart, false);
        event->accept();

    } else if (testSequence == QKeySequence::MoveToStartOfLine) {
        moveCursor(SvgTextCursor::MoveLineStart, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectStartOfLine) {
        moveCursor(SvgTextCursor::MoveLineStart, false);
        event->accept();
    } else if (testSequence == QKeySequence::MoveToEndOfLine) {
        moveCursor(SvgTextCursor::MoveLineEnd, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectEndOfLine) {
        moveCursor(SvgTextCursor::MoveLineEnd, false);
        event->accept();

    } else if (testSequence == QKeySequence::MoveToStartOfBlock
               || testSequence == QKeySequence::MoveToStartOfDocument) {
        moveCursor(SvgTextCursor::ParagraphStart, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectStartOfBlock
               || testSequence == QKeySequence::SelectStartOfDocument) {
        moveCursor(SvgTextCursor::ParagraphStart, false);
        event->accept();

    } else if (testSequence == QKeySequence::MoveToEndOfBlock
               || testSequence == QKeySequence::MoveToEndOfDocument) {
        moveCursor(SvgTextCursor::ParagraphEnd, true);
        event->accept();
    } else if (testSequence == QKeySequence::SelectEndOfBlock
               || testSequence == QKeySequence::SelectEndOfDocument) {
        moveCursor(SvgTextCursor::ParagraphEnd, false);
        event->accept();

    }else if (testSequence == QKeySequence::DeleteStartOfWord) {
        removeText(MoveWordStart, MoveNone);
        event->accept();
    } else if (testSequence == QKeySequence::DeleteEndOfWord) {
        removeText(MoveNone, MoveWordEnd);
        event->accept();
    } else if (testSequence == QKeySequence::DeleteEndOfLine) {
        removeText(MoveNone, MoveLineEnd);
        event->accept();
    } else if (testSequence == QKeySequence::DeleteCompleteLine) {
        removeText(MoveLineStart, MoveLineEnd);
        event->accept();
    } else if (testSequence == QKeySequence::Backspace) {
        removeText(MovePreviousChar, MoveNone);
        event->accept();
    } else if (testSequence == QKeySequence::Delete) {
        removeText(MoveNone, MoveNextChar);
        event->accept();

    } else if (testSequence == QKeySequence::InsertLineSeparator
               || testSequence == QKeySequence::InsertParagraphSeparator) {
        insertText("\n");
        event->accept();
    } else {
        event->ignore();
    }
}

bool SvgTextCursor::isAddingCommand() const
{
    return d->isAddingCommand;
}

void SvgTextCursor::updateCursor()
{
    if (d->shape) {
        d->oldCursorRect = d->shape->shapeToDocument(d->cursorShape.boundingRect());
        d->posIndex = d->shape->indexForPos(d->pos);
        d->anchorIndex = d->shape->indexForPos(d->anchor);
    }
    d->cursorShape = d->shape? d->shape->cursorForPos(d->pos, d->cursorCaret): QPainterPath();
    d->cursorFlash.start();
    d->cursorFlashLimit.start();
    d->cursorVisible = false;
    qApp->inputMethod()->update(Qt::ImQueryInput);
    blinkCursor();
}

void SvgTextCursor::updateSelection()
{
    if (d->shape) {
        d->oldSelectionRect = d->shape->shapeToDocument(d->selection.boundingRect());
        d->shape->cursorForPos(d->anchor, d->anchorCaret);
        d->selection = d->shape->selectionBoxes(d->pos, d->anchor);
        emit updateCursorDecoration(d->shape->shapeToDocument(d->selection.boundingRect()) | d->oldSelectionRect);
    }
}

void SvgTextCursor::updateIMEDecoration()
{
    if (d->shape) {
        d->oldIMEDecorationRect = d->shape->shapeToDocument(d->IMEDecoration.boundingRect());
        KoSvgText::TextDecorations decor;
        decor.setFlag(KoSvgText::DecorationUnderline, true);
        d->IMEDecoration = QPainterPath();
        if (d->preEditCommand) {
            Q_FOREACH(const IMEDecorationInfo info,  d->styleMap) {

                int startIndex = d->shape->indexForPos(d->preEditStart) + info.start;
                int endIndex = startIndex + info.length;
                qreal minimum = d->canvas->viewToDocument(QPointF(1, 1)).x();
                d->IMEDecoration.addPath(d->shape->underlines(d->shape->posForIndex(startIndex),
                                                              d->shape->posForIndex(endIndex),
                                                              info.decor,
                                                              info.style,
                                                              minimum,
                                                              info.thick));
                d->IMEDecoration.setFillRule(Qt::WindingFill);
            }
        }

        emit updateCursorDecoration(d->shape->shapeToDocument(d->IMEDecoration.boundingRect()) | d->oldIMEDecorationRect);
    }
}

void SvgTextCursor::addCommandToUndoAdapter(KUndo2Command *cmd)
{
    if (d->canvas) {
        if (cmd) {
            d->isAddingCommand = true;
            d->canvas->addCommand(cmd);
            d->isAddingCommand = false;
        }
    }
}

int SvgTextCursor::moveModeResult(SvgTextCursor::MoveMode &mode, int &pos, bool visual) const
{
    int newPos = pos;
    switch (mode) {
        case MoveNone:
        break;
    case MoveLeft:
        newPos = d->shape->posLeft(pos, visual);
        break;
    case MoveRight:
        newPos = d->shape->posRight(pos, visual);
        break;
    case MoveUp:
        newPos = d->shape->posUp(pos, visual);
        break;
    case MoveDown:
        newPos = d->shape->posDown(pos, visual);
        break;
    case MovePreviousChar:
        newPos = d->shape->previousCluster(pos);
        break;
    case MoveNextChar:
        newPos = d->shape->nextCluster(pos);
        break;
    case MovePreviousLine:
        newPos = d->shape->previousLine(pos);
        break;
    case MoveNextLine:
        newPos = d->shape->nextLine(pos);
        break;
    case MoveWordLeft:
        newPos = d->shape->wordLeft(pos, visual);
        if (newPos == pos) {
            newPos = d->shape->posLeft(pos, visual);
            newPos = d->shape->wordLeft(newPos, visual);
        }
        break;
    case MoveWordRight:
        newPos = d->shape->wordRight(pos, visual);
        if (newPos == pos) {
            newPos = d->shape->posRight(pos, visual);
            newPos = d->shape->wordRight(newPos, visual);
        }
        break;
    case MoveWordStart:
        newPos = d->shape->wordStart(pos);
        if (newPos == pos) {
            newPos = d->shape->previousCluster(pos);
            newPos = d->shape->wordStart(newPos);
        }
        break;
    case MoveWordEnd:
        newPos = d->shape->wordEnd(pos);
        if (newPos == pos) {
            newPos = d->shape->nextCluster(pos);
            newPos = d->shape->wordEnd(newPos);
        }
        break;
    case MoveLineStart:
        newPos = d->shape->lineStart(pos);
        break;
    case MoveLineEnd:
        newPos = d->shape->lineEnd(pos);
        break;
    case ParagraphStart:
        newPos = 0;
        break;
    case ParagraphEnd:
        newPos = d->shape->posForIndex(d->shape->plainText().size());
        break;
    }
    return newPos;
}

/// More or less copied from bool QInputControl::isAcceptableInput(const QKeyEvent *event) const
bool SvgTextCursor::acceptableInput(const QKeyEvent *event) const
{
    const QString text = event->text();
    if (text.isEmpty())
        return false;
    const QChar c = text.at(0);
    // Formatting characters such as ZWNJ, ZWJ, RLM, etc. This needs to go before the
    // next test, since CTRL+SHIFT is sometimes used to input it on Windows.
    if (c.category() == QChar::Other_Format)
        return true;
    // QTBUG-35734: ignore Ctrl/Ctrl+Shift; accept only AltGr (Alt+Ctrl) on German keyboards
    if (event->modifiers() == Qt::ControlModifier
            || event->modifiers() == (Qt::ShiftModifier | Qt::ControlModifier)) {
        return false;
    }
    if (c.isPrint())
        return true;
    if (c.category() == QChar::Other_PrivateUse)
        return true;
    if (c == QLatin1Char('\t'))
        return true;
    return false;
}

void SvgTextCursor::commitIMEPreEdit()
{
    if (!d->preEditCommand) {
        return;
    }

    qApp->inputMethod()->commit();

    if (!d->preEditCommand) {
        return;
    }

    d->preEditCommand->undo();
    d->preEditCommand = nullptr;
    d->preEditStart = -1;
    d->preEditLength = 0;
    updateIMEDecoration();
    updateCursor();
}
