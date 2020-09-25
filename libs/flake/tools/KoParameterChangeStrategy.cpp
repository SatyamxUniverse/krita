/* This file is part of the KDE project
   Copyright (C) 2006 Thorsten Zachmann <zachmann@kde.org>
   Copyright (C) 2007 Thomas Zander <zander@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#include "KoParameterChangeStrategy.h"
#include "KoParameterChangeStrategy_p.h"
#include "KoParameterShape.h"
#include "commands/KoParameterHandleMoveCommand.h"

#include <KoCanvasBase.h>
#include "KoSnapGuide.h"


KoParameterChangeStrategy::KoParameterChangeStrategy(KoToolBase *tool, KoParameterShape *parameterShape, int handleId)
    : KoInteractionStrategy(*(new KoParameterChangeStrategyPrivate(tool, parameterShape, handleId)))
{
    Q_D(KoParameterChangeStrategy);
    d->tool->canvas()->snapGuide()->setIgnoredShapes({parameterShape});
}

KoParameterChangeStrategy::KoParameterChangeStrategy(KoParameterChangeStrategyPrivate& dd)
: KoInteractionStrategy(dd)
{

}

KoParameterChangeStrategy::~KoParameterChangeStrategy()
{
}

void KoParameterChangeStrategy::handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers)
{
    Q_D(KoParameterChangeStrategy);

    const QPointF snappedPosition = d->tool->canvas()->snapGuide()->snap(mouseLocation, modifiers);

    d->parameterShape->moveHandle(d->handleId, snappedPosition, modifiers);
    d->lastModifierUsed = modifiers;
    d->releasePoint = snappedPosition;
}

KUndo2Command* KoParameterChangeStrategy::createCommand()
{
    Q_D(KoParameterChangeStrategy);

    d->tool->canvas()->snapGuide()->reset();

    KoParameterHandleMoveCommand *cmd = 0;
    // check if handle position changed
    if (d->startPoint != QPointF(0, 0) && d->startPoint != d->releasePoint) {
        cmd = new KoParameterHandleMoveCommand(d->parameterShape, d->handleId, d->startPoint, d->releasePoint, d->lastModifierUsed);
    }
    return cmd;
}

void KoParameterChangeStrategy::finishInteraction(Qt::KeyboardModifiers /*modifiers*/)
{
}


