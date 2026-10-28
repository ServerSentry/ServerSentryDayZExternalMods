class CfgPatches
{
    class ServerSentryCodeLockScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "ServerSentryScripts",
            "CodeLock"
        };
    };
};

class CfgMods
{
    class ServerSentryCodeLock
    {
        dir = "ServerSentryCodeLock";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry CodeLock Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.2.0";
        extra = 0;
        type = "mod";
        dependencies[] = {"Game","World"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryCodeLock/scripts/3_Game"
                };
            };

            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryCodeLock/scripts/4_World"
                };
            };
        };
    };
};
