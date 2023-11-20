#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32

#define isDirectory(attribute) ((attribute) & FILE_ATTRIBUTE_DIRECTORY)

/*
    traverse directory recursively, works for ascii.
*/
void traverseDir(const char* dirName){
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

    char searchPath[MAX_PATH];
    char fullDirPath[MAX_PATH];

    sprintf(searchPath, "%s\\*", dirName);

    hFind = FindFirstFileA(searchPath, &wfd);
    if (hFind == INVALID_HANDLE_VALUE){
        fprintf(stderr, "[%s %s] %s(%d) search file %s failed.\n", __DATE__, __TIME__, __FUNCTION__, __LINE__, dirName);
        return;
    }

    do {
        /* ignores . and .. */
        if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0){
            continue;
        }

        sprintf(fullDirPath, "%s\\%s", dirName, wfd.cFileName);
        printf("%s\n", fullDirPath);

        /* if path is a directoy, then traverse it recursively. */
        if (isDirectory(wfd.dwFileAttributes)) {
            traverseDir(fullDirPath);
        }        
    } while(FindNextFileA(hFind, &wfd));

    FindClose(hFind);
}

/* 
    string convert from unicode to ascii. 
    return NULL if out of memory.
    * finally, you must call free() on the return value.
    only works for windows operating system.
*/
char* conv_unicode_to_ascii(const wchar_t* wstr){
    int len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* buffer = (char*)malloc(len * sizeof(char));

    if (buffer != NULL){
        WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, len, NULL, NULL);
    }
    else {
        fprintf(stderr, "[%s %s] %s(%d) out of memory.\n", __DATE__, __TIME__, __FUNCTION__, __LINE__);
    }

    return buffer;
}

/*
    traverse directory recursively, works for unicode.
*/
void traverseDirWide(const wchar_t* dirName){
    WIN32_FIND_DATAW wfd;
    HANDLE hFind;
    char* asciiDirName;

    wchar_t searchPath[MAX_PATH];
    wchar_t fullDirPath[MAX_PATH];

    swprintf(searchPath, MAX_PATH, L"%s\\*", dirName);

    hFind = FindFirstFileW(searchPath, &wfd);
    if (hFind == INVALID_HANDLE_VALUE){
        asciiDirName = conv_unicode_to_ascii(dirName);  /* no need to check asciiDirName == NULL here, FindFirstFileW has already failed. */
        fprintf(stderr, "[%s %s] %s(%d) search file %s failed.\n", __DATE__, __TIME__, __FUNCTION__, __LINE__, asciiDirName);
        free(asciiDirName);
        
        return;
    }

    do {
        /* ignores . and .. */
        if (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0){
            continue;
        }

        swprintf(fullDirPath, MAX_PATH, L"%s\\%s", dirName, wfd.cFileName);
        asciiDirName = conv_unicode_to_ascii(fullDirPath);

        if (asciiDirName == NULL){
            exit(EXIT_FAILURE);
        }
        else {
            printf("%s\n", asciiDirName);
            free(asciiDirName);
        }

        /* if path is a directoy, then traverse it recursively. */
        if (isDirectory(wfd.dwFileAttributes)) {
            traverseDirWide(fullDirPath);
        }        
    } while(FindNextFileW(hFind, &wfd));

    FindClose(hFind);
}
#endif

int main(){
    const wchar_t* dir = L"E:\\Bjarne_Stroustrup";
    DWORD attributes = GetFileAttributesW(dir);

    if (attributes == INVALID_FILE_ATTRIBUTES){
        fprintf(stderr, "[%s %s] %s(%d) Given dir has problem, please check if it is valid.\n", __DATE__, __TIME__, __FUNCTION__, __LINE__);
        return -1;
    }

    if (isDirectory(attributes)){
        traverseDirWide(dir);
    }

    return 0;
}
