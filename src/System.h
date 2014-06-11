#pragma once

#include <QString>

void crashHandler(const char *message);
void initConsole(void);
void initErrorHandlers(void);
void printConsole(const char* text, const int &logLevel);
quint32 getCurrentThread(void);
