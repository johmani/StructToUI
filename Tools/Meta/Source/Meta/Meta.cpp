#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <map>
#include <regex>
#include <vector>
#include <unordered_map>
#include <array>
#include <chrono>
#include <set>

#include <clang-c/Index.h>

//#define ONLY_PRINT_AST

constexpr const char* c_LINE = "{:<30} {:<30} {:<30} {:<30} {:<10} {:<10} {:<15} {:<15} {:<5}";
#define HEADER std::format(c_LINE, "\n[Cursor Kind]", "[Spelling]", "[Type]", "[AccessSpecifier]", "[Size]", "[Offset]", "[IsAttribute]", "[HasAttribute]", "[BaseClasses]\n") 

namespace TempletText {

	const char* c_TypeArrrayText = R"(////////////////////////////////////////////
// AUTO GENERATED
////////////////////////////////////////////
INCLUDES

static Meta::Attribute s_Attributes[] = { 
	Meta::Attribute(), 
ATTRIBUTES 
};

static Meta::Field s_Fields[] = { 

FIELDS 
};
	
static Meta::Type s_Types[] = {

TYPES
};
		
static Meta::TypeRegistry s_Registry{ 
	s_Types     , std::size(s_Types), 
	s_Attributes, std::size(s_Attributes),
	s_Fields    , std::size(s_Fields) 
};

const Meta::TypeRegistry& Meta::NAME_SPACE::Registry() 
{ 
	return s_Registry; 
}
)";

	const char* c_TypeText = R"(    { "TYPE_NAME", "NAME", SIZE, FIELD_OFFSET, FIELD_COUNT, s_Fields })";
	const char* c_FieldText = R"(    { "TYPE_NAME", "NAME", SIZE, OFFSET, ATTRIBUTE_OFFSET, ATTRIBUTE_COUNT, s_Attributes })";
}

static std::unordered_map<std::filesystem::path, bool> s_Headers;
constexpr uint8_t c_AttrKeyLength = 9;
const std::set<std::string_view> c_TargetAttributes = {
	"TYPE____ ",
	"PROPERTY ",
};

struct Timer
{
	Timer()
	{
		Reset();
	}

	void Reset()
	{
		start = std::chrono::high_resolution_clock::now();
	}

