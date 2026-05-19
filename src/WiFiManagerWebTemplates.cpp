#include "WiFiManagerWebTemplates.h"
#include "WiFiManagerWebAssets.h"

namespace WiFiManagerWebTemplates {
    String setupPage() {
        return String(WiFiManagerWebAssets::kSetupPage);
    }

    String savedPage() {
        return String(WiFiManagerWebAssets::kSavedPage);
    }
}
