#pragma once

void CareerStorage_FillAddress();
void CareerStorage_InstallHooks();
void CareerStorage_UninstallHooks();

using CareerDifficultyCtor = void*(__fastcall*)(void*, int, const char*, void*);
void* CareerStorage_ConstructDifficulty(CareerDifficultyCtor original, void* self, int dummy, const char* name, void* parent);
