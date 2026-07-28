class ServerSentryExpansionSafeZoneResolver
{
    static ServerSentrySafeZoneTransitionData BuildData(PlayerBase player, string transitionType, string reason, bool wasInside, bool isInside)
    {
        ServerSentrySafeZoneTransitionData data = new ServerSentrySafeZoneTransitionData();
        data.sourceMod = "DayZExpansion_Core";
        data.transitionType = transitionType;
        data.result = "success";
        data.reason = reason;
        data.zoneClassName = "ExpansionSafeZone";
        // ServerSentry 1.4.8 exposes these two legacy boolean fields without
        // an "unknown" state. The bridge deliberately does not fabricate
        // values from unstable/private Expansion settings members.
        data.settingsPath = "ExpansionMod/Settings/SafeZoneSettings.json";
        data.wasInside = wasInside;
        data.isInside = isInside;

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

            FillMatchingZone(data, playerPosition);
        }

        return data;
    }

    static void Emit(PlayerBase player, string transitionType, string reason, bool wasInside, bool isInside)
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        if (!ServerSentryDefaults.LogExpansionSafeZoneEvents())
        {
            return;
        }

        if (!player || !player.GetIdentity())
        {
            return;
        }

#ifdef ENFUSION_AI_PROJECT
        if (player.IsAI())
        {
            return;
        }
#endif

        ServerSentrySafeZoneTransitionData data = BuildData(player, transitionType, reason, wasInside, isInside);
        ServerSentryApi.EmitSafeZoneTransition(data, DayZPlayer.Cast(player));
    }

    protected static void FillMatchingZone(ServerSentrySafeZoneTransitionData data, vector playerPosition)
    {
        if (!data)
        {
            return;
        }

        ExpansionSettings expansionSettings = GetExpansionSettings();
        if (!expansionSettings)
        {
            FillFallbackZone(data, playerPosition);
            return;
        }

        ExpansionSafeZoneSettings safeZoneSettings = expansionSettings.GetSafeZone(false);
        if (!safeZoneSettings)
        {
            FillFallbackZone(data, playerPosition);
            return;
        }

        float bestScore = 99999999.0;
        int globalIndex = 0;

        if (safeZoneSettings.CircleZones)
        {
            foreach (ExpansionSafeZoneCircle circleZone: safeZoneSettings.CircleZones)
            {
                if (circleZone)
                {
                    float distance = Distance2D(playerPosition, circleZone.Center);
                    bool inside = distance <= circleZone.Radius;
                    float score = ScoreZone(distance, inside);
                    if (score < bestScore)
                    {
                        bestScore = score;
                        data.zoneIndex = globalIndex;
                        data.zoneName = "Expansion circle safezone #" + globalIndex.ToString();
                        data.zoneClassName = "ExpansionSafeZoneCircle";
                        data.zonePosition.FromVector(circleZone.Center);
                        data.zoneRadius = circleZone.Radius;
                        data.distanceToCenter = distance;
                    }
                }

                globalIndex++;
            }
        }

        if (safeZoneSettings.CylinderZones)
        {
            foreach (ExpansionSafeZoneCylinder cylinderZone: safeZoneSettings.CylinderZones)
            {
                if (cylinderZone)
                {
                    float cylinderDistance = Distance2D(playerPosition, cylinderZone.Center);
                    bool cylinderInside = cylinderDistance <= cylinderZone.Radius && playerPosition[1] >= cylinderZone.Center[1] && playerPosition[1] <= cylinderZone.Center[1] + cylinderZone.Height;
                    float cylinderScore = ScoreZone(cylinderDistance, cylinderInside);
                    if (cylinderScore < bestScore)
                    {
                        bestScore = cylinderScore;
                        data.zoneIndex = globalIndex;
                        data.zoneName = "Expansion cylinder safezone #" + globalIndex.ToString();
                        data.zoneClassName = "ExpansionSafeZoneCylinder";
                        data.zonePosition.FromVector(cylinderZone.Center);
                        data.zoneRadius = cylinderZone.Radius;
                        data.distanceToCenter = cylinderDistance;
                    }
                }

                globalIndex++;
            }
        }

        if (safeZoneSettings.PolygonZones)
        {
            foreach (ExpansionSafeZonePolygon polygonZone: safeZoneSettings.PolygonZones)
            {
                if (polygonZone && polygonZone.Positions && polygonZone.Positions.Count() > 0)
                {
                    vector polygonCenter = GetPolygonCenter(polygonZone.Positions);
                    float polygonRadius = GetPolygonRadius(polygonZone.Positions, polygonCenter);
                    float polygonDistance = Distance2D(playerPosition, polygonCenter);
                    bool polygonInside = IsInsidePolygon2D(playerPosition, polygonZone.Positions);
                    float polygonScore = ScoreZone(polygonDistance, polygonInside);
                    if (polygonScore < bestScore)
                    {
                        bestScore = polygonScore;
                        data.zoneIndex = globalIndex;
                        data.zoneName = "Expansion polygon safezone #" + globalIndex.ToString();
                        data.zoneClassName = "ExpansionSafeZonePolygon";
                        data.zonePosition.FromVector(polygonCenter);
                        data.zoneRadius = polygonRadius;
                        data.distanceToCenter = polygonDistance;
                    }
                }

                globalIndex++;
            }
        }

        if (data.zoneName == "")
        {
            FillFallbackZone(data, playerPosition);
        }
    }

    protected static void FillFallbackZone(ServerSentrySafeZoneTransitionData data, vector playerPosition)
    {
        data.zoneIndex = -1;
        data.zoneName = "Expansion Safezone @ " + playerPosition.ToString();
        data.zoneClassName = "ExpansionSafeZone";
        data.zonePosition.FromVector(playerPosition);
        data.zoneRadius = 0;
        data.distanceToCenter = 0;
    }

    protected static float ScoreZone(float distance, bool inside)
    {
        if (inside)
        {
            return distance;
        }

        return distance + 1000000.0;
    }

    protected static float Distance2D(vector a, vector b)
    {
        vector delta = a - b;
        float dx = delta[0];
        float dz = delta[2];
        return Math.Sqrt((dx * dx) + (dz * dz));
    }

    protected static vector GetPolygonCenter(TVectorArray positions)
    {
        vector center = vector.Zero;
        if (!positions || positions.Count() == 0)
        {
            return center;
        }

        for (int i = 0; i < positions.Count(); i++)
        {
            center = center + positions.Get(i);
        }

        center[0] = center[0] / positions.Count();
        center[1] = center[1] / positions.Count();
        center[2] = center[2] / positions.Count();
        return center;
    }

    protected static float GetPolygonRadius(TVectorArray positions, vector center)
    {
        float radius = 0;
        if (!positions)
        {
            return radius;
        }

        for (int i = 0; i < positions.Count(); i++)
        {
            float distance = Distance2D(center, positions.Get(i));
            if (distance > radius)
            {
                radius = distance;
            }
        }

        return radius;
    }

    protected static bool IsInsidePolygon2D(vector position, TVectorArray positions)
    {
        if (!positions || positions.Count() < 3)
        {
            return false;
        }

        bool inside = false;
        int j = positions.Count() - 1;
        float x = position[0];
        float z = position[2];

        for (int i = 0; i < positions.Count(); i++)
        {
            vector pi = positions.Get(i);
            vector pj = positions.Get(j);

            float xi = pi[0];
            float zi = pi[2];
            float xj = pj[0];
            float zj = pj[2];

            if ((zi > z) != (zj > z))
            {
                float intersectX = (xj - xi) * (z - zi) / (zj - zi) + xi;
                if (x < intersectX)
                {
                    inside = !inside;
                }
            }

            j = i;
        }

        return inside;
    }
}

