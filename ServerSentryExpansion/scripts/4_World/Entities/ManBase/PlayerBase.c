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

/**
    \brief Retains zone geometry so unchanged settings do not rebuild map markers.
*/
class ServerSentryExpansionSafeZoneMarkerState
{
    /** \brief Expansion geometry type represented by this marker. */
    string shape;
    /** \brief Last exported circle or cylinder centre. */
    vector center;
    /** \brief Last exported circle or cylinder radius. */
    float radius;
    /** \brief Last exported cylinder height. */
    float height;
    /** \brief Owned copy of polygon points, independent of mutable settings. */
    ref array<vector> points;
    /** \brief Marker instance registered by this integration. */
    ref ServerSentryLiveWorldEventDto marker;
    /** \brief Whether the current refresh still contains this zone. */
    bool seen;

    /**
        \brief Compares typed geometry and detects removed or replaced registrations.
        \param shapeLabel \p Expansion shape name.
        \param position \p current centre.
        \param zoneRadius \p current radius.
        \param zoneHeight \p current cylinder height.
        \param positions \p current polygon points, or null for round zones.
        \return \p true when the registered marker already represents these settings.
    */
    bool Matches(string shapeLabel, vector position, float zoneRadius, float zoneHeight, array<vector> positions)
    {
        if (!ServerSentryMapMarkers.IsRegistered(marker))
        {
            return false;
        }

        if (shape != shapeLabel || center != position || radius != zoneRadius || height != zoneHeight)
        {
            return false;
        }

        if (!positions)
        {
            return !points;
        }

        if (!points || points.Count() != positions.Count())
        {
            return false;
        }

        for (int i = 0; i < points.Count(); i++)
        {
            if (points[i] != positions[i])
            {
                return false;
            }
        }

        return true;
    }
}

/**
    \brief Publishes changed Expansion safezones without rebuilding unchanged markers.
*/
class ServerSentryExpansionSafeZoneLiveMarkers
{
    /** \brief Namespace reserved for Expansion safezone markers. */
    const static string MARKER_PREFIX = "expansion-safezone-";
    /** \brief Settings reconciliation interval, independent of snapshot polling. */
    protected static int s_RefreshIntervalMs;
    /** \brief Prevents duplicate timers and ignores callbacks after shutdown. */
    protected static bool s_Started;
    /** \brief Prevents repeated registry clears while integration output is disabled. */
    protected static bool s_Cleared;
    /** \brief Last successfully registered geometry by stable settings index. */
    protected static ref map<int, ref ServerSentryExpansionSafeZoneMarkerState> s_Zones;

