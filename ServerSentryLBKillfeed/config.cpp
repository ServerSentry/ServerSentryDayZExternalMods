class CfgPatches
{
    class ServerSentryLBKillfeedScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "ServerSentryScripts",
            "LBmaster_Core",
            "LBmaster_Killfeed"
        };
    };
};

class CfgMods
{
    class ServerSentryLBKillfeed
    {
        dir = "ServerSentryLBKillfeed";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry LB Killfeed Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.1.0";
        extra = 0;
        type = "mod";
        dependencies[] = {"World"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentry/ServerSentryLBKillfeed/scripts/4_World"
                };
            };
        };
    };
};
