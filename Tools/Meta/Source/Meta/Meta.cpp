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

#define ONLY_PRINT_AST 0

constexpr const char* LINE = "{:<30} {:<30} {:<30} {:<30} {:<30} {:<10} {:<10} {:<15} {:<15} {:<5}";
#define HEADER std::format(LINE, "\n[Cursor Kind]", "[Spelling]", "[Type]", "[AccessSpecifier]", "[Attribute]", "[Size]", "[Offset]", "[IsAttribute]", "[HasAttribute]", "[BaseClasses]\n") 

constexpr bool PRETTY_PRINTED = false;

namespace TempletText {

	const char* TypeArrrayText = R"(////////////////////////////////////////////
// AUTO GENERATED
////////////////////////////////////////////
INCLUDES

static Meta::Field s_Fields[] = { 

FIELDS 
};
	
static Meta::Type s_Types[] = {

TYPES
};
		
static Meta::TypeRegistry s_Registry{ 
	s_Types, std::size(s_Types), 
	s_Fields, std::size(s_Fields) 
};

const Meta::TypeRegistry& Meta::NAME_SPACE::Registry() 
{ 
	return s_Registry; 
}
)";

	const char* TypeText = R"(    { "TYPE_NAME", "NAME", SIZE, FIELD_OFFSET, FIELD_COUNT, s_Fields })";
	const char* FieldText = R"(    { "TYPE_NAME", "NAME", SIZE, OFFSET })";
}

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

struct Field
{
	std::string TypeName;
	std::string Name;
	size_t Size = 0;
	size_t Offset = 0;
	size_t TypeIndex = 0;

	uint32_t attributesOffset = 0;
	uint8_t attributesCount = 0;

	AccessSpecifier AccessSpecifier;
};

struct Type
{
	std::string TypeName;
	std::string Name;
	std::string Parents;
	size_t Size = 0;
	std::vector<Field> Fields;
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

	void AddType(Type type)
	{
		types.emplace_back(type);
		typesMap[type.TypeName] = typesCount++;
	}

	template<typename T>
	const Type& GetMetaData()
	{
		auto index = typesMap.at(GetTypeName(typeid(T).name()));
		return typesMap.at(index);
	}
};

static std::unordered_map<std::filesystem::path, bool> Headers;
constexpr uint8_t AttrKeyLength = 9;
static const std::set<std::string_view> TargetAttributes = {
	"TYPE____ ",
	"PROPERTY ", 
};

struct VisitorData
{
	int depth = 1;
	TypeRegistry* registry;
	std::string currentAttribute;
};

std::vector<std::filesystem::path> FindFilesInDirectory(const std::filesystem::path& directory, const char* extension)
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


void GenerateCppFileMetaData(const std::string& includesText, const TypeRegistry& registry, const std::filesystem::path& ouputFilePath, const char* nameSpace)
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
			std::string temp = TempletText::FieldText;
			temp.replace(temp.find("TYPE_NAME"), 9, field.TypeName);
			temp.replace(temp.find("NAME"), 4, field.Name);
			temp.replace(temp.find("SIZE"), 4, std::to_string(field.Size));
			temp.replace(temp.find("OFFSET"), 6, std::to_string(field.Offset));
			//temp.replace(temp.find("TYPE_INDEX"), 10, std::to_string(field.TypeIndex));
			//temp.replace(temp.find("ATTRIBUTE_OFFSET"), 16, std::to_string(field.attributesOffset));
			//temp.replace(temp.find("ATTRIBUTE_COUNT"), 15, std::to_string(field.attributesCount));
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
		std::string temp = TempletText::TypeText;
		temp.replace(temp.find("TYPE_NAME"), 9, type.TypeName);
		temp.replace(temp.find("NAME"), 4, type.Name);
		//temp.replace(temp.find("BASE_CLASSES"), 12, type.Parents);
		temp.replace(temp.find("SIZE"), 4, std::to_string(type.Size));
		temp.replace(temp.find("FIELD_OFFSET"), 12, std::to_string(fieldCount));
		temp.replace(temp.find("FIELD_COUNT"), 11, std::to_string(type.Fields.size()));

		fieldCount += type.Fields.size();
		feildsText += GenerateFields(type.Fields);

		typesText += temp;
		if (i < registry.types.size() - 1)
		{
			typesText += ",\n";
			feildsText += ",\n";
		}
	}

	// Generate final output
	std::string finalText = TempletText::TypeArrrayText;
	finalText.replace(finalText.find("INCLUDES"), 8, includesText);
	finalText.replace(finalText.find("TYPES"), 5, typesText.empty() ? "    {}" : typesText);
	finalText.replace(finalText.find("FIELDS"), 6, feildsText.empty() ? "    {}" : feildsText);
	//finalText.replace(finalText.find("ATTRIBUTES"), 10, registry.attributes);
	finalText.replace(finalText.find("NAME_SPACE"), 10, nameSpace);
	
	outFile << finalText;
	outFile.close();
}

