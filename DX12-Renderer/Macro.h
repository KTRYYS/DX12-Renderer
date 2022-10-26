#pragma once
#include<windows.h>
#include<windowsx.h>
#include<string>
#include<WRL.h>
#include<comdef.h>
#include<iostream>

using namespace std;
using namespace Microsoft::WRL;

class DxException
{
public:
	DxException(string FileName, HRESULT result)
	{
		_com_error ComError(result);
		cout << "File Path:" << FileName << " Error:" << (TCHAR*)ComError.ErrorMessage() << endl;
	}
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
	HRESULT result = (x);                                             \
	string FileName = __FILE__;                                       \
	if (result < 0) {  DxException(FileName, result); }          \
}
#endif // !ThrowIfFailed