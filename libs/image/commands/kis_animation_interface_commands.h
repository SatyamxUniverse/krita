#ifndef KIS_ANIMATION_INTERFACE_COMMANDS_H
#define KIS_ANIMATION_INTERFACE_COMMANDS_H

#include <kundo2command.h>
#include "kis_time_span.h"

class KisFullPlaybackRangeSetCommand : public KUndo2Command
{
public:
    KisFullPlaybackRangeSetCommand(class KisImageAnimationInterface* interface, KisTimeSpan newSpan, KUndo2Command* parent);
    ~KisFullPlaybackRangeSetCommand() {};

    void undo() override;
    void redo() override;

private:
    QPair<KisTimeSpan, KisTimeSpan> m_spans;
    KisImageAnimationInterface* m_interface;
};

#endif // KISANIMINTERFACEFULLPLAYBACKRANGESET_H
