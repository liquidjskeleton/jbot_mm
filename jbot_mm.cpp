/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from 
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#include <stdio.h>
#include "jbot_mm.h"
#include "msvc10/func.h"
#include "toolframework/iserverenginetools.h"
#include "toolframework/itoolentity.h"
#include "msvc10/botman.h"
#include <IEngineTrace.h>
#include "msvc10/help.h"
//#define GAME_DLL 1
//#include "player.h"

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

#if SOURCE_ENGINE >= SE_ORANGEBOX
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif


TheJbotPlugin g_SamplePlugin;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;
IVEngineServer *engine = NULL;
IServerPluginHelpers *helpers = NULL;
IGameEventManager2 *gameevents = NULL;
IServerPluginCallbacks *vsp_callbacks = NULL;
IPlayerInfoManager *playerinfomanager = NULL;
ICvar *icvar = NULL;
CGlobalVars *gpGlobals = NULL;
IServerTools *serverTools = NULL;
IBotManager* botmanager = NULL;
IEngineTrace* enginetrace = NULL;

//ConVar sample_cvar("sample_cvar", "42", 0);

void CMD_JbotCreate(const CCommand& command)
{
	JBotManager::CreateBot();
}
ConCommand jbot_create("jbot_create", CMD_JbotCreate);


/** 
 * Something like this is needed to register cvars/CON_COMMANDs.
 */
class BaseAccessor : public IConCommandBaseAccessor
{
public:
	bool RegisterConCommandBase(ConCommandBase *pCommandBase)
	{
		/* Always call META_REGCVAR instead of going through the engine. */
		return META_REGCVAR(pCommandBase);
	}
} s_BaseAccessor;

PLUGIN_EXPOSE(TheJbotPlugin, g_SamplePlugin);
bool TheJbotPlugin::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, gameevents, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, playerinfomanager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);
	GET_V_IFACE_ANY(GetServerFactory, serverTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetServerFactory, botmanager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);
	//GET_V_IFACE_ANY(GetServerFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_CLIENT);

	gpGlobals = ismm->GetCGlobals();

	META_CONPRINTF("Starting plugin.\n");

	/* Load the VSP listener.  This is usually needed for IServerPluginHelpers. */
	if ((vsp_callbacks = ismm->GetVSPInfo(NULL)) == NULL)
	{
		ismm->AddListener(this, this);
		ismm->EnableVSPListener();
	}

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &TheJbotPlugin::Hook_LevelInit, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &TheJbotPlugin::Hook_ServerActivate, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &TheJbotPlugin::Hook_GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &TheJbotPlugin::Hook_LevelShutdown, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &TheJbotPlugin::Hook_ClientActive, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &TheJbotPlugin::Hook_ClientDisconnect, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &TheJbotPlugin::Hook_ClientPutInServer, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, gameclients, this, &TheJbotPlugin::Hook_SetCommandClient, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, gameclients, this, &TheJbotPlugin::Hook_ClientSettingsChanged, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this, &TheJbotPlugin::Hook_ClientConnect, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this, &TheJbotPlugin::Hook_ClientCommand, false);

	ENGINE_CALL(LogPrint)("All hooks started!\n");

#if SOURCE_ENGINE >= SE_ORANGEBOX
	g_pCVar = icvar;
	ConVar_Register(0, &s_BaseAccessor);
#else
	ConCommandBaseMgr::OneTimeInit(&s_BaseAccessor);
#endif

	JBotManager::CallOnMapChange(gpGlobals->mapname.ToCStr());

	return true;
}

bool TheJbotPlugin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, server, this, &TheJbotPlugin::Hook_LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, server, this, &TheJbotPlugin::Hook_ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &TheJbotPlugin::Hook_GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, server, this, &TheJbotPlugin::Hook_LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, gameclients, this, &TheJbotPlugin::Hook_ClientActive, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &TheJbotPlugin::Hook_ClientDisconnect, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, gameclients, this, &TheJbotPlugin::Hook_ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, gameclients, this, &TheJbotPlugin::Hook_SetCommandClient, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, gameclients, this, &TheJbotPlugin::Hook_ClientSettingsChanged, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, gameclients, this, &TheJbotPlugin::Hook_ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, gameclients, this, &TheJbotPlugin::Hook_ClientCommand, false);

	return true;
}

void TheJbotPlugin::OnVSPListening(IServerPluginCallbacks *iface)
{
	vsp_callbacks = iface;
}

void TheJbotPlugin::Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	META_CONPRINTF("ServerActivate() called: edictCount = %d, clientMax = %d", edictCount, clientMax);
}

void TheJbotPlugin::AllPluginsLoaded()
{
	/* This is where we'd do stuff that relies on the mod or other plugins 
	 * being initialized (for example, cvars added and events registered).
	 */
}

