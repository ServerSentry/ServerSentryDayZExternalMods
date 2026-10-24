class CfgPatches
{
    class ServerSentryBBPScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "ServerSentryScripts",
            "BaseBuildingPlus_Scripts"
        };
    };
};

class CfgMods
{
    class ServerSentryBBP
    {
        dir = "ServerSentryBBP";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ServerSentry BBP Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.1.0";
        extra = 0;
        type = "mod";
        dependencies[] = {"Game", "World"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryBBP/scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryBBP/scripts/4_World"
                };
            };
        };
    };
};
