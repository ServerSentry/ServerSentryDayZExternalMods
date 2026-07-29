/**
	\file ServerSentryLBGroupsFlagpoleActionIds.c
	\brief Defines stable action IDs for ServerSentry LB Groups live actions.
*/

/**
	\brief Stores the stable IDs used by ServerSentry LB Groups live actions.
*/
class ServerSentryLBGroupsActionIds
{
    static const string FLAGPOLE_RAISE = "object.flagpole.raise";
    static const string FLAGPOLE_LOWER = "object.flagpole.lower";
    static const string FLAGPOLE_DELETE_ITEMS_RADIUS = "object.flagpole.delete_items_radius";
    static const string WORLD_DELETE_ITEMS_RADIUS = "world.items.delete_radius";
}
