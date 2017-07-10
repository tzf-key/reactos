/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/available.cpp
 * PURPOSE:         Functions for working with available applications
 * PROGRAMMERS:     Dmitry Chapyshev           (dmitry@reactos.org)
 *                  Ismael Ferreras Morezuelas (swyterzone+ros@gmail.com)
 *                  Alexander Shaposhnikov     (chaez.san@gmail.com)
 */

#include "rapps.h"

ATL::CAtlList<PAPPLICATION_INFO> InfoList;

inline void InsertTextAfterLoaded_RichEdit(UINT uStringID, const ATL::CStringW& szText, DWORD StringFlags, DWORD TextFlags)
{
    ATL::CStringW szLoadedText;
    if (!szText.IsEmpty() && szLoadedText.LoadStringW(hInst, uStringID))
    {
        InsertRichEditText(szLoadedText, StringFlags);
        InsertRichEditText(szText, TextFlags);
    }
}

inline void InsertLoadedTextNewl_RichEdit(UINT uStringID, DWORD StringFlags)
{
    ATL::CStringW szLoadedText;
    if (szLoadedText.LoadStringW(hInst, uStringID))
    {
        InsertRichEditText(L"\n", 0);
        InsertRichEditText(szLoadedText, StringFlags);
        InsertRichEditText(L"\n", 0);
    }
}

inline BOOL GetString(LPCWSTR lpKeyName, ATL::CStringW& ReturnedString, const ATL::CStringW& cFileName)
{
    if (!ParserGetString(lpKeyName, cFileName, ReturnedString))
    {
        ReturnedString.Empty();
        return FALSE;
    }
    return TRUE;
}

//App is "installed" if the RegName or Name is in the registry
inline BOOL IsAppInstalledKey(PAPPLICATION_INFO Info, REGSAM key)
{
    return (!Info->szRegName.IsEmpty()
            && (IsInstalledApplication(Info->szRegName, TRUE, key)
                || IsInstalledApplication(Info->szRegName, FALSE, key)))
        || (!Info->szName.IsEmpty()
            && (IsInstalledApplication(Info->szName, TRUE, key)
                || IsInstalledApplication(Info->szName, FALSE, key)));
}


//Check both registry keys in 64bit system
//TODO: check system type beforehand to avoid double checks?
inline BOOL IsAppInstalled(PAPPLICATION_INFO Info)
{
    return  IsAppInstalledKey(Info, KEY_WOW64_32KEY)
        || IsAppInstalledKey(Info, KEY_WOW64_64KEY);
}

//App is "installed" if the RegName or Name is in the registry
inline BOOL GetInstalledVersionWithKey(PAPPLICATION_INFO Info, ATL::CStringW& szVersion, REGSAM key)
{
    return (!Info->szRegName.IsEmpty()
            && (InstalledVersion(szVersion, Info->szRegName, TRUE, key)
                || InstalledVersion(szVersion, Info->szRegName, FALSE, key)))
        || (!Info->szName.IsEmpty()
            && (InstalledVersion(szVersion, Info->szName, TRUE, key)
                || InstalledVersion(szVersion, Info->szName, FALSE, key)));
}

inline BOOL GetInstalledVersion(PAPPLICATION_INFO Info, ATL::CStringW& szVersion)
{
    return  GetInstalledVersionWithKey(Info, szVersion, KEY_WOW64_32KEY)
        || GetInstalledVersionWithKey(Info, szVersion, KEY_WOW64_64KEY);
}

LIST_ENTRY CachedEntriesHead = {&CachedEntriesHead, &CachedEntriesHead};
PLIST_ENTRY pCachedEntry = &CachedEntriesHead;

