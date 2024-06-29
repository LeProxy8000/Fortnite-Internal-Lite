#pragma once

// No signature scanning for you

class Signature
{
private:
	DWORD Count;

	PIMAGE_NT_HEADERS Sponge( uintptr_t Moudle );

	bool CheckMask( BYTE* Buffer , const char* Pattern , const char* Mask );

	BYTE* GetMask( BYTE* Start , DWORD Size , const char* Pattern , const char* Mask );
public:
	BYTE* Find( const char* Pattern , const char* Mask , DWORD Occurence = 0 );
};