    /** \brief Starts bounded settings reconciliation once per mission. */
    static void Start()
    {
        if (s_Started || !GetGame() || !GetGame().IsServer())
        {
            return;
        }

        s_Started = true;
        s_Cleared = false;
        s_Zones = new map<int, ref ServerSentryExpansionSafeZoneMarkerState>();
        s_RefreshIntervalMs = 60000;

        ServerSentryConfigManager configManager = ServerSentryConfigManager.Get();
        if (configManager)
        {
            int configuredDelay = configManager.GetLiveUpdatePollingDelay();
            s_RefreshIntervalMs = Math.Max(s_RefreshIntervalMs, configuredDelay);
        }

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Refresh, 5000, false);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Refresh, s_RefreshIntervalMs, true);
    }

    /** \brief Removes callbacks and mission-owned markers without scheduling an export. */
    static void Stop()
    {
        s_Started = false;
        if (GetGame())
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Refresh);
        }

        ServerSentryMapMarkers.ClearByPrefix(MARKER_PREFIX);
        s_Zones = null;
        s_Cleared = false;
    }

    /** \brief Reconciles loaded settings and requests one export only when markers change. */
    static void Refresh()
    {
        if (!s_Started || !GetGame() || !GetGame().IsServer())
        {
            return;
        }

        ServerSentryConfigManager configManager = ServerSentryConfigManager.Get();
        if (!configManager)
        {
            return;
        }

        if (!ServerSentryDefaults.LogExpansionSafeZoneEvents() || !configManager.IsAllowedToStartLiveLogging())
        {
            ClearMarkers();
            return;
        }

        ExpansionSettings expansionSettings = GetExpansionSettings();
        if (!expansionSettings)
        {
            return;
        }

        ExpansionSafeZoneSettings safeZoneSettings = expansionSettings.GetSafeZone(false);
        if (!safeZoneSettings || !safeZoneSettings.IsLoaded())
        {
            // A temporary reload must not publish the disappearance of every zone.
            return;
        }

        if (!safeZoneSettings.Enabled)
        {
            ClearMarkers();
            return;
        }

        s_Cleared = false;
        foreach (int key, ServerSentryExpansionSafeZoneMarkerState state : s_Zones)
        {
            state.seen = false;
        }

        bool changed;
        int globalIndex;
        if (safeZoneSettings.CircleZones)
        {
            foreach (ExpansionSafeZoneCircle circleZone : safeZoneSettings.CircleZones)
            {
                int circleIndex = globalIndex;
                globalIndex++;
                if (!circleZone)
                {
                    continue;
                }

                if (UpdateZone(circleIndex, "Circle", circleZone.Center, circleZone.Radius, 0, null))
                {
                    changed = true;
                }
            }
        }

        if (safeZoneSettings.CylinderZones)
        {
            foreach (ExpansionSafeZoneCylinder cylinderZone : safeZoneSettings.CylinderZones)
            {
                int cylinderIndex = globalIndex;
                globalIndex++;
                if (!cylinderZone)
                {
                    continue;
                }

                if (UpdateZone(cylinderIndex, "Cylinder", cylinderZone.Center, cylinderZone.Radius, cylinderZone.Height, null))
                {
                    changed = true;
                }
            }
        }

        if (safeZoneSettings.PolygonZones)
        {
            foreach (ExpansionSafeZonePolygon polygonZone : safeZoneSettings.PolygonZones)
            {
                int polygonIndex = globalIndex;
                globalIndex++;
                if (!polygonZone || !polygonZone.Positions || polygonZone.Positions.Count() < 3)
                {
                    continue;
                }

                if (UpdateZone(polygonIndex, "Polygon", vector.Zero, 0, 0, polygonZone.Positions))
                {
                    changed = true;
                }
            }
        }

        if (RemoveMissingZones())
        {
            changed = true;
        }

        if (changed)
        {
            ServerSentryLiveApi.WriteSnapshot();
        }
    }

    /** \brief Clears disabled output once and exports only an actual removal. */
    protected static void ClearMarkers()
    {
        if (s_Cleared)
        {
            return;
        }

        s_Cleared = true;
        s_Zones.Clear();
        if (ServerSentryMapMarkers.ClearByPrefix(MARKER_PREFIX) > 0)
        {
            ServerSentryLiveApi.WriteSnapshot();
        }
    }

    /**
        \brief Replaces only markers whose geometry or registry entry changed.
        \param index \p original index across all configured shape collections.
        \param shapeLabel \p Expansion shape name.
        \param position \p circle or cylinder centre.
        \param radius \p circle or cylinder radius.
        \param height \p cylinder height.
        \param positions \p polygon points, or null for round zones.
        \return \p true when a marker was successfully registered.
    */
    protected static bool UpdateZone(int index, string shapeLabel, vector position, float radius, float height, array<vector> positions)
    {
        ServerSentryExpansionSafeZoneMarkerState state = s_Zones.Get(index);
        if (state && state.Matches(shapeLabel, position, radius, height, positions))
        {
            state.seen = true;
            return false;
        }

        string markerId = MARKER_PREFIX + index.ToString();
        string name = "Expansion " + shapeLabel + " safezone #" + index.ToString();
        ServerSentryLiveWorldEventDto marker;
        if (positions)
        {
            marker = ServerSentryLiveApi.CreatePolygonMarker(markerId, name, positions, "ShieldCheck");
            string pointCount = positions.Count().ToString();
            ServerSentryLiveApi.AddMarkerDetail(marker, "points", "Points", pointCount);
        }
        else
        {
            marker = ServerSentryLiveApi.CreateMarker(markerId, name, position, radius, "ShieldCheck");
            marker.shape = "circle";
        }

        marker.eventType = "safezone";
        marker.className = "ExpansionSafeZone" + shapeLabel;
        AddCommonDetails(marker, index, shapeLabel);

        string radiusLabel = "Radius";
        if (positions)
        {
            radiusLabel = "Approx. radius";
        }

        string radiusText = Math.Round(marker.radius).ToString() + " m";
        ServerSentryLiveApi.AddMarkerDetail(marker, "radius", radiusLabel, radiusText);
        if (shapeLabel == "Cylinder")
        {
            string heightText = Math.Round(height).ToString() + " m";
            ServerSentryLiveApi.AddMarkerDetail(marker, "height", "Height", heightText);
        }

        if (!ServerSentryMapMarkers.Register(marker))
        {
            return false;
        }

        state = new ServerSentryExpansionSafeZoneMarkerState();
        state.shape = shapeLabel;
        state.center = position;
        state.radius = radius;
        state.height = height;
        state.marker = marker;
        state.seen = true;
        if (positions)
        {
            state.points = new array<vector>();
            state.points.Copy(positions);
        }

        s_Zones.Set(index, state);
        return true;
    }

    /**
        \brief Removes zones no longer present in the loaded settings.
        \return \p true when a registered marker was removed.
    */
    protected static bool RemoveMissingZones()
    {
        bool changed;
        for (int i = s_Zones.Count() - 1; i >= 0; i--)
        {
            ServerSentryExpansionSafeZoneMarkerState state = s_Zones.GetElement(i);
            if (state.seen)
            {
                continue;
            }

            if (ServerSentryMapMarkers.Remove(state.marker.objectId))
            {
                changed = true;
            }

            s_Zones.Remove(s_Zones.GetKey(i));
        }

        return changed;
    }

    /**
        \brief Adds metadata shared by all safezone shapes.
        \param marker \p marker being prepared for registration.
        \param index \p original settings index.
        \param shapeLabel \p human-readable shape name.
    */
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
