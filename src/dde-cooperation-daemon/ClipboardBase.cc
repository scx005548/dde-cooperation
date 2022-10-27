#include "ClipboardBase.h"

ClipboardBase::ClipboardBase(ClipboardObserver *observer)
    : m_observer(observer) {
}

void ClipboardBase::notifyTargetsChanged(const std::vector<std::string> &targets) {
    m_observer->onClipboardTargetsChanged(targets);
}
