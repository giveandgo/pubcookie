#define NUM_DIRECTIVES 16
#ifdef SERVER_VALS_INIT
	wchar_t dbuffer[BUFFSIZE];

	directive[0].name         = L"Debug_Trace";
	directive[0].type         = D_BOUND_INT;
	directive[0].value        = _itow(PBC_DEBUG_TRACE,dbuffer,10);
	directive[0].defined_in   = defined_in;
	directive[0].description  = L"Debug level:\n-1: Errors Only, 0:Warnings and Errors, 1:Informational, 2+:More Debugging";
	directive[0].bound_val[0] = L"-1";
	directive[0].bound_val[1] = L"0";
	directive[0].bound_val[2] = L"1";
	directive[0].bound_val[3] = L"2";
	directive[0].bound_val[4] = L"3";

	directive[1].name         = L"Login_URI";
	directive[1].type         = D_FREE_STRING;
	directive[1].value        = PBC_LOGIN_URI;
	directive[1].defined_in   = defined_in;
	directive[1].description  = L"URL of Pubcookie login server";

	directive[2].name         = L"Keymgt_URI";
	directive[2].type         = D_FREE_STRING;
	directive[2].value        = PBC_KEYMGT_URI;
	directive[2].defined_in   = defined_in;
	directive[2].description  = L"URL of Pubcookie key manangement server";

	directive[3].name         = L"AuthTypeName0";
	directive[3].type         = D_FREE_STRING;
	directive[3].value        = PBC_AUTHTYPE0;
	directive[3].defined_in   = defined_in;
	directive[3].description  = L"Name of authentication type that corresponds to no authentication";

	directive[4].name         = L"AuthTypeName1";
	directive[4].type         = D_FREE_STRING;
	directive[4].value        = PBC_AUTHTYPE1;
	directive[4].defined_in   = defined_in;
	directive[4].description  = L"Name of authentication type that corresponds to regular pubcookie authentication, flavor_basic in a 3.0 login server";

	directive[5].name         = L"AuthTypeName3";
	directive[5].type         = D_FREE_STRING;
	directive[5].value        = PBC_AUTHTYPE3;
	directive[5].defined_in   = defined_in;
	directive[5].description  = L"Name of authentication type that corresponds to pubcookie plus SecureID authentication";

	directive[6].name         = L"PUBLIC_dir_name";
	directive[6].type         = D_FREE_STRING;
	directive[6].value        = PBC_PUBLIC_NAME;
	directive[6].defined_in   = defined_in;
	directive[6].description  = L"If LegacyDirNames names is enabled, a directory with this name will be set to authentication: ";
	directive[6].description  += PBC_AUTHTYPE0;
	directive[6].description  += L" and SetHeaderValues will be enabled.";

	directive[7].name         = L"NETID_dir_name";
	directive[7].type         = D_FREE_STRING;
	directive[7].value        = PBC_NETID_NAME;
	directive[7].defined_in   = defined_in;
	directive[7].description  = L"If LegacyDirNames names is enabled, a directory with this name will be set to authentication: ";
	directive[7].description  += PBC_AUTHTYPE1;

	directive[8].name         = L"SECURID_dir_name";
	directive[8].type         = D_FREE_STRING;
	directive[8].value        = PBC_SECURID_NAME;
	directive[8].defined_in   = defined_in;
	directive[8].description  = L"If LegacyDirNames names is enabled, a directory with this name will be set to authentication: ";
	directive[8].description  += PBC_AUTHTYPE3;

	directive[9].name         = L"System_Root";
	directive[9].type         = D_FREE_STRING;
	directive[9].value        = L"";
	directive[9].defined_in   = L"(Program Default)";
	directive[9].description  = L"Base directory for Pubcookie debug and config files. Leave blank to use the Windows system dir";

	directive[10].name         = L"ClientLogFormat";
	directive[10].type         = D_FREE_STRING;
	directive[10].value        = PBC_CLIENT_LOG_FMT;
	directive[10].defined_in   = defined_in;
	directive[10].description  = L"Format to log client username in.  Use %w for Windows user and %p for Pubcookie user";

	directive[11].name         = L"WebVarLocation";
	directive[11].type         = D_FREE_STRING;
	directive[11].value        = PBC_WEB_VAR_LOCATION;
	directive[11].defined_in   = defined_in;
	directive[11].description  = L"Location in Windows registry for Pubcookie directive database.";

	directive[12].name         = L"Enterprise_Domain";
	directive[12].type         = D_FREE_STRING;
	directive[12].value        = PBC_ENTRPRS_DOMAIN;
	directive[12].defined_in   = defined_in;
	directive[12].description  = L"Domain for scoping granting request cookie";

	directive[13].name         = L"Default_App_Name";
	directive[13].type         = D_FREE_STRING;
	directive[13].value        = PBC_DEFAULT_APP_NAME;
	directive[13].defined_in   = defined_in;
	directive[13].description  = L"Name to assign if application name cannot be determined, e.g. request to /";

	directive[14].name         = L"Ignore_Poll";
	directive[14].type         = D_BOUND_INT;
	directive[14].value        = _itow(PBC_IGNORE_POLL,dbuffer,10);
	directive[14].defined_in   = defined_in;
	directive[14].description  = L"Set to 1 to ignore Network Dispatcher \"/\" polls";
	directive[14].bound_val[0] = L"0";
	directive[14].bound_val[1] = L"1";

	directive[15].name         = L"LegacyDirNames";
	directive[15].type         = D_BOUND_INT;
	directive[15].value        = _itow(PBC_LEGACY_DIR_NAMES,dbuffer,10);
	directive[15].defined_in   = defined_in;
	directive[15].description  = L"Support for legacy directory names. 1=On, 0=Off";
	directive[15].bound_val[0] = L"0";
	directive[15].bound_val[1] = L"1";

#endif



