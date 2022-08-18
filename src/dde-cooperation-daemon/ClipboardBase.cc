#include "ClipboardBase.h"

#include "Manager.h"

ClipboardBase::ClipboardBase(Manager *manager)
    : m_manager(manager) {
}

void ClipboardBase::notifyTargetsChanged(const std::vector<std::string> &targets) {
    m_manager->onClipboardTargetsChanged(targets);
}
