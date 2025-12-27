#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <iostream>
#include <string>
#include <atlbase.h>
#include <comdef.h>

#include <atlcom.h>
#pragma warning(disable : 4278)
#pragma warning(disable : 4146)
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") lcid("0") raw_interfaces_only named_guids
#pragma warning(default : 4146)
#pragma warning(default : 4278)

static bool tryAttach(IDispatch* dispatch, DWORD target)
{
	if (!dispatch) return false;

	EnvDTE::_DTEPtr dte;
	HRESULT hr = dispatch->QueryInterface(__uuidof(EnvDTE::_DTE), (void**)&dte);
	if (FAILED(hr) || !dte)
	{
		return false;
	}

	try
	{
		EnvDTE::DebuggerPtr debugger;
		dte->get_Debugger(&debugger);
		if (!debugger)
		{
			return false;
		}

		EnvDTE::ProcessesPtr procs;
		debugger->get_LocalProcesses(&procs);
		if (!procs)
		{
			return false;
		}

		long count = 0;
		procs->get_Count(&count);

		for (long i = 1; i <= count; ++i)
		{
			EnvDTE::ProcessPtr proc;
			VARIANT idx;
			VariantInit(&idx);
			idx.vt = VT_I4;
			idx.lVal = i;
			HRESULT qhr = procs->Item(idx, &proc);
			VariantClear(&idx);
			if (FAILED(qhr) || !proc) continue;

			long procId = 0;
			proc->get_ProcessID(&procId);
			if ((DWORD)procId == target)
			{
				proc->Attach();

				return true;
			}
		}
	}
	catch (_com_error& e)
	{
		std::cerr << "COM error: " << std::hex << e.Error() << " - " << (const char*)e.Description() << '\n';
		return false;
	}
	return false;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: auto-debug <pid>\n";
		return 1;
	}

	DWORD pid = 0;
	pid = (DWORD)std::stoi(argv[1]);
	if (pid == 0)
	{
		std::cerr << "Invalid PID.\n";
		return 1;
	}

	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		std::cerr << "CoInitializeEx failed: " << std::hex << hr << '\n';
		return 1;
	}

	IRunningObjectTable* ROT = nullptr;
	if (FAILED(GetRunningObjectTable(0, &ROT)) || !ROT)
	{
		std::cerr << "GetRunningObjectTable failed\n";
		CoUninitialize();
		return 1;
	}

	IEnumMoniker* enumMoniker = nullptr;
	if (FAILED(ROT->EnumRunning(&enumMoniker)) || !enumMoniker)
	{
		std::cerr << "EnumRunning failed\n";
		ROT->Release();
		CoUninitialize();
		return 1;
	}

	IMoniker* moniker = nullptr;
	ULONG fetched = 0;
	bool attached = false;

	while (enumMoniker->Next(1, &moniker, &fetched) == S_OK)
	{
		IBindCtx* bindContext = nullptr;
		if (SUCCEEDED(CreateBindCtx(0, &bindContext)))
		{
			LPOLESTR displayName = nullptr;
			if (SUCCEEDED(moniker->GetDisplayName(bindContext, NULL, &displayName)))
			{
				std::wstring name{ displayName ? displayName : L"" };
				CoTaskMemFree(displayName);

				if (name.find(L"VisualStudio.DTE") != std::wstring::npos)
				{
					IUnknown* unk = nullptr;
					if (SUCCEEDED(ROT->GetObject(moniker, &unk)) && unk)
					{
						if (tryAttach((IDispatch*)unk, pid))
						{
							attached = true;
							unk->Release();
							bindContext->Release();
							moniker->Release();
							break;
						}
						unk->Release();
					}
				}
			}
			bindContext->Release();
		}
		moniker->Release();
	}

	enumMoniker->Release();
	ROT->Release();
	CoUninitialize();

	if (!attached)
	{
		std::cerr << "Failed to attach.\n";
		return 2;
	}

	return 0;
}
