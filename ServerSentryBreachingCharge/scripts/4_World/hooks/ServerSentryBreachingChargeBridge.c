class ServerSentryBreachingChargeBridge
{
    static bool Enabled()
    {
        return ServerSentryBreachingChargeConsts.ENABLED && ServerSentryBreachingChargeConsts.LOG_BREACHING_CHARGE_EVENTS && ServerSentryDefaults.LogBreachingChargeEvents();
    }

    static bool ShouldLogAction(string action)
    {
        if (!Enabled())
        {
            return false;
        }

        if (action == "place")
        {
            return ServerSentryBreachingChargeConsts.LOG_PLACE_EVENTS;
        }
        if (action == "arm")
        {
            return ServerSentryBreachingChargeConsts.LOG_ARM_EVENTS;
        }
        if (action == "defuse")
        {
            return ServerSentryBreachingChargeConsts.LOG_DEFUSE_EVENTS;
        }
        if (action == "explode")
        {
            return ServerSentryBreachingChargeConsts.LOG_EXPLODE_EVENTS;
        }
        if (action == "target_damage")
        {
            return ServerSentryBreachingChargeConsts.LOG_TARGET_DAMAGE_EVENTS;
        }
        if (action == "target_destroyed")
        {
            return ServerSentryBreachingChargeConsts.LOG_TARGET_DESTROYED_EVENTS;
        }
        if (action == "player_damage")
        {
            return ServerSentryBreachingChargeConsts.LOG_PLAYER_DAMAGE_EVENTS;
        }
        if (action == "charge_delete")
        {
            return ServerSentryBreachingChargeConsts.LOG_CHARGE_DELETE_EVENTS;
        }
        if (action == "legacy_target_destroyed")
        {
            return ServerSentryBreachingChargeConsts.LOG_LEGACY_ON_TARGET_BEING_DESTROYED;
        }

        return true;
    }

    static void EmitChargeLifecycle(HDSN_BreachingChargeBase charge, string action, PlayerBase actor, PlayerBase defusingPlayer, string sourceHook, string reason = "")
    {
        if (!ShouldLogAction(action) || !charge)
        {
            return;
        }

        ServerSentryBreachingChargeEventData data = BuildBaseData(charge, actor, sourceHook, action, reason);
        FillPlayer(data, charge.GetPlacingPlayer(), "placing");
        FillPlayer(data, defusingPlayer, "defusing");
        FillTarget(data, charge.GetTarget());
        data.targetDestroyed = false;

        EmitData(data, actor);
    }

    static void EmitTargetDamage(Object target, HDSN_BreachingChargeBase charge, int healthBefore, int healthAfter, string sourceHook)
    {
        if (!ShouldLogAction("target_damage") || !charge || !target)
        {
            return;
        }

        PlayerBase placingPlayer = charge.GetPlacingPlayer();
        ServerSentryBreachingChargeEventData data = BuildBaseData(charge, placingPlayer, sourceHook, "target_damage", "explosion_damage");
        FillPlayer(data, placingPlayer, "placing");
        FillTarget(data, target);
        data.targetHealthBefore = healthBefore;
        data.targetHealthAfter = healthAfter;
        data.targetHealthKnown = true;
        data.targetDestroyed = healthAfter <= 0;

        EmitData(data, placingPlayer);
    }

    static ServerSentryBreachingChargeEventData PrepareTargetDestroyed(Object target, PlayerBase placingPlayer, HDSN_BreachingChargeBase charge, string sourceHook)
    {
        if (!ShouldLogAction("target_destroyed") || !charge || !target)
        {
            return null;
        }

        // Capture object metadata before the third-party method may delete it.
        // The prepared event is emitted only after DestroyTarget returns.
        ServerSentryBreachingChargeEventData data = BuildBaseData(charge, placingPlayer, sourceHook, "target_destroyed", "target_destroyed_by_breaching_charge");
        FillPlayer(data, placingPlayer, "placing");
        FillTarget(data, target);
        data.targetDestroyed = true;
        return data;
    }

    static void EmitPlayerDamageEstimate(PlayerBase targetPlayer, HDSN_BreachingChargeBase charge, float healthBefore, float healthAfter, float distance, string sourceHook)
    {
        if (!ShouldLogAction("player_damage") || !charge || !targetPlayer)
        {
            return;
        }

        PlayerBase placingPlayer = charge.GetPlacingPlayer();
        ServerSentryBreachingChargeEventData data = BuildBaseData(charge, placingPlayer, sourceHook, "player_damage", "pre_explosion_damage_estimate");
        data.result = "estimated";
        FillPlayer(data, placingPlayer, "placing");
        FillPlayer(data, targetPlayer, "affected");
        data.targetType = "player";
        data.targetClassName = targetPlayer.GetType();
        data.targetDisplayName = data.affectedPlayerName;
        data.targetPosition.FromVector(targetPlayer.GetPosition());
        data.playerHealthBefore = healthBefore;
        data.playerHealthAfter = healthAfter;
        data.playerDamage = healthBefore - healthAfter;
        data.distanceToCharge = distance;

        EmitData(data, placingPlayer);
    }

    static ServerSentryBreachingChargeEventData PrepareChargeDelete(HDSN_BreachingChargeBase charge, string sourceHook)
    {
        if (!ShouldLogAction("charge_delete") || !charge)
        {
            return null;
        }

        PlayerBase placingPlayer = charge.GetPlacingPlayer();
        ServerSentryBreachingChargeEventData data = BuildBaseData(charge, placingPlayer, sourceHook, "charge_delete", "charge_object_delete");
        FillPlayer(data, placingPlayer, "placing");
        FillTarget(data, charge.GetTarget());
        return data;
    }

    static ServerSentryBreachingChargeEventData BuildBaseData(HDSN_BreachingChargeBase charge, PlayerBase actor, string sourceHook, string action, string reason = "")
    {
        ServerSentryBreachingChargeEventData data = new ServerSentryBreachingChargeEventData();
        data.sourceMod = "HDSN_BreachingCharge";
        data.breachAction = action;
        data.result = "success";
        data.reason = reason;
        data.sourceHook = sourceHook;

        FillPlayer(data, actor, "actor");
        FillCharge(data, charge);
        return data;
    }

    static void FillCharge(ServerSentryBreachingChargeEventData data, HDSN_BreachingChargeBase charge)
    {
        if (!data || !charge)
        {
            return;
        }

        data.chargeClassName = charge.GetType();
        data.chargeDisplayName = GetObjectDisplayName(charge);
        data.chargeObjectId = ObjectId(charge);
        data.chargePosition.FromVector(charge.GetPosition());
        data.chargeOrientation.FromVector(charge.GetOrientation());
        data.chargeHealth = charge.GetHealth();
        data.chargeState = charge.GetChargeState();
        data.chargeStateName = ResolveStateName(data.chargeState);
        data.chargeTargetType = charge.GetChargeTargetType();

        data.chargeDamage = charge.GetChargeDamage();
        data.damageToObjectsRadius = charge.GetDamageToDestroyableObjectsRadius();
        data.damageToPlayersRadius = charge.GetDamageToPlayersRadius();
        data.minDamageToPlayers = charge.GetMinDamageToPlayers();
        data.maxDamageToPlayers = charge.GetMaxDamageToPlayers();
        data.maxDamageToPlayersRadius = charge.GetMaxDamageToPlayersRadius();
        data.timeToPlant = charge.GetTimeToPlant();
        data.timeToDefuse = charge.GetTimeToDefuse();
        data.chargeTime = charge.GetChargeTimeLeft();
        data.automaticallyArm = charge.GetAutomaticallyArm();
        data.deleteObjectsDirectly = charge.GetDeleteObjectsDirectly();
        data.onlyDestroyLocks = charge.GetOnlyDestroyLocks();
        data.destroyOtherCharges = charge.GetDestroyOtherCharges();
    }

    static void FillTarget(ServerSentryBreachingChargeEventData data, Object target)
    {
        if (!data || !target)
        {
            return;
        }

        data.targetType = "breach_target";
        data.targetClassName = target.GetType();
        data.targetDisplayName = GetObjectDisplayName(target);
        data.targetObjectId = ObjectId(target);
        data.targetPosition.FromVector(target.GetPosition());
        data.targetOrientation.FromVector(target.GetOrientation());

        EntityAI ent = EntityAI.Cast(target);
        if (ent)
        {
            data.targetHealthAfter = ent.GetHealth();
            data.targetHealthKnown = true;
        }
    }

    static void FillPlayer(ServerSentryBreachingChargeEventData data, PlayerBase player, string slot)
    {
        if (!data || !player)
        {
            return;
        }

        string steamId = "";
        string bohemiaId = "";
        string name = "";
        if (player.GetIdentity())
        {
            steamId = player.GetIdentity().GetPlainId();
            bohemiaId = player.GetIdentity().GetId();
            name = player.GetIdentity().GetName();
        }

        if (slot == "actor")
        {
            data.actorSteamId = steamId;
            data.actorBohemiaId = bohemiaId;
            data.actorName = name;
            data.actorPosition.FromVector(player.GetPosition());
        }
        else if (slot == "placing")
        {
            data.placingSteamId = steamId;
            data.placingBohemiaId = bohemiaId;
            data.placingName = name;
            data.placingPosition.FromVector(player.GetPosition());
        }
        else if (slot == "defusing")
        {
            data.defusingSteamId = steamId;
            data.defusingBohemiaId = bohemiaId;
            data.defusingName = name;
            data.defusingPosition.FromVector(player.GetPosition());
        }
        else if (slot == "affected")
        {
            data.affectedPlayerSteamId = steamId;
            data.affectedPlayerBohemiaId = bohemiaId;
            data.affectedPlayerName = name;
            data.affectedPlayerPosition.FromVector(player.GetPosition());
        }
    }

    static string GetObjectDisplayName(Object obj)
    {
        if (!obj)
        {
            return "";
        }

        EntityAI ent = EntityAI.Cast(obj);
        if (ent)
        {
            string name = ent.GetDisplayName();
            if (name != "")
            {
                return name;
            }
        }

        return obj.GetType();
    }

    static string ObjectId(Object obj)
    {
        if (!obj)
        {
            return "";
        }

        return obj.ToString();
    }

    static string ResolveStateName(int state)
    {
        if (state == HDSN_BreachingChargeStates.NOT_PLACED)
        {
            return "not_placed";
        }
        if (state == HDSN_BreachingChargeStates.IS_PLACED)
        {
            return "placed";
        }
        if (state == HDSN_BreachingChargeStates.IS_ARMED)
        {
            return "armed";
        }
        if (state == HDSN_BreachingChargeStates.HAS_EXPLODED)
        {
            return "exploded";
        }
        if (state == HDSN_BreachingChargeStates.IS_DEFUSED)
        {
            return "defused";
        }

        return "unknown";
    }

    static void EmitData(ServerSentryBreachingChargeEventData data, PlayerBase player)
    {
        if (!data)
        {
            return;
        }

        string sig = data.breachAction + "|" + data.chargeObjectId + "|" + data.targetObjectId + "|" + data.actorSteamId + "|" + data.sourceHook;
        if (!ServerSentryBreachingChargeDeduper.ShouldEmit(sig))
        {
            return;
        }

        ServerSentryApi.EmitBreachingChargeEvent(data, DayZPlayer.Cast(player));
    }

    static void EmitPlayerDamageEstimatesBeforeExplosion(HDSN_BreachingChargeBase charge, string sourceHook)
    {
        if (!ShouldLogAction("player_damage") || !charge)
        {
            return;
        }

        float damageToPlayersRadius = charge.GetDamageToPlayersRadius();
        float maxDamageToPlayersRadius = charge.GetMaxDamageToPlayersRadius();
        float maxDamageToPlayers = charge.GetMaxDamageToPlayers();
        float minDamageToPlayers = charge.GetMinDamageToPlayers();
        float maxVerticalDistancePlayer = charge.GetMaxVerticalDistancePlayers();

        if (damageToPlayersRadius < 0.01 || maxDamageToPlayers < 0.01)
        {
            return;
        }

        if (damageToPlayersRadius < maxDamageToPlayersRadius)
        {
            maxDamageToPlayersRadius = damageToPlayersRadius;
        }
        if (maxDamageToPlayers < minDamageToPlayers)
        {
            minDamageToPlayers = maxDamageToPlayers;
        }

        array<Object> objects = new array<Object>();
        array<CargoBase> cargos = new array<CargoBase>();
        GetGame().GetObjectsAtPosition3D(charge.GetPosition(), damageToPlayersRadius, objects, cargos);

        foreach (Object obj: objects)
        {
            PlayerBase player = PlayerBase.Cast(obj);
            if (!player)
            {
                continue;
            }

            float distance = vector.Distance(player.GetPosition(), charge.GetPosition());
            if (distance > damageToPlayersRadius)
            {
                continue;
            }

            if (maxVerticalDistancePlayer > 0.01)
            {
                if (AbsFloat(charge.GetPosition()[1] - player.GetPosition()[1]) > maxVerticalDistancePlayer)
                {
                    continue;
                }
            }

            float healthBefore = player.GetHealth();
            float damage = EstimatePlayerDamage(distance, damageToPlayersRadius, maxDamageToPlayersRadius, maxDamageToPlayers, minDamageToPlayers);
            float healthAfter = healthBefore - damage;
            if (healthAfter < 0)
            {
                healthAfter = 0;
            }

            if (damage > 0)
            {
                EmitPlayerDamageEstimate(player, charge, healthBefore, healthAfter, distance, sourceHook);
            }
        }
    }

    static float EstimatePlayerDamage(float distance, float damageToPlayersRadius, float maxDamageToPlayersRadius, float maxDamageToPlayers, float minDamageToPlayers)
    {
        if (distance <= maxDamageToPlayersRadius)
        {
            return maxDamageToPlayers;
        }

        float scalingDistance = damageToPlayersRadius - maxDamageToPlayersRadius;
        if (scalingDistance < 0.01)
        {
            return maxDamageToPlayers;
        }

        float distanceToChargeWithOffset = distance - maxDamageToPlayersRadius;
        float distancePercentage = distanceToChargeWithOffset / scalingDistance;
        return maxDamageToPlayers - (maxDamageToPlayers - minDamageToPlayers) * distancePercentage;
    }

    static float AbsFloat(float value)
    {
        if (value < 0)
        {
            return value * -1.0;
        }

        return value;
    }
}

