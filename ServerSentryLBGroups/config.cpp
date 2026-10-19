class CfgPatches
{
    class ServerSentryLBGroupsScripts
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
            "LBmaster_Groups",
            "LBmaster_GroupDLCPlotpole"
        };
    };
};

class CfgMods
{
    class ServerSentryLBGroups
    {
        dir = "ServerSentryLBGroups";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        name = "Server Sentry LB Groups Bridge";
        credits = "";
        author = "Krypton91";
        authorID = "0";
        version = "1.8.2";
        extra = 0;
        type = "mod";
        dependencies[] = {"Game","World","Mission"};
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentry/ServerSentryLBGroups/scripts/3_Game"
                };
            };

            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentry/ServerSentryLBGroups/scripts/4_World"
                };
            };

            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ServerSentry/ServerSentryLBGroups/scripts/5_Mission"
                };
            };
        };
    };
};
