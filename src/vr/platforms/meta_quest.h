#ifndef META_QUEST_H
#define META_QUEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vr_platform_base.h"
#include "../openxr_manager.h"

// Meta Quest platform structure
typedef struct MetaQuestPlatform MetaQuestPlatform;

// Creation
MetaQuestPlatform* meta_quest_platform_create(void);

// Quest-specific features
int meta_quest_get_guardian_bounds(MetaQuestPlatform *platform, 
                                   XrVector3f *bounds, uint32_t maxBounds, 
                                   uint32_t *boundCount);
int meta_quest_enable_passthrough(MetaQuestPlatform *platform, bool enable);
int meta_quest_setup_optimal_settings(MetaQuestPlatform *platform);

// Get base platform interface
VRPlatformBase* meta_quest_get_base(MetaQuestPlatform *platform);

#ifdef __cplusplus
}
#endif

#endif // META_QUEST_H
