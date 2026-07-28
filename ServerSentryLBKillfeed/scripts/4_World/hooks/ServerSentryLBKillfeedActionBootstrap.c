/**
\file ServerSentryLBKillfeedActionBootstrap.c
\brief Registers the independent LB Killfeed shop action with ServerSentry.
*/

/**
\brief Extends the ServerSentry action bootstrap with LB Killfeed actions.
*/
modded class ServerSentryModActionBootstrap
{
    /**
    \brief Registers the LB Killfeed player-color shop action.
    */
    override void RegisterExtensionActions()
    {
        super.RegisterExtensionActions();
        ServerSentryModActionRegistry.Register(new ServerSentryLBKillfeedSetPlayerColorAction());
    }
}
