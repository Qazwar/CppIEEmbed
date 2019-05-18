#include <utility>
#include <fstream>
#include "jsobject.h"
#include "atlbase.h"
#include <cstring>

JSObject::JSObject() : ref(0)
{
	idMap.insert(std::make_pair(L"execute", DISPID_USER_EXECUTE));
	idMap.insert(std::make_pair(L"writefile", DISPID_USER_WRITEFILE));
	idMap.insert(std::make_pair(L"readfile", DISPID_USER_READFILE));
	idMap.insert(std::make_pair(L"getevar", DISPID_USER_GETVAL));
	idMap.insert(std::make_pair(L"setevar", DISPID_USER_SETVAL));
	idMap.insert(std::make_pair(L"get_pos", DISPID_GET_POS));
	idMap.insert(std::make_pair(L"get_posv", DISPID_GET_POSV));
	idMap.insert(std::make_pair(L"log", DISPID_USER_LOG));
}

HRESULT STDMETHODCALLTYPE JSObject::QueryInterface(REFIID riid, void **ppv)
{
	*ppv = NULL;

	if (riid == IID_IUnknown || riid == IID_IDispatch) {
		*ppv = static_cast<IDispatch*>(this);
	}

	if (*ppv != NULL) {
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE JSObject::AddRef()
{
	return InterlockedIncrement(&ref);
}

ULONG STDMETHODCALLTYPE JSObject::Release()
{
	int tmp = InterlockedDecrement(&ref);

	if (tmp == 0) {
		OutputDebugString("JSObject::Release(): delete this");
		delete this;
	}
	
	return tmp;
}

HRESULT STDMETHODCALLTYPE JSObject::GetTypeInfoCount(UINT *pctinfo)
{
	*pctinfo = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE JSObject::GetTypeInfo(UINT iTInfo, LCID lcid,
	ITypeInfo **ppTInfo)
{
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE JSObject::GetIDsOfNames(REFIID riid,
	LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
	HRESULT hr = S_OK;

	for (UINT i = 0; i < cNames; i++) {
		std::map<std::wstring, DISPID>::iterator iter = idMap.find(rgszNames[i]);
		if (iter != idMap.end()) {
			rgDispId[i] = iter->second;
		} else {
			rgDispId[i] = DISPID_UNKNOWN;
			hr = DISP_E_UNKNOWNNAME;
		}
	}

	return hr;
}
#include "webwindow.h"
extern WebWindow* webWindow;

HRESULT STDMETHODCALLTYPE JSObject::Invoke(DISPID dispIdMember, REFIID riid,
	LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
	EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	if (wFlags & DISPATCH_METHOD) {
		HRESULT hr = S_OK;

		std::string *args = new std::string[pDispParams->cArgs];
		for (size_t i = 0; i < pDispParams->cArgs; ++i) {
			BSTR bstrArg = pDispParams->rgvarg[i].bstrVal;
			LPSTR arg = NULL;
			arg = BSTRToLPSTR(bstrArg, arg);
			args[pDispParams->cArgs - 1 - i] = arg; // also re-reverse order of arguments
			delete [] arg;
		}

		switch (dispIdMember) {
		case DISPID_USER_EXECUTE: {
			webWindow->webForm->RunJSFunction(args[0].c_str());
			break;
		}
		case DISPID_USER_LOG: {
			OutputDebugStringA(args[0].c_str());
			break;
		}
		case DISPID_GET_POS: {
			//https://stackoverflow.com/questions/45865768/iwebbrowser2-using-a-uint8array-filling-without-looping
			POINT pos;
			GetCursorPos(&pos);

			pVarResult->vt = VT_I4;
			pVarResult->lVal = MAKELONG(pos.x, pos.y);
			hr = S_OK;
			break;

		}

		case DISPID_GET_POSV: {
			//https://stackoverflow.com/questions/45865768/iwebbrowser2-using-a-uint8array-filling-without-looping
			POINT pos;
			GetCursorPos(&pos);
				//get league x/y
				int x = pos.x;
				int y = pos.y;

				auto doc = webWindow->webForm->GetDoc();

				if (doc == NULL) {
					break;
				}
				IHTMLWindow2 *win = NULL;
				doc->get_parentWindow(&win);
				doc->Release();

				if (win == NULL) {
					break;
				}

				IDispatchEx *winEx;
				win->QueryInterface(&winEx);
				win->Release();

				if (winEx == NULL) {
					break;
				}

				DISPID did = 0;
				LPOLESTR lpNames[] = { L"Uint32Array" };
				hr = winEx->GetIDsOfNames(IID_NULL, lpNames, 1,
					LOCALE_USER_DEFAULT, &did);
				if (FAILED(hr))
					return hr;
				VARIANT self;
				self.vt = VT_NULL;

				VARIANT length;
				length.vt = VT_I4;
				length.lVal = 2;

				VARIANT args[2] = { self, length };
				DISPID named_args[1] = { DISPID_THIS };

				DISPPARAMS params;
				params.rgvarg = args;
				params.rgdispidNamedArgs = named_args;
				params.cArgs = 2;
				params.cNamedArgs = 1;

				VARIANT result;
				result.vt = VT_EMPTY;
				HRESULT hr = winEx->Invoke(
					did, IID_NULL, LOCALE_USER_DEFAULT,
					DISPATCH_METHOD, &params, &result, nullptr, nullptr
				);
				//OutputDebugStringA(result.vt == VT_DISPATCH && hr == S_OK ? "GOT ARRAY2!\n" : "BAD!\n");

				// Variables used  
				DISPPARAMS dp = { NULL, NULL, 0, 0 };
				DISPID dispidNamed = DISPID_PROPERTYPUT;
				DISPID dispID;
				DISPID dispID2;
				LPOLESTR ptName = L"0";
				// Get DISPID for name passed  
				auto pDisp = result.pdispVal;
				hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID);
				ptName = L"1";
				hr = pDisp->GetIDsOfNames(IID_NULL, &ptName, 1, LOCALE_USER_DEFAULT, &dispID2);


				// Build DISPPARAMS  
				dp.cArgs = 1;
				VARIANT pArgs[1];
				pArgs[0].vt = VT_I4;
				pArgs[0].lVal = x;
				dp.rgvarg = pArgs;

				VARIANT pvResult;
				// Handle special-case for property-puts  
				dp.cNamedArgs = 1;
				dp.rgdispidNamedArgs = &dispidNamed;
				hr = pDisp->Invoke(dispID, IID_NULL, LOCALE_SYSTEM_DEFAULT,
					DISPATCH_PROPERTYPUT, &dp, &pvResult, NULL, NULL);

				pArgs[0].vt = VT_I4;
				pArgs[0].lVal = y;
				hr = pDisp->Invoke(dispID2, IID_NULL, LOCALE_SYSTEM_DEFAULT,
					DISPATCH_PROPERTYPUT, &dp, &pvResult, NULL, NULL);

				//OutputDebugStringA("GOOD!\n");

				pVarResult->vt = result.vt;
				pVarResult->pdispVal = result.pdispVal;
				break;
			}
			case DISPID_USER_WRITEFILE: {
				std::ofstream outfile;
				std::ios_base::openmode mode = std::ios_base::out;

				if (args[1] == "overwrite") {
					mode |= std::ios_base::trunc;
				} else if (args[1] == "append") {
					mode |= std::ios_base::app;
				}

				outfile.open(args[0].c_str());
				outfile << args[2];
				outfile.close();
				break;
			}
			case DISPID_USER_READFILE: {
				std::string buffer;
				std::string line;
				std::ifstream infile;
				infile.open(args[0].c_str());

				while(std::getline(infile, line)) {
					buffer += line;
					buffer += "\n";
				}

				int lenW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buffer.c_str(), -1, NULL, 0);
				BSTR bstrRet = SysAllocStringLen(0, lenW - 1);
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buffer.c_str(), -1, bstrRet, lenW);

				pVarResult->vt = VT_BSTR;
				pVarResult->bstrVal = bstrRet;

				break;
			}
			case DISPID_USER_GETVAL: {
				char *buf = new char[256];
				strncpy(buf, values[args[0]].c_str(), 256);

				int lenW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, -1, NULL, 0);
				BSTR bstrRet = SysAllocStringLen(0, lenW - 1);
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, -1, bstrRet, lenW);

				pVarResult->vt = VT_BSTR;
				pVarResult->bstrVal = bstrRet;

				break;
			}
			case DISPID_USER_SETVAL: {
				std::map<std::string, std::string>::iterator itr = values.find(args[0]);
				if (itr == values.end()) {
					values.insert(std::make_pair(args[0], args[1]));
				} else {
					values[args[0]] = args[1];
				}

				break;
			}
			default:
				hr = DISP_E_MEMBERNOTFOUND;
		}

		delete [] args;

		return hr;
	}

	return E_FAIL;
}

char *JSObject::BSTRToLPSTR(BSTR bStr, LPSTR lpstr)
{
	int lenW = SysStringLen(bStr);
	int lenA = WideCharToMultiByte(CP_ACP, 0, bStr, lenW, 0, 0, NULL, NULL);

	if (lenA > 0) {
		lpstr = new char[lenA + 1]; // allocate a final null terminator as well
		WideCharToMultiByte(CP_ACP, 0, bStr, lenW, lpstr, lenA, NULL, NULL);
		lpstr[lenA] = '\0'; // Set the null terminator yourself
	} else {
		lpstr = NULL;
	}

	return lpstr;
}