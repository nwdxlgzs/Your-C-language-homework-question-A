#ifndef COMMONUTIL_H
#define COMMONUTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "socketLib.h"
#ifdef __WIN32__
#include <winsock2.h>
#include <fileapi.h>
#include <windows.h>
#include <shellapi.h>
#endif

static inline char* encHex(char* data, int size) {
    if (data == NULL)   return NULL;
    if (size == 0) size = strlen(data);
    char* hex = malloc(size * 2 + 1);
    for (int i = 0; i < size; i++) {
        sprintf(hex + i * 2, "%02x", data[i]);
    }
    hex[size * 2] = '\0';
    return hex;
}
#define __HEX__(x)  (((x) >= '0' && (x) <= '9') ? ((x) - '0') : (((x) >= 'a' && (x) <= 'f') ? ((x) - 'a' + 10) : (((x) >= 'A' && (x) <= 'F') ? ((x) - 'A' + 10) : 0)))
static inline char* decHex(char* hex, int* size) {
    if (hex == NULL)    return NULL;
    int len = strlen(hex);
    char* data = malloc(len / 2 + 1);
    for (int i = 0; i < len; i += 2) {
        data[i / 2] = (__HEX__(hex[i]) << 4) | (__HEX__(hex[i + 1]));
    }
    data[len / 2] = '\0';
    if (size != NULL)
        *size = len / 2;
    return data;
}
static inline char* fmtYmdHMS(char* fmt, long long timestamp) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    if (timestamp != 0) {
        t = timestamp / 1000;
        tm = localtime(&t);
    }
    char* buf = malloc(64);
    if (fmt == NULL) {
        strftime(buf, 64, "%Y-%m-%d %H:%M:%S", tm);
    } else {
        strftime(buf, 64, fmt, tm);
    }
    return buf;
}

//Unix时间戳（省略毫秒）
static inline long long getTimestamp() {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    time_t timep;
    timep = mktime(tm);
    return (timep * 1000) + 0;
}
//Unix时间戳（省略毫秒）
static inline long long getTimestampByStr(char* str) {
    struct tm tm;
    time_t timep;
    sscanf(str, "%d-%d-%d %d:%d:%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
    tm.tm_year -= 1900;
    tm.tm_mon -= 1;
    timep = mktime(&tm);
    //FMT已经不支持毫秒了
    return timep * 1000 + 0;
}
//base64编码
static inline char* encBase64(char* data, int size) {
    if (data == NULL)   return NULL;
    if (size == 0) size = strlen(data);
    char* base64 = malloc(size * 2 + 1);
    int i = 0, j = 0;
    char* base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    while (i < size) {
        base64[j++] = base64char[(data[i] >> 2) & 0x3F];
        if (i + 1 < size) {
            base64[j++] = base64char[((data[i] & 0x3) << 4) | ((data[i + 1] >> 4) & 0xF)];
            if (i + 2 < size) {
                base64[j++] = base64char[((data[i + 1] & 0xF) << 2) | ((data[i + 2] >> 6) & 0x3)];
                base64[j++] = base64char[data[i + 2] & 0x3F];
            } else {
                base64[j++] = base64char[(data[i + 1] & 0xF) << 2];
                base64[j++] = '=';
            }
        } else {
            base64[j++] = base64char[(data[i] & 0x3) << 4];
            base64[j++] = '=';
            base64[j++] = '=';
        }
        i += 3;
    }
    base64[j] = '\0';
    return base64;
}
//base64解码
static inline char* decBase64(char* base64, int* size) {
    if (base64 == NULL) return NULL;
    int len = strlen(base64);
    char* data = malloc(len + 1);
    int i = 0, j = 0;
    char* base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    while (i < len) {
        int a = strchr(base64char, base64[i++]) - base64char;
        int b = strchr(base64char, base64[i++]) - base64char;
        int c = strchr(base64char, base64[i++]) - base64char;
        int d = strchr(base64char, base64[i++]) - base64char;
        data[j++] = (a << 2) | (b >> 4);
        if (c != 64) {
            data[j++] = (b << 4) | (c >> 2);
            if (d != 64) {
                data[j++] = (c << 6) | d;
            }
        }
    }
    data[j] = '\0';
    if (size != NULL)
        *size = j;
    return data;
}
//crc32(注意，他不是加密，他是哈希)
static inline unsigned int crc32(char* data, int size) {
    if (data == NULL)   return 0;
    if (size == 0) size = strlen(data);
    unsigned int crc = 0xFFFFFFFF;
    for (int i = 0; i < size; i++) {
        crc = crc ^ data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}
//rc4加密/解密
static inline char* Rc4Core(char* data, int size, char* key, int keySize) {
    if (data == NULL)   return NULL;
    if (size == 0) size = strlen(data);
    if (key == NULL)    return NULL;
    if (keySize == 0) keySize = strlen(key);
    char* rc4 = malloc(size + 1);
    char s[256];
    char k[256];
    for (int i = 0; i < 256; i++) {
        s[i] = i;
        k[i] = key[i % keySize];
    }
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + s[i] + k[i]) % 256;
        char t = s[i];
        s[i] = s[j];
        s[j] = t;
    }
    int i = 0;
    j = 0;
    for (int n = 0; n < size; n++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        char t = s[i];
        s[i] = s[j];
        s[j] = t;
        rc4[n] = data[n] ^ s[(s[i] + s[j]) % 256];
    }
    rc4[size] = '\0';
    return rc4;
}
#define encRc4(...) Rc4Core(__VA_ARGS__)
#define decRc4(...) Rc4Core(__VA_ARGS__)
//文件读取
static inline char* readFile(char* path, int* size) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    if (len == 0) {
        fclose(fp);
        return NULL;
    }
    fseek(fp, 0, SEEK_SET);
    char* buf = malloc(len + 1);
    fread(buf, 1, len, fp);
    fclose(fp);
    buf[len] = '\0';
    if (size != NULL)
        *size = len;
    return buf;
}
//文件写入
static inline int writeFile(char* path, char* data, int size) {
    if (path == NULL)   return 0;
    if (data == NULL)   return 0;
    if (size == 0) size = strlen(data);
    FILE* fp = fopen(path, "wb");
    if (fp == NULL) return 0;
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 1;
}
//文件追加
static inline int appendFile(char* path, char* data, int size) {
    if (path == NULL)   return 0;
    if (data == NULL)   return 0;
    if (size == 0) size = strlen(data);
    FILE* fp = fopen(path, "ab");
    if (fp == NULL) return 0;
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 1;
}
//获取文件大小
static inline int getFileSize(char* path) {
    if (path == NULL)   return 0;
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return 0;
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp);
    fclose(fp);
    return len;
}
//文件是否存在
static inline int fileExists(char* path) {
    if (path == NULL)   return 0;
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) return 0;
    fclose(fp);
    return 1;
}
//目录是否存在
static inline int dirExists(char* path) {
    if (path == NULL)   return 0;
#ifdef _WIN32
    //windows
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) return 0;
    return (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
#else
    //linux/mac
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
#endif
}
//删除文件或目录
static inline int deleteFile(char* path) {
    if (path == NULL)   return 0;
    char buf[4096];
    int len = strlen(path);
    if (len > 4096) return 0;
    strcpy(buf, path);
#ifdef _WIN32
    //windows
    SHFILEOPSTRUCTA op;
    op.hwnd = NULL;
    op.wFunc = FO_DELETE;
    op.pFrom = path;
    op.pTo = NULL;
    op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    op.fAnyOperationsAborted = FALSE;
    op.hNameMappings = NULL;
    op.lpszProgressTitle = NULL;
    return SHFileOperationA(&op);
#else
    //linux/mac
    struct stat st;
    DIR* dir;
    struct dirent* ptr;
    if ((dir = opendir(path)) == NULL) {
        return remove(path);
    }
    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        }
        sprintf(buf, "%s/%s", path, ptr->d_name);
        if (ptr->d_type == 8) {
            remove(buf);
        } else if (ptr->d_type == 10) {
            unlink(buf);
        } else if (ptr->d_type == 4) {
            deleteFile(buf);
        }
    }
    closedir(dir);
    return remove(path);