BOOL
ShowAvailableAppInfo(INT Index)
{
    PAPPLICATION_INFO Info = (PAPPLICATION_INFO) ListViewGetlParam(Index);
    ATL::CStringW szVersion;
    ATL::CStringW szLicense;
    BOOL bIsInstalled = IsAppInstalled(Info),
        bHasVersion = GetInstalledVersion(Info, szVersion);

    if (!Info) return FALSE;

    NewRichEditText(Info->szName, CFE_BOLD);
    //installed status
    if (bIsInstalled)
    {
        if (bHasVersion)
        {
            if (Info->szVersion.Compare(szVersion) > 0)
                InsertLoadedTextNewl_RichEdit(IDS_STATUS_UPDATE_AVAILABLE, CFE_ITALIC);
            else
                InsertLoadedTextNewl_RichEdit(IDS_STATUS_INSTALLED, CFE_ITALIC);

            InsertTextAfterLoaded_RichEdit(IDS_AINFO_VERSION, szVersion, CFE_BOLD, 0);
        }
        else
            InsertLoadedTextNewl_RichEdit(IDS_STATUS_INSTALLED, CFE_ITALIC);

    }
    else
        InsertLoadedTextNewl_RichEdit(IDS_STATUS_NOTINSTALLED, CFE_ITALIC);

    InsertTextAfterLoaded_RichEdit(IDS_AINFO_AVAILABLEVERSION, Info->szVersion, CFE_BOLD, 0);
    //license
    switch (Info->LicenseType)
    {
    case LICENSE_TYPE::OpenSource:
        szLicense.LoadStringW(hInst, IDS_LICENSE_OPENSOURCE);
        break;
    case LICENSE_TYPE::Freeware:
        szLicense.LoadStringW(hInst, IDS_LICENSE_FREEWARE);
        break;
    case LICENSE_TYPE::Trial:
        szLicense.LoadStringW(hInst, IDS_LICENSE_TRIAL);
        break;
    default:
        break;
    }

    if (szLicense.IsEmpty())
    {
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_LICENSE, Info->szLicense, CFE_BOLD, 0);
    }
    else
    {
        szLicense.Format(L"(%ls)", Info->szLicense);
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_LICENSE, szLicense, CFE_BOLD, 0);
    }

    InsertTextAfterLoaded_RichEdit(IDS_AINFO_SIZE, Info->szSize, CFE_BOLD, 0);
    InsertTextAfterLoaded_RichEdit(IDS_AINFO_URLSITE, Info->szUrlSite, CFE_BOLD, CFE_LINK);
    InsertTextAfterLoaded_RichEdit(IDS_AINFO_DESCRIPTION, Info->szDesc, CFE_BOLD, 0);
    InsertTextAfterLoaded_RichEdit(IDS_AINFO_URLDOWNLOAD, Info->szUrlDownload, CFE_BOLD, CFE_LINK);

    return TRUE;
}

