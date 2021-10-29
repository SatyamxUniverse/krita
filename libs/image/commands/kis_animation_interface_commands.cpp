#include "kis_animation_interface_commands.h"
#include "kis_image_animation_interface.h"

KisFullPlaybackRangeSetCommand::KisFullPlaybackRangeSetCommand(KisImageAnimationInterface *interface, KisTimeSpan newSpan, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_spans(interface->fullClipRange(), newSpan)
    , m_interface(interface)
{
}

void KisFullPlaybackRangeSetCommand::undo()
{
    m_interface->setFullClipRange(m_spans.first);
}


void KisFullPlaybackRangeSetCommand::redo()
{
    m_interface->setFullClipRange(m_spans.second);
}
