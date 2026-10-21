/**
    \file ServerSentryExpansionLiveMapBootstrap.c
    \brief Binds Expansion safezone marker reconciliation to the mission lifecycle.
*/

/** \brief Starts the Expansion Live Map integration. */
class ServerSentryExpansionLiveMapBootstrap
{
    /** \brief Registers mission-owned marker timers once. */
    static void Register()
    {
        ServerSentryExpansionSafeZoneLiveMarkers.Start();
    }
}

/** \brief Starts and stops Expansion marker callbacks with the server mission. */
modded class MissionServer
{
    /** \brief Starts marker reconciliation after the mission is initialized. */
    override void OnInit()
    {
        super.OnInit();
        ServerSentryExpansionLiveMapBootstrap.Register();
    }

    /** \brief Cancels marker callbacks before the shared mission cleanup runs. */
    override void OnMissionFinish()
    {
        ServerSentryExpansionSafeZoneLiveMarkers.Stop();
        super.OnMissionFinish();
    }

    /** \brief Releases callbacks when a mission is destroyed without finishing. */
    void ~MissionServer()
    {
        ServerSentryExpansionSafeZoneLiveMarkers.Stop();
    }
}