static BOOL
DeleteCurrentAppsDB(VOID)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;
    ATL::CStringW szCabPath;
    ATL::CStringW szSearchPath;
    ATL::CStringW szPath;
    BOOL result = TRUE;

    if (!GetStorageDirectory(szPath))
        return FALSE;

    szCabPath = szPath + L"\\rappmgr.cab";
    result = result && DeleteFileW(szCabPath.GetString());

    szPath += L"\\rapps\\";
    szSearchPath = szPath + L"*.txt";

    hFind = FindFirstFileW(szSearchPath.GetString(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
        return result;

    ATL::CStringW szTmp;
    do
    {
        szTmp = szPath + FindFileData.cFileName;
        result = result && DeleteFileW(szTmp.GetString());
    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return result;
}

BOOL
UpdateAppsDB(VOID)
{
    ATL::CStringW szPath;
    ATL::CStringW szAppsPath;
    ATL::CStringW szCabPath;

    if (!DeleteCurrentAppsDB())
        return FALSE;

    DownloadApplicationsDB(APPLICATION_DATABASE_URL);

    if (!GetStorageDirectory(szPath))
        return FALSE;

    szCabPath = szPath + L"\\rappmgr.cab";
    szAppsPath = szPath + L"\\rapps\\";

    if (!ExtractFilesFromCab(szCabPath, szAppsPath))
    {
        return FALSE;
    }

    return TRUE;
}


BOOL
EnumAvailableApplications(INT EnumType, AVAILENUMPROC lpEnumProc)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW FindFileData;
    ATL::CStringW szPath;
    ATL::CStringW szAppsPath;
    ATL::CStringW szCabPath;
    PAPPLICATION_INFO Info;


    if (!GetStorageDirectory(szPath))
        return FALSE;

    szCabPath = szPath + L"\\rappmgr.cab";
    szPath += L"\\rapps\\";
    szAppsPath = szPath;

    if (!CreateDirectoryW(szPath.GetString(), NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return FALSE;
    }

    szPath += L"*.txt";
    hFind = FindFirstFileW(szPath.GetString(), &FindFileData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        if (GetFileAttributesW(szCabPath) == INVALID_FILE_ATTRIBUTES)
            DownloadApplicationsDB(APPLICATION_DATABASE_URL);

        ExtractFilesFromCab(szCabPath, szAppsPath);
        hFind = FindFirstFileW(szPath, &FindFileData);

        if (hFind == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    do
    {
        /* loop for all the cached entries */
        POSITION CurrentListPosition = InfoList.GetHeadPosition();

        while (CurrentListPosition != NULL)
        {
            POSITION LastListPosition = CurrentListPosition;
            Info = InfoList.GetNext(CurrentListPosition);

            /* do we already have this entry in cache? */
            if (!Info->cFileName.Compare(FindFileData.cFileName))
            {
                /* is it current enough, or the file has been modified since our last time here? */
                if (CompareFileTime(&FindFileData.ftLastWriteTime, &Info->ftCacheStamp) == 1)
                {
                    /* recreate our cache, this is the slow path */
                    InfoList.RemoveAt(LastListPosition);
                    delete Info;
                    break;
                }
                else
                {
                    /* speedy path, compare directly, we already have the data */
                    goto skip_if_cached;
                }
            }
        }

        /* create a new entry */
        Info = new APPLICATION_INFO();

        if (!Info)
            break;

        Info->Category = ParserGetInt(L"Category", FindFileData.cFileName);
        INT IntBuffer = ParserGetInt(L"LicenseType", FindFileData.cFileName);
        if (IntBuffer < LICENSE_TYPE::Max)
        {
            Info->LicenseType = (LICENSE_TYPE) IntBuffer;
        }

        /* copy the cache-related fields for the next time */
        Info->cFileName = FindFileData.cFileName;
        RtlCopyMemory(&Info->ftCacheStamp, &FindFileData.ftLastWriteTime, sizeof(FILETIME));

        /* add our cached entry to the cached list */
        InfoList.AddTail(Info);

skip_if_cached:

        if (Info->Category == FALSE)
            continue;

        if (EnumType != Info->Category && EnumType != ENUM_ALL_AVAILABLE)
            continue;

        /* if our cache hit was only partial, we need to parse
           and lazily fill the rest of fields only when needed */

        if (Info->szUrlDownload.IsEmpty())
        {
            if (!GetString(L"Name", Info->szName, FindFileData.cFileName)
                || !GetString(L"URLDownload", Info->szUrlDownload, FindFileData.cFileName))
            {
                continue;
            }

            GetString(L"RegName", Info->szRegName, FindFileData.cFileName);
            GetString(L"Version", Info->szVersion, FindFileData.cFileName);
            GetString(L"License", Info->szLicense, FindFileData.cFileName);
            GetString(L"Description", Info->szDesc, FindFileData.cFileName);
            GetString(L"Size", Info->szSize, FindFileData.cFileName);
            GetString(L"URLSite", Info->szUrlSite, FindFileData.cFileName);
            GetString(L"CDPath", Info->szCDPath, FindFileData.cFileName);
            GetString(L"Language", Info->szRegName, FindFileData.cFileName);
            GetString(L"SHA1", Info->szSHA1, FindFileData.cFileName);
        }

        if (!lpEnumProc(Info))
            break;

    } while (FindNextFileW(hFind, &FindFileData) != 0);

    FindClose(hFind);

    return TRUE;
}

VOID FreeCachedAvailableEntries(VOID)
{
    PAPPLICATION_INFO Info;
    POSITION InfoListPosition = InfoList.GetHeadPosition();
    /* loop and deallocate all the cached app infos in the list */
    while (InfoListPosition)
    {
        Info = InfoList.GetAt(InfoListPosition);
        InfoList.RemoveHead();

        /* flush them down the toilet :D */
        delete Info;

        InfoListPosition = InfoList.GetHeadPosition();
    }
}