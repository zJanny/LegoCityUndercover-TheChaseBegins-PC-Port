#include "runtime/reference_trigger_apply.hpp"

namespace chase::runtime {

std::vector<ReferenceNavNodeRequest> applyReferenceActivation(const ReferenceActivation& activation, ScriptRunner& scripts) {
    std::vector<ReferenceNavNodeRequest> navNodes;
    for (const ReferenceEffect& effect : activation.effects) {
        switch (effect.kind) {
        case ReferenceEffectKind::Event: scripts.fireObjectEvent(referenceEventName(effect.event), effect.object); break;
        case ReferenceEffectKind::Message: scripts.sendObjectMessage(effect.object, effect.message); break;
        case ReferenceEffectKind::Enable: scripts.enableObject(effect.object); break;
        case ReferenceEffectKind::NavNode: navNodes.push_back({effect.navNode, effect.navNodeActive}); break;
        }
    }
    return navNodes;
}

}
