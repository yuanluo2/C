/*
    on my windows cmd, the default code page is 936(GBK), not 65001(UTF-8), so
    I have to prevent the mojibake, then I write these code. This file itself is
    encoded in utf-8. 

    These code is only works for windows operating system, so never use it on 
    linux or other systems.
*/
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
/* 
    string convert from unicode to ascii. 
    return NULL when out of memory.
    finally, you must call free() on the return value.
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
    string convert from utf8 to unicode. 
    return NULL if out of memory.
    finally, you must call free() on the return value.
*/
wchar_t* conv_utf8_to_unicode(const char* utf8str){
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, NULL,0);
	wchar_t* wstr = (wchar_t*)malloc(len * sizeof(wchar_t));

    if (wstr != NULL){
        MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, wstr, len);
    }
    else {
        fprintf(stderr, "[%s %s] %s(%d) out of memory.\n", __DATE__, __TIME__, __FUNCTION__, __LINE__);
    }
    
    return wstr;
}

/* 
    string convert from utf8 to ascii. 
    finally, you must call free() on the return value.
*/
char* conv_utf8_to_ascii(const char* utf8str){
    wchar_t* wstr = conv_utf8_to_unicode(utf8str);

    if (wstr == NULL){
        return NULL;
    }

    char* str = conv_unicode_to_ascii(wstr);
    if (str == NULL){
        free(wstr);
        return NULL;
    }

    free(wstr);
    return str;
}
#endif

int main(){
    const char* testing = "文字测试, yeah, very interesting, はつね みく";

    #ifdef _WIN32
    char* result = conv_utf8_to_ascii(testing);
    #endif
    
    if (result != NULL) {
        printf("%s\n", result);
        free(result);
    }
}