	float ElapsedSeconds() const
	{
		return std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - start).count();
	}

	float ElapsedMilliseconds() const
	{
		return (float)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
	}

	float ElapsedMicroseconds() const
	{
		return (float)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
	}

	float ElapsedNanoseconds() const
	{
		return (float)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

enum class AccessSpecifier
{
	Invalid,
	Public,
	Protected,
	Private
};

enum class FieldType
{
	None = 0,

	Float, Float2, Float3, Float4,
	UInt, UInt2, UInt3, UInt4,
	Int, Int2, Int3, Int4,
	Bool, Bool2, Bool3, Bool4,

	Uint8, Uint16, Uint64,
	Int8, Int16, Int64
};

struct Field
{
	std::string typeName;
	std::string name;
	size_t size = 0;
	size_t offset = 0;
	size_t typeIndex = 0;
	uint32_t attributesOffset = 0;
	uint8_t attributesCount = 0;
	AccessSpecifier accessSpecifier;
	FieldType type;
};

struct Type
{
	std::string typeName;
	std::string name;
	std::string parents;
	size_t size = 0;
	std::vector<Field> fields;
};

const char* ToStrinig(AccessSpecifier accessSpecifier)
{
	switch (accessSpecifier)
	{
	case AccessSpecifier::Invalid:   return "AccessSpecifier::Invalid";
	case AccessSpecifier::Public:    return "AccessSpecifier::Public";
	case AccessSpecifier::Protected: return "AccessSpecifier::Protected";
	case AccessSpecifier::Private:   return "AccessSpecifier::Private";
	default:break;
	}

	return "Invalid";
}

constexpr const char* ToString(FieldType type)
{
	switch (type)
	{
	case FieldType::None:   return "Meta::FieldType::None";
	case FieldType::Float:  return "Meta::FieldType::Float";
	case FieldType::Float2: return "Meta::FieldType::Float2";
	case FieldType::Float3: return "Meta::FieldType::Float3";
	case FieldType::Float4: return "Meta::FieldType::Float4";
	case FieldType::UInt:   return "Meta::FieldType::UInt";
	case FieldType::UInt2:  return "Meta::FieldType::UInt2";
	case FieldType::UInt3:  return "Meta::FieldType::UInt3";
	case FieldType::UInt4:  return "Meta::FieldType::UInt4";
	case FieldType::Int:    return "Meta::FieldType::Int";
	case FieldType::Int2:   return "Meta::FieldType::Int2";
	case FieldType::Int3:   return "Meta::FieldType::Int3";
	case FieldType::Int4:   return "Meta::FieldType::Int4";
	case FieldType::Bool:   return "Meta::FieldType::Bool";
	case FieldType::Bool2:  return "Meta::FieldType::Bool2";
	case FieldType::Bool3:  return "Meta::FieldType::Bool3";
	case FieldType::Bool4:  return "Meta::FieldType::Bool4";
	case FieldType::Uint8:  return "Meta::FieldType::Uint8";
	case FieldType::Uint16: return "Meta::FieldType::Uint16";
	case FieldType::Uint64: return "Meta::FieldType::Uint64";
	case FieldType::Int8:   return "Meta::FieldType::Int8";
	case FieldType::Int16:  return "Meta::FieldType::Int16";
	case FieldType::Int64:  return "Meta::FieldType::Int64";
	}

	return "Unknown";
}

const char* ToStrinig(CX_CXXAccessSpecifier accessSpecifier)
{
	switch (accessSpecifier)
	{
	case CX_CXXInvalidAccessSpecifier: return "Invalid";
	case CX_CXXPublic: return "Public";
	case CX_CXXProtected: return "Protected";
	case CX_CXXPrivate: return "Private";
	default:break;
	}

	return "Invalid";
}

struct TypeRegistry
{
	std::vector<Type> types;
	std::map<std::string, uint32_t> typesMap;
	
	std::string attributes;
	uint32_t attributCount = 1;

	uint32_t typesCount = 0;

	void AddType(const Type& type)
	{
		types.emplace_back(type);
		typesMap[type.typeName] = typesCount++;
	}
};

struct VisitorData
{
	int depth = 1;
	TypeRegistry* registry;
	std::vector<std::string> currentAttributes;
};

std::vector<std::filesystem::path> FindFilesInDirectory(
	const std::filesystem::path& directory, 
	const char* extension
)
{
	std::vector<std::filesystem::path> filesPaths;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
	{
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			filesPaths.push_back(entry.path());
		}
	}

	return filesPaths;
}