void PrintNode(VisitorData* data, const char* accessSpecifier,const CXString display_name,const CXString kind_spelling, const CXString type_spelling,const char* baseClasses, const char* att, size_t size,size_t byteOffset, const bool is_attr, const bool has_attr)
{
	if (strcmp("StructDecl", clang_getCString(kind_spelling)) == 0 || strcmp("ClassDecl", clang_getCString(kind_spelling)) == 0)
		printf("\n");

	std::string indentedKind = std::format("{:<{}}{}",
		"",								// Placeholder for indentation
		data->depth * 2,				// Indentation width based on depth
		clang_getCString(kind_spelling) // Actual kind spelling
	);

	std::string msg = std::format(
		LINE,
		indentedKind,                     
		clang_getCString(display_name),   
		clang_getCString(type_spelling),  
		accessSpecifier,
		att,
		size,
		byteOffset ,
		is_attr ? "[IS_ATTRIB]" : "",        
		has_attr ? "[HAS_ATTRIB]" : "",     
		baseClasses
	);

	printf("%s\n", msg.c_str());
}

std::string_view GetAttribute(CXCursor cursor, VisitorData* data)
{
	clang_visitChildren(
		cursor,
		[](CXCursor c, CXCursor parent, CXClientData clientData) {

			CXCursorKind kind = clang_getCursorKind(c);
			if (kind == CXCursor_AnnotateAttr || kind == CXCursor_UnexposedAttr)
			{
				CXString attrNameCX = clang_getCursorSpelling(c);
				const char* attrName = clang_getCString(attrNameCX);
				VisitorData* data = reinterpret_cast<VisitorData*>(clientData);

				std::string_view key = attrName;
				std::string_view k = key.substr(0, AttrKeyLength);

				data->currentAttribute = attrName;
				clang_disposeString(attrNameCX);

				if (TargetAttributes.contains(k))
					return CXChildVisit_Break;
			}
			return CXChildVisit_Continue; // Continue visiting other children
		},
		data
	);

	return data->currentAttribute;
}

#if ONLY_PRINT_AST

