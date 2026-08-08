class CfgPatches
{
    class ServerSentryDrJonesScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "Trader"
        };
    };
};

class CfgMods
{
    class ServerSentryDrJones
    {
        dir = "ServerSentryDrJones";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry DrJones Bridge";
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
                    "ServerSentry/ServerSentryDrJones/scripts/4_World"
                };
            };
        };
    };
};