void GenerateCppFileMetaData(
	const std::string& includesText, 
	const TypeRegistry& registry, 
	const std::filesystem::path& ouputFilePath, 
	const char* nameSpace
)
{
	std::ofstream outFile(ouputFilePath);

	if (!outFile.is_open())
	{
		std::cerr << "Error opening file for writing: " << ouputFilePath << "\n";
		return;
	}

	// Generate fields for each type
	auto GenerateFields = [&](const std::vector<Field>& fields)
	{
		std::string fieldText;
		for (size_t i = 0; i < fields.size(); ++i)
		{
			const auto& field = fields[i];
			std::string temp = TempletText::c_FieldText;
			temp.replace(temp.find("TYPE_NAME"), 9, field.typeName);
			temp.replace(temp.find("NAME"), 4, field.name);
			temp.replace(temp.find("SIZE"), 4, std::to_string(field.size));
			temp.replace(temp.find("OFFSET"), 6, std::to_string(field.offset));
			//temp.replace(temp.find("TYPE_INDEX"), 10, std::to_string(field.typeIndex));
			temp.replace(temp.find("ATTRIBUTE_OFFSET"), 16, std::to_string(field.attributesOffset));
			temp.replace(temp.find("ATTRIBUTE_COUNT"), 15, std::to_string(field.attributesCount));
			//temp.replace(temp.find("FIELD_TYPE"), 10, ToString(field.type));
			//temp.replace(temp.find("ACCESS_SPECIFIER"), 16, ToStrinig(field.AccessSpecifier));
			
			fieldText += temp;
			if (i < fields.size() - 1)
				fieldText += ",\n";
		}
		return fieldText;
	};

	// Generate types
	std::string typesText;
	std::string feildsText;
	size_t fieldCount = 0;
	for (size_t i = 0; i < registry.types.size(); ++i)
	{
		const auto& type = registry.types[i];
		std::string temp = TempletText::c_TypeText;
		temp.replace(temp.find("TYPE_NAME"), 9, type.typeName);
		temp.replace(temp.find("NAME"), 4, type.name);
		//temp.replace(temp.find("BASE_CLASSES"), 12, type.parents);
		temp.replace(temp.find("SIZE"), 4, std::to_string(type.size));
		temp.replace(temp.find("FIELD_OFFSET"), 12, std::to_string(fieldCount));
		temp.replace(temp.find("FIELD_COUNT"), 11, std::to_string(type.fields.size()));

		fieldCount += type.fields.size();
		feildsText += GenerateFields(type.fields);

		typesText += temp;
		if (i < registry.types.size() - 1)
		{
			typesText += ",\n";
			feildsText += ",\n";
		}
	}

	// Generate final output
	std::string finalText = TempletText::c_TypeArrrayText;
	finalText.replace(finalText.find("INCLUDES"), 8, includesText);
	finalText.replace(finalText.find("TYPES"), 5, typesText.empty() ? "    {}" : typesText);
	finalText.replace(finalText.find("FIELDS"), 6, feildsText.empty() ? "    {}" : feildsText);
	finalText.replace(finalText.find("ATTRIBUTES"), 10, registry.attributes);
	finalText.replace(finalText.find("NAME_SPACE"), 10, nameSpace);
	
	outFile << finalText;
	outFile.close();
}

void PrintNode(
	VisitorData* data, 
	const char* accessSpecifier, 
	const CXString displayName, 
	const CXString kindSpelling, 
	const CXString typeSpelling, 
	const char* baseClasses,
	size_t size, 
	size_t byteOffset, 
	const bool isAttr, 
	const bool hasAttr
)
{
	if (strcmp("StructDecl", clang_getCString(kindSpelling)) == 0 || strcmp("ClassDecl", clang_getCString(kindSpelling)) == 0)
		printf("\n");

	std::string indentedKind = std::format("{:<{}}{}",
		"",								// Placeholder for indentation
		data->depth * 2,				// Indentation width based on depth
		clang_getCString(kindSpelling)  // Actual kind spelling
	);

	std::string msg = std::format(
		c_LINE,
		indentedKind,                     
		clang_getCString(displayName),   
		clang_getCString(typeSpelling),
		accessSpecifier,
		size,
		byteOffset ,
		isAttr ? "[IS_ATTRIB]" : "",
		hasAttr ? "[HAS_ATTRIB]" : "",
		baseClasses
	);

	printf("%s\n", msg.c_str());
}

