#ifndef DDE_COOPERATION_DAEMON_CLIPBOARDBASE_H
#define DDE_COOPERATION_DAEMON_CLIPBOARDBASE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

class Manager;
class Machine;

class ClipboardBase {
public:
    explicit ClipboardBase(Manager *manager);
    virtual ~ClipboardBase() = default;

    virtual void newClipboardOwnerTargets(const std::weak_ptr<Machine> &machine,
                                          const std::vector<std::string> &targets) = 0;
    virtual void readTargetContent(
        const std::string &target,
        const std::function<void(const std::vector<char> &data)> &callback) = 0;
    virtual void updateTargetContent(const std::string &target, const std::vector<char> &data) = 0;
    virtual bool isFiles() = 0;

protected:
    Manager *m_manager;

    void notifyTargetsChanged(const std::vector<std::string> &targets);

private:
};

#endif // !DDE_COOPERATION_DAEMON_CLIPBOARDBASE_H