#endif
    return 1;
}
// 创建目录
static inline int createDir(char* path) {
    if (path == NULL)   return 0;
    char buf[4096];
    int len = strlen(path);
    if (len > 4096) return 0;
    strcpy(buf, path);
#ifdef _WIN32
    //windows
    if (buf[len - 1] != '\\') { // use backslash for Windows
        strcat(buf, "\\");
    }
    len = strlen(buf);
    for (int i = 1; i < len; i++) {
        if (buf[i] == '\\') { // use backslash for Windows
            buf[i] = '\0';
            if (_access(buf, 0) != 0) { // use _access for Windows
                if (mkdir(buf) == -1) { // use _mkdir for Windows
                    return 0;
                }
            }
            buf[i] = '\\';
        }
    }
#else
    //linux/mac
    if (buf[len - 1] != '/') {
        strcat(buf, "/");
    }
    len = strlen(buf);
    for (int i = 1; i < len; i++) {
        if (buf[i] == '/') {
            buf[i] = '\0';
            if (access(buf, NULL) != 0) {
                if (mkdir(buf, 0777) == -1) {
                    return 0;
                }
            }
            buf[i] = '/';
        }
    }
#endif
    return 1;
}
//获取目录下所有文件（和目录）
static inline char** getFiles(char* path, int* count) {
    if (path == NULL)   return NULL;
    if (count == NULL)  return NULL;
    char buf[4096];
    int len = strlen(path);
    if (len > 4096) return NULL;
    strcpy(buf, path);
#ifdef _WIN32
    //windows
    if (buf[len - 1] != '\\') { // use backslash for Windows
        strcat(buf, "\\");
    }
    len = strlen(buf);
    strcat(buf, "*");
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(buf, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    int max = 100;
    char** files = malloc(sizeof(char*) * max);
    int i = 0;
    do {
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0) {
            continue;
        }
        if (i >= max) {
            max += 100;
            files = realloc(files, sizeof(char*) * max);
        }
        files[i] = malloc(len + strlen(ffd.cFileName) + 1);
        strcpy(files[i], ffd.cFileName);
        i++;
    } while (FindNextFileA(hFind, &ffd) != 0);
    FindClose(hFind);
    if (count != NULL)
        *count = i;
    files = realloc(files, sizeof(char*) * (i + 1));
    files[i] = NULL;
    return files;
#else
    //linux/mac
    if (buf[len - 1] != '/') {
        strcat(buf, "/");
    }
    len = strlen(buf);
    DIR* dir;
    struct dirent* ptr;
    if ((dir = opendir(path)) == NULL) {
        return NULL;
    }
    int max = 100;
    char** files = malloc(sizeof(char*) * max);
    int i = 0;
    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        }
        if (i >= max) {
            max += 100;
            files = realloc(files, sizeof(char*) * max);
        }
        files[i] = malloc(len + strlen(ptr->d_name) + 1);
        strcpy(files[i], ptr->d_name);
        i++;
    }
    closedir(dir);
    if (count != NULL)
        *count = i;
    files = realloc(files, sizeof(char*) * (i + 1));
    files[i] = NULL;
    return files;
#endif
}

#endif // COMMONUTIL_H