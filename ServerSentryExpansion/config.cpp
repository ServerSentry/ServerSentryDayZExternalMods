class CfgPatches
{
    class ServerSentryExpansionScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts",
            "ServerSentryScripts",
            "DayZExpansion_Core_Scripts",
            "DayZExpansion_Market_Scripts"
#ifdef EXPANSIONMODCHAT
            , "DayZExpansion_Chat_Scripts"
#endif
        };
    };
};

class CfgMods
{
    class ServerSentryExpansion
    {
        dir = "ServerSentryExpansion";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry Expansion Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.1.0";
        extra = 0;
        type = "mod";
        dependencies[] = {"World","Mission"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryExpansion/scripts/4_World"
                };
            };

            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryExpansion/scripts/5_Mission"
                };
            };
        };
    };
};
