/**
 * ServerSentry integration hooks for BaseBuildingPlus.
 *
 * Snapshot data is prepared before destructive BBP operations so target
 * metadata remains available, while emission occurs after the original
 * implementation returns. Every override preserves the modded-class chain by
 * calling super exactly once.
 */
class ServerSentryBBPBridge
{
    static bool Enabled()
    {
        return GetGame() && GetGame().IsServer() && ServerSentryBBPConsts.LOG_BBP_EVENTS;
    }

    static bool BaseBuildingAllowed()
    {
        return Enabled() && ServerSentryConfigManager.Get().IsAllowedToLogBaseBuilding();
    }

    static bool DismantleAllowed()
    {
        return Enabled() && ServerSentryConfigManager.Get().IsAllowedToLogDismantle();
    }

    static bool DestroyAllowed()
    {
        return Enabled() && ServerSentryConfigManager.Get().IsAllowedToLogDestroy();
    }

    static bool IsBBPAction(ActionData actionData)
    {
        if (!actionData)
        {
            return false;
        }

        if (!ServerSentryBBPConsts.LOG_ONLY_BBP_TARGETS)
        {
            return true;
        }

        Object targetObject;
        if (actionData.m_Target)
        {
            targetObject = actionData.m_Target.GetObject();
        }

        if (IsBBPObject(targetObject))
        {
            return true;
        }

        return IsBBPObject(actionData.m_MainItem);
    }

    static bool IsBBPObject(Object obj)
    {
        if (!obj)
        {
            return false;
        }

        if (obj.IsInherited(BBP_BASE))
        {
            return true;
        }

        // Kits and helper items do not necessarily inherit BBP_BASE. Keep the
        // fallback deliberately narrow and case-insensitive.
        string typeName = obj.GetType();
        typeName.ToLower();
        return typeName.Contains("bbp") || typeName.Contains("basebuildingplus");
    }

    static bool IsAdminBuild(ActionData actionData)
    {
        return actionData && actionData.m_MainItem && actionData.m_MainItem.isBBPAdminTool();
    }

    static ServerSentryEvent CreateBBPAction(
        ActionData actionData,
        string actionName,
        string displayName,
        string actionClass,
        string actionKind,
        string sourceHook)
    {
        if (!Enabled() || !IsBBPAction(actionData))
        {
            return null;
        }

        PlayerBase player = actionData.m_Player;
        ServerSentryEvent ev = ServerSentryBuildActionLifecycle.CreateBaseBuildingSnapshot(
            player,
            actionData,
            actionName,
            displayName,
            actionClass,
            actionKind,
            sourceHook);

        if (ev)
        {
            ev.mod = "BaseBuildingPlus";
            ev.action.reason = "bbp_bridge";
        }

        return ev;
    }

    static ServerSentryEvent CreateBBPPlacement(ActionData actionData, string sourceHook)
    {
        if (!Enabled() || !ServerSentryBBPConsts.LOG_PLACEMENT || !IsBBPAction(actionData))
        {
            return null;
        }

        PlaceObjectActionData placementData = PlaceObjectActionData.Cast(actionData);
        if (!placementData || !placementData.m_AlreadyPlaced)
        {
            return null;
        }

        PlayerBase player = actionData.m_Player;
        EntityAI placedEntity = EntityAI.Cast(actionData.m_MainItem);
        if (!player || !placedEntity)
        {
            return null;
        }

        ServerSentryEvent ev = ServerSentryBuildActionLifecycle.CreatePlacementSnapshot(
            player,
            actionData,
            placedEntity,
            placementData.m_Position,
            placementData.m_Orientation,
            "ActionDeployObject",
            sourceHook);

        if (ev)
        {
            ev.mod = "BaseBuildingPlus";
            ev.action.reason = "bbp_bridge_optional_place";
        }

        return ev;
    }

    static void Emit(ServerSentryEvent ev)
    {
        ServerSentryBuildActionLifecycle.EmitSnapshot(ev);
    }
};

modded class ActionBuildPart
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryEvent ssEvent;
        bool adminBuild = ServerSentryBBPBridge.IsAdminBuild(action_data);

        if (ServerSentryBBPBridge.BaseBuildingAllowed())
        {
            if (adminBuild && ServerSentryBBPConsts.LOG_ADMIN_BUILD_PART)
            {
                ssEvent = ServerSentryBBPBridge.CreateBBPAction(action_data, "basebuilding_admin_build_part", "Admin build BBP part", "ActionBuildPart", "build_part", "BBP.ActionBuildPart.OnFinishProgressServer");

                if (ssEvent)
                {
                    ssEvent.action.reason = "bbp_admin_tool";
                }
            }
            else if (!adminBuild && ServerSentryBBPConsts.LOG_BUILD_PART)
            {
                ssEvent = ServerSentryBBPBridge.CreateBBPAction(action_data, "basebuilding_build_part", "Build BBP part", "ActionBuildPart", "build_part", "BBP.ActionBuildPart.OnFinishProgressServer");
            }
        }

        super.OnFinishProgressServer(action_data);
        ServerSentryBBPBridge.Emit(ssEvent);
    }
};

modded class ActionDismantleBBP
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryEvent ssEvent;
        if (ServerSentryBBPConsts.LOG_DISMANTLE && ServerSentryBBPBridge.DismantleAllowed())
        {
            ssEvent = ServerSentryBBPBridge.CreateBBPAction(
                action_data,
                "basebuilding_dismantle_bbp",
                "Dismantle BBP part",
                "ActionDismantleBBP",
                "dismantle_bbp",
                "BBP.ActionDismantleBBP.OnFinishProgressServer");
        }

        super.OnFinishProgressServer(action_data);
        ServerSentryBBPBridge.Emit(ssEvent);
    }
};

modded class ActionDestroyBBP
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryEvent ssEvent;
        if (ServerSentryBBPConsts.LOG_DESTROY && ServerSentryBBPBridge.DestroyAllowed())
        {
            ssEvent = ServerSentryBBPBridge.CreateBBPAction(
                action_data,
                "basebuilding_destroy_bbp",
                "Destroy BBP part",
                "ActionDestroyBBP",
                "destroy_bbp",
                "BBP.ActionDestroyBBP.OnFinishProgressServer");
        }

        super.OnFinishProgressServer(action_data);
        ServerSentryBBPBridge.Emit(ssEvent);
    }
};

modded class ActionFoldBBP
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        ServerSentryEvent ssEvent;
        if (ServerSentryBBPConsts.LOG_FOLD && ServerSentryBBPBridge.BaseBuildingAllowed())
        {
            ssEvent = ServerSentryBBPBridge.CreateBBPAction(
                action_data,
                "basebuilding_fold_bbp",
                "Fold BBP object",
                "ActionFoldBBP",
                "fold_bbp",
                "BBP.ActionFoldBBP.OnFinishProgressServer");
        }

        super.OnFinishProgressServer(action_data);
        ServerSentryBBPBridge.Emit(ssEvent);
    }
};

modded class ActionDeployObject
{
    override void OnEndServer(ActionData action_data)
    {
        // Reserve the BBP-tagged snapshot before the shared ServerSentry core
        // hook executes. The shared lifecycle deduplicator then prevents a
        // second generic placement event for the same operation.
        ServerSentryEvent ssEvent = ServerSentryBBPBridge.CreateBBPPlacement(
            action_data,
            "BBP.ActionDeployObject.OnEndServer");

        super.OnEndServer(action_data);
        ServerSentryBBPBridge.Emit(ssEvent);
    }
};