void VisitAttributes(CXCursor cursor, VisitorData* data)
{
	data->currentAttributes.clear();

	clang_visitChildren(
		cursor,
		[](CXCursor c, CXCursor parent, CXClientData clientData) {

			CXCursorKind kind = clang_getCursorKind(c);
			if (kind == CXCursor_AnnotateAttr || kind == CXCursor_UnexposedAttr)
			{
				CXString attrNameCX = clang_getCursorSpelling(c);
				const char* attrName = clang_getCString(attrNameCX);

				VisitorData* data = reinterpret_cast<VisitorData*>(clientData);

				std::string_view att = attrName;
				std::string_view key = att.substr(0, c_AttrKeyLength);

				if (c_TargetAttributes.contains(key))
					data->currentAttributes.emplace_back(att);
				
				clang_disposeString(attrNameCX);
			}
			return CXChildVisit_Continue; // Continue visiting other children
		},
		data
	);
}

#if ONLY_PRINT_AST

static CXChildVisitResult VisitTU(CXCursor current_cursor, CXCursor parent, CXClientData clientData)
{
	VisitorData* data = reinterpret_cast<VisitorData*>(clientData);

	const CXCursorKind cursor_kind = clang_getCursorKind(current_cursor);
	const bool is_attr = clang_isAttribute(cursor_kind) != 0;
	const bool has_attr = clang_Cursor_hasAttrs(current_cursor);
	const CXType cursorType = clang_getCursorType(current_cursor);

	const CXString displayName = clang_getCursorDisplayName(current_cursor);
	const CXString spelling = clang_getCursorSpelling(current_cursor);
	const CXString kindSpelling = clang_getCursorKindSpelling(cursor_kind);
	const CXString typeSpelling = clang_getTypeSpelling(cursorType);

	VisitAttributes(current_cursor, data);

	if (!data->currentAttributes.empty())
	{
		for (int i = 0; i < data->depth * 2; ++i)
			printf("-");

		printf(
			" %s '%s' <%s> : ",
			clang_getCString(kindSpelling),
			clang_getCString(spelling),
			clang_getCString(typeSpelling)
		);

		for (int i = 0; i < data->currentAttributes.size(); i++)
		{
			if(i < data->currentAttributes.size() - 1) 
				printf("<%s>", data->currentAttributes[i].c_str());
			else 
				printf("<%s>\n", data->currentAttributes[i].c_str());
		}
	}

	clang_disposeString(typeSpelling);
	clang_disposeString(kindSpelling);
	clang_disposeString(spelling);
	clang_disposeString(displayName);

	VisitorData child_data;
	child_data.depth = data->depth + 1;
	clang_visitChildren(current_cursor, VisitTU, &child_data);

	return CXChildVisit_Continue;
};

#else

std::string GetBaseClasses(CXCursor cursor)
{
	struct Data
	{
		std::string baseClasses;
	} clientData;
	
	clang_visitChildren(
		cursor,
		[](CXCursor c, CXCursor parent, CXClientData client_data)
		{
			Data* data = (Data*)client_data;

			if (clang_getCursorKind(c) == CXCursor_CXXBaseSpecifier)
			{
				CXString baseName = clang_getCursorSpelling(c);
				data->baseClasses += data->baseClasses.empty() ? clang_getCString(baseName) : std::string(",") + clang_getCString(baseName);
				clang_disposeString(baseName);
			}
			return CXChildVisit_Continue;
		},
		(void*)&clientData);

	return clientData.baseClasses;
}

std::filesystem::path GetCursorSourceFilePath(CXCursor cursor)
{
	CXSourceLocation location = clang_getCursorLocation(cursor);

	CXFile file;
	unsigned line, column, offset;
	clang_getSpellingLocation(location, &file, &line, &column, &offset);

	if (file) 
	{
		CXString filename = clang_getFileName(file);
		return clang_getCString(filename);
		clang_disposeString(filename);
	}

	return {};
}

