class ServerSentryExpansionLiveMapBootstrap
{
    static void Register()
    {
        ServerSentryExpansionSafeZoneLiveMarkers.Start();
    }
}

modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ServerSentryExpansionLiveMapBootstrap.Register();
    }
}
