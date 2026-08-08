/**
	\file ServerSentryDrJonesSafeZone.c
	\brief Connects ServerSentry to DayZ or third-party hooks for Dr jones safe zone.
*/

#ifdef TRADER
/**
	\brief Provides ServerSentry behavior for Dr jones safe zone resolver.
*/
class ServerSentryDrJonesSafeZoneResolver
{
    /**
    	\brief Builds data.
    	\param player \p player entity associated with the operation.
    	\param trigger \p trigger used by the operation.
    	\param transitionType \p transition type used by the operation.
    	\param reason \p human-readable operation description stored with fulfillment state.
    	\return \p The resulting safe zone transition data instance, or null when unavailable.
    */
    static ServerSentrySafeZoneTransitionData BuildData(PlayerBase player, SafeZoneTrigger trigger, string transitionType, string reason)
    {
        ServerSentrySafeZoneTransitionData data = new ServerSentrySafeZoneTransitionData();
        data.sourceMod = "Trader";
        data.transitionType = transitionType;
        data.result = "success";
        data.reason = reason;
        data.zoneClassName = "DrJonesSafeZoneTrigger";
        data.safeZonePvPEnabled = false;
        data.killOnEntry = false;
        data.settingsPath = "Trader safezone trigger";
        data.wasInside = transitionType == "leave";
        data.isInside = transitionType == "enter";
        data.zoneIndex = -1;
        data.zoneName = "Dr Jones Trader Safezone";

        if (player)
        {
            vector playerPosition = player.GetPosition();
            data.actorPosition.FromVector(playerPosition);

            PlayerIdentity identity = player.GetIdentity();
            if (identity)
            {
                data.actorSteamId = identity.GetPlainId();
                data.actorBohemiaId = identity.GetId();
                data.actorName = identity.GetName();
            }
        }

        if (trigger)
        {
            vector zonePosition = trigger.GetPosition();
            data.zonePosition.FromVector(zonePosition);
            data.zoneRadius = trigger.GetCollisionRadius();

            if (player)
            {
                data.distanceToCenter = Distance2D(player.GetPosition(), zonePosition);
            }

            data.zoneName = "Dr Jones Trader Safezone @ " + zonePosition.ToString();
        }

        return data;
    }

    /**
    	\brief Processes the distance 2 d operation.
    	\param a \p a used by the operation.
    	\param b \p b used by the operation.
    	\return \p The resulting floating-point value.
    */
    static float Distance2D(vector a, vector b)
    {
        vector delta = a - b;
        float dx = delta[0];
        float dz = delta[2];
        return Math.Sqrt((dx * dx) + (dz * dz));
    }

    /**
    	\brief Emits the requested operation.
    	\param player \p player entity associated with the operation.
    	\param trigger \p trigger used by the operation.
    	\param transitionType \p transition type used by the operation.
    	\param reason \p human-readable operation description stored with fulfillment state.
    */
    static void Emit(PlayerBase player, SafeZoneTrigger trigger, string transitionType, string reason)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        if (!ServerSentryDefaults.LogDrJonesSafeZoneEvents())
        {
            return;
        }

        if (!player || !player.GetIdentity())
        {
            return;
        }

        ServerSentrySafeZoneTransitionData data = BuildData(player, trigger, transitionType, reason);
        ServerSentrySafeZoneEvent ev = ServerSentryEventFactory.CreateSafeZoneTransition(data);
        ServerSentryLogger.Get().EmitSafeZoneTransition(ev);
    }
}

/**
	\brief Extends the DayZ Safe zone trigger class with ServerSentry integration.
*/
modded class SafeZoneTrigger
{
    /**
    	\brief Handles the DayZ Enter server event callback.
    	\param insider \p insider used by the operation.
    */
    override void OnEnterServerEvent(TriggerInsider insider)
    {
        super.OnEnterServerEvent(insider);

        if (!insider)
        {
            return;
        }

        PlayerBase playerInsider = PlayerBase.Cast(insider.GetObject());
        if (!playerInsider)
        {
            return;
        }

#ifdef ENFUSION_AI_PROJECT
        if (playerInsider.IsAI())
        {
            return;
        }
#endif

        ServerSentryDrJonesSafeZoneResolver.Emit(playerInsider, this, "enter", "trigger_enter");
    }

    /**
    	\brief Handles the DayZ Leave server event callback.
    	\param insider \p insider used by the operation.
    */
    override void OnLeaveServerEvent(TriggerInsider insider)
    {
        super.OnLeaveServerEvent(insider);

        if (!insider)
        {
            return;
        }

        PlayerBase playerInsider = PlayerBase.Cast(insider.GetObject());
        if (!playerInsider)
        {
            return;
        }

#ifdef ENFUSION_AI_PROJECT
        if (playerInsider.IsAI())
        {
            return;
        }
#endif

        ServerSentryDrJonesSafeZoneResolver.Emit(playerInsider, this, "leave", "trigger_leave");
    }
}
#endif // TRADER
