/************************************************************************************

+Filename    :   Settings.cpp
Content     :
Created     :	7/22/2015
Authors     :   Michael Grosse Huelsewiesche

Copyright   :   Copyright 2015 VRMatter All Rights reserved.

This source code is licensed under the GPL license found in the
LICENSE file in the StreamTheater/ directory.

*************************************************************************************/


#include "Settings.h"
#include <fstream>
#include <string>
#include "vector"
#include "OVR_JSON.h"


namespace OculusCinema
{

void PrintFileToLog(const char* filename)
{
    std::ifstream file(filename);
    std::string str;
    OVR_LOG("%s ----------------------", filename);
    while (std::getline(file, str))
    {
        OVR_LOG("%s",str.c_str());
    }
    OVR_LOG("END ---------------------");
}

/*
 * Variable holder
 */
class Settings::IVariable {
public:
	virtual ~IVariable() { free(name); name = NULL;  }
	virtual IVariable* Clone() = 0;
	// The three basic types of OVR::JSON values, since we don't know what type we're storing in this instance
	// Could have template<typename T> Load(T), but this way we don't really have to save type info to OVR::JSON
	virtual void LoadNumber(double num) = 0;
	virtual void LoadCStr(const char* str) = 0;
	virtual void LoadBool(bool b) = 0;
	virtual std::shared_ptr<OVR::JSON> Serialize() = 0;
	virtual bool IsChanged() = 0;
	virtual void SaveValue() = 0;

public:
	char* name;
	std::shared_ptr<OVR::JSON> json;
};


template<typename T>
class Settings::Variable: public Settings::IVariable
{
public:
	Variable(): initialValue(0) { name = NULL; json = NULL;}
	Variable(const char* varName, T* ptr)
	{
		name = strdup(varName);
		json = NULL;
		varPtr = ptr;
		initialValue = *ptr;
	}
	virtual ~Variable(){ free(name); name = NULL; }
	virtual IVariable* Clone()
	{
		Variable<T>* newVar = new Variable<T>();
		newVar->name = strdup(name);
		newVar->varPtr = varPtr;
		newVar->initialValue = initialValue;
		return newVar;
	}
	virtual void LoadNumber(double num) { initialValue = (T)num; *varPtr = (T)num; }
	virtual void LoadCStr(const char* str) { OVR_LOG("Loaded the wrong type! %s is not text.", name);}
	virtual void LoadBool(bool b) { initialValue = (T)b; *varPtr = (T)b;}
	virtual std::shared_ptr<OVR::JSON> Serialize()
	{
		std::shared_ptr<OVR::JSON> newJSON = OVR::JSON::CreateNumber((double)(*varPtr));
		newJSON->Name = name;
		json = NULL; // can't be sure we'll be pointing to the right thing soon
		return newJSON;
	}
	virtual bool IsChanged() { return ( *varPtr != initialValue ); }
	virtual void SaveValue() { initialValue = *varPtr; }

public:
	T* varPtr;
	T initialValue;
	typedef T type;
};

// Specialization for copying value of a c-string
template<>
class Settings::Variable<char*>: public Settings::IVariable
{
public:
	Variable(): initialValue(NULL) { name = NULL; json = NULL;}
	Variable(const char* varName, char** ptr)
	{
		name = strdup(varName);
		json = NULL;
		varPtr = ptr;
		initialValue = strdup(*ptr);
	}
	virtual ~Variable() { free(name); name = NULL; free(initialValue); }
	virtual IVariable* Clone()
	{
		Variable<char*>* newVar = new Variable<char*>();
		newVar->name = strdup(name);
		newVar->varPtr = varPtr;
		newVar->initialValue = strdup(initialValue);
		return newVar;
	}
	virtual void LoadNumber(double num) {OVR_LOG("Loaded the wrong type! %s is text.", name);}
	virtual void LoadCStr(const char* str)
	{
		if(initialValue)
		{
			free(initialValue);
		}
		initialValue = strdup(str);
		if(*varPtr)
		{
			free(*varPtr);
		}
		*varPtr = strdup(str);
	}
	virtual void LoadBool(bool b) { OVR_LOG("Loaded the wrong type! %s is text.", name);}
	virtual std::shared_ptr<OVR::JSON> Serialize()
	{
		std::shared_ptr<OVR::JSON> newJSON = OVR::JSON::CreateString(*varPtr);
		newJSON->Name = name;
		json = NULL; // can't be sure we'll be pointing to the right thing soon
		return newJSON;
	}
	virtual bool IsChanged() { return 0 != strcmp(*varPtr, initialValue); }
	virtual void SaveValue() {
		if(initialValue)
		{
			free(initialValue);
		}
		initialValue = strdup(*varPtr);
	}

public:
	char** varPtr;
	char* initialValue;
	typedef char* type;
};

// Specialization for OVR_Strings
template<>
class Settings::Variable<std::string>: public Settings::IVariable
{
public:
	Variable(): initialValue("") { name = NULL; json = NULL;}
	Variable(const char* varName, std::string* ptr)
	{
		name = strdup(varName);
		json = NULL;
		varPtr = ptr;
		initialValue = *ptr;
	}
	virtual ~Variable(){ free(name); name = NULL;  }
	virtual IVariable* Clone()
	{
		Variable<std::string>* newVar = new Variable<std::string>();
		newVar->name = strdup(name);
		newVar->varPtr = varPtr;
		newVar->initialValue = initialValue;
		return newVar;
	}
	virtual void LoadNumber(double num) { OVR_LOG("Loaded the wrong type! %s is text.", name); }
	virtual void LoadCStr(const char* str) {
		initialValue=str;
		varPtr=new std::string(str);
	}
	virtual void LoadBool(bool b) { OVR_LOG("Loaded the wrong type! %s is text.", name); }
	virtual std::shared_ptr<OVR::JSON> Serialize()
	{
		std::shared_ptr<OVR::JSON> newJSON = OVR::JSON::CreateString(varPtr->c_str());
		newJSON->Name = name;
		json = NULL; // can't be sure we'll be pointing to the right thing soon
		return newJSON;
	}
	virtual bool IsChanged() { return ( *varPtr != initialValue ); }
	virtual void SaveValue() { initialValue = *varPtr; }

public:
	std::string* varPtr;
	std::string initialValue;
	typedef std::string type;
};

/*
 * Template type helpers for GetVal
 * (Specializations for new types shouldn't have to go past this section)
 * ((unless you're adding arrays or objects))
 */
template<typename T> void SetHelper(T* source, T* toSet) { *toSet = *source; }
template<> 			 void SetHelper(char** source, char** toSet) { *toSet = strdup(*source); }
template<typename T> void JSONToTypeHelper(OVR::JSON* json, T* toSet)
{
	*toSet = (T) json->GetDoubleValue();
}
template<> void JSONToTypeHelper(OVR::JSON* json, bool* toSet)
{
	*toSet = json->GetBoolValue();
}
template<> void JSONToTypeHelper(OVR::JSON* json, char** toSet)
{
	*toSet = strdup(json->GetStringValue().c_str());
}
template<> void JSONToTypeHelper(OVR::JSON* json, std::string* toSet)
{
	*toSet = json->GetStringValue();
}


/***************************
 * Settings                *
 ***************************/
Settings::Settings() :
		settingsFileName(NULL),
		rootSettingsJSON(NULL),
		settingsJSON(NULL),
		variables()
{
	;
}

Settings::Settings(const char* filename) :
		settingsFileName(NULL),
		rootSettingsJSON(NULL),
		settingsJSON(NULL),
		variables()
{
	OpenOrCreate(filename);
}

Settings::~Settings()
{
	while(variables.size() > 0)
	{
		IVariable* var = variables.back();
		variables.pop_back();
		delete(var);
	}
}

void Settings::OpenOrCreate(const char* filename)
{
	settingsFileName = strdup(filename);
	if(!(rootSettingsJSON = OVR::JSON::Load(filename)))
	{
		OVR_LOG("Creating new settings file: %s", filename);
		rootSettingsJSON = OVR::JSON::CreateObject();
		rootSettingsJSON->AddNumberItem("SettingsVersion",SETTINGS_VERSION);
		rootSettingsJSON->AddItem("Settings",OVR::JSON::CreateObject());
		if(!rootSettingsJSON->Save(filename))
		{
			OVR_LOG("Error creating settings file: %s", filename);
			return;
		}
	}
	else
	{
		OVR_LOG("Opening existing settings file: %s", filename);
		PrintFileToLog(filename);
	}

	// Any incompatible settings versions should be dealt with here!
	settingsJSON = rootSettingsJSON->GetItemByName("Settings");

	if(settingsJSON == NULL)
	{
		OVR_LOG("Error! Invalid settings file!");
	}
}

void Settings::CopyDefines(const Settings& source)
{
	for(int i=0;i<static_cast< int >(source.variables.size());i++)
	{
		variables.push_back(source.variables[i]->Clone());
	}
}


template<typename T> void Settings::Define(const char* varName, T* ptr)
{
	Variable<T>* newVar = new Variable<T>(varName, ptr);
	variables.push_back(newVar);
}

// Specializations
template<>
void Settings::Define<int>(const char* varName, int* ptr)
{
	Variable<int>* newVar = new Variable<int>(varName, ptr);
	variables.push_back(newVar);
}

template<>
void Settings::Define<float>(const char* varName, float* ptr)
{
	Variable<float>* newVar = new Variable<float>(varName, ptr);
	variables.push_back(newVar);
}

template<>
void Settings::Define<bool>(const char* varName, bool* ptr)
{
	Variable<bool>* newVar = new Variable<bool>(varName, ptr);
	variables.push_back(newVar);
}

template<typename T> bool Settings::GetVal(const char* varName, T* toSet)
{
	if(settingsJSON == NULL) return false;

	// Check all the variables first
	for(int i = 0; i < static_cast< int >(variables.size()); i++)
	{
		if(strcmp(variables[i]->name, varName) == 0)
		{
			Variable<T>* var = dynamic_cast<Variable<T>*>(variables[i]);
			if(var == NULL)
			{
				return false;
			}
			SetHelper(var->varPtr, toSet);
			return true;
		}
	}

	// Still here?  It wasn't in the defined objects.
	auto valJSON = settingsJSON->GetItemByName(varName);
	if(valJSON)
	{
		JSONToTypeHelper<T>(valJSON, toSet);
		return true;
	}
	return false;
}

bool Settings::IsChanged()
{
	if(settingsJSON == NULL) return false;
	for(int i = 0; i < static_cast< int >(variables.size()); i++)
	{
		IVariable* var = variables[i];
		var->SaveValue();
		if(var->IsChanged())
		{
			return true;
		}
	}
	return false;
}


/*
void Settings::DeleteVar(const char* varName)
{
	if(settingsJSON == NULL) return;

	// Undefine the variable
	for(int i = 0; i < static_cast< int >(variables.size()); i++)
	{
		if(strcmp(variables[i]->name, varName) == 0)
		{
			IVariable* toDel = variables[i];
			variables[i] = variables.back();
			variables.pop_back();
			delete(toDel);
			break;
		}
	}

	auto valJSON = settingsJSON->GetItemByName(varName);
	if(valJSON)
	{
		valJSON->RemoveNode();
		rootSettingsJSON->Save(settingsFileName);
	}
}
*/
void Settings::Load()
{
	if(settingsJSON == NULL) return;

	for(int i = 0; i < static_cast<int>(variables.size()); i++)
	{
		IVariable* var = variables[i];
		auto varJSON = var->json;
		if(varJSON == NULL)
		{
			varJSON = settingsJSON->GetItemByName(var->name);
		}
		if(varJSON)
		{
			var->json = varJSON;
			switch(varJSON->Type)
			{
			case OVR::JSON_Bool:
				var->LoadBool(varJSON->GetBoolValue());
				break;
			case OVR::JSON_Number:
				var->LoadNumber(varJSON->GetDoubleValue());
				break;
			case OVR::JSON_String:
				var->LoadCStr(varJSON->GetStringValue().c_str());
				break;
			default:
				break;
			}
		}
	}
}

void Settings::SaveAll()
{
	if(settingsJSON == NULL) return;

	for(int i = 0; i < static_cast<int>(variables.size()); i++)
	{
		IVariable* var = variables[i];
		auto varJSON = var->json;
		if(varJSON == NULL)
		{
			varJSON = settingsJSON->GetItemByName(var->name);
			var->json = varJSON;
		}
		if(varJSON == NULL)
		{
			settingsJSON->AddItem(var->name,var->Serialize());
		}
		else
		{
			varJSON->ReplaceNodeWith( var->name, var->Serialize() );
		}
	}
	rootSettingsJSON->Save(settingsFileName);
}

void Settings::SaveChanged()
{
	if(settingsJSON == NULL) return;
	for(int i = 0; i < static_cast<int>(variables.size()); i++)
	{
		IVariable* var = variables[i];
		if(!var->IsChanged())
		{
			continue;
		}
		var->SaveValue();

		auto varJSON = var->json;
		if(varJSON == NULL)
		{
			varJSON = settingsJSON->GetItemByName(var->name);
			var->json = varJSON;
		}
		if(varJSON == NULL)
		{
			settingsJSON->AddItem(var->name,var->Serialize());
		}
		else
		{
			varJSON->ReplaceNodeWith( var->name, var->Serialize() );
		}
	}
	rootSettingsJSON->Save(settingsFileName);
	PrintFileToLog(settingsFileName);
}

void Settings::SaveOnly(const std::vector<const char*> &varNames)
{
	if(settingsJSON == NULL) return;

	for(int i = 0; i < static_cast<int>(variables.size()); i++)
	{
		IVariable* var = variables[i];
		bool found = false;
		for(int namesIndex = 0; namesIndex < static_cast<int>(varNames.size()); namesIndex++)
		{
			if(strcmp(var->name, varNames[namesIndex]) == 0)
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			continue;
		}
		var->SaveValue();
		auto varJSON = var->json;
		if(varJSON == NULL)
		{
			varJSON = settingsJSON->GetItemByName(var->name);
			var->json = varJSON;
		}
		if(varJSON == NULL)
		{
			settingsJSON->AddItem(var->name,var->Serialize());
		}
		else
		{
			varJSON->ReplaceNodeWith( var->name, var->Serialize() );
		}
	}
	rootSettingsJSON->Save(settingsFileName);
}

void	Settings::SaveVarNames()
{
	if(settingsJSON == NULL) return;

	for(int i = 0; i < static_cast<int>(variables.size()); i++)
	{
		IVariable* var = variables[i];
		auto varJSON = var->json;
		if(varJSON == NULL)
		{
			varJSON = settingsJSON->GetItemByName(var->name);
			var->json = varJSON;
		}
		if(varJSON == NULL)
		{
			settingsJSON->AddItem(var->name,OVR::JSON::CreateNull());
		}
	}
	rootSettingsJSON->Save(settingsFileName);
}
/*
#ifndef NDEBUG
#include <assert.h>
void SettingsTest(std::string packageName)
{
	bool b = false;
	int i = 1;
	float f = 2.0f;
	double d = 3.0;
	char* cstr = strdup("cstring");
	std::string str("string");

	std::string appFileStoragePath = "/data/data/";
	appFileStoragePath += packageName;
	appFileStoragePath += "/files/";

    std::string FilePath = appFileStoragePath + "settingstest.OVR::JSON";

	OVR_LOG("Opening file");
	Settings* s = new Settings(FilePath.c_str());

	OVR_LOG("Deleting variables");
	s->DeleteVar("testbool");
	s->DeleteVar("testint");
	s->DeleteVar("testfloat");
	s->DeleteVar("testdouble");
	s->DeleteVar("testcstr");
	s->DeleteVar("teststr");

	OVR_LOG("Defining variables");
	s->Define("testbool",&b);
	s->Define("testint",&i);
	s->Define("testfloat",&f);
	s->Define("testdouble",&d);
	s->Define("testcstr",&cstr);
	s->Define("teststr",&str);

	OVR_LOG("Changing values");
	b = true;
	i = 5;
	cstr = strdup("changed cstr");

	OVR_LOG("Saving changes");
	s->SaveChanged();

	OVR_LOG("resetting values");
	b = false;
	i = 4;
	f = 6.0f;
	str="changed String";

	std::vector<const char*> onlysave;
	onlysave.push_back("teststr");
	OVR_LOG("Saving only string value");
	s->SaveOnly(onlysave);

	OVR_LOG("Changing string value");
	cstr = strdup("changed cstr 2");
	str="changed String 2";

	OVR_LOG("Closing settings");
	delete(s);

	OVR_LOG("Opening file again");
	s = new Settings(FilePath.c_str());

	OVR_LOG("Defining stuff");
	s->Define("testint",&i);
	s->Define("testfloat",&f);
	s->Define("testdouble",&d);
	s->Define("testcstr",&cstr);
	s->Define("teststr",&str);

	OVR_LOG("Loading variables");
	s->Load();

	OVR_LOG("Checking values");

	assert( b == false ); // didn't define it
	assert( i == 5 ); // should revert
	assert( f == 6.0f ); // shouldn't have been saved
	assert( d == 3.0 ); // should stay same
	assert( strcmp(cstr,"changed cstr") == 0 ); // C-Strings revert ok?
	OVR_LOG("Test %s",str.c_str());
	assert( str == "changed String" ); // Saved on its own?
}
#endif
*/
} // namespace 