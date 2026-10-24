class CfgPatches
{
    class ServerSentryBreachingChargeScripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "ServerSentryScripts",
            "HDSN_BreachingCharge"
        };
    };
};

class CfgMods
{
    class ServerSentryBreachingCharge
    {
        dir = "ServerSentryBreachingCharge";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "ServerSentry BreachingCharge Bridge";
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
                    "ServerSentryDayZExternalMods/ServerSentryBreachingCharge/scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentryDayZExternalMods/ServerSentryBreachingCharge/scripts/4_World"
                };
            };
        };
    };
};