FieldType GetFieldType(std::string& typeName)
{
	FieldType fieldType = FieldType::None;

	if (typeName.find("float") != std::string::npos)
	{
		if (typeName.find("float2") != std::string::npos)
			fieldType = FieldType::Float2;
		else if (typeName.find("float3") != std::string::npos)
			fieldType = FieldType::Float3;
		else if (typeName.find("float4") != std::string::npos)
			fieldType = FieldType::Float4;
		else
			fieldType = FieldType::Float;
	}
	else if (typeName.find("vec") != std::string::npos)
	{
		if (typeName.find("vec2") != std::string::npos)
			fieldType = FieldType::Float2;
		else if (typeName.find("vec3") != std::string::npos)
			fieldType = FieldType::Float3;
		else if (typeName.find("vec4") != std::string::npos)
			fieldType = FieldType::Float4;
	}
	else if (typeName.find("Vector") != std::string::npos)
	{
		if (typeName.find("Vector2") != std::string::npos)
			fieldType = FieldType::Float2;
		else if (typeName.find("Vector3") != std::string::npos)
			fieldType = FieldType::Float3;
		else if (typeName.find("Vector4") != std::string::npos)
			fieldType = FieldType::Float4;
	}
	else if (typeName.find("uint") != std::string::npos)
	{
		if (typeName.find("uint2") != std::string::npos)
			fieldType = FieldType::UInt2;
		else if (typeName.find("uint3") != std::string::npos)
			fieldType = FieldType::UInt3;
		else if (typeName.find("uint4") != std::string::npos)
			fieldType = FieldType::UInt4;
		else if (typeName == "uint8")
			fieldType = FieldType::Uint8;
		else if (typeName.find("uint16") != std::string::npos)
			fieldType = FieldType::Uint16;
		else if (typeName.find("uint64") != std::string::npos)
			fieldType = FieldType::Uint64;
		else
			fieldType = FieldType::UInt;
	}
	else if (typeName.find("int") != std::string::npos)
	{
		if (typeName.find("int2") != std::string::npos)
			fieldType = FieldType::Int2;
		else if (typeName.find("int3") != std::string::npos)
			fieldType = FieldType::Int3;
		else if (typeName.find("int4") != std::string::npos)
			fieldType = FieldType::Int4;
		if (typeName == "int8")
			fieldType = FieldType::Uint8;
		else if (typeName.find("int16") != std::string::npos)
			fieldType = FieldType::Uint16;
		else if (typeName.find("int64") != std::string::npos)
			fieldType = FieldType::Uint64;
		else
			fieldType = FieldType::Int;
	}
	else if (typeName.find("bool") != std::string::npos)
	{
		if (typeName.find("bool2") != std::string::npos)
			fieldType = FieldType::Bool2;
		else if (typeName.find("bool3") != std::string::npos)
			fieldType = FieldType::Bool3;
		else if (typeName.find("bool4") != std::string::npos)
			fieldType = FieldType::Bool4;
		else
			fieldType = FieldType::Bool;
	}

	return fieldType;
}

