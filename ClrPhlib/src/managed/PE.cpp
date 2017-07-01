#include <ClrPhlib.h>
#include <UnmanagedPh.h>
#include <atlstr.h>

using namespace System;
using namespace ClrPh;

PE::PE(
    _In_ String ^ Filepath
)
{
    CString PvFilePath(Filepath);
    m_Impl = new UnmanagedPE();

	this->LoadSuccessful = m_Impl->LoadPE(PvFilePath.GetBuffer());
	this->Filepath = gcnew String(Filepath);

	if (LoadSuccessful)
		InitProperties();
	 
}

void PE::InitProperties()
{
	LARGE_INTEGER time;
	SYSTEMTIME systemTime;

	PH_MAPPED_IMAGE PvMappedImage = m_Impl->m_PvMappedImage;
	
	Properties = gcnew PeProperties();
	Properties->Machine = PvMappedImage.NtHeaders->FileHeader.Machine;
	Properties->Magic = m_Impl->m_PvMappedImage.Magic;
	Properties->Checksum = PvMappedImage.NtHeaders->OptionalHeader.CheckSum;
	Properties->CorrectChecksum = (Properties->Checksum == PhCheckSumMappedImage(&PvMappedImage));

	RtlSecondsSince1970ToTime(PvMappedImage.NtHeaders->FileHeader.TimeDateStamp, &time);
	PhLargeIntegerToLocalSystemTime(&systemTime, &time);
	Properties->Time = gcnew DateTime (systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds, DateTimeKind::Local);

	if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		PIMAGE_OPTIONAL_HEADER32 OptionalHeader = (PIMAGE_OPTIONAL_HEADER32) &PvMappedImage.NtHeaders->OptionalHeader;
		
		Properties->ImageBase = (IntPtr) (Int32) OptionalHeader->ImageBase;
		Properties->SizeOfImage = OptionalHeader->SizeOfImage;
		Properties->EntryPoint = (IntPtr) (Int32) OptionalHeader->AddressOfEntryPoint;
	}
	else
	{
		PIMAGE_OPTIONAL_HEADER64 OptionalHeader = (PIMAGE_OPTIONAL_HEADER64)&PvMappedImage.NtHeaders->OptionalHeader;

		Properties->ImageBase = (IntPtr)(Int64)OptionalHeader->ImageBase;
		Properties->SizeOfImage = OptionalHeader->SizeOfImage;
		Properties->EntryPoint = (IntPtr)(Int64)OptionalHeader->AddressOfEntryPoint;

	}

	Properties->Subsystem = PvMappedImage.NtHeaders->OptionalHeader.Subsystem;
	Properties->Characteristics = PvMappedImage.NtHeaders->FileHeader.Characteristics;
	Properties->DllCharacteristics = PvMappedImage.NtHeaders->OptionalHeader.DllCharacteristics;
}

PE::~PE()
{
    delete m_Impl;
}

Collections::Generic::List<PeExport^> ^ PE::GetExports()
{
	Collections::Generic::List<PeExport^> ^Exports = gcnew Collections::Generic::List<PeExport^>();

	if (NT_SUCCESS(PhGetMappedImageExports(&m_Impl->m_PvExports, &m_Impl->m_PvMappedImage)))
	{
		for (size_t Index = 0; Index < m_Impl->m_PvExports.NumberOfEntries; Index++)
		{
			Exports->Add(gcnew PeExport(*m_Impl, Index));
		}
	}

	return Exports;
}


Collections::Generic::List<PeImportDll^> ^ PE::GetImports()
{
	Collections::Generic::List<PeImportDll^> ^Imports = gcnew Collections::Generic::List<PeImportDll^>();

	// Standard Imports
	if (NT_SUCCESS(PhGetMappedImageImports(&m_Impl->m_PvImports, &m_Impl->m_PvMappedImage)))
	{
		for (size_t IndexDll = 0; IndexDll< m_Impl->m_PvImports.NumberOfDlls; IndexDll++)
		{
			Imports->Add(gcnew PeImportDll(&m_Impl->m_PvImports, IndexDll));
		}
	}

	// Delayed Imports
	if (NT_SUCCESS(PhGetMappedImageDelayImports(&m_Impl->m_PvDelayImports, &m_Impl->m_PvMappedImage)))
	{
		for (size_t IndexDll = 0; IndexDll< m_Impl->m_PvDelayImports.NumberOfDlls; IndexDll++)
		{
			Imports->Add(gcnew PeImportDll(&m_Impl->m_PvDelayImports, IndexDll));
		}
	}

	return Imports;
}