class ServerSentryExpansionSafeZoneLiveMarkers
{
    const static string MARKER_PREFIX = "expansion-safezone-";
    protected static int s_RefreshIntervalMs;
    protected static bool s_Started;

    static void Start()
    {
        if (s_Started)
        {
            return;
        }

        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        s_Started = true;
        s_RefreshIntervalMs = 60000;

        if (ServerSentryConfigManager.Get())
        {
            int configuredDelay = ServerSentryConfigManager.Get().GetLiveUpdatePollingDelay();
            if (configuredDelay > s_RefreshIntervalMs)
            {
                s_RefreshIntervalMs = configuredDelay;
            }
        }

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Refresh, 5000, false);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Refresh, s_RefreshIntervalMs, true);
    }

    static void Refresh()
    {
        if (!GetGame() || !GetGame().IsServer())
        {
            return;
        }

        if (!ServerSentryDefaults.LogExpansionSafeZoneEvents())
        {
            ServerSentryLiveApi.ClearMarkersByPrefix(MARKER_PREFIX);
            return;
        }

        ServerSentryConfigManager configManager = ServerSentryConfigManager.Get();
        if (!configManager || !configManager.IsAllowedToStartLiveLogging())
        {
            ServerSentryLiveApi.ClearMarkersByPrefix(MARKER_PREFIX);
            return;
        }

        ExpansionSettings expansionSettings = GetExpansionSettings();
        if (!expansionSettings)
        {
            ServerSentryLiveApi.ClearMarkersByPrefix(MARKER_PREFIX);
            return;
        }

        ExpansionSafeZoneSettings safeZoneSettings = expansionSettings.GetSafeZone(false);
        if (!safeZoneSettings)
        {
            ServerSentryLiveApi.ClearMarkersByPrefix(MARKER_PREFIX);
            return;
        }

        ServerSentryLiveApi.ClearMarkersByPrefix(MARKER_PREFIX);
        int globalIndex = 0;

        if (safeZoneSettings.CircleZones)
        {
            foreach (ExpansionSafeZoneCircle circleZone: safeZoneSettings.CircleZones)
            {
                if (circleZone)
                {
                    UpsertCircleZone(circleZone, globalIndex);
                }

                globalIndex++;
            }
        }

        if (safeZoneSettings.CylinderZones)
        {
            foreach (ExpansionSafeZoneCylinder cylinderZone: safeZoneSettings.CylinderZones)
            {
                if (cylinderZone)
                {
                    UpsertCylinderZone(cylinderZone, globalIndex);
                }

                globalIndex++;
            }
        }

        if (safeZoneSettings.PolygonZones)
        {
            foreach (ExpansionSafeZonePolygon polygonZone: safeZoneSettings.PolygonZones)
            {
                if (polygonZone && polygonZone.Positions && polygonZone.Positions.Count() >= 3)
                {
                    UpsertPolygonZone(polygonZone, globalIndex);
                }

                globalIndex++;
            }
        }
    }

    protected static void UpsertCircleZone(ExpansionSafeZoneCircle circleZone, int index)
    {
        ServerSentryLiveWorldEventDto marker = ServerSentryLiveApi.CreateMarker(MARKER_PREFIX + index.ToString(), "Expansion circle safezone #" + index.ToString(), circleZone.Center, circleZone.Radius, "ShieldCheck");
        marker.eventType = "safezone";
        marker.className = "ExpansionSafeZoneCircle";
        marker.shape = "circle";
        AddCommonDetails(marker, index, "Circle");
        ServerSentryLiveApi.AddMarkerDetail(marker, "radius", "Radius", Math.Round(circleZone.Radius).ToString() + " m");
        ServerSentryLiveApi.UpsertMarker(marker);
    }

    protected static void UpsertCylinderZone(ExpansionSafeZoneCylinder cylinderZone, int index)
    {
        ServerSentryLiveWorldEventDto marker = ServerSentryLiveApi.CreateMarker(MARKER_PREFIX + index.ToString(), "Expansion cylinder safezone #" + index.ToString(), cylinderZone.Center, cylinderZone.Radius, "ShieldCheck");
        marker.eventType = "safezone";
        marker.className = "ExpansionSafeZoneCylinder";
        marker.shape = "circle";
        AddCommonDetails(marker, index, "Cylinder");
        ServerSentryLiveApi.AddMarkerDetail(marker, "radius", "Radius", Math.Round(cylinderZone.Radius).ToString() + " m");
        ServerSentryLiveApi.AddMarkerDetail(marker, "height", "Height", Math.Round(cylinderZone.Height).ToString() + " m");
        ServerSentryLiveApi.UpsertMarker(marker);
    }

    protected static void UpsertPolygonZone(ExpansionSafeZonePolygon polygonZone, int index)
    {
        ServerSentryLiveWorldEventDto marker = ServerSentryLiveApi.CreatePolygonMarker(MARKER_PREFIX + index.ToString(), "Expansion polygon safezone #" + index.ToString(), polygonZone.Positions, "ShieldCheck");
        marker.eventType = "safezone";
        marker.className = "ExpansionSafeZonePolygon";
        AddCommonDetails(marker, index, "Polygon");
        ServerSentryLiveApi.AddMarkerDetail(marker, "points", "Points", polygonZone.Positions.Count().ToString());
        ServerSentryLiveApi.AddMarkerDetail(marker, "radius", "Approx. radius", Math.Round(marker.radius).ToString() + " m");
        ServerSentryLiveApi.UpsertMarker(marker);
    }

    protected static void AddCommonDetails(ServerSentryLiveWorldEventDto marker, int index, string shapeLabel)
    {
        ServerSentryLiveApi.AddMarkerDetail(marker, "source", "Source", "DayZ Expansion Core");
        ServerSentryLiveApi.AddMarkerDetail(marker, "settings", "Settings", "ExpansionMod/Settings/SafeZoneSettings.json");
        ServerSentryLiveApi.AddMarkerDetail(marker, "index", "Zone #", index.ToString());
        ServerSentryLiveApi.AddMarkerDetail(marker, "shape", "Shape", shapeLabel);
    }
}

modded class PlayerBase
{
    override void OnEnterSafeZone()
    {
        bool wasInside = Expansion_IsInSafeZone();
        super.OnEnterSafeZone();
        bool isInside = Expansion_IsInSafeZone();

        if (!wasInside && isInside)
        {
            ServerSentryExpansionSafeZoneResolver.Emit(this, "enter", "expansion_on_enter_safezone", wasInside, isInside);
        }
    }

    override void OnLeftSafeZone()
    {
        bool wasInside = Expansion_IsInSafeZone();
        super.OnLeftSafeZone();
        bool isInside = Expansion_IsInSafeZone();

        if (wasInside && !isInside)
        {
            ServerSentryExpansionSafeZoneResolver.Emit(this, "leave", "expansion_on_left_safezone", wasInside, isInside);
        }
    }
}