static CXChildVisitResult VisitTU(CXCursor currentCursor, CXCursor parent, CXClientData clientData)
{
	std::filesystem::path filePath = std::filesystem::absolute(GetCursorSourceFilePath(currentCursor)).lexically_normal();

	if(!s_Headers.contains(filePath))
		return CXChildVisit_Continue;

	VisitorData* data = reinterpret_cast<VisitorData*>(clientData);

	//const CXString parentDisplayName = clang_getCursorDisplayName(parent);
	const CXCursorKind cursorKind = clang_getCursorKind(currentCursor);
	const bool isAttr = clang_isAttribute(cursorKind) != 0;
	const bool has_attr = clang_Cursor_hasAttrs(currentCursor);
	const CXType parentCursorType = clang_getCursorType(parent);
	const CXType cursorType = clang_getCursorType(currentCursor);
	const CXString parentTypeSpelling = clang_getTypeSpelling(parentCursorType);
	const CXString displayName = clang_getCursorDisplayName(currentCursor);
	const CXString spelling = clang_getCursorSpelling(currentCursor);
	const CXString kindSpelling = clang_getCursorKindSpelling(cursorKind);
	const CXString typeSpelling = clang_getTypeSpelling(cursorType);
	const size_t offset = clang_Type_getOffsetOf(parentCursorType, clang_getCString(spelling)) / 8;
	const size_t size = clang_Type_getSizeOf(cursorType);
	const char* typeSpellingStr = clang_getCString(typeSpelling);
	const char* displayNameStr = clang_getCString(displayName);
	std::string baseClasses = GetBaseClasses(currentCursor);
	
	VisitAttributes(currentCursor, data);

	if (!data->currentAttributes.empty())
	{
		if (clang_getCursorKind(currentCursor) == CXCursor_StructDecl || clang_getCursorKind(currentCursor) == CXCursor_ClassDecl)
		{
			if (data->currentAttributes[0].find("TYPE____") != std::string::npos)
			{
				Type t = {
					.typeName = typeSpellingStr,
					.name = displayNameStr,
					.parents = baseClasses,
					.size = size,
				};

				data->registry->AddType(t);
			
				PrintNode(data, "", displayName, kindSpelling, typeSpelling, baseClasses.c_str(), size, offset, isAttr, has_attr);
			}
		}
		else if (clang_getCursorKind(currentCursor) == CXCursor_FieldDecl)
		{
			CX_CXXAccessSpecifier accessSpecifier = clang_getCXXAccessSpecifier(currentCursor);

			uint8_t attributeCount = 0;
			bool hasFieldAtt = false;

			for (int i = 0; i < data->currentAttributes.size(); i++)
			{
				if (data->currentAttributes[i].length() > c_AttrKeyLength)
				{
					data->registry->attributes += "    " + std::string(data->currentAttributes[i].substr(c_AttrKeyLength)) + ",\n";
					attributeCount++;

					hasFieldAtt = true;
				}
			}

			auto parentName = clang_getCString(parentTypeSpelling);
			auto& index = data->registry->typesMap.at(parentName);

			std::string typeName = typeSpellingStr;
			FieldType fieldType = GetFieldType(typeName);

			Field field = {
				.typeName = typeSpellingStr,
				.name = displayNameStr,
				.size = size,
				.offset = offset,
				.typeIndex = index,
				.attributesOffset = hasFieldAtt ? data->registry->attributCount : 0,
				.attributesCount = hasFieldAtt ? attributeCount : (uint8_t)1,
				.accessSpecifier = (AccessSpecifier)accessSpecifier,
				.type = fieldType
			};
	
			PrintNode(data, ToStrinig(accessSpecifier), displayName, kindSpelling, typeSpelling, baseClasses.c_str(), size, offset, isAttr, has_attr);
			
			data->registry->types[index].fields.push_back(field);
			data->registry->attributCount += attributeCount;
		}
	}

	VisitorData child_data;
	child_data.depth = data->depth + 1;
	child_data.registry = data->registry;
	child_data.currentAttributes = data->currentAttributes;

	//clang_disposeString(parentDisplayName);
	clang_visitChildren(currentCursor, VisitTU, &child_data);
	clang_disposeString(typeSpelling);
	clang_disposeString(parentTypeSpelling);
	clang_disposeString(kindSpelling);
	clang_disposeString(spelling);
	clang_disposeString(displayName);

	return CXChildVisit_Continue;
};
#endif

std::string GenerateParserInputFile(const std::filesystem::path& sourceDir, const std::filesystem::path& outputFile)
{
	auto files = FindFilesInDirectory(sourceDir, ".h");
	
	std::ofstream outFile(outputFile);
	if (!outFile.is_open())
	{
		std::cerr << "Error opening file for writing: " << outputFile << "\n";
		return {};
	}

	std::string includes;

	for (auto& file : files)
	{
		auto f = file.string();
		printf("header : %s \n", f.c_str());
		auto strPath = std::filesystem::relative(file, std::filesystem::path(outputFile).parent_path()).lexically_normal().string();
		includes += std::format("#include \"{}\"\n", strPath);
		s_Headers[file] = true;
	}

	outFile << includes;
	outFile.close();

	return includes;
}