void TheJbotPlugin::Hook_ClientActive(edict_t *pEntity, bool bLoadGame)
{
	
	META_CONPRINTF("Hook_ClientActive(%d, %d)\n", IndexOfEdict(pEntity), bLoadGame);
}

#if SOURCE_ENGINE >= SE_ORANGEBOX
void TheJbotPlugin::Hook_ClientCommand(edict_t *pEntity, const CCommand &args)
#else
void SamplePlugin::Hook_ClientCommand(edict_t *pEntity)
#endif
{
#if SOURCE_ENGINE <= SE_DARKMESSIAH
	CCommand args;
#endif
	if (!pEntity || pEntity->IsFree() || IndexOfEdict(pEntity) == -1)
	{
		return;
	}
	META_CONPRINTF("Hook_ClientCommand(%d)\n", IndexOfEdict(pEntity));
	const char *cmd = args.Arg(0);
	if (strcmp(cmd, "sex") == 0)
	{
		META_CONPRINTF("calling \n", IndexOfEdict(pEntity));
		Player_Respawn(pEntity);
	}
}

void TheJbotPlugin::Hook_ClientSettingsChanged(edict_t *pEdict)
{
	if (playerinfomanager)
	{
		IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEdict);

		const char *name = engine->GetClientConVarValue(IndexOfEdict(pEdict), "name");

		if (playerinfo != NULL
			&& name != NULL
			&& strcmp(engine->GetPlayerNetworkIDString(pEdict), "BOT") != 0
			&& playerinfo->GetName() != NULL
			&& strcmp(name, playerinfo->GetName()) == 0)
		{
			char msg[128];
			MM_Format(msg, sizeof(msg), "Your name changed to \"%s\" (from \"%s\")\n", name, playerinfo->GetName());
			engine->ClientPrintf(pEdict, msg);
		}
	}
}

bool TheJbotPlugin::Hook_ClientConnect(edict_t *pEntity,
									const char *pszName,
									const char *pszAddress,
									char *reject,
									int maxrejectlen)
{
	META_CONPRINTF("Hook_ClientConnect(%d, \"%s\", \"%s\")\n", IndexOfEdict(pEntity), pszName, pszAddress);

	return true;
}

void TheJbotPlugin::Hook_ClientPutInServer(edict_t *pEntity, char const *playername)
{
	KeyValues *kv = new KeyValues( "msg" );
	kv->SetString( "title", "Hello" );
	kv->SetString( "msg", "Hello there" );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "level", 5);
	kv->SetInt( "time", 10);
	helpers->CreateMessage(pEntity, DIALOG_MSG, kv, vsp_callbacks);
	kv->deleteThis();
}

void TheJbotPlugin::Hook_ClientDisconnect(edict_t *pEntity)
{
	META_CONPRINTF("Hook_ClientDisconnect(%d)\n", IndexOfEdict(pEntity));
}
bool j_stopexecution = false;

void TheJbotPlugin::Hook_GameFrame(bool simulating)
{
	/**
	 * simulating:
	 * ***********
	 * true  | game is ticking
	 * false | game is not ticking
	 */

	//man i miss using c instead of c++
	if (*((byte*)&j_stopexecution) == 1)
	{
		*((byte*)&j_stopexecution) = 2;
		jprintf("----------------\n");
		jprintf("stopped execution\n");
		jprintf("----------------\n");
	}

	if (simulating && !j_stopexecution)
	{
		JBotManager::Bot_ThinkAll();
	}
}

bool TheJbotPlugin::Hook_LevelInit(const char *pMapName,
								char const *pMapEntities,
								char const *pOldLevel,
								char const *pLandmarkName,
								bool loadGame,
								bool background)
{
	META_CONPRINTF("Hook_LevelInit(%s)\n", pMapName);
	JBotManager::CallOnMapChange(pMapName);
	return true;
}

void TheJbotPlugin::Hook_LevelShutdown()
{
	META_CONPRINTF("Hook_LevelShutdown()\n");
}

void TheJbotPlugin::Hook_SetCommandClient(int index)
{
	META_CONPRINTF("Hook_SetCommandClient(%d)\n", index);
}

bool TheJbotPlugin::Pause(char *error, size_t maxlen)
{
	return true;
}

bool TheJbotPlugin::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *TheJbotPlugin::GetLicense()
{
	return "unknown";
}

const char *TheJbotPlugin::GetVersion()
{
	return "1.0.0.0";
}

const char *TheJbotPlugin::GetDate()
{
	return __DATE__;
}

const char *TheJbotPlugin::GetLogTag()
{
	return "SAMPLE";
}

const char *TheJbotPlugin::GetAuthor()
{
	return "liquidjskeleton";
}

const char *TheJbotPlugin::GetDescription()
{
	return "aweseome plugin";
}

const char *TheJbotPlugin::GetName()
{
	return "JBOT";
}

const char *TheJbotPlugin::GetURL()
{
	return "";
}