modded class HDSN_BreachingChargeBase
{
    override void OnChargePlaced()
    {
        super.OnChargePlaced();
        ServerSentryBreachingChargeBridge.EmitChargeLifecycle(this, "place", GetPlacingPlayer(), null, "HDSN_BreachingChargeBase.OnChargePlaced", "charge_placed");
    }

    override void OnChargeArmed()
    {
        super.OnChargeArmed();
        ServerSentryBreachingChargeBridge.EmitChargeLifecycle(this, "arm", GetPlacingPlayer(), null, "HDSN_BreachingChargeBase.OnChargeArmed", "charge_armed");
    }

    override void OnChargeDefused(PlayerBase defusingPlayer)
    {
        super.OnChargeDefused(defusingPlayer);
        ServerSentryBreachingChargeBridge.EmitChargeLifecycle(this, "defuse", defusingPlayer, defusingPlayer, "HDSN_BreachingChargeBase.OnChargeDefused", "charge_defused");
    }

    override void OnChargeExploded()
    {
        ServerSentryBreachingChargeBridge.EmitPlayerDamageEstimatesBeforeExplosion(this, "HDSN_BreachingChargeBase.OnChargeExploded");
        super.OnChargeExploded();
        ServerSentryBreachingChargeBridge.EmitChargeLifecycle(this, "explode", GetPlacingPlayer(), null, "HDSN_BreachingChargeBase.OnChargeExploded", "charge_exploded");
    }

    override void OnTargetBeingDestroyed()
    {
        super.OnTargetBeingDestroyed();

        if (ServerSentryBreachingChargeBridge.ShouldLogAction("legacy_target_destroyed"))
        {
            ServerSentryBreachingChargeBridge.EmitChargeLifecycle(this, "legacy_target_destroyed", GetPlacingPlayer(), null, "HDSN_BreachingChargeBase.OnTargetBeingDestroyed", "legacy_charge_target_destroyed");
        }
    }

    override void EEDelete(EntityAI parent)
    {
        PlayerBase placingPlayer = GetPlacingPlayer();
        ServerSentryBreachingChargeEventData prepared = ServerSentryBreachingChargeBridge.PrepareChargeDelete(this, "HDSN_BreachingChargeBase.EEDelete");
        super.EEDelete(parent);
        if (prepared)
        {
            ServerSentryBreachingChargeBridge.EmitData(prepared, placingPlayer);
        }
    }
}

modded class HDSN_DestructionManager
{
    override int HandleHealth(Object target, HDSN_BreachingChargeBase charge)
    {
        int healthBefore = -1;
        if (target)
        {
            healthBefore = GetTargetHealth(target);
        }

        int healthAfter = super.HandleHealth(target, charge);

        if (healthAfter >= 0)
        {
            ServerSentryBreachingChargeBridge.EmitTargetDamage(target, charge, healthBefore, healthAfter, "HDSN_DestructionManager.HandleHealth");
        }

        return healthAfter;
    }

    override void DestroyTarget(Object target, PlayerBase placingPlayer, HDSN_BreachingChargeBase charge)
    {
        ServerSentryBreachingChargeEventData prepared = ServerSentryBreachingChargeBridge.PrepareTargetDestroyed(target, placingPlayer, charge, "HDSN_DestructionManager.DestroyTarget");
        super.DestroyTarget(target, placingPlayer, charge);

        // Reaching this point confirms that the original destruction handler
        // completed normally. The target metadata was captured beforehand.
        if (prepared)
        {
            ServerSentryBreachingChargeBridge.EmitData(prepared, placingPlayer);
        }
    }
}