int main(int argc, char* argv[])
{
	Timer totalTime;

	const char* sourceDir = argv[1];
	const char* ouputFilePathStr = argv[2];
	const char* nameSpace = argv[3];
	std::filesystem::path ouputFilePath = std::filesystem::path(ouputFilePathStr);

	if (!std::filesystem::exists(sourceDir))
	{
		printf("[HeaderTool] : sourceDir %s not exist\n", sourceDir);
		return 0;
	}

	if (!std::filesystem::exists(ouputFilePath.parent_path()))
	{
		std::string str = ouputFilePath.parent_path().string();
		printf("[HeaderTool] : ouput directory %s not exist\n", str.c_str());
		return 0;
	}


	std::string includesText = GenerateParserInputFile(sourceDir, ouputFilePath);

	{
		CXString clang_ver = clang_getClangVersion();
		printf("clang ver: %s\n", clang_getCString(clang_ver));
		clang_disposeString(clang_ver);
	}

	constexpr const char* header_args[] = {
		"-x",
		"c++",
	
		"-std=c++23",
	
		"--pedantic",
		"--pedantic-errors",

		//"-Wall",
		//"-Weverything",
		//"-Werror",
		//"-Wno-c++98-compat",
		
		"-Wno-language-extension-token",
		"-Wpragma-once-outside-header",
		"-Wno-switch",
		"-Wextra",

		"-DHE_DEBUG",
		"-DMETA",
	};

	std::vector<const char*> combinedArgs;

	for (const auto& arg : header_args) 
		combinedArgs.push_back(arg);

	for (int i = 1; i < argc; ++i)
	{
		if (std::string(argv[i]).find("-I") == 0)
			combinedArgs.push_back(argv[i]);
	}

	TypeRegistry reg;
	VisitorData data;
	data.registry = &reg;
	
	{
		Timer headerParsingTime;

		CXIndex index = clang_createIndex(1, 0);
		if (index == nullptr)
		{
			printf("error\n");
			return 1;
		}

		const int tu_flags = CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_VisitImplicitAttributes;

		CXTranslationUnit tu;
		CXErrorCode err = clang_parseTranslationUnit2
		(
			index,
			ouputFilePathStr,
			combinedArgs.data(), (int)combinedArgs.size(),
			nullptr, 0,
			tu_flags,
			&tu
		);

		if (tu == nullptr || err != CXError_Success)
		{
			printf("tu creation error: %d\n", int(err));
			return 123;
		}

		// diagnostics
		{
			const int num_diags = clang_getNumDiagnostics(tu);
			printf("diagnostics (%d):\n", num_diags);

			for (int i = 0; i < num_diags; ++i)
			{
				CXDiagnostic diag = clang_getDiagnostic(tu, i);
				CXString s = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());

				printf("%s\n", clang_getCString(s));

				clang_disposeString(s);
				clang_disposeDiagnostic(diag);
			}
		}

		printf("Meta NameSpace : %s\n", nameSpace);
		
#ifndef ONLY_PRINT_AST
		printf("%s", HEADER.c_str());
#endif // 


		CXCursor cursor = clang_getTranslationUnitCursor(tu);

		clang_visitChildren(cursor, VisitTU, &data);

		clang_disposeTranslationUnit(tu);
		tu = nullptr;

		clang_disposeIndex(index);
		index = nullptr;

		printf("headerParsingTime : %f ms\n", headerParsingTime.ElapsedMilliseconds());
	}

	GenerateCppFileMetaData(includesText, reg, ouputFilePath.parent_path() / (ouputFilePath.stem().string() + ".cpp"), nameSpace);

	printf("totalTime : %f ms\n", totalTime.ElapsedMilliseconds());

	return 0;
}