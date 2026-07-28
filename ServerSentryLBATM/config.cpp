class CfgPatches
{
    class ServerSentryLBATMScripts
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
            "LBmaster_Groups_ATM"
        };
    };
};

class CfgMods
{
    class ServerSentryLBATM
    {
        dir = "ServerSentryLBATM";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry LB ATM Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.1.0";
        extra = 0;
        type = "mod";
        dependencies[] = {"Game"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentry/ServerSentryLBATM/scripts/3_Game"
                };
            };
        };
    };
};
