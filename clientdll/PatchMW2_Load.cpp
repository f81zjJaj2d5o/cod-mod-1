// ==========================================================
// MW2 coop
// 
// Component: IW4SP
// Sub-component: clientdll
// Purpose: Fastfile loading modifications
//
// Initial author: momo5502
// Started: 2014-03-08
// ==========================================================

#include "stdinc.h"

dvar_t* specialops;

void* ReallocateAssetPool(int type, unsigned int newSize);

// Allow civilians to be killed in 'No Russian' if game is censored.
void uncutGame(XZoneInfo* data, int count)
{
	// Only in campaign!
	if(!specialops)
	{
		specialops = Dvar_FindVar("specialops");
	}

	bool uncensored = false;
	for(int i = 0; i<count && !specialops->current.boolean; i++)
	{
		if(!strcmp(data[0].name, "airport"))
		{
			Dvar_SetCommand("friendlyfire_dev_disabled", "1");
			uncensored = true;
		}
	}

	if(!uncensored)
	{
		Dvar_SetCommand("friendlyfire_dev_disabled", "0");
	}
}

char returnPath[MAX_PATH];

char* addAlterZones(char* zone)
{
	if(GetFileAttributes(va( "zone\\alter\\%s", zone)) != INVALID_FILE_ATTRIBUTES)
	{
		strcpy(returnPath, "zone\\alter\\");
	}
	else if(GetFileAttributes(va("zone\\dlc\\%s", zone)) != INVALID_FILE_ATTRIBUTES)
	{
		strcpy(returnPath, "zone\\dlc\\");
	}
	else
	{
		sprintf(returnPath, "zone\\%s\\", language); // User's language
	}

	return returnPath;
}

void __cdecl loadTeamFile(XZoneInfo* data, int count, int unknown)
{
	// Still bugged. probably need to compile an own fastfile
	//data[count].name = "team_tf141";
	//data[count].type1 = 4;
	//data[count].type2 = 0;
	//count++;

	uncutGame(data, count);

	_allowZoneChange = true;

	DB_LoadXAssets(data, count, unknown);
}

static DWORD gameWorldSP;
static DWORD gameWorldMP;

void GetBSPNameHookFunc(char* buffer, size_t size, const char* format, const char* mapname)
{
	// the usual stuff
	if (!_strnicmp("mp_", mapname, 3))
	{
		format = "maps/mp/%s.d3dbsp";
	}

	_snprintf(buffer, size, format, mapname);

	// check for being MP/SP, and change data accordingly
	if (_strnicmp("mp_", mapname, 3))
	{
		if(version == 159)
		{
			// SP
			*(DWORD*)0x4B0921 = gameWorldSP + 4;		// some game data structure
		}
		else
		{
			// SP
			*(DWORD*)0x4D4AA1 = gameWorldSP + 4;		// some game data structure
		}
	}
	else
	{
		if(version == 159)
		{
			// MP
			*(DWORD*)0x4B0921 = gameWorldMP + 52;		// some game data structure
		}
		else
		{
			// MP
			*(DWORD*)0x4D4AA1 = gameWorldMP + 52;		// some game data structure
		}
	}
}

typedef struct  
{
	char unknown[16];
} xAssetEntry_t;

static xAssetEntry_t xEntries[789312];

void ReallocXAssetEntries()
{
	int newsize = 516 * 2048;
	//newEnts = malloc(newsize);

	// get (obfuscated) memory locations
	unsigned int origMin = 0xB2B360;
	unsigned int origMax = 0xB2B370;

	// scan the .text memory
	char* scanMin = (char*)0x401000;
	char* scanMax = (char*)0x694000;

	if(version == 184)
	{
		origMin = 0xB27DE0;
		origMax = 0xB27DF0;
		scanMax = (char*)0x691000;
	}

	unsigned int difference = (unsigned int)xEntries - origMin;

	char* current = scanMin;

	for (; current < scanMax; current += 1) {
		unsigned int* intCur = (unsigned int*)current;

		// if the address points to something within our range of interest
		if (*intCur == origMin || *intCur == origMax) {
			// patch it
			*intCur += difference;
		}
	}

	if(version == 159)
		*(DWORD*)0x581740 = 789312;
	else
		*(DWORD*)0x57EB40 = 789312;
}

void PatchMW2_Load()
{
	if(version == 159)
	{
		// Ignore zone version missmatch
		*(BYTE*)0x4256D8 = 0xEB;

		// Ignore 'Disc read error.'
		nop(0x4B7335, 2);
		//*(BYTE*)0x4B7356 = 0xEB;
		//*(BYTE*)0x413629 = 0xEB;
		//*(BYTE*)0x581227 = 0xEB;
		*(BYTE*)0x4256B9 = 0xEB;

		gameWorldSP = (*(DWORD*)0x4B0921) - 4;
	}
	else if(version == 184)
	{
		// Ignore zone version missmatch
		*(BYTE*)0x4A4E98 = 0xEB;

		// Ignore 'Disc read error.'
		nop(0x4F6E05, 2);
		//*(BYTE*)0x4F6E26 = 0xEB;
		//*(BYTE*)0x47FE69 = 0xEB;
		//*(BYTE*)0x57E637 = 0xEB;
		*(BYTE*)0x4A4E79 = 0xEB;

		gameWorldSP = (*(DWORD*)0x4D4AA1) - 4;
	}

	if(version == 177)
	{
		// Add zone support some time.
	}
	else
	{
		gameWorldMP = (DWORD)ReallocateAssetPool(ASSET_TYPE_GAME_MAP_MP, 1);

		call(getBSPNameHookLoc, GetBSPNameHookFunc, PATCH_CALL);
		call(ffLoadHook1Loc, loadTeamFile, PATCH_CALL);
		call(zoneLoadHookLoc, addAlterZones, PATCH_CALL);

		ReallocXAssetEntries();
	}
}