/**
 * Runtime feature switches for the BaseBuildingPlus bridge.
 *
 * ServerSentry's own server configuration remains authoritative. These
 * constants can only narrow logging further; they never override a disabled
 * ServerSentry category.
 */
class ServerSentryBBPConsts
{
    const static bool LOG_BBP_EVENTS = true;

    // BBP overrides several vanilla construction actions, so explicit bridge
    // hooks are enabled by default for the BBP-specific action classes.
    const static bool LOG_BUILD_PART = true;
    const static bool LOG_ADMIN_BUILD_PART = true;
    const static bool LOG_DISMANTLE = true;
    const static bool LOG_DESTROY = true;
    const static bool LOG_FOLD = true;

    // Disabled by default because ServerSentry core already observes normal
    // ActionDeployObject placement. Enable only when the installed BBP/load
    // order bypasses the core placement hook.
    const static bool LOG_PLACEMENT = false;

    // Prevent the shared ActionBuildPart/ActionDeployObject hooks from
    // recording unrelated vanilla or third-party objects.
    const static bool LOG_ONLY_BBP_TARGETS = true;
};
