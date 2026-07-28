/**
	\file ServerSentryLBGroupsActionBootstrap.c
	\brief Connects ServerSentry to DayZ or third-party hooks for LB Groups action bootstrap.
*/

// Stable ServerSentry 1.2 extension point. Multiple integration PBOs can mod
// this method as long as each implementation calls super.
/**
	\brief Registers built-in actions and exposes the extension registration hook.
*/
modded class ServerSentryModActionBootstrap
{
    /**
    	\brief Registers live actions supplied by extension PBOs.
    */
    override void RegisterExtensionActions()
    {
        super.RegisterExtensionActions();
        ServerSentryModActionRegistry.Register(new ServerSentryLBGroupsSetClanTagColorAction());
#ifndef LB_DISABLE_CHAT
        ServerSentryModActionRegistry.Register(new ServerSentryLBGroupsAssignChatTagAction());
#endif
    }
}