static CXChildVisitResult VisitTU(CXCursor current_cursor, CXCursor parent, CXClientData client_data)
{
	VisitorData* data = reinterpret_cast<VisitorData*>(client_data);

	const CXCursorKind cursor_kind = clang_getCursorKind(current_cursor);
	const bool is_attr = clang_isAttribute(cursor_kind) != 0;
	const bool has_attr = clang_Cursor_hasAttrs(current_cursor);

	const CXString display_name = clang_getCursorDisplayName(current_cursor);
	const CXString spelling = clang_getCursorSpelling(current_cursor);
	const CXString kind_spelling = clang_getCursorKindSpelling(cursor_kind);
	const CXPrintingPolicy print_policy = clang_getCursorPrintingPolicy(current_cursor);
	const CXString pretty_printed = clang_getCursorPrettyPrinted(current_cursor, print_policy);
	const CXType cursor_type = clang_getCursorType(current_cursor);
	const CXString type_spelling = clang_getTypeSpelling(cursor_type);

	auto attribute = GetAttribute(current_cursor, data);
	if (!attribute.empty())
	{
		for (int i = 0; i < data->depth * 2; ++i)
			printf("-");

		printf(
			" %s '%s' <%s>\n",
			clang_getCString(kind_spelling),
			clang_getCString(spelling),
			clang_getCString(type_spelling)
		);

		
	}

	clang_disposeString(type_spelling);
	clang_disposeString(pretty_printed);
	clang_PrintingPolicy_dispose(print_policy);
	clang_disposeString(kind_spelling);
	clang_disposeString(spelling);
	clang_disposeString(display_name);

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

uint8_t CountAttributes(const std::string& str)
{
	std::regex attrRegex(R"((\w+::)*\w+\s*(\([^)]*\))?)");
	auto matchesBegin = std::sregex_iterator(str.begin(), str.end(), attrRegex);
	auto matchesEnd = std::sregex_iterator();
	return (uint8_t)std::distance(matchesBegin, matchesEnd);
}


static CXChildVisitResult VisitTU(CXCursor current_cursor, CXCursor parent, CXClientData client_data)
{
	std::filesystem::path filePath = std::filesystem::absolute(GetCursorSourceFilePath(current_cursor)).lexically_normal();
	if(!Headers.contains(filePath))
		return CXChildVisit_Continue;

	VisitorData* data = reinterpret_cast<VisitorData*>(client_data);

	const CXModule currentmModule = clang_Cursor_getModule (current_cursor);
	
	const CXCursorKind cursor_kind = clang_getCursorKind(current_cursor);
	const bool is_attr = clang_isAttribute(cursor_kind) != 0;
	const bool has_attr = clang_Cursor_hasAttrs(current_cursor);

	const CXString parent_display_name = clang_getCursorDisplayName(parent);
	CXType parent_cursor_type = clang_getCursorType(parent);
	const CXString parent_type_spelling = clang_getTypeSpelling(parent_cursor_type);

	const CXString display_name = clang_getCursorDisplayName(current_cursor);
	const CXString spelling = clang_getCursorSpelling(current_cursor);
	const CXString kind_spelling = clang_getCursorKindSpelling(cursor_kind);
	const CXType cursor_type = clang_getCursorType(current_cursor);
	const CXString type_spelling = clang_getTypeSpelling(cursor_type);
	//const CXPrintingPolicy print_policy = clang_getCursorPrintingPolicy(current_cursor);
	//const CXString pretty_printed = clang_getCursorPrettyPrinted(current_cursor, print_policy);

	size_t offset = clang_Type_getOffsetOf(parent_cursor_type, clang_getCString(spelling)) / 8;
	size_t size = 0;

	auto type_spelling_str = clang_getCString(type_spelling);
	auto display_name_str = clang_getCString(display_name);
	std::string baseClasses = GetBaseClasses(current_cursor);
	
	auto attribute = GetAttribute(current_cursor, data);

	if (!attribute.empty())
	{
		if (clang_getCursorKind(current_cursor) == CXCursor_StructDecl || clang_getCursorKind(current_cursor) == CXCursor_ClassDecl)
		{
			size = clang_Type_getSizeOf(cursor_type);

			if (attribute.find("TYPE____") != std::string::npos)
			{
				Type t = {
					.TypeName = type_spelling_str,
					.Name = display_name_str,
					.Parents = baseClasses,
					.Size = size,
				};
				data->registry->AddType(t);
			
				PrintNode(data, "", display_name, kind_spelling, type_spelling, baseClasses.c_str(), attribute.data(), size, offset, is_attr, has_attr);
			}
		}
		else if (clang_getCursorKind(current_cursor) == CXCursor_FieldDecl)
		{
			size = clang_Type_getSizeOf(cursor_type);
			CX_CXXAccessSpecifier accessSpecifier = clang_getCXXAccessSpecifier(current_cursor);

			uint8_t count = 0;
			bool hasFeildAtt = !attribute.substr(AttrKeyLength).empty();
			if (hasFeildAtt)
			{
				count = CountAttributes(std::string(attribute.substr(AttrKeyLength)));
				data->registry->attributes += std::string(attribute.substr(AttrKeyLength)) + ",";
				
			}

			auto parentName = clang_getCString(parent_type_spelling);
			auto& index = data->registry->typesMap.at(parentName);
			
			Field f = {
				.TypeName = type_spelling_str,
				.Name = display_name_str,
				.Size = size,
				.Offset = offset,
				.TypeIndex = index,
				.attributesOffset = hasFeildAtt ? data->registry->attributCount : 0,
				.attributesCount = hasFeildAtt ? count : uint8_t(1),
				.AccessSpecifier = (AccessSpecifier)accessSpecifier,
			};
	
			PrintNode(data, ToStrinig(accessSpecifier), display_name, kind_spelling, type_spelling, baseClasses.c_str(), attribute.data(), size, offset, is_attr, has_attr);
			
			data->registry->types[index].Fields.push_back(f);
			data->registry->attributCount += count;
		}
	}

	//PrintNode(data, "", display_name, kind_spelling, type_spelling, baseClasses.c_str(), attribute.data(), size, offset, is_attr, has_attr);

	data->currentAttribute = "";

	//if constexpr (PRETTY_PRINTED)
	//	printf("%s\n", clang_getCString(pretty_printed));

	VisitorData child_data;
	child_data.depth = data->depth + 1;
	child_data.registry = data->registry;
	child_data.currentAttribute = data->currentAttribute;

	clang_visitChildren(current_cursor, VisitTU, &child_data);

	clang_disposeString(parent_display_name);

	clang_disposeString(type_spelling);
	clang_disposeString(parent_type_spelling);
	clang_disposeString(kind_spelling);
	clang_disposeString(spelling);
	clang_disposeString(display_name);
	//clang_disposeString(pretty_printed);
	//clang_PrintingPolicy_dispose(print_policy);

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
		Headers[file] = true;
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
		
		if (std::string(argv[i]).find("-fprebuilt-module-path") == 0)
		{
			printf("%s \n", argv[i]);
			combinedArgs.push_back(argv[i]);
		}
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