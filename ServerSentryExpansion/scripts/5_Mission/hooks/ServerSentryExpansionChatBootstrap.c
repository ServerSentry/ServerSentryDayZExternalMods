#ifdef EXPANSIONMODCHAT

class ServerSentryExpansionChatBootstrap
{
    static void Register()
    {
        ServerSentryChatProviderRegistry.RegisterExternalChatProvider("DayZExpansion_Chat");
    }
}

modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ServerSentryExpansionChatBootstrap.Register();
    }
}

#endif // EXPANSIONMODCHAT
